
#include "CL/opencl.h"

#include "Utils.h"

#include "opencrun/Core/Command.h"
#include "opencrun/Core/CommandQueue.h"
#include "opencrun/Core/Device.h"

#define CL_QUEUE_PROPERTIES_ALL             \
  (CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | \
   CL_QUEUE_PROFILING_ENABLE)

static inline bool clValidQueueProperties(cl_command_queue_properties props) {
  return !(props & ~CL_QUEUE_PROPERTIES_ALL);
}

CL_API_ENTRY cl_command_queue CL_API_CALL
clCreateCommandQueue(cl_context context,
                     cl_device_id device,
                     cl_command_queue_properties properties,
                     cl_int *errcode_ret) CL_API_SUFFIX__VERSION_1_0 {
  if(!context)
    RETURN_WITH_ERROR(errcode_ret, CL_INVALID_CONTEXT);

  opencrun::Context &Ctx = *llvm::cast<opencrun::Context>(context);

  if(!device)
    RETURN_WITH_ERROR(errcode_ret, CL_INVALID_DEVICE);

  opencrun::Device &Dev = *llvm::cast<opencrun::Device>(device);

  if(!clValidQueueProperties(properties))
    RETURN_WITH_ERROR(errcode_ret, CL_INVALID_VALUE);

  opencrun::CommandQueue *Queue = Ctx.GetQueueForDevice(
                                    Dev,
                                    properties &
                                    CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE,
                                    properties & CL_QUEUE_PROFILING_ENABLE,
                                    errcode_ret);

  if(Queue)
    Queue->Retain();

  return Queue;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0 {
  if(!command_queue)
    return CL_INVALID_COMMAND_QUEUE;

  opencrun::CommandQueue *Queue;
  
  Queue = llvm::cast<opencrun::CommandQueue>(command_queue);
  Queue->Retain();

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0 {
  if(!command_queue)
    return CL_INVALID_COMMAND_QUEUE;

  opencrun::CommandQueue *Queue;

  Queue = llvm::cast<opencrun::CommandQueue>(command_queue);
  Queue->Flush();
  Queue->Release();

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetCommandQueueInfo(cl_command_queue command_queue,
                      cl_command_queue_info param_name,
                      size_t param_value_size,
                      void *param_value,
                      size_t *param_value_size_ret) CL_API_SUFFIX__VERSION_1_0 {
  if(!command_queue)
    return CL_INVALID_COMMAND_QUEUE;

  opencrun::CommandQueue *Queue;

  Queue = llvm::cast<opencrun::CommandQueue>(command_queue);
  switch(param_name) {
  #define PROPERTY(PARAM, FUN, PARAM_TY, FUN_TY)   \
  case PARAM: {                                    \
    return clFillValue<PARAM_TY, FUN_TY>(          \
             static_cast<PARAM_TY *>(param_value), \
             Queue->FUN(),                         \
             param_value_size,                     \
             param_value_size_ret);                \
  }
  #include "CommandQueueProperties.def"
  #undef PROPERTY

  case CL_QUEUE_PROPERTIES: {
    cl_command_queue_properties Props = 0;

    if(llvm::isa<opencrun::OutOfOrderQueue>(Queue))
      Props |= CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
    if(Queue->ProfilingEnabled())
      Props |= CL_QUEUE_PROFILING_ENABLE;

    return clFillValue<cl_command_queue_properties,
                       cl_command_queue_properties>(
             static_cast<cl_command_queue_properties *>(param_value),
             Props,
             param_value_size,
             param_value_size_ret);
  }

  default:
    return CL_INVALID_VALUE;
  }
}
