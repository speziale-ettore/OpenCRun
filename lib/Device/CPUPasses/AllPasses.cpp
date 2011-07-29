
#include "opencrun/Device/CPUPasses/AllPasses.h"

using namespace opencrun;

// Never called, however we reference all passes in order to force linking.
extern "C" void LinkInOpenCRunCPUPasses() {
  CreateGroupParallelStubPass();
  CreateBlockSplittingPass();
}
