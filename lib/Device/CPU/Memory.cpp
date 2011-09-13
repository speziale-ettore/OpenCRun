
#include "opencrun/Device/CPU/Memory.h"

using namespace opencrun;
using namespace opencrun::cpu;

//
// GlobalMemory implementation.
//

GlobalMemory::GlobalMemory(size_t Size) : Size(Size), Available(Size) { }

GlobalMemory::~GlobalMemory() {
  for(MappingsContainer::iterator I = Mappings.begin(),
                                  E = Mappings.end();
                                  I != E;
                                  ++I)
    sys::Free(I->second);
}

void *GlobalMemory::Alloc(MemoryObj &MemObj) {
  size_t RequestedSize = MemObj.GetSize();

  llvm::sys::ScopedLock Lock(ThisLock);

  if(Available < RequestedSize)
    return NULL;

  void *Addr = sys::CacheAlignedAlloc(RequestedSize);

  if(Addr) {
    Mappings[&MemObj] = Addr;
    Available -= RequestedSize;
  }

  return Addr;
}

void *GlobalMemory::Alloc(HostBuffer &Buf) {
  return NULL;
}

void *GlobalMemory::Alloc(HostAccessibleBuffer &Buf) {
  return NULL;
}

void *GlobalMemory::Alloc(DeviceBuffer &Buf) {
  void *Addr = Alloc(llvm::cast<MemoryObj>(Buf));

  if(Buf.HasInitializationData())
    std::memcpy(Addr, Buf.GetInitializationData(), Buf.GetSize());

  return Addr;
}

void GlobalMemory::Free(MemoryObj &MemObj) {
  llvm::sys::ScopedLock Lock(ThisLock);

  MappingsContainer::iterator I = Mappings.find(&MemObj);
  if(!Mappings.count(&MemObj))
    return;

  Available += MemObj.GetSize();

  sys::Free(I->second);
  Mappings.erase(I);
}
