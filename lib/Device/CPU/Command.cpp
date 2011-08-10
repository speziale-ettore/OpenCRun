
#include "opencrun/Device/CPU/Command.h"
#include "opencrun/System/OS.h"

using namespace opencrun;
using namespace opencrun::cpu;

NDRangeKernelBlockCPUCommand::NDRangeKernelBlockCPUCommand(
                                EnqueueNDRangeKernel &Cmd,
                                Signature Entry,
                                ArgsMappings &GlobalArgs,
                                DimensionInfo::iterator I,
                                DimensionInfo::iterator E,
                                CPUCommand::ResultRecorder &Result) :
  CPUMultiExecCommand(CPUCommand::NDRangeKernelBlock,
                      Cmd,
                      Result,
                      I.GetWorkGroup()),
  Entry(Entry),
  Start(I),
  End(E) {
  Kernel &Kern = GetKernel();

  // Hold arguments to be passed to stubs.
  Args = static_cast<void **>(sys::CAlloc(Kern.GetArgCount(), sizeof(void *)));

  // We can start filling some arguments: global/constant buffers and arguments
  // passed by value.
  unsigned J = 0;
  for(Kernel::arg_iterator I = Kern.arg_begin(),
                           E = Kern.arg_end();
                           I != E;
                           ++I) {
    // A buffer can be allocated by the CPUDevice.
    if(BufferKernelArg *Arg = llvm::dyn_cast<BufferKernelArg>(*I)) {
      // Only global and constant buffers are handled here. Local buffers are
      // handled later.
      if(Arg->OnGlobalAddressSpace() || Arg->OnConstantAddressSpace())
        Args[J] = GlobalArgs[J];

    // For arguments passed by copy, we need to setup a pointer for the stub.
    } else if(ByValueKernelArg *Arg = llvm::dyn_cast<ByValueKernelArg>(*I))
      Args[J] = Arg->GetArg();

    ++J;
  }
}

NDRangeKernelBlockCPUCommand::~NDRangeKernelBlockCPUCommand() {
  sys::Free(Args);
}
