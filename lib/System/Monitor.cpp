
#include "opencrun/System/Monitor.h"

using namespace opencrun::sys;

Monitor::Monitor() {
  pthread_mutexattr_t Attrs;
  
  pthread_mutexattr_init(&Attrs);
  pthread_mutexattr_settype(&Attrs, PTHREAD_MUTEX_RECURSIVE_NP);

  pthread_mutex_init(&Lock, &Attrs);
  pthread_cond_init(&Cond, NULL);

  pthread_mutexattr_destroy(&Attrs);
}

Monitor::~Monitor() {
  pthread_mutex_destroy(&Lock);
  pthread_cond_destroy(&Cond);
}

void Monitor::Enter() { pthread_mutex_lock(&Lock); }

void Monitor::Exit() { pthread_mutex_unlock(&Lock); }

void Monitor::Wait() { pthread_cond_wait(&Cond, &Lock); }

void Monitor::Signal() { pthread_cond_signal(&Cond); } 

void Monitor::Broadcast() { pthread_cond_broadcast(&Cond); }
