
#ifndef OPENCRUN_SYSTEM_MONITOR_H
#define OPENCRUN_SYSTEM_MONITOR_H

#include <pthread.h>

namespace opencrun {
namespace sys {

class Monitor {
public:
  Monitor();
  ~Monitor();

private:
  Monitor(const Monitor &That); // Do not implement.
  void operator=(const Monitor &That); // Do not implement.

public:
  void Enter();
  void Exit();
  void Wait();
  void Signal();
  void Broadcast();

private:
  pthread_mutex_t Lock;
  pthread_cond_t Cond;
};

class ScopedMonitor {
public:
  ScopedMonitor(Monitor &Mnt) : Mnt(Mnt) { Enter(); }
  ~ScopedMonitor() { Exit(); }

public:
  void Enter() { Mnt.Enter(); }
  void Exit() { Mnt.Exit(); }
  void Wait() { Mnt.Wait(); }
  void Signal() { Mnt.Signal(); }
  void Broadcast() { Mnt.Broadcast(); }

private:
  Monitor &Mnt;
};

} // End namespace sys.
} // End namespace opencrun.

#endif // OPENCRUN_SYSTEM_MONITOR_H
