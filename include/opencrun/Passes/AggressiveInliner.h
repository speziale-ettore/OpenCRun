
#ifndef OPENCRUN_PASSES_AGGRESSIVEINLINER_H
#define OPENCRUN_PASSES_AGGRESSIVEINLINER_H

#include "opencrun/Util/PassOptions.h"

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Analysis/InlineCost.h"
#include "llvm/Transforms/IPO/InlinerPass.h"

#include <climits>

namespace opencrun {

class AggressiveInliner : public llvm::Inliner {
public:
  static char ID;

public:
  AggressiveInliner(llvm::StringRef Kernel = "") :
    llvm::Inliner(ID, INT_MIN),
    Kernel(GetKernelOption(Kernel)),
    NotAForest(false) { }

public:
  virtual bool runOnSCC(llvm::CallGraphSCC &SCC);

  virtual const char *getPassName() const {
    return "Aggressive inliner";
  }

  virtual llvm::InlineCost getInlineCost(llvm::CallSite CS) {
    if(NotVisit.count(CS.getCaller()) ||
       NotVisit.count(CS.getCalledFunction()))
      return llvm::InlineCost::getNever();
    else
      return llvm::InlineCost::getAlways();
  }

  virtual float getInlineFudgeFactor(llvm::CallSite CS) {
    return 1.0f;
  }

  virtual void resetCachedCostInfo(llvm::Function *Caller) { }

  virtual void growCachedCostInfo(llvm::Function *Caller,
                                  llvm::Function *Callee) { }

  virtual bool doInitialization(llvm::CallGraph &CG);
  virtual bool doFinalization(llvm::CallGraph &CG);

public:
  bool IsAllInlined() const { return NotAForest; }

private:
  std::string Kernel;

  llvm::SmallPtrSet<const llvm::Function *, 16> NotVisit;
  bool NotAForest;
};

} // End namespace opencrun.

#endif // OPENCRUN_PASSES_AGGRESSIVEINLINER_H
