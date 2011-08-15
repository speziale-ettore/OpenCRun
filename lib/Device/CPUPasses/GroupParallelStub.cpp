
#include "opencrun/Device/CPUPasses/AllPasses.h"
#include "opencrun/Util/OpenCLMetadataHandler.h"
#include "opencrun/Util/PassOptions.h"

#define DEBUG_TYPE "CPU-group-parallel-stub"

#include "llvm/ADT/Statistic.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Type.h"
#include "llvm/DerivedTypes.h"

using namespace opencrun;

STATISTIC(GroupParallelStubs, "Number of CPU group parallel stubs created");

namespace {

//===----------------------------------------------------------------------===//
/// GroupParallelStub - Builds stubs to invoke OpenCL kernels from inside the
///  CPU runtime. The CPU runtime stores kernel arguments inside an array of
///  pointers. To correctly invoke the kernel, it is needed to unpack arguments
///  from the array. Each array element is a pointer. If the i-th kernel
///  parameter is a buffer, then the pointer directly points to it. If it is a
///  a value type, then it points to the value supplied by the user -- it must
///  be passed by copy to the kernel.
///
///  Example for void foo(int a, global int *b):
///
///  args: [ int * ][ int *]
///          |        |
///          |        +-> b: [ int ] ... [ int ]
///          |
///          +-> a: [ int ]
//===----------------------------------------------------------------------===//
class GroupParallelStub : public llvm::ModulePass {
public:
  static char ID;

public:
  GroupParallelStub(llvm::StringRef Kernel = "") :
    llvm::ModulePass(ID),
    Kernel(GetKernelOption(Kernel)) { }

public:
  virtual bool runOnModule(llvm::Module &Mod);

  virtual const char *getPassName() const {
    return "Group parallel stub builder";
  }

private:
  bool BuildStub(llvm::Function &Kern);

  std::string MangleKernelName(llvm::StringRef Name) {
    return "_GroupParallelStub_" + Name.str();
  }

private:
  std::string Kernel;
  llvm::Function *Barrier;
};

} // End anonymous namespace.

bool GroupParallelStub::runOnModule(llvm::Module &Mod) {
  OpenCLMetadataHandler MDHandler(Mod);

  llvm::Function *KernelFun;
  bool Modified = false;

  Barrier = MDHandler.GetBuiltin("barrier");

  if(Kernel != "" && (KernelFun = MDHandler.GetKernel(Kernel)))
    Modified = BuildStub(*KernelFun);
  
  else for(OpenCLMetadataHandler::kernel_iterator I = MDHandler.kernel_begin(),
                                                  E = MDHandler.kernel_end();
                                                  I != E;
                                                  ++I)
    // Use BuildStub(*I) as first || operand to force evaluation and avoid
    // short-circuiting by the compiler.
    Modified = BuildStub(*I) || Modified;

  return Modified;
}

bool GroupParallelStub::BuildStub(llvm::Function &Kern) {  
  llvm::Module *Mod = Kern.getParent();
  
  if(!Mod)
    return false;

  llvm::LLVMContext &Ctx = Mod->getContext();

  // Make function type.
  llvm::Type *RetTy = llvm::Type::getVoidTy(Ctx);
  llvm::Type *Int8PtrTy = llvm::Type::getInt8PtrTy(Ctx);
  llvm::Type *ArgTy = llvm::PointerType::getUnqual(Int8PtrTy);
  llvm::FunctionType *StubTy = llvm::FunctionType::get(RetTy, ArgTy, false);

  // Create the stub.
  llvm::Function *Stub;
  Stub = llvm::Function::Create(StubTy,
                                llvm::Function::ExternalLinkage,
                                MangleKernelName(Kern.getName()),
                                Mod);

  // Set argument name.
  llvm::Argument *Arg = Stub->arg_begin();
  Arg->setName("args");

  // Entry basic block.
  llvm::BasicBlock *Entry = llvm::BasicBlock::Create(Ctx, "entry", Stub);

  llvm::SmallVector<llvm::Value *, 8> KernArgs;

  // Copy each argument into a local variable.
  for(llvm::Function::const_arg_iterator I = Kern.arg_begin(),
                                         E = Kern.arg_end();
                                         I != E;
                                         ++I) {
    // Get J-th element address.
    llvm::Value *J = llvm::ConstantInt::get(llvm::Type::getInt32Ty(Ctx),
                                            I->getArgNo());
    llvm::Value *Addr = llvm::GetElementPtrInst::Create(Arg, J, "", Entry);

    // Get its type.
    llvm::Type *ArgTy = I->getType();

    // If it is passed by value, then we have to do an extra load.
    if(!ArgTy->isPointerTy())
      Addr = new llvm::LoadInst(Addr, "", Entry);

    // Cast to the appropriate type.
    llvm::Type *ArgPtrTy = llvm::PointerType::getUnqual(I->getType());
    llvm::CastInst *CastedAddr = llvm::CastInst::CreatePointerCast(Addr,
                                                                   ArgPtrTy,
                                                                   "",
                                                                   Entry);

    // Load argument.
    KernArgs.push_back(new llvm::LoadInst(CastedAddr, "", Entry));
  }

  // Call the kernel.
  llvm::CallInst::Create(&Kern, KernArgs, "", Entry);

  // Call the implicit barrier.
  llvm::Value *Flag = llvm::ConstantInt::get(llvm::Type::getInt64Ty(Ctx), 0);
  llvm::CallInst *BarrierCall;
  BarrierCall = llvm::CallInst::Create(Barrier,
                                       Flag,
                                       "",
                                       Entry);
  BarrierCall->setTailCall();

  // End stub.
  llvm::ReturnInst::Create(Ctx, Entry);

  // Update statistics.
  ++GroupParallelStubs;

  return true;
}

char GroupParallelStub::ID = 0;

static llvm::RegisterPass<GroupParallelStub> X("cpu-group-parallel-stub",
                                               "Create kernel stub for "
                                               "cpu group parallel scheduler");

llvm::Pass *opencrun::CreateGroupParallelStubPass(llvm::StringRef Kernel) {
  return new GroupParallelStub(Kernel);
}
