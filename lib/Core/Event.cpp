
#include "opencrun/Core/Event.h"
#include "opencrun/Core/CommandQueue.h"
#include "opencrun/Core/Context.h"

using namespace opencrun;

//
// Event implementation.
//

int Event::Wait() {
  sys::ScopedMonitor Mnt(Monitor);

  while(Status != CL_COMPLETE && Status >= 0)
    Mnt.Wait();

  return Status;
}

void Event::Signal(int Status) {
  sys::ScopedMonitor Mnt(Monitor);

  // Sometimes can happen that a logically previous event is delayed. If so,
  // ignore it. OpenCL events are ordered from CL_QUEUED (3) to CL_COMPLETE (0).
  // Error events are lesser than 0, so the < operator can be used to filter out
  // delayed events.
  if(this->Status < Status)
    return;

  // This OpenCL event has not been delayed, update internal status.
  this->Status = Status;

  if(Status == CL_COMPLETE || Status < 0)
    Mnt.Broadcast();
}

//
// InternalEvent implementation.
//

#define RETURN_WITH_ERROR(MSG)               \
  {                                          \
  Queue->GetContext().ReportDiagnostic(MSG); \
  return;                                    \
  }

InternalEvent::InternalEvent(CommandQueue &Queue,
                             Command &Cmd,
                             ProfileSample *Sample) :
  Event(Event::InternalEvent, CL_QUEUED),
  Queue(&Queue),
  Cmd(Cmd) {
  Profile.SetEnabled(Sample != NULL);
  Profile << Sample;
}

InternalEvent::~InternalEvent() {
  GetProfiler().DumpTrace(Cmd, Profile);

  delete &Cmd;
}

Context &InternalEvent::GetContext() const {
  return Queue->GetContext();
}

void InternalEvent::MarkSubmitted(ProfileSample *Sample) {
  Profile << Sample;
  Signal(CL_SUBMITTED);
}

void InternalEvent::MarkRunning(ProfileSample *Sample) {
  Profile << Sample;
  Signal(CL_RUNNING);
}

void InternalEvent::MarkSubCompleted(ProfileSample *Sample) {
  Profile << Sample;
}

void InternalEvent::MarkCompleted(int Status, ProfileSample *Sample) {
  if(Status != CL_COMPLETE && Status >= 0)
    RETURN_WITH_ERROR("invalid event status");

  Profile << Sample;
  Signal(Status);

  Queue->CommandDone(*this);
}
