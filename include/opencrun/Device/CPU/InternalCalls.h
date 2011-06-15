
#ifndef OPENCRUN_DEVICE_CPU_INTERNALCALLS_H
#define OPENCRUN_DEVICE_CPU_INTERNALCALLS_H

#include "CL/opencl.h"

// Add some missing types.
typedef cl_ulong cl_mem_fence_flags;

namespace opencrun {
namespace cpu {

// Work-Item Functions.
size_t GetGlobalId(cl_uint I);

// Synchronization Functions.
void Barrier(cl_mem_fence_flags Flags);

} // End namespace cpu.
} // End namespace opencrun.

#endif // OPENCRUN_DEVICE_CPU_INTERNALCALLS_H
