
#include "CL/opencl.h"

#include "opencrun/Core/Kernel.h"

#include "Utils.h"

CL_API_ENTRY cl_kernel CL_API_CALL
clCreateKernel(cl_program program,
               const char *kernel_name,
               cl_int *errcode_ret) CL_API_SUFFIX__VERSION_1_0 {
  if(!program)
    RETURN_WITH_ERROR(errcode_ret, CL_INVALID_PROGRAM);

  opencrun::Program &Prog = *llvm::cast<opencrun::Program>(program);

  if(!kernel_name)
    RETURN_WITH_ERROR(errcode_ret, CL_INVALID_VALUE);

  llvm::StringRef KernName(kernel_name);

  opencrun::Kernel *Kern = Prog.CreateKernel(KernName, errcode_ret);
  
  if(Kern)
    Kern->Retain();

  return Kern;
}

CL_API_ENTRY cl_int CL_API_CALL
clCreateKernelsInProgram(cl_program program,
                         cl_uint num_kernels,
                         cl_kernel *kernels,
                         cl_uint *num_kernels_ret) CL_API_SUFFIX__VERSION_1_0 {
  llvm_unreachable("Not yet implemented");
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainKernel(cl_kernel kernel) CL_API_SUFFIX__VERSION_1_0 {
  if(!kernel)
    return CL_INVALID_KERNEL;

  opencrun::Kernel &Kern = *llvm::cast<opencrun::Kernel>(kernel);
  Kern.Retain();

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseKernel(cl_kernel kernel) CL_API_SUFFIX__VERSION_1_0 {
  if(!kernel)
    return CL_INVALID_KERNEL;

  opencrun::Kernel &Kern = *llvm::cast<opencrun::Kernel>(kernel);
  Kern.Release();

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetKernelArg(cl_kernel kernel,
               cl_uint arg_index,
               size_t arg_size,
               const void *arg_value) CL_API_SUFFIX__VERSION_1_0 {
  if(!kernel)
    return CL_INVALID_KERNEL;

  opencrun::Kernel &Kern = *llvm::cast<opencrun::Kernel>(kernel);

  return Kern.SetArg(arg_index, arg_size, arg_value);
}



CL_API_ENTRY cl_int CL_API_CALL
clGetKernelInfo(cl_kernel kernel,
                cl_kernel_info param_name,
                size_t param_value_size,
                void *param_value,
                size_t *param_value_size_ret) CL_API_SUFFIX__VERSION_1_0 {
  if(!kernel)
    return CL_INVALID_KERNEL;

  opencrun::Kernel &Kern = *llvm::cast<opencrun::Kernel>(kernel);
  switch(param_name) {
  #define PROPERTY(PARAM, FUN, PARAM_TY, FUN_TY)   \
  case PARAM:                                      \
    return clFillValue<PARAM_TY, FUN_TY>(          \
             static_cast<PARAM_TY *>(param_value), \
             Kern.FUN(),                           \
             param_value_size,                     \
             param_value_size_ret);
  #include "KernelProperties.def"
  #undef PROPERTY
  default:
    return CL_INVALID_VALUE;
  }
}

CL_API_ENTRY cl_int CL_API_CALL
clGetKernelWorkGroupInfo(cl_kernel kernel,
                         cl_device_id device,
                         cl_kernel_work_group_info param_name,
                         size_t param_value_size,
                         void *param_value,
                         size_t *param_value_size_ret)
CL_API_SUFFIX__VERSION_1_0 {
  llvm_unreachable("Not yet implemented");
  return CL_SUCCESS;
}
