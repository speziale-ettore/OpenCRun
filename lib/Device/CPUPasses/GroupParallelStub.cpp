
#include "opencrun/Device/CPUPasses/AllPasses.h"
#include "opencrun/Util/OpenCLMetadataHandler.h"

#define DEBUG_TYPE "CPU-group-parallel-stub"

#include "llvm/ADT/Statistic.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Type.h"
#include "llvm/DerivedTypes.h"

using namespace opencrun;

static llvm::cl::opt<std::string> Kernel("kernel",
                                         llvm::cl::init(""),
                                         llvm::cl::Hidden,
                                         llvm::cl::desc("Kernel function "
                                                        "for which the stub "
                                                        "must be built"));

STATISTIC(GroupParallelStubs, "Number of CPU group parallel stubs created");

namespace {

class GroupParallelStub : public llvm::ModulePass {
public:
  static char ID;

public:
  GroupParallelStub(llvm::StringRef Kernel = "") :
    llvm::ModulePass(ID),
    Kernel(::Kernel != "" ? ::Kernel : Kernel) { }

public:
  virtual bool runOnModule(llvm::Module &Mod);

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    // TODO: correctly set AU.
  }

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
    Modified = Modified || BuildStub(*I);

  return Modified;
}

bool GroupParallelStub::BuildStub(llvm::Function &Kern) {  
  llvm::Module *Mod = Kern.getParent();
  
  if(!Mod)
    return false;

  llvm::LLVMContext &Ctx = Mod->getContext();

  // Make function type.
  const llvm::Type *RetTy = llvm::Type::getVoidTy(Ctx);
  const llvm::Type *Int8PtrTy = llvm::Type::getInt8PtrTy(Ctx);
  const llvm::Type *ArgTy = llvm::PointerType::getUnqual(Int8PtrTy);
  llvm::FunctionType *StubTy = llvm::FunctionType::get(RetTy, ArgTy, false);

  // Create the stub.
  llvm::Function *Stub = llvm::Function::Create(StubTy,
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
    llvm::GetElementPtrInst *Addr = llvm::GetElementPtrInst::Create(Arg,
                                                                    J,
                                                                    "",
                                                                    Entry);

    // Cast to the appropriate type.
    const llvm::Type *ArgPtrTy = llvm::PointerType::getUnqual(I->getType());
    llvm::CastInst *CastedAddr = llvm::CastInst::CreatePointerCast(Addr,
                                                                   ArgPtrTy,
                                                                   "",
                                                                   Entry);

    // Load argument.
    KernArgs.push_back(new llvm::LoadInst(CastedAddr, "", Entry));
  }

  // Call the kernel.
  llvm::CallInst::Create(&Kern, KernArgs.begin(), KernArgs.end(), "", Entry);

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
