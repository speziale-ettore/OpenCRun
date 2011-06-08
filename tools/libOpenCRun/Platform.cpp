
#include "CL/opencl.h"

#include "Utils.h"

#include "opencrun/Core/Platform.h"

CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformIDs(cl_uint num_entries,
                 cl_platform_id *platforms,
                 cl_uint *num_platforms) CL_API_SUFFIX__VERSION_1_0 {
  if((num_entries == 0 && platforms) || (!num_platforms && !platforms))
    return CL_INVALID_VALUE;

  if(num_platforms)
    *num_platforms = 1;

  if(num_entries)
    platforms[0] = &opencrun::GetOpenCRunPlatform();

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformInfo(cl_platform_id platform,
                  cl_platform_info param_name,
                  size_t param_value_size,
                  void *param_value,
                  size_t *param_value_size_ret) CL_API_SUFFIX__VERSION_1_0 {
  if(!platform)
    return CL_INVALID_PLATFORM;

  opencrun::Platform &Plat = *llvm::cast<opencrun::Platform>(platform);
  switch(param_name) {
  #define PROPERTY(PARAM, FUN, PARAM_TY, FUN_TY)   \
  case PARAM:                                      \
    return clFillValue<PARAM_TY, FUN_TY>(          \
             static_cast<PARAM_TY *>(param_value), \
             Plat.FUN(),                           \
             param_value_size,                     \
             param_value_size_ret);
  #include "PlatformProperties.def"
  #undef PROPERTY

  default:
    return CL_INVALID_VALUE;
  }
}
