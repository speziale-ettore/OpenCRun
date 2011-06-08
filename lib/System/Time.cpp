
#include "opencrun/System/Time.h"

#include <time.h>

using namespace opencrun::sys;

Time Time::GetWallClock() {
  struct timespec Now;

  clock_gettime(CLOCK_REALTIME, &Now);

  return Time(Now.tv_sec, Now.tv_nsec);
}

const int64_t Time::NanoInSeconds = 1e9;
