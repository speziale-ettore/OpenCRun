
#ifndef OPENCRUN_DEVICE_CPU_MULTIPROCESSOR_H
#define OPENCRUN_DEVICE_CPU_MULTIPROCESSOR_H

#include "opencrun/Core/Profiler.h"
#include "opencrun/Device/CPU/Command.h"
#include "opencrun/Device/CPU/CPUThread.h"
#include "opencrun/System/Hardware.h"

#include "llvm/ADT/SmallPtrSet.h"

namespace opencrun {

class CPUDevice;

namespace cpu {

class Multiprocessor {
public:
  typedef llvm::SmallPtrSet<CPUThread *, 4> CPUThreadsContainer;

public:
  Multiprocessor(CPUDevice &Dev, const sys::HardwareCache &LLC);
  ~Multiprocessor();

  Multiprocessor(const Multiprocessor &That); // Do not implement.
  void operator=(const Multiprocessor &That); // Do not implement.

public:
  bool Submit(ReadBufferCPUCommand *Cmd);
  bool Submit(WriteBufferCPUCommand *Cmd);
  bool Submit(NDRangeKernelBlockCPUCommand *Cmd);
  bool Submit(NativeKernelCPUCommand *Cmd);

  bool Submit(RunStaticConstructorsCPUCommand *Cmd);
  bool Submit(RunStaticDestructorsCPUCommand *Cmd);

  void NotifyDone(CPUServiceCommand *Cmd);
  void NotifyDone(CPUExecCommand *Cmd, int ExitStatus);

private:
  CPUThread &GetLesserLoadedThread();

private:
  CPUDevice &Dev;
  CPUThreadsContainer Threads;

  friend class ProfilerTraits<Multiprocessor>;
};

} // End namespace cpu.
} // End namespace opencrun.

using namespace opencrun::cpu;

namespace opencrun {

template <>
class ProfilerTraits<Multiprocessor> {
public:
  static sys::Time ReadTime(Multiprocessor &Profilable);
};

} // End namespace opencrun.

#endif // OPENCRUN_DEVICE_CPU_MULTIPROCESSOR_H
