
#include "opencrun/Device/CPU/Multiprocessor.h"
#include "opencrun/Device/CPU/CPUDevice.h"
#include "opencrun/System/OS.h"

#include <float.h>

using namespace opencrun;
using namespace opencrun::cpu;

//
// Multiprocessor implementation.
//

Multiprocessor::Multiprocessor(CPUDevice &Dev, const sys::HardwareCache &LLC)
  : Dev(Dev) {
  for(sys::HardwareCache::const_cpu_iterator I = LLC.cpu_begin(),
                                             E = LLC.cpu_end();
                                             I != E;
                                             ++I)
    Threads.insert(new CPUThread(*this, *I));
}

Multiprocessor::~Multiprocessor() {
  llvm::DeleteContainerPointers(Threads);
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
