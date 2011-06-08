
#ifndef OPENCRUN_CORE_PLATFORM_H
#define OPENCRUN_CORE_PLATFORM_H

#include "opencrun/Device/Devices.h"

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/StringRef.h"

struct _cl_platform_id { };

namespace opencrun {

class PlatformInfo {
public:
  llvm::StringRef GetProfile() const { return "FULL_PROFILE"; }
  llvm::StringRef GetVersion() const { return "OpenCL 1.1"; }
  llvm::StringRef GetName() const { return "OpenCRun"; }
  llvm::StringRef GetVendor() const { return "UC 2.0"; }
  llvm::StringRef GetExtensions() const { return ""; }
};

class Platform : public _cl_platform_id, public PlatformInfo {
public:
  static bool classof(const _cl_platform_id *Plat) { return true; }

public:
  typedef llvm::SmallPtrSet<CPUDevice *, 2> CPUsContainer;

  typedef CPUsContainer::iterator cpu_iterator;

  typedef llvm::SmallPtrSet<GPUDevice *, 2> GPUsContainer;

  typedef GPUsContainer::iterator gpu_iterator;

  typedef llvm::SmallPtrSet<AcceleratorDevice *, 2> AcceleratorsContainer;

  typedef AcceleratorsContainer::iterator accelerator_iterator;

public:
  cpu_iterator cpu_begin() { return CPUs.begin(); }
  cpu_iterator cpu_end() { return CPUs.end(); }

  gpu_iterator gpu_begin() { return GPUs.begin(); }
  gpu_iterator gpu_end() { return GPUs.end(); }

  accelerator_iterator accelerator_begin() { return Accelerators.begin(); }
  accelerator_iterator accelerator_end() { return Accelerators.end(); }

public:
  Platform();
  Platform(const Platform &That); // Do not implement.
  void operator=(const Platform &That); // Do not implement.

private:
  CPUsContainer CPUs;
  GPUsContainer GPUs;
  AcceleratorsContainer Accelerators;
};

Platform &GetOpenCRunPlatform();

} // End namespace opencrun.

#endif // OPENCRUN_CORE_PLATFORM_H
