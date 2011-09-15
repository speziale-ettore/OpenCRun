
#include "opencrun/Core/Kernel.h"
#include "opencrun/Core/Context.h"
#include "opencrun/Core/Device.h"
#include "opencrun/Passes/AllPasses.h"

#include "llvm/PassManager.h"

using namespace opencrun;

#define RETURN_WITH_ERROR(ERR, MSG)  \
  {                                  \
  Context &Ctx = Prog->GetContext(); \
  Ctx.ReportDiagnostic(MSG);         \
  return ERR;                        \
  }

Kernel::~Kernel() {
  Prog->UnregisterKernel(*this);

  for(CodesContainer::iterator I = Codes.begin(),
                               E = Codes.end();
                               I != E;
                               ++I) {
    Device &Dev = *I->first;

    Dev.UnregisterKernel(*this);
  }

  llvm::DeleteContainerPointers(Arguments);
}

cl_int Kernel::SetArg(unsigned I, size_t Size, const void *Arg) {
  llvm::Type *ArgTy = GetArgType(I);

  if(!ArgTy)
    RETURN_WITH_ERROR(CL_INVALID_ARG_INDEX,
                      "argument number exceed kernel argument count");

  if(IsBuffer(*ArgTy))
    return SetBufferArg(I, Size, Arg);

  else if(IsByValue(*ArgTy))
    return SetByValueArg(I, Size, Arg);

  llvm_unreachable("Unknown argument type");
}

bool Kernel::GetMaxWorkGroupSize(size_t &Size, Device *Dev) {
  if(!(Dev = RequireEstimates(Dev)))
    return false;

  Size = Estimates->GetMaxWorkGroupSize(Dev->GetPrivateMemorySize());

  size_t DevMaxSize = Dev->GetMaxWorkGroupSize();
  if(Size > DevMaxSize)
    Size = DevMaxSize;

  return true;
}

bool Kernel::GetMinLocalMemoryUsage(size_t &Size, Device *Dev) {
  if(!(Dev = RequireEstimates(Dev)))
    return false;

  Size = Estimates->GetLocalMemoryUsage();

  return true;
}

bool Kernel::GetMinPrivateMemoryUsage(size_t &Size, Device *Dev) {
  if(!(Dev = RequireEstimates(Dev)))
    return false;

  Size = Estimates->GetPrivateMemoryUsage();

  return true;
}

cl_int Kernel::SetBufferArg(unsigned I, size_t Size, const void *Arg) {
  Buffer *Buf;

  Context &Ctx = GetContext();

  clang::LangAS::ID AddrSpace = GetArgAddressSpace(I);

  switch(AddrSpace) {
  case clang::LangAS::opencl_global:
    if(Size != sizeof(cl_mem))
      RETURN_WITH_ERROR(CL_INVALID_ARG_SIZE,
                        "kernel argument size does not match");

    std::memcpy(&Buf, Arg, Size);

    break;

  case clang::LangAS::opencl_constant:
    llvm_unreachable("Not yet supported");

  case clang::LangAS::opencl_local:
    if(Arg)
      RETURN_WITH_ERROR(CL_INVALID_ARG_VALUE, "cannot set a local pointer");

    if(!Size)
      RETURN_WITH_ERROR(CL_INVALID_ARG_SIZE, "local buffer size unspecified");

    Buf = Ctx.CreateVirtualBuffer(Size, MemoryObj::ReadWrite);

    break;

  default:
    llvm_unreachable("Invalid address space");
  }

  if(Buf) {
    if(Buf->GetContext() != Ctx)
      RETURN_WITH_ERROR(CL_INVALID_MEM_OBJECT,
                        "buffer and kernel contexts does not match");
  }

  ThisLock.acquire();
  Arguments[I] = new BufferKernelArg(I, Buf, AddrSpace);
  ThisLock.release();

  return CL_SUCCESS;
}

cl_int Kernel::SetByValueArg(unsigned I, size_t Size, const void *Arg) {
  // We cannot check Size with respect to the size of Arg on the host, because
  // we cannot known the size of the type on the host -- we cannot a type
  // declared on the device domain with one declared on the host domain. Trust
  // the user!
  ThisLock.acquire();
  Arguments[I] = new ByValueKernelArg(I, Arg, Size);
  ThisLock.release();

  return CL_SUCCESS;
}

bool Kernel::IsBuffer(llvm::Type &Ty) {
  if(llvm::PointerType *PointerTy = llvm::dyn_cast<llvm::PointerType>(&Ty)) {
    llvm::Type *PointeeTy = PointerTy->getElementType();

    if(llvm::isa<llvm::VectorType>(PointeeTy))
      PointeeTy = PointeeTy->getScalarType();

    return PointeeTy->isIntegerTy() ||
           PointeeTy->isFloatTy() ||
           PointeeTy->isDoubleTy();
  }

  return false;
}

bool Kernel::IsByValue(llvm::Type &Ty) {
  llvm::Type *CheckTy = &Ty;

  if(llvm::isa<llvm::VectorType>(CheckTy))
    CheckTy = CheckTy->getScalarType();

  return CheckTy->isIntegerTy() ||
         CheckTy->isFloatTy() ||
         CheckTy->isDoubleTy();
}

Device *Kernel::RequireEstimates(Device *Dev) {
  CodesContainer::iterator I;
  if(!Dev && Codes.size() == 1) {
    CodesContainer::iterator I = Codes.begin();
    Dev = I->first;
  }

  if(!Dev || !IsBuiltFor(*Dev))
    return NULL;

  // Double checked lock, unsafe read.
  if(Estimates)
    return Dev;

  llvm::sys::ScopedLock Lock(ThisLock);

  // Double checked lock, safe read.
  // TODO: is this thread safe with respect to compiler invocation?
  if(!Estimates) {
    llvm::PassManager PM;

    FootprintEstimator *Pass = CreateFootprintEstimatorPass(GetName());
    llvm::Function *Fun = GetFunction(*Dev);

    PM.add(Pass);
    PM.run(*Fun->getParent());

    Estimates.reset(new Footprint(*Pass->GetFootprint(*Fun)));
  }

  return Dev;
}
