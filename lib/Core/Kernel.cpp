
#include "opencrun/Core/Kernel.h"
#include "opencrun/Core/Context.h"
#include "opencrun/Core/Device.h"

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

cl_int Kernel::SetBufferArg(unsigned I, size_t Size, const void *Arg) {
  if(Size != sizeof(cl_mem))
    RETURN_WITH_ERROR(CL_INVALID_ARG_SIZE,
                      "kernel argument size does not match");

  clang::LangAS::ID AddrSpace = GetArgAddressSpace(I);

  Buffer *Buf;
  std::memcpy(&Buf, Arg, Size);

  if(Buf) {
    if(Buf->GetContext() != GetContext())
      RETURN_WITH_ERROR(CL_INVALID_MEM_OBJECT,
                        "buffer and kernel contexts does not match");

    if(AddrSpace == clang::LangAS::opencl_local)
      RETURN_WITH_ERROR(CL_INVALID_ARG_VALUE, "cannot set a local pointer");
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
