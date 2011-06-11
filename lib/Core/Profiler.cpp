
#include "opencrun/Core/Profiler.h"
#include "opencrun/Core/Command.h"
#include "opencrun/Core/Context.h"
#include "opencrun/System/Env.h"
#include "opencrun/Util/Table.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ManagedStatic.h"

#include <string>

using namespace opencrun;

Profiler::Profiler() : ProfileStream(STDERR_FILENO, false, true),
                       ToProfile(None) {
  std::string Raw = sys::GetEnv("OPENCRUN_PROFILED_COUNTERS");

  llvm::SmallVector<llvm::StringRef, 4> RawCounters;

  SplitString(Raw, RawCounters, ":");

  for(llvm::SmallVector<llvm::StringRef, 4>::iterator I = RawCounters.begin(),
                                                      E = RawCounters.end();
                                                      I != E;
                                                      ++I)
    ToProfile |= llvm::StringSwitch<Counter>(*I)
                   .Case("time", Time)
                   .Default(None);
}

void Profiler::DumpTrace(Command &Cmd,
                         const ProfileTrace &Trace,
                         bool Force) {
  llvm::sys::ScopedLock Lock(ThisLock);

  if(!IsProfilingForcedFromEnvironment() && !Force)
    return;

  util::Table Tab;

  Tab << "Label" << "Time" << "Delta" << util::Table::EOL;

  sys::Time Last;
  for(ProfileTrace::const_iterator I = Trace.begin(),
                                   E = Trace.end();
                                   I != E;
                                   ++I) {
    const sys::Time &Now = (*I)->GetTime();
    ProfileSample::Label Label = (*I)->GetLabel();
    int SubLabelId = (*I)->GetSubLabelId();

    Tab << FormatLabel(Label, SubLabelId)
        << Now
        << Now - Last
        << util::Table::EOL;
    Last = Now;
  }

  DumpPrefix() << " ";
  DumpCommandName(Cmd) << "\n";
  Tab.Dump(ProfileStream, "profile");
  DumpPrefix() << "\n";
}

std::string Profiler::FormatLabel(ProfileSample::Label Label, int SubId) {
  std::string Text;

  switch(Label) {
  case ProfileSample::Unknown:
    Text = "Unknown";
    break;

  case ProfileSample::CommandEnqueued:
    Text = "CommandEnqueued";
    break;

  case ProfileSample::CommandSubmitted:
    Text = "CommandSubmitted";
    break;

  case ProfileSample::CommandRunning:
    Text = "CommandRunning";
    break;

  case ProfileSample::CommandCompleted:
    Text = "CommandCompleted";
    break;

  default:
    llvm_unreachable("Unknown sample label");
  }

  if(SubId >= 0)
    Text += "-" + llvm::itostr(SubId);

  return Text;
}

llvm::raw_ostream &Profiler::DumpPrefix() {
  ProfileStream.changeColor(llvm::raw_ostream::GREEN) <<  "profile:";
  ProfileStream.resetColor();

  return ProfileStream;
}

llvm::raw_ostream &Profiler::DumpCommandName(Command &Cmd) {
  if(llvm::isa<EnqueueReadBuffer>(Cmd))
    ProfileStream << "Enqueue Read Buffer (" << &Cmd << ")";
  else if(llvm::isa<EnqueueWriteBuffer>(Cmd))
    ProfileStream << "Enqueue Write Buffer (" << &Cmd << ")";
  else if(llvm::isa<EnqueueNDRangeKernel>(Cmd))
    ProfileStream << "Enqueue ND-Range Kernel (" << &Cmd << ")";
  else if(llvm::isa<EnqueueNativeKernel>(Cmd))
    ProfileStream << "Enqueue Native Kernel (" << &Cmd << ")";
  else
    llvm_unreachable("Unknown command type");

  return ProfileStream;
}

llvm::ManagedStatic<Profiler> GlobalProfiler;

Profiler &opencrun::GetProfiler() {
  return *GlobalProfiler;
}
