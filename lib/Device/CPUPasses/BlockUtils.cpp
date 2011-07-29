
#include "PassUtils.h"

#include "llvm/BasicBlock.h"

using namespace opencrun;

llvm::Instruction *opencrun::FindNextInst(llvm::Instruction *Inst) {
  // TODO: surely there are cleaner methods to find the successor of an
  // instruction, use one of them.
  llvm::BasicBlock *Block = Inst->getParent();
  for(llvm::BasicBlock::iterator I = Block->begin(),
                                 E = Block->end();
                                 I != E;
                                 ++I) {
    if(I == *Inst) {
      ++I;
      if(I != Block->end() && I != *Block->getTerminator())
        return I;
      return NULL;
    }
  }
  return NULL;
}
