
#include "CL/opencl.h"

#include "Utils.h"

#include "opencrun/Core/Context.h"
#include "opencrun/Core/Platform.h"

#include <map>

#define CL_DEVICE_TYPE_ALL_INTERNAL \
  (CL_DEVICE_TYPE_CPU |             \
   CL_DEVICE_TYPE_GPU |             \
   CL_DEVICE_TYPE_ACCELERATOR |     \
   CL_DEVICE_TYPE_DEFAULT)

static inline bool clValidDeviceType(cl_device_type device_type) {
  // CL_DEVICE_TYPE_ALL is defined to all 1 bitmask, so it needs a special case.
  if(device_type == CL_DEVICE_TYPE_ALL)
    return true;

  // For the same reason, it cannot be used to check via & if device_type is
  // valid.
  return !(device_type & ~CL_DEVICE_TYPE_ALL_INTERNAL);
}

static bool clParseProperties(
  std::map<cl_context_properties, cl_context_properties> &mappings,
  const cl_context_properties *properties,
  cl_int *errcode_ret);

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

  // Error signalling performed inside clParseProperties.
  if(!clParseProperties(Props, properties, errcode_ret))
    return NULL;

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
  if(!properties)
    RETURN_WITH_ERROR(errcode_ret, CL_INVALID_PLATFORM);

  std::map<cl_context_properties, cl_context_properties> Props;

  // Error signalling performed inside clParseProperties.
  if(!clParseProperties(Props, properties, errcode_ret))
    return NULL;

  opencrun::Platform &Plat = *reinterpret_cast<opencrun::Platform *>(
                                Props[CL_CONTEXT_PLATFORM]);

  if(!clValidDeviceType(device_type))
    RETURN_WITH_ERROR(errcode_ret, CL_INVALID_DEVICE_TYPE);

  if(!pfn_notify && user_data)
    RETURN_WITH_ERROR(errcode_ret, CL_INVALID_VALUE);

  opencrun::ContextErrCallbackClojure Callback(pfn_notify, user_data);

  opencrun::Context::DevicesContainer Devs;

  if(device_type & CL_DEVICE_TYPE_CPU || device_type & CL_DEVICE_TYPE_DEFAULT)
    Devs.append(Plat.cpu_begin(), Plat.cpu_end());
  else if(device_type & CL_DEVICE_TYPE_GPU)
    Devs.append(Plat.gpu_begin(), Plat.gpu_end());
  else if(device_type & CL_DEVICE_TYPE_ACCELERATOR)
    Devs.append(Plat.accelerator_begin(), Plat.accelerator_end());

  if(Devs.empty())
    RETURN_WITH_ERROR(errcode_ret, CL_DEVICE_NOT_FOUND);

  if(errcode_ret)
    *errcode_ret = CL_SUCCESS;

  opencrun::Context *Ctx = new opencrun::Context(Plat, Devs, Callback);
  Ctx->Retain();

  return Ctx;
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

  case CL_CONTEXT_DEVICES:
    return clFillValue<cl_device_id, opencrun::Context::device_iterator>(
             static_cast<cl_device_id *>(param_value),
             Ctx.device_begin(),
             Ctx.device_end(),
             param_value_size,
             param_value_size_ret);

  // Currently we do no support custom properties. The standard prescribes that
  // when the context was created with a NULL properties list we can either set
  // param_value_size_ret to 0 or return a list containing only the terminator.
  // However, since OpenCL runtime will be managed through the ICD, we cannot
  // create a context with a NULL properties list, so always return the default
  // properties list here.
  case CL_CONTEXT_PROPERTIES: {
    opencrun::Platform &Plat = opencrun::GetOpenCRunPlatform();
    cl_context_properties Props[] =
      { CL_CONTEXT_PLATFORM,
        reinterpret_cast<cl_context_properties>(&Plat),
        0
      };
    return clFillValue<cl_context_properties, cl_context_properties *>(
             static_cast<cl_context_properties *>(param_value),
             Props,
             Props + 3,
             param_value_size,
             param_value_size_ret);
  }

  default:
    return CL_INVALID_VALUE;
  }
}

static bool clParseProperties(
  std::map<cl_context_properties, cl_context_properties> &mappings,
  const cl_context_properties *properties,
  cl_int *errcode_ret) {
  while(*properties) {
    cl_context_properties Name = *properties++,
                          Value = *properties++;

    switch(Name) {
    case CL_CONTEXT_PLATFORM:
      // The OpenCL specification states that when a NULL value is given, the
      // behavior is implementation specific. Use the AMD convention, and use
      // the default platform.
      if(!reinterpret_cast<cl_platform_id>(Value)) {
        opencrun::Platform &Plat = opencrun::GetOpenCRunPlatform();
        Value = reinterpret_cast<cl_context_properties>(&Plat);
      }
      break;

    default:
      RETURN_WITH_FLAG(errcode_ret, CL_INVALID_PROPERTY);
    }

    if(mappings.find(Name) != mappings.end())
      RETURN_WITH_FLAG(errcode_ret, CL_INVALID_PROPERTY);

    mappings[Name] = Value;
  }

  if(mappings.find(CL_CONTEXT_PLATFORM) == mappings.end())
    RETURN_WITH_FLAG(errcode_ret, CL_INVALID_PLATFORM);

  return true;
}
