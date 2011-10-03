
#include "opencrun/Passes/AllPasses.h"
#include "opencrun/Passes/AggressiveInliner.h"
#include "opencrun/Util/OpenCLMetadataHandler.h"

#define DEBUG_TYPE "aggressive-inliner"

using namespace opencrun;

bool AggressiveInliner::runOnSCC(llvm::CallGraphSCC &SCC) {
  if(!SCC.isSingular()) {
    NotAForest = true;
    return false;
  }

  return llvm::Inliner::runOnSCC(SCC);
}

bool AggressiveInliner::doInitialization(llvm::CallGraph &CG) {
  llvm::Module &Mod = CG.getModule();
  OpenCLMetadataHandler MD(Mod);

  llvm::SmallVector<const llvm::Function *, 16> ToVisit;
  llvm::SmallPtrSet<const llvm::Function *, 16> Visited;

  llvm::Function *KernelFun;

  // First, get the roots of the graph.
  if(Kernel != "" && (KernelFun = MD.GetKernel(Kernel)))
    ToVisit.push_back(KernelFun);
  else for(OpenCLMetadataHandler::kernel_iterator I = MD.kernel_begin(),
                                                  E = MD.kernel_end();
                                                  I != E;
                                                  ++I)
    ToVisit.push_back(&*I);

  // Then, find all reachable functions.
  while(!ToVisit.empty()) {
    const llvm::Function *CurFun = ToVisit.pop_back_val();
    llvm::CallGraphNode &CurNode = *CG[CurFun];

    for(unsigned I = 0, E = CurNode.size(); I != E ; ++I) {
      const llvm::Function *CurFun = CurNode[I]->getFunction();

      if(!Visited.count(CurFun))
        ToVisit.push_back(CurFun);
    }

    Visited.insert(CurFun);
  }

  // At last, filter out functions to not visit during the pass.
  for(llvm::Module::iterator I = Mod.begin(), E = Mod.end(); I != E; ++I)
    if(!I->isDeclaration() && !Visited.count(&*I))
      NotVisit.insert(&*I);

  return false;
}

bool AggressiveInliner::doFinalization(llvm::CallGraph &CG) {
  return removeDeadFunctions(CG, &NotVisit);
}

char AggressiveInliner::ID = 0;

static llvm::RegisterPass<AggressiveInliner> X("aggressive-inliner",
                                               "Inline all possible functions");

AggressiveInliner *
opencrun::CreateAggressiveInlinerPass(llvm::StringRef Kernel) {
  return new AggressiveInliner(Kernel);
}
