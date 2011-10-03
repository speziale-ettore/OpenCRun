
#ifndef OPENCRUN_DEVICE_PASSES_ALLPASSES_H
#define OPENCRUN_DEVICE_PASSES_ALLPASSES_H

#include "llvm/ADT/StringRef.h"

namespace opencrun {

class FootprintEstimator;
class AggressiveInliner;

FootprintEstimator *CreateFootprintEstimatorPass(llvm::StringRef Kernel = "");

AggressiveInliner *CreateAggressiveInlinerPass(llvm::StringRef Kernel = "");

} // End namespace opencrun.

extern "C" void LinkInOpenCRunPasses();

#endif // OPENCRUN_DEVICE_PASSES_ALLPASSES_H
