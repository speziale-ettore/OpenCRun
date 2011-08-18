
#include "opencrun/Core/Context.h"
#include "opencrun/Core/CommandQueue.h"
#include "opencrun/Core/Device.h"
#include "opencrun/System/Env.h"

#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/Support/raw_ostream.h"

using namespace opencrun;

#define RETURN_WITH_ERROR(VAR, ERRCODE, MSG) \
  {                                          \
  ReportDiagnostic(MSG);                     \
  if(VAR)                                    \
    *VAR = ERRCODE;                          \
  return NULL;                               \
  }

Context::Context(Platform &Plat,
                 llvm::SmallVector<Device *, 2> &Devs,
                 ContextErrCallbackClojure &ErrNotify) :
  Devices(Devs),
  UserDiag(ErrNotify) {
  if(sys::HasEnv("OPENCRUN_INTERNAL_DIAGNOSTIC")) {
    clang::TextDiagnosticPrinter *DiagClient;
    
    DiagOptions.ShowColors = true;

    DiagClient = new clang::TextDiagnosticPrinter(llvm::errs(), DiagOptions);
    DiagClient->setPrefix("opencrun");

    llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagIDs;

    DiagIDs = new clang::DiagnosticIDs();
    Diag.reset(new clang::Diagnostic(DiagIDs, DiagClient));
  }
}

CommandQueue *Context::GetQueueForDevice(Device &Dev,
                                         bool OutOfOrder,
                                         bool EnableProfile,
                                         cl_int *ErrCode) {
  if(!std::count(Devices.begin(), Devices.end(), &Dev))
    RETURN_WITH_ERROR(ErrCode,
                      CL_INVALID_DEVICE,
                      "device not associated with this context");

  if(ErrCode)
    *ErrCode = CL_SUCCESS;

  if(OutOfOrder)
    return new OutOfOrderQueue(*this, Dev, EnableProfile);
  else
    return new InOrderQueue(*this, Dev, EnableProfile);
}

HostBuffer *Context::CreateHostBuffer(size_t Size,
                                      void *Storage,
                                      MemoryObj::AccessProtection AccessProt,
                                      cl_int *ErrCode) {
  return NULL;
}

HostAccessibleBuffer *Context::CreateHostAccessibleBuffer(
                                 size_t Size,
                                 void *Src,
                                 MemoryObj::AccessProtection AccessProt,
                                 cl_int *ErrCode) {
  return NULL;
}

DeviceBuffer *Context::CreateDeviceBuffer(
                         size_t Size,
                         void *Src,
                         MemoryObj::AccessProtection AccessProt,
                         cl_int *ErrCode) {
  DeviceBuffer *Buf = new DeviceBuffer(*this, Size, Src, AccessProt);
  bool Rollback = false;

  for(device_iterator I = Devices.begin(),
                      E = Devices.end();
                      I != E && !Rollback;
                      ++I)
    if(!(*I)->CreateDeviceBuffer(*Buf))
      Rollback = true;

  if(Rollback) {
    for(device_iterator I = Devices.begin(), E = Devices.end(); I != E; ++I)
      (*I)->DestroyMemoryObj(*Buf);

    delete Buf;

    RETURN_WITH_ERROR(ErrCode,
                      CL_OUT_OF_RESOURCES,
                      "failed allocating resources for device buffer");

  }
  
  if(ErrCode)
    *ErrCode = CL_SUCCESS;

  return Buf;
}

void Context::DestroyMemoryObj(MemoryObj &MemObj) {
  for(device_iterator I = Devices.begin(), E = Devices.end(); I != E; ++I)
    (*I)->DestroyMemoryObj(MemObj);
}

void Context::ReportDiagnostic(llvm::StringRef Msg) {
  if(Diag) {
    llvm::sys::ScopedLock Lock(DiagLock);

    Diag->Report(Diag->getCustomDiagID(clang::Diagnostic::Error, Msg));
  }

  UserDiag.Notify(Msg);
}

void Context::ReportDiagnostic(clang::TextDiagnosticBuffer &DiagInfo) {
  if(Diag) {
    llvm::sys::ScopedLock Lock(DiagLock);

    DiagInfo.FlushDiagnostics(*Diag);
  }
}
