
#include "opencrun/Device/CPU/InternalCalls.h"
#include "opencrun/Device/CPU/Multiprocessor.h"

using namespace opencrun;
using namespace opencrun::cpu;

uint opencrun::cpu::GetWorkDim() {
  CPUThread &Thr = GetCurrentThread();
  const DimensionInfo::iterator &Cur = Thr.GetCurrentIndex();

  return Cur.GetWorkDim();
}

size_t opencrun::cpu::GetGlobalSize(cl_uint I) {
  CPUThread &Thr = GetCurrentThread();
  const DimensionInfo::iterator &Cur = Thr.GetCurrentIndex();

  return Cur.GetGlobalSize(I);
}

size_t opencrun::cpu::GetGlobalId(cl_uint I) {
  CPUThread &Thr = GetCurrentThread();
  const DimensionInfo::iterator &Cur = Thr.GetCurrentIndex();

  return Cur.GetGlobalId(I);
}

size_t opencrun::cpu::GetLocalSize(cl_uint I) {
  CPUThread &Thr = GetCurrentThread();
  const DimensionInfo::iterator &Cur = Thr.GetCurrentIndex();

  return Cur.GetLocalSize(I);
}

size_t opencrun::cpu::GetLocalId(cl_uint I) {
  CPUThread &Thr = GetCurrentThread();
  const DimensionInfo::iterator &Cur = Thr.GetCurrentIndex();

  return Cur.GetLocalId(I);
}

size_t opencrun::cpu::GetNumGroups(cl_uint I) {
  CPUThread &Thr = GetCurrentThread();
  const DimensionInfo::iterator &Cur = Thr.GetCurrentIndex();

  return Cur.GetWorkGroupsCount(I);
}

size_t opencrun::cpu::GetGroupId(uint I) {
  CPUThread &Thr = GetCurrentThread();
  const DimensionInfo::iterator &Cur = Thr.GetCurrentIndex();

  return Cur.GetWorkGroup(I);
}

size_t opencrun::cpu::GetGlobalOffset(uint I) {
  CPUThread &Thr = GetCurrentThread();
  const DimensionInfo::iterator &Cur = Thr.GetCurrentIndex();

  return Cur.GetGlobalOffset(I);
}
