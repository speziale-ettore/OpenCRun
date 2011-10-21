
#ifndef OPENCRUN_PASSES_FOOTPRINTESTIMATE_H
#define OPENCRUN_PASSES_FOOTPRINTESTIMATE_H

#include "opencrun/Util/PassOptions.h"

#include "llvm/Instruction.h"
#include "llvm/Pass.h"

#include <map>

namespace opencrun {

class Footprint {
public:
  Footprint() : LocalMemoryUsage(0),
                PrivateMemoryUsage(0),
                TempMemoryUsage(0) { }

public:
  size_t GetMaxWorkGroupSize(size_t AvailablePrivateMemory) const {
    size_t MaxWorkItems;

    // Estimate by giving to each work-item the same amount of private memory.
    if(PrivateMemoryUsage)
      MaxWorkItems = AvailablePrivateMemory / PrivateMemoryUsage;

    // We do not have precise information about the amount of private memory
    // used by the work-items, since we need to compute an upper bound, suppose
    // each work-item uses at least 1 byte.
    else
      MaxWorkItems = AvailablePrivateMemory;

    return MaxWorkItems;
  }

  size_t GetLocalMemoryUsage() const {
    return LocalMemoryUsage;
  }

  size_t GetPrivateMemoryUsage() const {
    return PrivateMemoryUsage;
  }

  float GetPrivateMemoryUsageAccuracy() const {
    size_t TotalMemory = PrivateMemoryUsage + TempMemoryUsage;
    float Accuracy;

    // Estimate: fraction of private over all data (private + temporaries).
    if(TotalMemory != 0)
      Accuracy = static_cast<float>(PrivateMemoryUsage) / TotalMemory;

    // Perfect estimate, maybe useless.
    else
      Accuracy = 1.0;

    return Accuracy;
  }

protected:
  void AddPrivateMemoryUsage(size_t Usage) {
    PrivateMemoryUsage += Usage;
  }

  void AddTempMemoryUsage(size_t Usage) {
    TempMemoryUsage += Usage;
  }

private:
  size_t LocalMemoryUsage;
  size_t PrivateMemoryUsage;
  size_t TempMemoryUsage;
};

class FootprintEstimate : public Footprint, public llvm::FunctionPass {
public:
  static char ID;

public:
  FootprintEstimate(llvm::StringRef Kernel = "") :
    llvm::FunctionPass(ID),
    Kernel(GetKernelOption(Kernel)) { }

public:
  virtual bool runOnFunction(llvm::Function &Fun);

  virtual const char *getPassName() const {
    return "Footprint estimate";
  }

  virtual void print(llvm::raw_ostream &OS,
                     const llvm::Module *Mod = NULL) const;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;

  virtual void verifyAnalysis() const {
    // Now it makes no sense to verify this analysis. If it will becomes more
    // complex, then some verification code should be written.
  }

private:
  void AnalyzeMemoryUsage(llvm::Instruction &I);

  size_t GetSizeLowerBound(llvm::Type &Ty);

private:
  std::string Kernel;
};

} // End namespace opencrun.

#endif // OPENCRUN_PASSES_FOOTPRINTESTIMATE_H
