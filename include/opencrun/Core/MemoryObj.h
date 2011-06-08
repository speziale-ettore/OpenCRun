
#ifndef OPENCRUN_CORE_MEMORYOBJ_H
#define OPENCRUN_CORE_MEMORYOBJ_H

#include "CL/opencl.h"

#include "opencrun/Util/MTRefCounted.h"

struct _cl_mem { };

namespace opencrun {

class Context;

class MemoryObj : public _cl_mem, public MTRefCountedBaseVPTR<MemoryObj> {
public:
  enum Type {
    HostBuffer,
    HostAccessibleBuffer,
    DeviceBuffer,
    LastBuffer
  };

  enum AccessProtection {
    ReadWrite         = (1 << 0),
    WriteOnly         = (1 << 1),
    ReadOnly          = (1 << 2),
    InvalidProtection = 0
  };

public:
  static bool classof(const _cl_mem *MemObj) { return true; }

public:
  MemoryObj(Type MemTy,
            Context &Ctx,
            size_t Size,
            AccessProtection AccessProt) : MemTy(MemTy),
                                           Ctx(&Ctx),
                                           Size(Size),
                                           AccessProt(AccessProt) { }
  ~MemoryObj();

public:
  size_t GetSize() const { return Size; }
  Type GetType() const { return MemTy; }
  Context &GetContext() const { return *Ctx; }

private:
  Type MemTy;

  llvm::IntrusiveRefCntPtr<Context> Ctx;
  size_t Size;
  AccessProtection AccessProt;
};

class Buffer : public MemoryObj {
public:
  static bool classof(const MemoryObj *MemObj) {
    return MemObj->GetType() < MemoryObj::LastBuffer;
  }

protected:
  Buffer(Type MemTy,
         Context &Ctx,
         size_t Size,
         MemoryObj::AccessProtection AccessProt) :
  MemoryObj(MemTy, Ctx, Size, AccessProt) { }
};

class HostBuffer : public Buffer {
private:
  HostBuffer(Context &Ctx,
             size_t Size,
             void *Storage,
             MemoryObj::AccessProtection AccessProt)
    : Buffer(MemoryObj::HostBuffer, Ctx, Size, AccessProt) { }

  HostBuffer(const HostBuffer &That); // Do not implement.
  void operator=(const HostBuffer &That); // Do not implement.

  friend class Context;
};

class HostAccessibleBuffer : public Buffer {
private:
  HostAccessibleBuffer(Context &Ctx,
                       size_t Size,
                       void *Src,
                       MemoryObj::AccessProtection AccessProt)
    : Buffer(MemoryObj::HostAccessibleBuffer, Ctx, Size, AccessProt) { }

  HostAccessibleBuffer(const HostAccessibleBuffer &That); // Do not implement.
  void operator=(const HostAccessibleBuffer &That); // Do not implement.

  friend class Context;
};

class DeviceBuffer : public Buffer {
private:
  DeviceBuffer(Context &Ctx,
               size_t Size,
               void *Src,
               MemoryObj::AccessProtection AccessProt)
    : Buffer(MemoryObj::DeviceBuffer, Ctx, Size, AccessProt),
      Src(Src) { }

  DeviceBuffer(const DeviceBuffer &That); // Do not implement.
  void operator=(const DeviceBuffer &That); // Do not implement.

public:
  const void *GetInitializationData() const { return Src; }
  bool HasInitializationData() const { return Src; }

private:
  void *Src;

  friend class Context;
};

class BufferBuilder {
public:
  BufferBuilder(Context &Ctx, size_t Size);

public:
  BufferBuilder &SetUseHostMemory(bool Enabled, void* Storage);
  BufferBuilder &SetAllocHostMemory(bool Enabled);
  BufferBuilder &SetCopyHostMemory(bool Enabled, void* Src);
  BufferBuilder &SetReadWrite(bool Enabled);
  BufferBuilder &SetWriteOnly(bool Enabled);
  BufferBuilder &SetReadOnly(bool Enabled);

  Buffer *Create(cl_int *ErrCode = NULL);

private:
  BufferBuilder &NotifyError(cl_int ErrCode, const char *Msg = "");

private:
  Context &Ctx;
  size_t Size;

  bool UseHostMemory;
  bool AllocHost;
  bool CopyHost;
  void *HostPtr;

  MemoryObj::AccessProtection AccessProt;

  cl_int ErrCode;
};

} // End namespace opencrun.

#endif // OPENCRUN_CORE_MEMORYOBJ_H
