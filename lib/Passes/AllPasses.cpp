
#include "opencrun/Passes/AllPasses.h"

#include "llvm/PassRegistry.h"

using namespace opencrun;

// Never called, however we reference all passes in order to force linking.
extern "C" void LinkInOpenCRunPasses() {
  // Analysis.
  CreateFootprintEstimatePass();

  // Transform.
  CreateAggressiveInlinerPass();
}

namespace {

class ForceInitialization {
public:
  ForceInitialization() {
    llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();

    initializeFootprintEstimatePass(Registry);

    initializeAggressiveInlinerPass(Registry);
  }
};

ForceInitialization FI;

} // End anonymous namespace.
