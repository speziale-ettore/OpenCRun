
#include "CL/opencl.h"

#include "Utils.h"

#include "opencrun/Core/Event.h"

#include "llvm/Support/ErrorHandling.h"

CL_API_ENTRY cl_int CL_API_CALL
clWaitForEvents(cl_uint num_events,
                const cl_event *event_list) CL_API_SUFFIX__VERSION_1_0 {
  if(!num_events || !event_list)
    return CL_INVALID_VALUE;

  // Need to obtain the base context, special handling of first event.
  if(!event_list[0])
    return CL_INVALID_EVENT;

  opencrun::Event &Fst = *llvm::cast<opencrun::Event>(event_list[0]);

  for(unsigned I = 1; I < num_events; ++I) {
    if(!event_list[I])
      return CL_INVALID_EVENT;

    opencrun::Event &Cur = *llvm::cast<opencrun::Event>(event_list[I]);

    if(Cur.GetContext() != Fst.GetContext())
      return CL_INVALID_CONTEXT;
  }

  // Separate waiting phase from validation phase to guarantee deterministic
  // behavior.
  bool ErrDetected = false;
  for(unsigned I = 0; I < num_events; ++I) {
    opencrun::Event &Cur = *llvm::cast<opencrun::Event>(event_list[I]);
    ErrDetected = ErrDetected || Cur.Wait() != CL_SUCCESS;
  }

  return ErrDetected ? CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST
                     : CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetEventInfo(cl_event event,
               cl_event_info param_name,
               size_t param_value_size,
               void *param_value,
               size_t *param_value_size_ret) CL_API_SUFFIX__VERSION_1_0 {
  if(!event)
    return CL_INVALID_EVENT;

  opencrun::Event &Ev = *llvm::cast<opencrun::Event>(event);
  switch(param_name) {
  #define PROPERTY(PARAM, FUN, PARAM_TY, FUN_TY)   \
  case PARAM: {                                    \
    return clFillValue<PARAM_TY, FUN_TY>(          \
             static_cast<PARAM_TY *>(param_value), \
             Ev.FUN(),                             \
             param_value_size,                     \
             param_value_size_ret);                \
  }
  #include "EventProperties.def"
  #undef PROPERTY

  case CL_EVENT_COMMAND_QUEUE: {
    opencrun::CommandQueue *Queue;

    if(opencrun::InternalEvent *InternalEv =
        llvm::dyn_cast<opencrun::InternalEvent>(&Ev))
      Queue = &InternalEv->GetCommandQueue();
    else
      Queue = NULL;

    return clFillValue<cl_command_queue,
                       opencrun::CommandQueue &>(
             static_cast<cl_command_queue *>(param_value),
             *Queue,
             param_value_size,
             param_value_size_ret);
  }

  case CL_EVENT_COMMAND_TYPE: {
    opencrun::InternalEvent *InternalEv;
    InternalEv = llvm::dyn_cast<opencrun::InternalEvent>(&Ev);

    if(!InternalEv)
      return CL_INVALID_VALUE;

    return clFillValue<cl_command_type,
                       opencrun::Command &>(
             static_cast<cl_command_type *>(param_value),
             InternalEv->GetCommand(),
             param_value_size,
             param_value_size_ret);
  }

  default:
    return CL_INVALID_VALUE;
  }
}

CL_API_ENTRY cl_event CL_API_CALL
clCreateUserEvent(cl_context context,
                  cl_int *errcode_ret) CL_API_SUFFIX__VERSION_1_1 {
  llvm_unreachable("Not yet implemented");
  return 0;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainEvent(cl_event event) CL_API_SUFFIX__VERSION_1_0 {
  if(!event)
    return CL_INVALID_EVENT;

  opencrun::Event &Ev = *llvm::cast<opencrun::Event>(event);
  Ev.Retain();

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseEvent(cl_event event) CL_API_SUFFIX__VERSION_1_0 {
  if(!event)
    return CL_INVALID_EVENT;

  opencrun::Event &Ev = *llvm::cast<opencrun::Event>(event);
  Ev.Release();

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetUserEventStatus(cl_event event,
                     cl_int execution_status) CL_API_SUFFIX__VERSION_1_1 {
  llvm_unreachable("Not yet implemented");
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetEventCallback(cl_event event,
                   cl_int command_exec_callback_type,
                   void (CL_CALLBACK *pfn_notify)(cl_event,
                                                  cl_int,
                                                  void *),
                   void *user_data) CL_API_SUFFIX__VERSION_1_1 {
  llvm_unreachable("Not yet implemented");
  return CL_SUCCESS;
}
