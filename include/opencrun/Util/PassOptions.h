
#ifndef OPENCRUN_UTIL_PASSOPTIONS_H
#define OPENCRUN_UTIL_PASSOPTIONS_H

#include "llvm/ADT/StringRef.h"

namespace opencrun {

//
// Environment functions.
//

std::string GetKernelOption(llvm::StringRef Default = "");

} // End namespace opencrun.

#endif // OPENCRUN_UTIL_PASSOPTIONS_H
