
#include "opencrun/Device/CPU/InternalCalls.h"
#include "opencrun/Device/CPU/Multiprocessor.h"

using namespace opencrun;
using namespace opencrun::cpu;

size_t opencrun::cpu::GetGlobalId(cl_uint I) {
  CPUThread &Thr = GetCurrentThread();

  return Thr.GetGlobalId(I);
}
