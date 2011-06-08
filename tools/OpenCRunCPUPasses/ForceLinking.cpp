
#include "opencrun/Device/CPUPasses/AllPasses.h"

using namespace opencrun;

namespace {

class ForceLinking {
public:
  ForceLinking() {
    // We must reference the passes in such a way that compilers will not
    // delete it all as dead code, even with whole program optimization,
    // yet is effectively a NO-OP. As the compiler isn't smart enough
    // to know that getenv() never returns -1, this will do the job.
    if(std::getenv("bar") != (char*) -1)
      return;

    LinkInOpenCRunCPUPasses();
  }
};

ForceLinking Link;

} // End anonymous namespace.
