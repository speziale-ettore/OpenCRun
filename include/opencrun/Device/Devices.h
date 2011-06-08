
#ifndef OPENCRUN_DEVICE_DEVICES_H
#define OPENCRUN_DEVICE_DEVICES_H

#include "opencrun/Device/CPU/CPUDevice.h"

namespace opencrun {

void GetCPUDevices(llvm::SmallPtrSet<CPUDevice *, 2> &CPUs);

} // End namespace opencrun.

#endif // OPENCRUN_DEVICE_DEVICES_H
