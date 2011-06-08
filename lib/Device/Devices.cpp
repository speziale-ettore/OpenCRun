
#include "opencrun/Device/Devices.h"

#include "llvm/Support/ManagedStatic.h"

using namespace opencrun;

//
// CPUContainer implementation.
//

namespace {

class CPUContainer {
public:
  typedef llvm::SmallPtrSet<CPUDevice *, 2> CPUsContainer;

public:
  CPUContainer() {
    sys::Hardware &HW = sys::GetHardware();

    // A device for each NUMA node.
    for(sys::Hardware::node_iterator I = HW.node_begin(),
                                     E = HW.node_end();
                                     I != E;
                                     ++I)
      CPUs.insert(new CPUDevice(*I));
  }

  ~CPUContainer() { llvm::DeleteContainerPointers(CPUs); }

public:
  CPUsContainer &GetSystemCPUs() { return CPUs; }

private:
  CPUsContainer CPUs;
};

} // End anonymous namespace.

llvm::ManagedStatic<CPUContainer> CPUDevices;

void opencrun::GetCPUDevices(llvm::SmallPtrSet<CPUDevice *,2> &CPUs) {
    CPUs = CPUDevices->GetSystemCPUs();
}
