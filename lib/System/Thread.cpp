
#include "opencrun/System/Thread.h"
#include "opencrun/System/Hardware.h"

#include <sched.h>

using namespace opencrun::sys;

extern "C" {
  void *ThreadLauncher(void* That) {
    Thread &Thr = *reinterpret_cast<Thread *>(That);

    if(const HardwareCPU *CPU = Thr.GetPinnedToCPU()) {
      cpu_set_t CPUs;

      CPU_ZERO(&CPUs);
      CPU_SET(CPU->GetCoreID(), &CPUs);

      sched_setaffinity(0, sizeof(cpu_set_t), &CPUs);
    }

    Thr.Run();
    return NULL;
  }
}

void Thread::Start() {
  pthread_create(&Thr, NULL, ThreadLauncher, this);
}

void Thread::Join() {
  pthread_join(Thr, NULL);
}
