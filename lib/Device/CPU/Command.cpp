
#include "opencrun/Device/CPU/Command.h"
#include "opencrun/System/OS.h"

using namespace opencrun;
using namespace opencrun::cpu;

NDRangeKernelBlockCPUCommand::NDRangeKernelBlockCPUCommand(
                                EnqueueNDRangeKernel &Cmd,
                                Signature Entry,
                                unsigned WorkGroup,
                                ArgsMappings &GlobalArgs,
                                CPUCommand::ResultRecorder &Result) :
  CPUMultiExecCommand(CPUCommand::NDRangeKernelBlock, Cmd, Result, WorkGroup),
  Entry(Entry) {
  Kernel &Kern = GetKernel();

  Args = static_cast<void **>(sys::CAlloc(Kern.GetArgCount(), sizeof(void *)));

  // Setup references to global and constant memory.
  for(ArgsMappings::iterator I = GlobalArgs.begin(),
                             E = GlobalArgs.end();
                             I != E;
                             ++I)
    Args[I->first] = I->second;

  // TODO: setup by-copy parameters.
}

NDRangeKernelBlockCPUCommand::~NDRangeKernelBlockCPUCommand() {
  sys::Free(Args);
}
