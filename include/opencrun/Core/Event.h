
#ifndef OPENCRUN_CORE_EVENT_H
#define OPENCRUN_CORE_EVENT_H

#include "opencrun/Core/Command.h"
#include "opencrun/Core/Context.h"
#include "opencrun/Core/Profiler.h"
#include "opencrun/System/Monitor.h"
#include "opencrun/Util/MTRefCounted.h"

struct _cl_event { };

namespace opencrun {

class CommandQueue;

class Event : public _cl_event, public MTRefCountedBaseVPTR<Event> {
public:
  enum Type {
    InternalEvent
  };

public:
  static bool classof(const _cl_event *Ev) { return true; }

protected:
  Event(Type EvTy, int Status) : EvTy(EvTy), Status(Status) { }
  virtual ~Event() { }

public:
  int Wait();

public:
  virtual Context &GetContext() const = 0;

  int GetStatus() const { return Status; }

  bool HasCompleted() const { return Status == CL_COMPLETE || Status < 0; }
  bool IsError() const { return Status < 0; }

  Type GetType() const { return EvTy; }

protected:
  void Signal(int Status);

private:
  Type EvTy;

  // Positive values are normal status. Negative values are platform/runtime
  // errors. See OpenCL specs, version 1.1, table 5.15.
  volatile int Status;

  sys::Monitor Monitor;
};

class InternalEvent : public Event {
public:
  static bool classof(const Event *Ev) {
    return Ev->GetType() == Event::InternalEvent;
  }

public:
  InternalEvent(CommandQueue &Queue,
                Command &Cmd,
                ProfileSample *Sample = NULL);
  virtual ~InternalEvent();

  InternalEvent(const InternalEvent *That); // Do not implement.
  void operator=(const InternalEvent *That); // Do not implement.

public:
  Context &GetContext() const;
  CommandQueue &GetCommandQueue() const { return *Queue; }
  Command &GetCommand() const { return Cmd; }

  bool IsProfiled() const { return Profile.IsEnabled(); }

  const ProfileTrace &GetProfile() const { return Profile; }

public:
  void MarkSubmitted(ProfileSample *Sample = NULL);
  void MarkRunning(ProfileSample *Sample = NULL);
  void MarkSubRunning(ProfileSample *Sample = NULL);
  void MarkSubCompleted(ProfileSample *Sample = NULL);
  void MarkCompleted(int Status, ProfileSample *Sample = NULL);

private:
  llvm::IntrusiveRefCntPtr<CommandQueue> Queue;
  Command &Cmd;

  ProfileTrace Profile;
};

} // End namespace opencrun.

#endif // OPENCRUN_CORE_EVENT_H
