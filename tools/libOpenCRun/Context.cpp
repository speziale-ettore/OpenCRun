
#include "CL/opencl.h"

#include "Utils.h"

#include "opencrun/Core/Context.h"
#include "opencrun/Core/Platform.h"

#include <map>

CL_API_ENTRY cl_context CL_API_CALL
clCreateContext(const cl_context_properties *properties,
                cl_uint num_devices,
                const cl_device_id *devices,
                void (CL_CALLBACK *pfn_notify)(const char *,
                                               const void *,
                                               size_t, void *),
                void *user_data,
                cl_int *errcode_ret) CL_API_SUFFIX__VERSION_1_0 {
  if(!properties)
    RETURN_WITH_ERROR(errcode_ret, CL_INVALID_PLATFORM);

  std::map<cl_context_properties, cl_context_properties> Props;
  while(*properties) {
    cl_context_properties Name = *properties++,
                          Value = *properties++;

    switch(Name) {
    case CL_CONTEXT_PLATFORM:
      if(!reinterpret_cast<cl_platform_id>(Value))
        RETURN_WITH_ERROR(errcode_ret, CL_INVALID_PLATFORM);
      break;

    default:
      RETURN_WITH_ERROR(errcode_ret, CL_INVALID_PROPERTY);
    }

    if(Props.find(Name) != Props.end())
      RETURN_WITH_ERROR(errcode_ret, CL_INVALID_PROPERTY);

    Props[Name] = Value;
  }

  if(Props.find(CL_CONTEXT_PLATFORM) == Props.end())
    RETURN_WITH_ERROR(errcode_ret, CL_INVALID_PLATFORM);
  opencrun::Platform &Plat = *reinterpret_cast<opencrun::Platform *>(
                                Props[CL_CONTEXT_PLATFORM]);

  if(!devices || num_devices == 0 || (!pfn_notify && user_data))
    RETURN_WITH_ERROR(errcode_ret, CL_INVALID_VALUE);

  opencrun::ContextErrCallbackClojure Callback(pfn_notify, user_data);

  opencrun::Context::DevicesContainer Devs;
  for(unsigned I = 0; I < num_devices; ++I) {
    if(!devices[I])
      RETURN_WITH_ERROR(errcode_ret, CL_INVALID_DEVICE);

    Devs.push_back(llvm::cast<opencrun::Device>(devices[I]));
  }

  if(errcode_ret)
    *errcode_ret = CL_SUCCESS;

  opencrun::Context *Ctx = new opencrun::Context(Plat, Devs, Callback);
  Ctx->Retain();

  return Ctx;
}

CL_API_ENTRY cl_context CL_API_CALL
clCreateContextFromType(const cl_context_properties *properties,
                        cl_device_type device_type,
                        void (CL_CALLBACK *pfn_notify)(const char *,
                                                       const void *,
                                                       size_t,
                                                       void *),
                        void *user_data,
                        cl_int *errcode_ret) CL_API_SUFFIX__VERSION_1_0 {
  return 0;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainContext(cl_context context) CL_API_SUFFIX__VERSION_1_0 {
  if(!context)
    return CL_INVALID_CONTEXT;

  opencrun::Context &Ctx = *llvm::cast<opencrun::Context>(context);
  Ctx.Retain();

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseContext(cl_context context) CL_API_SUFFIX__VERSION_1_0 {
  if(!context)
    return CL_INVALID_CONTEXT;

  opencrun::Context &Ctx = *llvm::cast<opencrun::Context>(context);
  Ctx.Release();

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetContextInfo(cl_context context,
                 cl_context_info param_name,
                 size_t param_value_size,
                 void *param_value,
                 size_t *param_value_size_ret) CL_API_SUFFIX__VERSION_1_0 {
  if(!context)
    return CL_INVALID_CONTEXT;

  opencrun::Context &Ctx = *llvm::cast<opencrun::Context>(context);
  switch(param_name) {
  #define PROPERTY(PARAM, FUN, PARAM_TY, FUN_TY)   \
  case PARAM:                                      \
    return clFillValue<PARAM_TY, FUN_TY>(          \
             static_cast<PARAM_TY *>(param_value), \
             Ctx.FUN(),                            \
             param_value_size,                     \
             param_value_size_ret);
  #include "ContextProperties.def"
  #undef PROPERTY

  default:
    return CL_INVALID_VALUE;
  }
}
