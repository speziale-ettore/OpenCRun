
#include "opencrun/Core/Platform.h"

#include "llvm/Support/ManagedStatic.h"

using namespace opencrun;

namespace {
class ForceInitialization {
public:
  ForceInitialization() { llvm::llvm_start_multithreaded(); }
  ~ForceInitialization() { llvm::llvm_shutdown(); }
};

ForceInitialization FI;
}

Platform::Platform() {
  GetCPUDevices(CPUs);
}

static llvm::ManagedStatic<Platform> OpenCRunPlat;

Platform &opencrun::GetOpenCRunPlatform() {
  return *OpenCRunPlat;
}
