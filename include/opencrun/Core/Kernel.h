
#ifndef OPENCRUN_CORE_KERNEL_H
#define OPENCRUN_CORE_KERNEL_H

#include "CL/cl.h"

#include "opencrun/Core/MemoryObj.h"
#include "opencrun/Core/Program.h"
#include "opencrun/Util/MTRefCounted.h"

#include "llvm/DerivedTypes.h"
#include "llvm/Support/ErrorHandling.h"

struct _cl_kernel { };

namespace opencrun {

class KernelArg {
public:
  enum Type {
    BufferArg
  };

protected:
  KernelArg(Type Ty, unsigned Position) : Ty(Ty), Position(Position) { }

public:
  Type GetType() const { return Ty; }
  unsigned GetPosition() const { return Position; }

private:
  Type Ty;
  unsigned Position;
};

class BufferKernelArg : public KernelArg {
public:
  static bool classof(const KernelArg *Arg) {
    return Arg->GetType() == KernelArg::BufferArg;
  }

public:
  BufferKernelArg(unsigned Position, Buffer *Buf, clang::LangAS::ID AddrSpace) :
    KernelArg(KernelArg::BufferArg, Position),
    Buf(Buf),
    AddrSpace(AddrSpace) { }

public:
  Buffer *GetBuffer() { return Buf; }

  bool OnLocalAddressSpace() const {
    return AddrSpace == clang::LangAS::opencl_local;
  }

private:
  Buffer *Buf;
  clang::LangAS::ID AddrSpace;
};

class Kernel : public _cl_kernel, public MTRefCountedBase<Kernel> {
public:
  static bool classof(const _cl_kernel *Kern) { return true; }

public:
  typedef std::map<Device *, llvm::Function *> CodesContainer;

  typedef llvm::SmallVector<KernelArg *, 8> ArgumentsContainer;
  
  typedef ArgumentsContainer::iterator arg_iterator;

public:
  arg_iterator arg_begin() { return Arguments.begin(); }
  arg_iterator arg_end() { return Arguments.end(); }

public:
  Kernel(Program &Prog, CodesContainer &Codes) : Prog(&Prog),
                                                 Codes(Codes),
                                                 Arguments(GetArgCount()) { }

  ~Kernel();

public:
  cl_int SetArg(unsigned I, size_t Size, const void *Arg);

public:
  llvm::Function *GetFunction(Device &Dev) {
    return Codes.count(&Dev) ? Codes[&Dev] : NULL;
  }

  const llvm::FunctionType &GetType() const {
    // All stored functions share the same signature, just return the first.
    CodesContainer::const_iterator I = Codes.begin();
    const llvm::Function &Func = *I->second;

    return *Func.getFunctionType();
  }

  llvm::StringRef GetName() const {
    // All stored functions share the same signature, just return the first.
    CodesContainer::const_iterator I = Codes.begin();
    const llvm::Function &Func = *I->second;

    return Func.getName();
  }

  unsigned GetArgCount() const {
    const llvm::FunctionType &FuncTy = GetType();

    return FuncTy.getNumParams();
  }

  llvm::Module *GetModule(Device &Dev) {
    return Codes.count(&Dev) ? Codes[&Dev]->getParent() : NULL;
  }

  Context &GetContext() const { return Prog->GetContext(); }

  // TODO: implement. Attribute must be gathered from metadata.
  llvm::SmallVector<size_t, 4> &GetRequiredWorkGroupSizes() const {
    llvm_unreachable("Not yet implemented");
  }

  bool IsBuiltFor(Device &Dev) const { return Prog->IsBuiltFor(Dev); }

  // TODO: implement. Depends on kernel argument setting code, not well written.
  bool AreAllArgsSpecified() const { return true; }

  // TODO: implement. Attribute must be gathered from metadata.
  bool RequireWorkGroupSizes() const { return false; }

private:
  const llvm::Type *GetArgType(unsigned I) const {
    const llvm::FunctionType &KernTy = GetType();

    return I < KernTy.getNumParams() ? KernTy.getParamType(I) : NULL;
  }

  cl_int SetBufferArg(unsigned I, size_t Size, const void *Arg);

  clang::LangAS::ID GetArgAddressSpace(unsigned I) {
    // All stored functions share the same signature, use the first.
    CodesContainer::iterator J = Codes.begin();
    llvm::Function &Kern = *J->second;

    OpenCLMetadataHandler OpenCLMDHandler(*Kern.getParent());

    return OpenCLMDHandler.GetArgAddressSpace(Kern, I);
  }

  bool IsBuffer(const llvm::Type &Ty);

private:
  llvm::sys::Mutex ThisLock;

  llvm::IntrusiveRefCntPtr<Program> Prog;
  CodesContainer Codes;

  ArgumentsContainer Arguments;
};

} // End namespace opencrun.

#endif // OPENCRUN_CORE_KERNEL_H
