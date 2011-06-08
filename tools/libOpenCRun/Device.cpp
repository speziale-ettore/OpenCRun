
#include "CL/opencl.h"

#include "Utils.h"

#include "opencrun/Core/Platform.h"

CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceIDs(cl_platform_id platform,
               cl_device_type device_type,
               cl_uint num_entries,
               cl_device_id *devices,
               cl_uint *num_devices) CL_API_SUFFIX__VERSION_1_0 {
  if(!platform)
    return CL_INVALID_PLATFORM;

  if((num_entries == 0 && devices) || (!num_devices && !devices))
    return CL_INVALID_VALUE;

  opencrun::Platform &Plat = *llvm::cast<opencrun::Platform>(platform);
  llvm::SmallVector<opencrun::Device *, 4> Devs;

  switch(device_type) {
  case CL_DEVICE_TYPE_DEFAULT:
  case CL_DEVICE_TYPE_CPU:
    for(opencrun::Platform::cpu_iterator I = Plat.cpu_begin(),
                                         E = Plat.cpu_end();
                                         I != E;
                                         ++I)
      Devs.push_back(*I);
    break;

  case CL_DEVICE_TYPE_GPU:
    for(opencrun::Platform::gpu_iterator I = Plat.gpu_begin(),
                                         E = Plat.gpu_end();
                                         I != E;
                                         ++I)
      Devs.push_back(*I);
    break;

  case CL_DEVICE_TYPE_ACCELERATOR:
    for(opencrun::Platform::accelerator_iterator I = Plat.accelerator_begin(),
                                                 E = Plat.accelerator_end();
                                                 I != E;
                                                 ++I)
      Devs.push_back(*I);
    break;

  case CL_DEVICE_TYPE_ALL:
    for(opencrun::Platform::cpu_iterator I = Plat.cpu_begin(),
                                         E = Plat.cpu_end();
                                         I != E;
                                         ++I)
      Devs.push_back(*I);
    for(opencrun::Platform::gpu_iterator I = Plat.gpu_begin(),
                                         E = Plat.gpu_end();
                                         I != E;
                                         ++I)
      Devs.push_back(*I);
    for(opencrun::Platform::accelerator_iterator I = Plat.accelerator_begin(),
                                                 E = Plat.accelerator_end();
                                                 I != E;
                                                 ++I)
      Devs.push_back(*I);
    break;

  default:
    return CL_INVALID_DEVICE_TYPE;
  }

  if(Devs.empty())
    return CL_DEVICE_NOT_FOUND;

  if(num_devices)
    *num_devices = Devs.size();

  if(devices)
    for(unsigned I = 0; I < num_entries && I < Devs.size(); ++I)
      *devices++ = Devs[I];

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceInfo(cl_device_id device,
                cl_device_info param_name,
                size_t param_value_size,
                void *param_value,
                size_t *param_value_size_ret) CL_API_SUFFIX__VERSION_1_0 {
  if(!device)
    return CL_INVALID_DEVICE;

  opencrun::Device &Dev = *llvm::cast<opencrun::Device>(device);
  switch(param_name) {
  #define PROPERTY(PARAM, FUN, PARAM_TY, FUN_TY)   \
  case PARAM: {                                    \
    return clFillValue<PARAM_TY, FUN_TY>(          \
             static_cast<PARAM_TY *>(param_value), \
             Dev.FUN(),                            \
             param_value_size,                     \
             param_value_size_ret);                \
  }
  #include "DeviceProperties.def"
  #undef PROPERTY

  case CL_DEVICE_TYPE: {
    cl_device_type DevTy;

    if(llvm::isa<opencrun::CPUDevice>(&Dev))
      DevTy = CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_DEFAULT;
    else if(llvm::isa<opencrun::GPUDevice>(&Dev))
      DevTy = CL_DEVICE_TYPE_GPU;
    else if(llvm::isa<opencrun::AcceleratorDevice>(&Dev))
      DevTy = CL_DEVICE_TYPE_ACCELERATOR;

    return clFillValue<cl_device_type, cl_device_type>(
             static_cast<cl_device_type *>(param_value),
             DevTy,
             param_value_size,
             param_value_size_ret);
  }

  case CL_DEVICE_PLATFORM:
    return clFillValue<cl_platform_id, opencrun::Platform &>(
             static_cast<cl_platform_id *>(param_value),
             opencrun::GetOpenCRunPlatform(),
             param_value_size,
             param_value_size_ret
             );

  default:
    return CL_INVALID_VALUE;
  }
}
