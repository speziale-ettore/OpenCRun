
#ifndef OPENCRUN_CORE_PROGRAM_H
#define OPENCRUN_CORE_PROGRAM_H

#include "CL/opencl.h"

#include "opencrun/Util/MTRefCounted.h"
#include "opencrun/Util/OpenCLMetadataHandler.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/LLVMContext.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Mutex.h"

#include <map>

// OpenCL extensions defines macro that clashes with clang internal symbols.
// This is a workaround to include clang needed headers. Is this the right
// solution?
#define cl_khr_gl_sharing_save cl_khr_gl_sharing
#undef cl_khr_gl_sharing
#include "clang/Frontend/CompilerInvocation.h"
#define cl_khr_gl_sharing cl_khr_gl_sharing_save

struct _cl_program { };

namespace opencrun {

class Context;
class Device;
class Kernel;
class Program;

class CompilerCallbackClojure {
public:
  typedef void (CL_CALLBACK *Signature)(cl_program, void *);
  typedef void *UserDataSignature; 

public:
  CompilerCallbackClojure(Signature Callback, UserDataSignature UserData) :
    Callback(Callback),
    UserData(UserData) { }

public:
  void CompilerDone(Program &Prog);

private:
  const Signature Callback;
  const UserDataSignature UserData;
};

class BuildInformation {

public:
  BuildInformation() : BuildStatus(CL_BUILD_NONE) { }
  BuildInformation(const BuildInformation &That); // Do not implement.
  void operator=(const BuildInformation &That); // Do not implement.

public:
  void RegisterBuildInProgress() { BuildStatus = CL_BUILD_IN_PROGRESS; }

  void RegisterBuildDone(bool Success = false) {
    BuildStatus = Success ? CL_BUILD_SUCCESS : CL_BUILD_ERROR;
  }

public:
  void SetBitCode(llvm::Module *BitCode) {
    this->BitCode.reset(BitCode);
  }

  void SetBuildOptions(llvm::StringRef Opts) {
    BuildOpts = Opts;
  }

public:
  std::string &GetBuildLog() { return BuildLog; }

  llvm::Function *GetKernel(llvm::StringRef KernName) {
    if(!BitCode)
      return NULL;

    OpenCLMetadataHandler MDHandler(*BitCode); 

    return MDHandler.GetKernel(KernName);
  }

  const llvm::FunctionType *GetKernelType(llvm::StringRef KernName) {
    llvm::Function *Kern = GetKernel(KernName);

    return Kern ? Kern->getFunctionType() : NULL;
  }

public:
  bool IsBuilt() const { return BuildStatus == CL_BUILD_SUCCESS; }
  bool BuildInProgress() const { return BuildStatus == CL_BUILD_IN_PROGRESS; }
  bool HasBitCode() const { return BitCode; }
  bool DefineKernel(llvm::StringRef KernName) { return GetKernel(KernName); }

private:
  cl_build_status BuildStatus;
  std::string BuildLog;
  llvm::SmallString<32> BuildOpts;

  llvm::OwningPtr<llvm::Module> BitCode;
};

class Program : public _cl_program, public MTRefCountedBase<Program> {
public:
  static bool classof(const _cl_program *Prog) { return true; }

public:
  typedef llvm::SmallVector<Device *, 2> DevicesContainer;
  typedef std::map<Device *, BuildInformation *> BuildInformationContainer;
  typedef llvm::SmallPtrSet<Kernel *, 4> AttachedKernelsContainer;

public:
  ~Program() { llvm::DeleteContainerSeconds(BuildInfo); }

private:
  Program(Context &Ctx, llvm::MemoryBuffer &Src) : Ctx(&Ctx),
                                                   Src(&Src) { }

public:
  cl_int Build(DevicesContainer &Devs,
               llvm::StringRef Opts,
               CompilerCallbackClojure &CompilerNotify);

  Kernel *CreateKernel(llvm::StringRef KernName, cl_int *ErrCode);

  void UnregisterKernel(Kernel &Kern) {
    llvm::sys::ScopedLock Lock(ThisLock);

    AttachedKernels.erase(&Kern);
  }

  Context &GetContext() { return *Ctx; }

public:
  bool HasAttachedKernels() {
    llvm::sys::ScopedLock Lock(ThisLock);

    return !AttachedKernels.empty();
  }

  bool IsBuiltFor(Device &Dev) {
    llvm::sys::ScopedLock Lock(ThisLock);

    return BuildInfo.count(&Dev);
  }

private: 
  cl_int Build(Device &Dev, llvm::StringRef Opts);

  void RegisterKernel(Kernel &Kern) {
    llvm::sys::ScopedLock Lock(ThisLock);

    AttachedKernels.insert(&Kern);
  }

private:
  llvm::sys::Mutex ThisLock;

  llvm::IntrusiveRefCntPtr<Context> Ctx;
  llvm::OwningPtr<llvm::MemoryBuffer> Src;

  // Default diagnostic: don't colorize or format the output, it must write to a
  // memory buffer.
  clang::DiagnosticOptions DiagOptions;

  BuildInformationContainer BuildInfo;
  AttachedKernelsContainer AttachedKernels;

  friend class ProgramBuilder;
};

class ProgramBuilder {
public:
  ProgramBuilder(Context &Ctx);

public:
  ProgramBuilder &SetSources(unsigned Count,
                             const char *Srcs[],
                             const size_t Lengths[] = NULL);

  Program *Create(cl_int *ErrCode);

private:
  ProgramBuilder &NotifyError(cl_int ErrCode, const char *Msg = "");

private:
  Context &Ctx;
  llvm::MemoryBuffer *Srcs;

  cl_int ErrCode;
};

} // End namespace opencrun.

#endif // OPENCRUN_CORE_PROGRAM_H
