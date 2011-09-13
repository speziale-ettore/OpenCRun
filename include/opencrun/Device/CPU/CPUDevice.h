
#ifndef OPENCRUN_DEVICE_CPU_CPUDEVICE_H
#define OPENCRUN_DEVICE_CPU_CPUDEVICE_H

#include "opencrun/Core/Device.h"
#include "opencrun/Device/CPU/Memory.h"
#include "opencrun/Device/CPU/Multiprocessor.h"

#include "llvm/ExecutionEngine/JIT.h"

using namespace opencrun::cpu;

namespace opencrun {

class CPUDevice : public Device {
public:
  static bool classof(const Device *Dev) { return true; }

public:
  typedef NDRangeKernelBlockCPUCommand::Signature BlockParallelEntryPoint;

  typedef std::map<Kernel *, BlockParallelEntryPoint> BlockParallelEntryPoints;

  typedef llvm::DenseMap<unsigned, void *> GlobalArgMappingsContainer;

public:
  typedef llvm::SmallPtrSet<Multiprocessor *, 4> MultiprocessorsContainer;

public:
  CPUDevice(sys::HardwareNode &Node);
  ~CPUDevice();

public:
  virtual bool CreateHostBuffer(HostBuffer &Buf);
  virtual bool CreateHostAccessibleBuffer(HostAccessibleBuffer &Buf);
  virtual bool CreateDeviceBuffer(DeviceBuffer &Buf);

  virtual void DestroyMemoryObj(MemoryObj &MemObj);

  virtual bool Submit(Command &Cmd);

  virtual void UnregisterKernel(Kernel &Kern);

  void NotifyDone(CPUServiceCommand *Cmd) { delete Cmd; }
  void NotifyDone(CPUExecCommand *Cmd, int ExitStatus);

private:
  void InitDeviceInfo(sys::HardwareNode &Node);
  void InitJIT();
  void InitInternalCalls();
  void InitMultiprocessors(sys::HardwareNode &Node);

  void DestroyJIT();
  void DestroyMultiprocessors();

  bool Submit(EnqueueReadBuffer &Cmd);
  bool Submit(EnqueueWriteBuffer &Cmd);
  bool Submit(EnqueueNDRangeKernel &Cmd);
  bool Submit(EnqueueNativeKernel &Cmd);

  bool BlockParallelSubmit(EnqueueNDRangeKernel &Cmd,
                           GlobalArgMappingsContainer &GlobalArgs);

  CPUDevice::BlockParallelEntryPoint GetBlockParallelEntryPoint(Kernel &Kern);
  void *LinkLibFunction(const std::string &Name);

  void LocateMemoryObjArgAddresses(Kernel &Kern,
                                   GlobalArgMappingsContainer &GlobalArgs);

  std::string MangleBlockParallelKernelName(llvm::StringRef Name) {
    return MangleKernelName("_GroupParallelStub_", Name);
  }

  std::string MangleKernelName(llvm::StringRef Prefix, llvm::StringRef Name) {
    return Prefix.str() + Name.str();
  }

private:
  MultiprocessorsContainer Multiprocessors;
  GlobalMemory Global;

  llvm::OwningPtr<llvm::ExecutionEngine> JIT;

  BlockParallelEntryPoints BlockParallelEntriesCache;

  friend void *LibLinker(const std::string &);
};

void *LibLinker(const std::string &Name);

template <>
class ProfilerTraits<CPUDevice> {
public:
  static sys::Time ReadTime(CPUDevice &Profilable) {
    return sys::Time::GetWallClock();
  }
};

} // End namespace opencrun.

#endif // OPENCRUN_DEVICE_CPU_CPUDEVICE_H
