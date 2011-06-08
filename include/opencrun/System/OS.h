
#ifndef OPENCRUN_SYSTEM_OS_H
#define OPENCRUN_SYSTEM_OS_H

#include "llvm/Support/DataTypes.h"

namespace opencrun {
namespace sys {

void *PageAlignedAlloc(size_t Size);
void *CacheAlignedAlloc(size_t Size, size_t &AllocSize);

void *Alloc(size_t Size);
void *CAlloc(size_t N, size_t Size);
void Free(void *Addr);

} // End namespace sys.
} // End namespace opencrun.

#endif // OPENCRUN_SYSTEM_OS_H
