
#ifndef OPENCRUN_CORE_CONTEXT_H
#define OPENCRUN_CORE_CONTEXT_H

#include "CL/opencl.h"

#include "opencrun/Core/MemoryObj.h"
#include "opencrun/Util/MTRefCounted.h"

#include "clang/Basic/Diagnostic.h"
#include "clang/Frontend/DiagnosticOptions.h"
#include "clang/Frontend/TextDiagnosticBuffer.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Mutex.h"

struct _cl_context { };

namespace opencrun {

class Platform;
class Device;
class CommandQueue;

class ContextErrCallbackClojure {
public:
  typedef void (CL_CALLBACK *Signature)(const char *,
                                        const void *,
                                        size_t,
                                        void *);
  typedef void *UserDataSignature;

public:
  ContextErrCallbackClojure(Signature Callback, UserDataSignature UserData) :
    Callback(Callback),
    UserData(UserData) { }

public:
  void Notify(llvm::StringRef Msg) const {
    if(Callback)
      Callback(Msg.begin(), NULL, 0, UserData);
  }

private:
  const Signature Callback;
  const UserDataSignature UserData;
};

class Context : public _cl_context, public MTRefCountedBase<Context> {
public:
  static bool classof(const _cl_context *Ctx) { return true; }

public:
  typedef llvm::SmallVector<Device *, 2> DevicesContainer;

  typedef DevicesContainer::iterator device_iterator;

public:
  device_iterator device_begin() { return Devices.begin(); }
  device_iterator device_end() { return Devices.end(); }

public:
  DevicesContainer::size_type devices_size() const { return Devices.size(); }

public:
  Context(Platform &Plat,
          DevicesContainer &Devs,
          ContextErrCallbackClojure &ErrNotify);

public:
  CommandQueue *GetQueueForDevice(Device &Dev,
                                  bool OutOfOrder,
                                  bool EnableProfile,
                                  cl_int *ErrCode = NULL);

  HostBuffer *CreateHostBuffer(size_t Size,
                               void *Storage,
                               MemoryObj::AccessProtection AccessProt,
                               cl_int *ErrCode = NULL);
  HostAccessibleBuffer *CreateHostAccessibleBuffer(
                          size_t Size,
                          void *Src,
                          MemoryObj::AccessProtection AccessProt,
                          cl_int *ErrCode = NULL);
  DeviceBuffer *CreateDeviceBuffer(size_t Size,
                                   void *Src,
                                   MemoryObj::AccessProtection AccessProt,
                                   cl_int *ErrCode = NULL);

  void DestroyBuffer(MemoryObj &MemObj);

  void ReportDiagnostic(llvm::StringRef Msg);
  void ReportDiagnostic(clang::TextDiagnosticBuffer &DiagInfo);

public:
  bool IsAssociatedWith(const Device &Dev) const {
    return std::count(Devices.begin(), Devices.end(), &Dev);
  }

public:
  bool operator==(const Context &That) const { return this == &That; }
  bool operator!=(const Context &That) const { return this != &That; }

private:
  DevicesContainer Devices;

  ContextErrCallbackClojure UserDiag;

  llvm::sys::Mutex DiagLock;
  clang::DiagnosticOptions DiagOptions;
  llvm::OwningPtr<clang::Diagnostic> Diag;
};

} // End namespace opencrun.

#endif // OPENCRUN_CORE_CONTEXT_H
