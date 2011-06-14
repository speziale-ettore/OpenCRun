
#ifndef OPENCRUN_DEVICE_CPU_MULTIPROCESSOR_H
#define OPENCRUN_DEVICE_CPU_MULTIPROCESSOR_H

#include "opencrun/Core/Profiler.h"
#include "opencrun/Device/CPU/Command.h"
#include "opencrun/System/Hardware.h"
#include "opencrun/System/Monitor.h"
#include "opencrun/System/Thread.h"

#include "llvm/ADT/SmallPtrSet.h"

#include <deque>

namespace opencrun {

class CPUDevice;

namespace cpu {

class Multiprocessor;

class CPUThread : public sys::Thread {
public:
  typedef std::deque<CPUCommand *> CPUCommands;

  enum WorkingMode {
    FullyOperational = 1 << 0,
    TearDown = 1 << 1,
    Stopped = 1 << 2,
    NoNewJobs = TearDown | Stopped,
    ExecJobs = FullyOperational | TearDown
  };

public:
  CPUThread(Multiprocessor &MP, const sys::HardwareCPU &CPU);
  virtual ~CPUThread();

public:
  bool Submit(CPUCommand *Cmd);

  virtual void Run();

public:
  float GetLoadIndicator();

  size_t GetGlobalId(unsigned I) { return Index->GetGlobalId(I); }

private:
  bool Submit(CPUServiceCommand *Cmd);
  bool Submit(RunStaticConstructorsCPUCommand *Cmd) { return true; }
  bool Submit(RunStaticDestructorsCPUCommand *Cmd) { return true; }
  bool Submit(StopDeviceCPUCommand *Cmd) { Mode = TearDown; return true; }

  bool Submit(CPUExecCommand *Cmd);
  bool Submit(ReadBufferCPUCommand *Cmd) { return true; }
  bool Submit(WriteBufferCPUCommand *Cmd) { return true; }
  bool Submit(NDRangeKernelBlockCPUCommand *Cmd) { return true; }
  bool Submit(NativeKernelCPUCommand *Cmd) { return true; }

  void Execute(CPUCommand *Cmd);

  void Execute(CPUServiceCommand *Cmd);
  void Execute(RunStaticConstructorsCPUCommand *Cmd);
  void Execute(RunStaticDestructorsCPUCommand *Cmd);
  void Execute(StopDeviceCPUCommand *Cmd) { Mode = Stopped; }

  void Execute(CPUExecCommand *Cmd);
  int Execute(ReadBufferCPUCommand &Cmd);
  int Execute(WriteBufferCPUCommand &Cmd);
  int Execute(NDRangeKernelBlockCPUCommand &Cmd);
  int Execute(NativeKernelCPUCommand &Cmd);

private:
  sys::Monitor ThisMnt;

  volatile WorkingMode Mode;
  CPUCommands Commands;
  Multiprocessor &MP;

  DimensionInfo::iterator *Index;
};

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

  void *LocalMemory;
  size_t LocalMemorySize;

  friend class ProfilerTraits<Multiprocessor>;
};

CPUThread &GetCurrentThread();

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
