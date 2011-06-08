
#include "CL/opencl.h"

#include "opencrun/Core/CommandQueue.h"

CL_API_ENTRY cl_int CL_API_CALL
clFlush(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0 {
  if(!command_queue)
    return CL_INVALID_COMMAND_QUEUE;

  opencrun::CommandQueue *Queue;
 
  Queue = static_cast<opencrun::CommandQueue *>(command_queue);
  Queue->Flush();

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clFinish(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0 {
  if(!command_queue)
    return CL_INVALID_COMMAND_QUEUE;

  opencrun::CommandQueue *Queue;

  Queue = static_cast<opencrun::CommandQueue *>(command_queue);
  Queue->Finish();

  return CL_SUCCESS;
}
