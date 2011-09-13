
#include "opencrun/System/OS.h"
#include "opencrun/System/Hardware.h"

#include "llvm/Support/ErrorHandling.h"

#include <cstdlib>

#include <sys/mman.h>

using namespace opencrun::sys;

void *opencrun::sys::PageAlignedAlloc(size_t Size) {
  void *Addr;

  if(posix_memalign(&Addr, Hardware::GetPageSize(), Size))
    llvm::report_fatal_error("Unable to allocate page aligned memory");

  return Addr;
}

void *opencrun::sys::CacheAlignedAlloc(size_t Size) {
  void *Addr;

  if(posix_memalign(&Addr, Hardware::GetCacheLineSize(), Size))
    llvm::report_fatal_error("Unable to allocate cache aligned memory");

  return Addr;
}

void *opencrun::sys::NaturalAlignedAlloc(size_t Size) {
  void *Addr;

  if(posix_memalign(&Addr, Hardware::GetMaxNaturalAlignment(), Size))
    llvm::report_fatal_error("Unable to allocate natural aligned memory");

  return Addr;
}

void *opencrun::sys::Alloc(size_t Size) {
  void *Addr = std::malloc(Size);

  if(!Addr)
    llvm::report_fatal_error("Unable to allocate memory");

  return Addr;
}

void *opencrun::sys::CAlloc(size_t N, size_t Size) {
  void *Addr = std::calloc(N, Size);

  if(!Addr)
    llvm::report_fatal_error("Unable to allocate memory");

  return Addr;
}

void opencrun::sys::Free(void *Addr) {
  std::free(Addr);
}

void opencrun::sys::MarkPageReadOnly(void *Addr) {
  if(mprotect(Addr, Hardware::GetPageSize(), PROT_READ))
    llvm::report_fatal_error("Unable to mark page read-only");
}

void opencrun::sys::MarkPagesReadWrite(void *Addr, size_t Size) {
  if(mprotect(Addr, Size, PROT_READ | PROT_WRITE))
    llvm::report_fatal_error("Unable to mark pages read-write");
}
