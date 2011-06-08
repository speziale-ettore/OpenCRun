
#include "opencrun/Core/CommandQueue.h"
#include "opencrun/Core/Device.h"
#include "opencrun/Core/Command.h"
#include "opencrun/Core/Context.h"
#include "opencrun/Core/Event.h"

using namespace opencrun;

//
// CommandQueue implementation.
//

#define RETURN_WITH_ERROR(VAR, ERRCODE, MSG) \
  {                                          \
  Ctx->ReportDiagnostic(MSG);                \
  if(VAR)                                    \
    *VAR = ERRCODE;                          \
  return NULL;                               \
  }

InternalEvent *CommandQueue::Enqueue(Command &Cmd, cl_int *ErrCode) {
  for(Command::event_iterator I = Cmd.wait_begin(), E = Cmd.wait_end();
                              I != E;
                              ++I)
    if((*I)->GetContext() != *Ctx)
      RETURN_WITH_ERROR(ErrCode,
                        CL_INVALID_CONTEXT,
                        "cannot wait for events of a different context");

  if(llvm::isa<EnqueueNativeKernel>(&Cmd) && !Dev.SupportsNativeKernels())
    RETURN_WITH_ERROR(ErrCode,
                      CL_INVALID_OPERATION,
                      "device does not support native kernels");

  ThisLock.acquire();

  unsigned Cnts = EnableProfile ? Profiler::Time : Profiler::None;
  ProfileSample *Sample = GetProfilerSample(Dev,
                                            Cnts,
                                            ProfileSample::CommandEnqueued);
  llvm::IntrusiveRefCntPtr<InternalEvent> Ev;
  Ev = new InternalEvent(*this, Cmd, Sample);

  Cmd.SetNotifyEvent(*Ev);
  Commands.push_back(&Cmd);

  // TODO: use a more suitable data structure.
  Ev->Retain();
  Events.insert(Ev.getPtr());

  ThisLock.release();

  RunScheduler();

  if(ErrCode)
    *ErrCode = CL_SUCCESS;

  if(Cmd.IsBlocking())
    Ev->Wait();

  // We must ensure caller can access a valid event, even if the associated
  // command is terminated.
  Ev->Retain();

  return Ev.getPtr();
}

void CommandQueue::CommandDone(InternalEvent &Ev) {
  RunScheduler();

  ThisLock.acquire();
  Events.erase(&Ev);
  ThisLock.release();

  Ev.Release();
}

void CommandQueue::Flush() {
  while(RunScheduler()) { }
}

void CommandQueue::Finish() {
  Flush();

  // Safely copy events to wait in a new container. Reference counting is
  // incremented, because current thread can not be the same who has enqueued
  // the commands.
  ThisLock.acquire();
  EventsContainer ToWait = Events;
  ThisLock.release();

  // Wait for all events. If an event was linked to a command that is terminated
  // after leaving the critical section, the reference counting mechanism had
  // prevented the runtime to delete the memory associated with the event: we
  // can safely use the event to wait for a already terminated command.
  for(event_iterator I = ToWait.begin(), E = ToWait.end(); I != E; ++I)
    (*I)->Wait();
}

//
// OutOfOrderQueue implementation.
//

OutOfOrderQueue::~OutOfOrderQueue() {
  Finish(); // Performs virtual calls, must be called here.
}

bool OutOfOrderQueue::RunScheduler() {
  return false;
}

//
// InOrderQueue implementation.
//

InOrderQueue::~InOrderQueue() {
  Finish(); // Performs virtual calls, must be called here.
}

bool InOrderQueue::RunScheduler() {
  llvm::sys::ScopedLock Lock(ThisLock);

  if(Commands.empty())
    return false;

  Command &Cmd = *Commands.front();

  if(Cmd.CanRun() && Dev.Submit(Cmd))
    Commands.pop_front();

  return Commands.size();
}
