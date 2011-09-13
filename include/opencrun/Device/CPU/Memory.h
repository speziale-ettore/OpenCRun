
#ifndef OPENCRUN_DEVICE_CPU_MEMORY_H
#define OPENCRUN_DEVICE_CPU_MEMORY_H

#include "opencrun/Core/Context.h"
#include "opencrun/Core/MemoryObj.h"
#include "opencrun/System/OS.h"

#include "llvm/Support/Mutex.h"

#include <map>

using namespace opencrun;

namespace opencrun {
namespace cpu {

class Memory {
public:
  typedef std::map<MemoryObj *, void *> MappingsContainer;
};

class GlobalMemory : public Memory {
public:
  GlobalMemory(size_t Size);
  ~GlobalMemory();

public:
  void *Alloc(MemoryObj &MemObj);
  void *Alloc(HostBuffer &Buf);
  void *Alloc(HostAccessibleBuffer &Buf);
  void *Alloc(DeviceBuffer &Buf);

  void Free(MemoryObj &MemObj);

public:
  void GetMappings(MappingsContainer &Mappings) {
    llvm::sys::ScopedLock Lock(ThisLock);

    Mappings = this->Mappings;
  }

  void *operator[](MemoryObj &MemObj) {
    llvm::sys::ScopedLock Lock(ThisLock);

    return Mappings.count(&MemObj) ? Mappings[&MemObj] : NULL;
  }

private:
  llvm::sys::Mutex ThisLock;
  MappingsContainer Mappings;

  size_t Size;
  size_t Available;
};

} // End namespace cpu.
} // End namespace opencrun.

#endif // OPENCRUN_DEVICE_CPU_MEMORY_H
