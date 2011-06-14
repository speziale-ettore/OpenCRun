
#include "opencrun/Device/CPULib/InternalCall.h"
#include "opencrun/Device/CPU/Multiprocessor.h"

using namespace opencrun;
using namespace opencrun::cpu;

size_t opencrun::cpu::GetGlobalId(unsigned int I) {
  CPUThread &Thr = GetCurrentThread();

  return Thr.GetGlobalId(I);
}
