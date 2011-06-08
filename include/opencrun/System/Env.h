
#ifndef OPENCRUN_SYSTEM_ENV_H
#define OPENCRUN_SYSTEM_ENV_H

#include "llvm/ADT/StringRef.h"

#include <string>

namespace opencrun {
namespace sys {

bool HasEnv(llvm::StringRef Name);
std::string GetEnv(llvm::StringRef Name);

} // End namespace sys.
} // End namespace opencrun.

#endif // OPENCRUN_SYSTEM_ENV_H
