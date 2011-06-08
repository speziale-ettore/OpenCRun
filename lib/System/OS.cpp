
#include "opencrun/System/OS.h"
#include "opencrun/System/Hardware.h"

#include "llvm/Support/ErrorHandling.h"

#include <cstdlib>

using namespace opencrun::sys;

void *opencrun::sys::PageAlignedAlloc(size_t Size) {
  void *Addr;

  if(posix_memalign(&Addr, Hardware::GetPageSize(), Size))
    llvm::report_fatal_error("Unable to allocate page aligned memory");

  return Addr;
}

void *opencrun::sys::CacheAlignedAlloc(size_t Size, size_t &AllocSize) {
  size_t CacheLineSize = Hardware::GetCacheLineSize();
  void *Addr;

  if(posix_memalign(&Addr, CacheLineSize, Size))
    llvm::report_fatal_error("Unable to allocate cache aligned memory");

  AllocSize = Size + Size % CacheLineSize;

  return Addr;
}

void *opencrun::sys::Alloc(size_t Size) {
  void *Addr;

  Addr = std::malloc(Size);

  if(!Addr)
    llvm::report_fatal_error("Unable to allocate memory");

  return Addr;
}

void *opencrun::sys::CAlloc(size_t N, size_t Size) {
  void *Addr;

  Addr = std::calloc(N, Size);

  if(!Addr)
    llvm::report_fatal_error("Unable to allocate memory");

  return Addr;
}

void opencrun::sys::Free(void *Addr) {
  std::free(Addr);
}
