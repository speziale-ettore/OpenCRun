
#include "CL/opencl.h"

CL_API_ENTRY cl_int CL_API_CALL
clGetEventProfilingInfo(cl_event event,
                        cl_profiling_info param_name,
                        size_t param_value_size,
                        void *param_value,
                        size_t *param_value_size_ret)
CL_API_SUFFIX__VERSION_1_0 {
  return CL_SUCCESS;
}
