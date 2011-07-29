
#include "opencrun/Device/CPUPasses/AllPasses.h"
#include "opencrun/Util/OpenCLMetadataHandler.h"

#include "PassUtils.h"

#define DEBUG_TYPE "CPU-split-blocks"

#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/ADT/Statistic.h"

#define SUFFIX ".splitted"

using namespace opencrun;

STATISTIC(SplittedBlocks, "Number of splitted blocks");

namespace {

//===----------------------------------------------------------------------===//
/// BlockSplitting - Perform the splitting of the basic blocks which contain
///  calls to the barrier function. It ensures that a barrier call is always the
///  last instruction of a basic block.
//===----------------------------------------------------------------------===//
class BlockSplitting : public llvm::RegionPass {
public:
  static char ID;

public:
  BlockSplitting(llvm::StringRef Kernel = "") :
    llvm::RegionPass(ID),
    Kernel(GetKernelOption(Kernel)) { }

public:
    virtual bool runOnRegion(llvm::Region *Reg, llvm::RGPassManager &RGM);

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
      AU.addRequired<llvm::DominatorTree>();
    }

    virtual const char *getPassName() const {
      return "Block splitter";
    }

private:
  bool SplitBlocks(llvm::Region *Reg);

  /// Verify BlockSplitting's guarantees.
  void VerifyBlockSplitting(llvm::Region *Reg) const;

private:
  std::string Kernel;
};

} // End anonymous namespace.

bool BlockSplitting::runOnRegion(llvm::Region *Reg, llvm::RGPassManager &RGM) {
  SplittedBlocks = 0;

  llvm::BasicBlock *Entry = Reg->getEntry();
  llvm::Function *Fun = Entry->getParent();
  llvm::Module *Mod = Fun->getParent();

  OpenCLMetadataHandler MDHandler(*Mod);

  bool Modified = false;

  // If the kernel name is specified then apply the transformation
  // only the regions in the specified function.
  if(Kernel != "") {
    llvm::Function* KernelFun = MDHandler.GetKernel(Kernel);
    if(KernelFun == Fun)
      Modified = SplitBlocks(Reg);
  }
  // If no kernel is specified then apply the transformation
  // to every region in the function.
  else {
    Modified = SplitBlocks(Reg);
  }

  VERIFY(VerifyBlockSplitting(Reg));

  return Modified;
}

bool BlockSplitting::SplitBlocks(llvm::Region *Reg) {
  // Start of Analysis.

  // Initialize all the analysis passes.
  llvm::DominatorTree* DT = &getAnalysis<llvm::DominatorTree>();

  // List blocks containing barriers.
  std::vector<llvm::CallInst*> Barriers;
  for(llvm::Region::block_iterator I = Reg->block_begin(),
                                   E = Reg->block_end();
                                   I != E;
                                   ++I) {
    llvm::BasicBlock* Block = (*I)->getEntry();
    FindBarriers(Block, Barriers);
  }

  // End of Analysis / Start of Transformation.

  // Split blocks containing barriers.
  for(std::vector<llvm::CallInst*>::iterator I = Barriers.begin(),
                                             E = Barriers.end();
                                             I != E;
                                             ++I) {
    llvm::Instruction *Inst = *I;
    llvm::BasicBlock *Block = Inst->getParent();
    llvm::Instruction *SplitInst = FindNextInst(*I);

    if(SplitInst) {
      llvm::Twine NewName = Block->getName() + SUFFIX;

      // Split the block.
      llvm::BasicBlock *NewBlock = Block->splitBasicBlock(SplitInst, NewName);

      // Add the new block to the dominator tree.
      DT->addNewBlock(NewBlock, Block);

      // Update statistics.
      ++SplittedBlocks;
    }
  }

  return (SplittedBlocks != 0);
}

void BlockSplitting::VerifyBlockSplitting(llvm::Region *Reg) const {
  for(llvm::Region::block_iterator I = Reg->block_begin(),
                                   E = Reg->block_end();
                                   I != E;
                                   ++I) {
    llvm::BasicBlock *Block = (*I)->getEntry();
    if(FindLastBarrier(Block))
      assert(EndsWithAnUniqueBarrier(Block) && "barrier is not last "
                                               "instruction of basic block");
  }
}

char BlockSplitting::ID = 0;

static llvm::RegisterPass<BlockSplitting> X("cpu-block-splitting",
                                            "Split blocks containing "
                                            "barriers");

llvm::Pass *opencrun::CreateBlockSplittingPass(llvm::StringRef Kernel) {
  return new BlockSplitting(Kernel);
}
