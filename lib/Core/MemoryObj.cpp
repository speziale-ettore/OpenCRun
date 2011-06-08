
#include "opencrun/Core/MemoryObj.h"
#include "opencrun/Core/Context.h"
#include "opencrun/Core/Device.h"

using namespace opencrun;

//
// MemoryObj implementation.
//

MemoryObj::~MemoryObj() {
  Ctx->DestroyBuffer(*this);
}

//
// BufferBuilder implementation.
//

#define RETURN_WITH_ERROR(VAR) \
  {                            \
  if(VAR)                      \
    *VAR = this->ErrCode;      \
  return NULL;                 \
  }

BufferBuilder::BufferBuilder(Context &Ctx, size_t Size) :
  Ctx(Ctx),
  Size(Size),
  UseHostMemory(false),
  AllocHost(false),
  CopyHost(false),
  HostPtr(NULL),
  AccessProt(MemoryObj::InvalidProtection),
  ErrCode(CL_SUCCESS) {
  if(!Size) {
    NotifyError(CL_INVALID_BUFFER_SIZE, "buffer size must be greater than 0");
    return;
  }

  for(Context::device_iterator I = Ctx.device_begin(), E = Ctx.device_end();
                               I != E;
                               ++I)
    if(Size > (*I)->GetMaxMemoryAllocSize()) {
      NotifyError(CL_INVALID_BUFFER_SIZE,
                  "buffer size exceed device capabilities");
      return;
    }
}

BufferBuilder &BufferBuilder::SetUseHostMemory(bool Enabled, void* Storage) {
  if(Enabled) {
    if(!Storage)
      return NotifyError(CL_INVALID_HOST_PTR, "missing host storage pointer");

    if(AllocHost || CopyHost)
      return NotifyError(CL_INVALID_VALUE,
                         "multiple buffer storage specifiers not allowed");

    UseHostMemory = true;
    HostPtr = Storage;
  }

  return *this;
}

BufferBuilder &BufferBuilder::SetAllocHostMemory(bool Enabled) {
  if(Enabled) {
    if(UseHostMemory)
      return NotifyError(CL_INVALID_VALUE,
                         "multiple buffer storage specifiers not allowed");

    AllocHost = true;
  }

  return *this;
}

BufferBuilder &BufferBuilder::SetCopyHostMemory(bool Enabled, void* Src) {
  if(Enabled) {
    if(!Src)
      return NotifyError(CL_INVALID_HOST_PTR,
                         "missed pointer to initialization data");

    if(UseHostMemory)
      return NotifyError(CL_INVALID_VALUE,
                         "multiple buffer storage specifiers not allowed");

    CopyHost = true;
    HostPtr = Src;
  }

  return *this;
}

BufferBuilder &BufferBuilder::SetReadWrite(bool Enabled) {
  if(Enabled) {
    if(AccessProt == MemoryObj::ReadOnly || AccessProt == MemoryObj::WriteOnly)
      return NotifyError(CL_INVALID_VALUE,
                         "multiple access protection flags not allowed");

    AccessProt = MemoryObj::ReadWrite;
  }

  return *this;
}

BufferBuilder &BufferBuilder::SetWriteOnly(bool Enabled) {
  if(Enabled) {
    if(AccessProt == MemoryObj::ReadWrite || AccessProt == MemoryObj::ReadOnly)
      return NotifyError(CL_INVALID_VALUE,
                         "multiple access protection flags not allowed");

    AccessProt = MemoryObj::WriteOnly;
  }

  return *this;
}

BufferBuilder &BufferBuilder::SetReadOnly(bool Enabled) {
  if(Enabled) {
    if(AccessProt == MemoryObj::ReadWrite || AccessProt == MemoryObj::WriteOnly)
      return NotifyError(CL_INVALID_VALUE,
                         "multiple access protection flags not allowed");

    AccessProt = MemoryObj::ReadOnly;
  }

  return *this;
}

Buffer *BufferBuilder::Create(cl_int *ErrCode) {
  if(this->ErrCode != CL_SUCCESS)
    RETURN_WITH_ERROR(ErrCode);

  if(UseHostMemory)
    return Ctx.CreateHostBuffer(Size, HostPtr, AccessProt, ErrCode);
  else if(AllocHost)
    return Ctx.CreateHostAccessibleBuffer(Size, HostPtr, AccessProt, ErrCode);
  else
    return Ctx.CreateDeviceBuffer(Size, HostPtr, AccessProt, ErrCode);
}

BufferBuilder &BufferBuilder::NotifyError(cl_int ErrCode, const char *Msg) {
  Ctx.ReportDiagnostic(Msg);
  this->ErrCode = ErrCode;

  return *this;
}
