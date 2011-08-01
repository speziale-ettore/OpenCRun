
#ifndef OPENCRUN_DEVICE_CPUPASSES_PASSUTILS_H
#define OPENCRUN_DEVICE_CPUPASSES_PASSUTILS_H

#include "llvm/ADT/SmallString.h"
#include "llvm/Instructions.h"

#include <string>

namespace opencrun {

//
// Environment functions.
//

std::string GetKernelOption(llvm::StringRef Default = "");
bool GetVerifyCPUPassOption();

} // End namespace opencrun.

#endif // OPENCRUN_DEVICE_CPUPASSES_PASSUTILS_H
