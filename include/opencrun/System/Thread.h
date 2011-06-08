
#ifndef OPENCRUN_SYSTEM_THREAD_H
#define OPENCRUN_SYSTEM_THREAD_H

#include <pthread.h>

extern "C" {

void *ThreadLauncher(void* That);

}

namespace opencrun {
namespace sys {

class HardwareCPU;

class Thread {
public:
  Thread() : PinnedTo(NULL) { };

  // Pin the thread to the given CPU.
  Thread(const sys::HardwareCPU &CPU) : PinnedTo(&CPU) { }

  Thread(const Thread &That); // Do not implement.
  void operator=(const Thread &That); // Do not implement.

  virtual ~Thread() { }

public:
  bool operator==(const Thread &That) const;
  bool operator<(const Thread &That) const;
  bool operator>(const Thread &That) const;

public:
  const sys::HardwareCPU *GetPinnedToCPU() const { return PinnedTo; }

protected:
  virtual void Run() { }
  void Start();
  void Join();

private:
  pthread_t Thr;
  const sys::HardwareCPU *PinnedTo;

  friend void *::ThreadLauncher(void *);
};

} // End namespace sys.
} // End namespace opencrun.

#endif // OPENCRUN_SYSTEM_THREAD_H
