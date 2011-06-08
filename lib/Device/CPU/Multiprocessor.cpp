
#include "opencrun/Device/CPU/Multiprocessor.h"
#include "opencrun/Device/CPU/CPUDevice.h"
#include "opencrun/Core/Event.h"
#include "opencrun/System/OS.h"

#include <float.h>

using namespace opencrun;
using namespace opencrun::cpu;

//
// CPUThread implementation.
//

CPUThread::~CPUThread() {
  // Send a command to stop the device thread. Do not employ sys:FastRendevouz
  // because potentially we can wait for a long time.
  CPUCommand *Cmd = new StopDeviceCPUCommand();
  Submit(Cmd);

  // Use the Thread::Join method instead, that suspend the current thread until
  // the joined thread exits.
  Join();
}

bool CPUThread::Submit(CPUCommand *Cmd) {
  bool Enqueued;

  sys::ScopedMonitor Mnt(ThisMnt);

  if(Mode & NoNewJobs)
    return false;

  if(CPUServiceCommand *Enq = llvm::dyn_cast<CPUServiceCommand>(Cmd))
    Enqueued = Submit(Enq);
  else if(CPUExecCommand *Enq = llvm::dyn_cast<CPUExecCommand>(Cmd))
    Enqueued = Submit(Enq);
  else
    Enqueued = false;

  if(Enqueued) {
    Commands.push_back(Cmd);
    Mnt.Signal();
  }

  return Enqueued;
}

void CPUThread::Run() {
  while(Mode & ExecJobs) {
    ThisMnt.Enter();

    while(Commands.empty())
      ThisMnt.Wait();

    CPUCommand *Cmd = Commands.front();
    Commands.pop_front();

    ThisMnt.Exit();

    Execute(Cmd);
  }
}

float CPUThread::GetLoadIndicator() {
  sys::ScopedMonitor Mnt(ThisMnt);

  return Commands.size();
}

bool CPUThread::Submit(CPUServiceCommand *Cmd) {
  if(RunStaticConstructorsCPUCommand *Ctor =
       llvm::dyn_cast<RunStaticConstructorsCPUCommand>(Cmd))
    return Submit(Ctor);

  else if(RunStaticDestructorsCPUCommand *Dtor =
            llvm::dyn_cast<RunStaticDestructorsCPUCommand>(Cmd))
    return Submit(Dtor);

  else if(StopDeviceCPUCommand *Enq = llvm::dyn_cast<StopDeviceCPUCommand>(Cmd))
    return Submit(Enq);

  else
    llvm::report_fatal_error("unknown command submitted");

  return false;
}

bool CPUThread::Submit(CPUExecCommand *Cmd) {
  if(ReadBufferCPUCommand *Read = llvm::dyn_cast<ReadBufferCPUCommand>(Cmd))
    return Submit(Read);

  else if(WriteBufferCPUCommand *Write =
            llvm::dyn_cast<WriteBufferCPUCommand>(Cmd))
    return Submit(Write);

  else if(NDRangeKernelBlockCPUCommand *NDBlock =
            llvm::dyn_cast<NDRangeKernelBlockCPUCommand>(Cmd))
    return Submit(NDBlock);

  else if(NativeKernelCPUCommand *Native =
            llvm::dyn_cast<NativeKernelCPUCommand>(Cmd))
    return Submit(Native);

  else
    llvm::report_fatal_error("unknown command submitted");

  return false;
}

void CPUThread::Execute(CPUCommand *Cmd) {
  if(CPUServiceCommand *OnFly = llvm::dyn_cast<CPUServiceCommand>(Cmd))
    Execute(OnFly);
  else if(CPUExecCommand *OnFly = llvm::dyn_cast<CPUExecCommand>(Cmd))
    Execute(OnFly);
}

void CPUThread::Execute(CPUServiceCommand *Cmd) {
  if(RunStaticConstructorsCPUCommand *OnFly =
       llvm::dyn_cast<RunStaticConstructorsCPUCommand>(Cmd))
    Execute(OnFly);

  else if(RunStaticDestructorsCPUCommand *OnFly =
            llvm::dyn_cast<RunStaticDestructorsCPUCommand>(Cmd))
    Execute(OnFly);

  else if(StopDeviceCPUCommand *OnFly =
            llvm::dyn_cast<StopDeviceCPUCommand>(Cmd))
    Execute(OnFly);

  MP.NotifyDone(Cmd);
}

void CPUThread::Execute(RunStaticConstructorsCPUCommand *Cmd) {
  RunStaticConstructorsCPUCommand::ConstructorsInvoker &Invoker =
    Cmd->GetInvoker();

  Invoker();
}

void CPUThread::Execute(RunStaticDestructorsCPUCommand *Cmd) {
  RunStaticDestructorsCPUCommand::DestructorsInvoker &Invoker =
    Cmd->GetInvoker();

  Invoker();
}

void CPUThread::Execute(CPUExecCommand *Cmd) {
  InternalEvent &Ev = Cmd->GetQueueCommand().GetNotifyEvent();
  unsigned Counters = Cmd->IsProfiled() ? Profiler::Time : Profiler::None;
  int ExitStatus;

  ProfileSample *Sample = GetProfilerSample(MP,
                                            Counters,
                                            ProfileSample::CommandRunning);
  Ev.MarkRunning(Sample);

  if(ReadBufferCPUCommand *OnFly = llvm::dyn_cast<ReadBufferCPUCommand>(Cmd))
    ExitStatus = Execute(*OnFly);

  else if(WriteBufferCPUCommand *OnFly =
            llvm::dyn_cast<WriteBufferCPUCommand>(Cmd))
    ExitStatus = Execute(*OnFly);

  else if(NDRangeKernelBlockCPUCommand *OnFly =
            llvm::dyn_cast<NDRangeKernelBlockCPUCommand>(Cmd))
    ExitStatus = Execute(*OnFly);

  else if(NativeKernelCPUCommand *OnFly =
            llvm::dyn_cast<NativeKernelCPUCommand>(Cmd))
    ExitStatus = Execute(*OnFly);
  
  else
    ExitStatus = CPUCommand::Unsupported;
  
  MP.NotifyDone(Cmd, ExitStatus);
}

int CPUThread::Execute(ReadBufferCPUCommand &Cmd) {
  std::memcpy(Cmd.GetTarget(), Cmd.GetSource(), Cmd.GetSize());

  return CPUCommand::NoError;
}

int CPUThread::Execute(WriteBufferCPUCommand &Cmd) {
  std::memcpy(Cmd.GetTarget(), Cmd.GetSource(), Cmd.GetSize());

  return CPUCommand::NoError;
}

int CPUThread::Execute(NDRangeKernelBlockCPUCommand &Cmd) {
  NDRangeKernelBlockCPUCommand::Signature Func = Cmd.GetFunction();
  void **Args = Cmd.GetArgumentsPointer();

  Func(Args);

  return CPUCommand::NoError;
}

int CPUThread::Execute(NativeKernelCPUCommand &Cmd) {
  NativeKernelCPUCommand::Signature Func = Cmd.GetFunction();
  void *Args = Cmd.GetArgumentsPointer();

  Func(Args);

  return CPUCommand::NoError;
}

//
// Multiprocessor implementation.
//

Multiprocessor::Multiprocessor(CPUDevice &Dev, const sys::HardwareCache &LLC)
  : Dev(Dev),
    LocalMemorySize(LLC.GetSize()) {
  for(sys::HardwareCache::const_cpu_iterator I = LLC.cpu_begin(),
                                             E = LLC.cpu_end();
                                             I != E;
                                             ++I) {
    Threads.insert(new CPUThread(*this, *I));
  }

  LocalMemory = sys::PageAlignedAlloc(LocalMemorySize);
}

Multiprocessor::~Multiprocessor() {
  llvm::DeleteContainerPointers(Threads);
  std::free(LocalMemory);
}

bool Multiprocessor::Submit(ReadBufferCPUCommand *Cmd) {
  // TODO: implement a smarter selection policy.
  CPUThread &Thr = **Threads.begin();

  return Thr.Submit(static_cast<CPUCommand *>(Cmd));
}

bool Multiprocessor::Submit(WriteBufferCPUCommand *Cmd) {
  // TODO: implement a smarter selection policy.
  CPUThread &Thr = **Threads.begin();

  return Thr.Submit(static_cast<CPUCommand *>(Cmd));
}

bool Multiprocessor::Submit(NDRangeKernelBlockCPUCommand *Cmd) {
  CPUThread &Thr = GetLesserLoadedThread();

  return Thr.Submit(static_cast<CPUCommand *>(Cmd));
}

bool Multiprocessor::Submit(NativeKernelCPUCommand *Cmd) {
  // TODO: implement a smarter selection policy.
  CPUThread &Thr = **Threads.begin();

  return Thr.Submit(static_cast<CPUCommand *>(Cmd));
}

bool Multiprocessor::Submit(RunStaticConstructorsCPUCommand *Cmd) {
  CPUThread &Thr = **Threads.begin();

  return Thr.Submit(static_cast<CPUCommand *>(Cmd));
}

bool Multiprocessor::Submit(RunStaticDestructorsCPUCommand *Cmd) {
  CPUThread &Thr = **Threads.begin();

  return Thr.Submit(static_cast<CPUCommand *>(Cmd));
}

void Multiprocessor::NotifyDone(CPUServiceCommand *Cmd) {
  Dev.NotifyDone(Cmd);
}

void Multiprocessor::NotifyDone(CPUExecCommand *Cmd, int ExitStatus) {
  Dev.NotifyDone(Cmd, ExitStatus);
}

CPUThread &Multiprocessor::GetLesserLoadedThread() {
  CPUThreadsContainer::iterator I = Threads.begin(),
                                E = Threads.end();
  CPUThread *Thr = *I;
  float MinLoad = FLT_MAX;

  for(; I != E; ++I)
    if((*I)->GetLoadIndicator() < MinLoad)
      Thr = *I;

  return *Thr;
}

//
// ProfilerTraits<Multiprocessor> implementation.
//

sys::Time ProfilerTraits<Multiprocessor>::ReadTime(Multiprocessor &Profilable) {
  return ProfilerTraits<CPUDevice>::ReadTime(Profilable.Dev);
}
