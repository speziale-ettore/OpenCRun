
#include "opencrun/Passes/AllPasses.h"

using namespace opencrun;

// Never called, however we reference all passes in order to force linking.
extern "C" void LinkInOpenCRunPasses() {
  // Analysis.
  CreateFootprintEstimatorPass();

  // Transform.
  CreateAggressiveInlinerPass();
}
