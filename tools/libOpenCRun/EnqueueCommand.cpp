
#include "CL/opencl.h"

#include "Utils.h"

#include "opencrun/Core/Command.h"
#include "opencrun/Core/CommandQueue.h"
#include "opencrun/Core/Event.h"
#include "opencrun/Core/MemoryObj.h"

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadBuffer(cl_command_queue command_queue,
                    cl_mem buffer,
                    cl_bool blocking_read,
                    size_t offset,
                    size_t cb,
                    void *ptr,
                    cl_uint num_events_in_wait_list,
                    const cl_event *event_wait_list,
                    cl_event *event) CL_API_SUFFIX__VERSION_1_0 {
  if(!command_queue)
    return CL_INVALID_COMMAND_QUEUE;

  opencrun::CommandQueue *Queue;

  Queue = llvm::cast<opencrun::CommandQueue>(command_queue);

  cl_int ErrCode;

  opencrun::EnqueueReadBufferBuilder Bld(Queue->GetContext(), buffer, ptr);
  opencrun::Command *Cmd = Bld.SetBlocking(blocking_read)
                              .SetCopyArea(offset, cb)
                              .SetWaitList(num_events_in_wait_list,
                                           event_wait_list)
                              .Create(&ErrCode);

  if(!Cmd)
    return ErrCode;

  opencrun::Event *Ev = Queue->Enqueue(*Cmd, &ErrCode);

  if(!Ev)
    return ErrCode;

  RETURN_WITH_EVENT(event, Ev);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadBufferRect(cl_command_queue command_queue,
                        cl_mem buffer,
                        cl_bool blocking_read,
                        const size_t *buffer_origin,
                        const size_t *host_origin,
                        const size_t *region,
                        size_t buffer_row_pitch,
                        size_t buffer_slice_pitch,
                        size_t host_row_pitch,
                        size_t host_slice_pitch,
                        void *ptr,
                        cl_uint num_events_in_wait_list,
                        const cl_event *event_wait_list,
                        cl_event *event) CL_API_SUFFIX__VERSION_1_1 {
  llvm_unreachable("Not yet implemented");
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteBuffer(cl_command_queue command_queue,
                     cl_mem buffer,
                     cl_bool blocking_write,
                     size_t offset,
                     size_t cb,
                     const void *ptr,
                     cl_uint num_events_in_wait_list,
                     const cl_event *event_wait_list,
                     cl_event *event) CL_API_SUFFIX__VERSION_1_0 {
  if(!command_queue)
    return CL_INVALID_COMMAND_QUEUE;

  opencrun::CommandQueue *Queue;

  Queue = llvm::cast<opencrun::CommandQueue>(command_queue);

  cl_int ErrCode;

  opencrun::EnqueueWriteBufferBuilder Bld(Queue->GetContext(), buffer, ptr);
  opencrun::Command *Cmd = Bld.SetBlocking(blocking_write)
                              .SetCopyArea(offset, cb)
                              .SetWaitList(num_events_in_wait_list,
                                           event_wait_list)
                              .Create(&ErrCode);

  if(!Cmd)
    return ErrCode;

  opencrun::Event *Ev = Queue->Enqueue(*Cmd, &ErrCode);

  if(!Ev)
    return ErrCode;

  RETURN_WITH_EVENT(event, Ev);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteBufferRect(cl_command_queue command_queue,
                         cl_mem buffer,
                         cl_bool blocking_write,
                         const size_t *buffer_origin,
                         const size_t *host_origin,
                         const size_t *region,
                         size_t buffer_row_pitch,
                         size_t buffer_slice_pitch,
                         size_t host_row_pitch,
                         size_t host_slice_pitch,
                         const void *ptr,
                         cl_uint num_events_in_wait_list,
                         const cl_event *event_wait_list,
                         cl_event *event) CL_API_SUFFIX__VERSION_1_1 {
  llvm_unreachable("Not yet implemented");
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBuffer(cl_command_queue command_queue,
                    cl_mem src_buffer,
                    cl_mem dst_buffer,
                    size_t src_offset,
                    size_t dst_offset,
                    size_t cb,
                    cl_uint num_events_in_wait_list,
                    const cl_event *event_wait_list,
                    cl_event *event) CL_API_SUFFIX__VERSION_1_0 {
  llvm_unreachable("Not yet implemented");
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBufferRect(cl_command_queue command_queue,
                        cl_mem src_buffer,
                        cl_mem dst_buffer,
                        const size_t *src_origin,
                        const size_t *dst_origin,
                        const size_t *region,
                        size_t src_row_pitch,
                        size_t src_slice_pitch,
                        size_t dst_row_pitch,
                        size_t dst_slice_pitch,
                        cl_uint num_events_in_wait_list,
                        const cl_event *event_wait_list,
                        cl_event *event) CL_API_SUFFIX__VERSION_1_1 {
  llvm_unreachable("Not yet implemented");
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadImage(cl_command_queue command_queue,
                   cl_mem image,
                   cl_bool blocking_read,
                   const size_t *origin,
                   const size_t *region,
                   size_t row_pitch,
                   size_t slice_pitch,
                   void *ptr,
                   cl_uint num_events_in_wait_list,
                   const cl_event *event_wait_list,
                   cl_event *event) CL_API_SUFFIX__VERSION_1_0 {
  llvm_unreachable("Not yet implemented");
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteImage(cl_command_queue command_queue,
                    cl_mem image,
                    cl_bool blocking_write,
                    const size_t *origin,
                    const size_t *region,
                    size_t input_row_pitch,
                    size_t input_slice_pitch,
                    const void *ptr,
                    cl_uint num_events_in_wait_list,
                    const cl_event *event_wait_list,
                    cl_event *event) CL_API_SUFFIX__VERSION_1_0 {
  llvm_unreachable("Not yet implemented");
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyImage(cl_command_queue command_queue,
                   cl_mem src_image,
                   cl_mem dst_image,
                   const size_t *src_origin,
                   const size_t *dst_origin,
                   const size_t *region,
                   cl_uint num_events_in_wait_list,
                   const cl_event *event_wait_list,
                   cl_event *event) CL_API_SUFFIX__VERSION_1_0 {
  llvm_unreachable("Not yet implemented");
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyImageToBuffer(cl_command_queue command_queue,
                           cl_mem src_image,
                           cl_mem dst_buffer,
                           const size_t *src_origin,
                           const size_t *region,
                           size_t dst_offset,
                           cl_uint num_events_in_wait_list,
                           const cl_event *event_wait_list,
                           cl_event *event) CL_API_SUFFIX__VERSION_1_0 {
  llvm_unreachable("Not yet implemented");
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBufferToImage(cl_command_queue command_queue,
                           cl_mem src_buffer,
                           cl_mem dst_image,
                           size_t src_offset,
                           const size_t *dst_origin,
                           const size_t *region,
                           cl_uint num_events_in_wait_list,
                           const cl_event *event_wait_list,
                           cl_event *event) CL_API_SUFFIX__VERSION_1_0 {
  llvm_unreachable("Not yet implemented");
  return CL_SUCCESS;
}

CL_API_ENTRY void * CL_API_CALL
clEnqueueMapBuffer(cl_command_queue command_queue,
                   cl_mem buffer,
                   cl_bool blocking_map,
                   cl_map_flags map_flags,
                   size_t offset,
                   size_t cb,
                   cl_uint num_events_in_wait_list,
                   const cl_event *event_wait_list,
                   cl_event *event,
                   cl_int *errcode_ret) CL_API_SUFFIX__VERSION_1_0 {
  llvm_unreachable("Not yet implemented");
  return 0;
}

CL_API_ENTRY void * CL_API_CALL
clEnqueueMapImage(cl_command_queue command_queue,
                  cl_mem image,
                  cl_bool blocking_map,
                  cl_map_flags map_flags,
                  const size_t *origin,
                  const size_t *region,
                  size_t *image_row_pitch,
                  size_t *image_slice_pitch,
                  cl_uint num_events_in_wait_list,
                  const cl_event *event_wait_list,
                  cl_event *event,
                  cl_int *errcode_ret) CL_API_SUFFIX__VERSION_1_0 {
  llvm_unreachable("Not yet implemented");
  return 0;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueUnmapMemObject(cl_command_queue command_queue,
                        cl_mem memobj,
                        void *mapped_ptr,
                        cl_uint num_events_in_wait_list,
                        const cl_event *event_wait_list,
                        cl_event *event) CL_API_SUFFIX__VERSION_1_0 {
  llvm_unreachable("Not yet implemented");
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNDRangeKernel(cl_command_queue command_queue,
                       cl_kernel kernel,
                       cl_uint work_dim,
                       const size_t *global_work_offset,
                       const size_t *global_work_size,
                       const size_t *local_work_size,
                       cl_uint num_events_in_wait_list,
                       const cl_event *event_wait_list,
                       cl_event *event) CL_API_SUFFIX__VERSION_1_0 {
  if(!command_queue)
    return CL_INVALID_COMMAND_QUEUE;

  opencrun::CommandQueue *Queue;
  Queue = llvm::cast<opencrun::CommandQueue>(command_queue);

  cl_int ErrCode;

  opencrun::EnqueueNDRangeKernelBuilder Bld(Queue->GetContext(),
                                            Queue->GetDevice(),
                                            kernel,
                                            work_dim,
                                            global_work_size);
  opencrun::Command *Cmd = Bld.SetGlobalWorkOffset(global_work_offset)
                              .SetLocalWorkSize(local_work_size)
                              .SetWaitList(num_events_in_wait_list,
                                           event_wait_list)
                              .Create(&ErrCode);

  if(!Cmd)
    return ErrCode;

  opencrun::Event *Ev = Queue->Enqueue(*Cmd, &ErrCode);

  if(!Ev)
    return ErrCode;

  RETURN_WITH_EVENT(event, Ev);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueTask(cl_command_queue command_queue,
              cl_kernel kernel,
              cl_uint num_events_in_wait_list,
              const cl_event *event_wait_list,
              cl_event *event) CL_API_SUFFIX__VERSION_1_0 {
  llvm_unreachable("Not yet implemented");
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNativeKernel(cl_command_queue command_queue,
                      void (*user_func)(void *),
                      void *args,
                      size_t cb_args,
                      cl_uint num_mem_objects,
                      const cl_mem *mem_list,
                      const void **args_mem_loc,
                      cl_uint num_events_in_wait_list,
                      const cl_event *event_wait_list,
                      cl_event *event) CL_API_SUFFIX__VERSION_1_0 {
  if(!command_queue)
    return CL_INVALID_COMMAND_QUEUE;

  opencrun::CommandQueue *Queue;
  
  Queue = llvm::cast<opencrun::CommandQueue>(command_queue);

  cl_int ErrCode;

  opencrun::EnqueueNativeKernel::Arguments RawArgs(args, cb_args);
  opencrun::EnqueueNativeKernelBuilder Bld(Queue->GetContext(),
                                           user_func,
                                           RawArgs);
  opencrun::Command *Cmd = Bld.SetMemoryMappings(num_mem_objects,
                                                 mem_list,
                                                 args_mem_loc)
                              .SetWaitList(num_events_in_wait_list,
                                           event_wait_list)
                              .Create(&ErrCode);

  if(!Cmd)
    return ErrCode;

  opencrun::Event *Ev = Queue->Enqueue(*Cmd, &ErrCode);

  if(!Ev)
    return ErrCode;

  RETURN_WITH_EVENT(event, Ev);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueMarker(cl_command_queue command_queue,
                cl_event *event) CL_API_SUFFIX__VERSION_1_0 {
  llvm_unreachable("Not yet implemented");
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWaitForEvents(cl_command_queue command_queue,
                       cl_uint num_events,
                       const cl_event *event_list) CL_API_SUFFIX__VERSION_1_0 {
  llvm_unreachable("Not yet implemented");
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueBarrier(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0 {
  llvm_unreachable("Not yet implemented");
  return CL_SUCCESS;
}
