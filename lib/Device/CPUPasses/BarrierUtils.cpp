
#include "PassUtils.h"

#include "llvm/Function.h"

using namespace opencrun;

llvm::CallInst *opencrun::GetAsBarrierCall(llvm::Instruction *Inst) {
  // TODO: move to the OpenCLMDHandler interface.
  if(llvm::CallInst *CI = llvm::dyn_cast<llvm::CallInst>(Inst)) {
    llvm::Function *Fun = CI->getCalledFunction();
    if(Fun->getName() == "__builtin_ocl_barrier")
      return CI;
  }
  return NULL;
}

void opencrun::FindBarriers(llvm::BasicBlock *Block,
                            std::vector<llvm::CallInst *> &Barriers) {
  // Search for all calls to OpenCL barriers.
  for(llvm::BasicBlock::iterator I = Block->begin(),
                                 E = Block->end();
                                 I != E;
                                 ++I)
    if(llvm::CallInst *Call = GetAsBarrierCall(I))
      Barriers.push_back(Call);
}

llvm::CallInst *opencrun::FindLastBarrier(llvm::BasicBlock *Block) {
  llvm::CallInst *BarrierCall = NULL;
  // Search for last OpenCL barrier call.
  for(llvm::BasicBlock::iterator I = Block->begin(),
                                 E = Block->end();
                                 I != E;
                                 ++I)
    if(llvm::CallInst* Call = GetAsBarrierCall(I))
      BarrierCall = Call;

  return BarrierCall;
}

bool opencrun::EndsWithAnUniqueBarrier(llvm::BasicBlock *Block) {
  if(llvm::CallInst *Barrier = FindLastBarrier(Block))
    return FindNextInst(Barrier) == NULL;
  return false;
}
