
#include "opencrun/Core/Program.h"
#include "opencrun/Core/Context.h"
#include "opencrun/Core/Device.h"
#include "opencrun/Core/Kernel.h"

#include "clang/Frontend/ChainedDiagnosticConsumer.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Frontend/TextDiagnosticBuffer.h"

using namespace opencrun;

//
// CompilerCallbackClojure implementation.
//

void CompilerCallbackClojure::CompilerDone(Program &Prog) {
  if(Callback)
    Callback(&Prog, UserData);
}

//
// Program implementation.
//

#define RETURN_WITH_ERROR(ERR, MSG) \
  {                                 \
  Ctx->ReportDiagnostic(MSG);       \
  return ERR;                       \
  }

cl_int Program::Build(DevicesContainer &Devs,
                      llvm::StringRef Opts,
                      CompilerCallbackClojure &CompilerNotify) {
  if(HasAttachedKernels())
    RETURN_WITH_ERROR(CL_INVALID_OPERATION,
                      "cannot build a program with attached kernel[s]");

  DevicesContainer::iterator I, E;

  if(Devs.empty()) {
    I = Ctx->device_begin();
    E = Ctx->device_end();
  } else {
    I = Devs.begin();
    E = Devs.end();
  }

  cl_int ErrCode = CL_SUCCESS;

  for(; I != E && ErrCode == CL_SUCCESS; ++I)
    ErrCode = Build(**I, Opts);

  // No concurrent builds.
  CompilerNotify.CompilerDone(*this);

  return ErrCode;
}

// Program requires two version of this macro.

#undef RETURN_WITH_ERROR

#define RETURN_WITH_ERROR(VAR, ERRCODE, MSG) \
  {                                          \
  Ctx->ReportDiagnostic(MSG);                \
  if(VAR)                                    \
    *VAR = ERRCODE;                          \
  return NULL;                               \
  }

Kernel *Program::CreateKernel(llvm::StringRef KernName, cl_int *ErrCode) {
  llvm::sys::ScopedLock Lock(ThisLock);

  if(BuildInfo.empty())
    RETURN_WITH_ERROR(ErrCode,
                      CL_INVALID_PROGRAM_EXECUTABLE,
                      "no program has been built");

  BuildInformationContainer::iterator I = BuildInfo.begin(),
                                      E = BuildInfo.end();

  for(; I != E; ++I) {
    BuildInformation &Fst = *I->second;

    if(Fst.DefineKernel(KernName))
      break;
  }

  if(I == E)
    RETURN_WITH_ERROR(ErrCode, CL_INVALID_KERNEL_NAME, "no kernel definition");

  BuildInformation &Fst = *I->second;

  Kernel::CodesContainer Codes;
  Codes[I->first] = Fst.GetKernel(KernName);

  for(BuildInformationContainer::iterator J = ++I; J != E; ++J) {
    Device &Dev = *J->first;
    BuildInformation &Cur = *J->second;

    if(!Cur.IsBuilt())
      continue;

    if(Cur.GetKernelType(KernName) != Fst.GetKernelType(KernName))
      RETURN_WITH_ERROR(ErrCode,
                        CL_INVALID_KERNEL_DEFINITION,
                        "kernel signatures do not match");

    Codes[&Dev] = Cur.GetKernel(KernName);
  }

  Kernel *Kern = new Kernel(*this, Codes);
  RegisterKernel(*Kern);

  if(ErrCode)
    *ErrCode = CL_SUCCESS;

  return Kern;
}

// Switch again, only to keep naming convention consistent.

#undef RETURN_WITH_ERROR

#define RETURN_WITH_ERROR(ERR, MSG) \
  {                                 \
  Ctx->ReportDiagnostic(MSG);       \
  return ERR;                       \
  }

cl_int Program::Build(Device &Dev, llvm::StringRef Opts) {
  llvm::sys::ScopedLock Lock(ThisLock);

  if(!BuildInfo.count(&Dev))
    BuildInfo[&Dev] = new BuildInformation();

  BuildInformation &Info = *BuildInfo[&Dev];

  if(!Ctx->IsAssociatedWith(Dev))
    RETURN_WITH_ERROR(CL_INVALID_DEVICE,
                      "device not associated with build context");

  if(!Src) {
    // No bit-code available, cannot build anything.
    if(!Info.HasBitCode())
      RETURN_WITH_ERROR(CL_INVALID_BINARY,
                        "no program source/binary available");

    // No sources, but bit-code available.
    return CL_SUCCESS;
  }

  if(Info.BuildInProgress())
    RETURN_WITH_ERROR(CL_INVALID_OPERATION,
                      "previously started build not yet terminated");

  // Needed to log compilation results into the program structure.
  llvm::raw_string_ostream Log(Info.GetBuildLog());
  clang::TextDiagnosticPrinter *ToLog;
  ToLog = new clang::TextDiagnosticPrinter(Log, DiagOptions);

  // Needed to log compilation results for internal diagnostic.
  clang::TextDiagnosticBuffer *ToCtx = new clang::TextDiagnosticBuffer();

  // Chain diagnostics.
  clang::ChainedDiagnosticConsumer *Diag;
  Diag = new clang::ChainedDiagnosticConsumer(ToLog, ToCtx);

  // Build in progress.
  Info.RegisterBuildInProgress();

  // Invoke the compiler.
  llvm::Module *BitCode;
  bool Success = Dev.TranslateToBitCode(Opts, *Diag, *Src, BitCode);

  if(Success)
    Info.SetBitCode(BitCode);
  else if(BitCode)
    delete BitCode;

  Info.SetBuildOptions(Opts);

  // Build done.
  Info.RegisterBuildDone(Success);

  // Dump log for debug purposes, only if needed.
  Ctx->ReportDiagnostic(*ToCtx);

  delete Diag;

  return Success ? CL_SUCCESS : CL_BUILD_PROGRAM_FAILURE;
}

// Both Program and ProgramBuilder need this macro.

#undef RETURN_WITH_ERROR

//
// ProgramBuilder implementation.
//

#define RETURN_WITH_ERROR(VAR) \
  {                            \
  if(VAR)                      \
    *VAR = this->ErrCode;      \
  return NULL;                 \
  }

ProgramBuilder::ProgramBuilder(Context &Ctx) : Ctx(Ctx),
                                               ErrCode(CL_SUCCESS) { }

ProgramBuilder &ProgramBuilder::SetSources(unsigned Count,
                                           const char *Srcs[],
                                           const size_t Lengths[]) {
  if(!Count || !Srcs)
    return NotifyError(CL_INVALID_VALUE, "no program given");

  std::string Buf;

  for(unsigned I = 0; I < Count; ++I) {
    if(!Srcs[I])
      return NotifyError(CL_INVALID_VALUE, "null program given");

    llvm::StringRef Src;

    if(Lengths && Lengths[I])
      Src = llvm::StringRef(Srcs[I], Lengths[I]);
    else
      Src = llvm::StringRef(Srcs[I]);

    Buf += Src;
    Buf += '\n';
  }

  this->Srcs = llvm::MemoryBuffer::getMemBufferCopy(Buf);

  return *this;
}

Program *ProgramBuilder::Create(cl_int *ErrCode) {
  if(this->ErrCode != CL_SUCCESS)
    RETURN_WITH_ERROR(ErrCode);

  if(ErrCode)
    *ErrCode = CL_SUCCESS;

  return new Program(Ctx, *Srcs);
}

ProgramBuilder &ProgramBuilder::NotifyError(cl_int ErrCode, const char *Msg) {
  Ctx.ReportDiagnostic(Msg);
  this->ErrCode = ErrCode;

  return *this;
}
