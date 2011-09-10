
#include "opencrun/Device/CPU/CPUThread.h"
#include "opencrun/Core/Event.h"
#include "opencrun/Device/CPU/Multiprocessor.h"
#include "opencrun/System/OS.h"

#include "llvm/Support/Format.h"
#include "llvm/Support/ThreadLocal.h"

using namespace opencrun;
using namespace opencrun::cpu;

namespace {

//
// Routines to manage current work-item stack pointer in the TLS.
//

extern "C" {

void SetCurrentWorkItemStack(void *Stack);
void *GetCurrentWorkItemStack();

}

//
// Routines to manage current thread pointer in the TLS.
//

void SetCurrentThread(CPUThread &Thr);
void ResetCurrentThread();

//
// Include low-level routines for current architecture related to stack
// management. All symbols should use the C linking convention.
//

#if defined(__x86_64__)
#include "StackX8664.inc"
#elif defined(__i386__)
#include "StackX8632.inc"
#else
#error "architecture not supported"
#endif

} // End anonymous namespace.

//
// ExecutionStack implementation.
//

ExecutionStack::ExecutionStack(const sys::HardwareComponent *Mem) {
  // Ideally, all the stack should reside in the L1 cache, toghether with the
  // local memory. By default, allocate a 4 * sizeof(L1D) data segment for
  // holding all work-item stacks.
  if(const sys::HardwareCache *Cache = llvm::cast<sys::HardwareCache>(Mem)) {
    StackSize = 4 * Cache->GetSize();
    Stack = sys::PageAlignedAlloc(StackSize);
  } else
    llvm_unreachable("Unable to allocate work item stacks");
}

ExecutionStack::~ExecutionStack() {
  sys::Free(Stack);
}

void ExecutionStack::Reset(EntryPoint Entry, void **Args, unsigned N) {
  size_t PageSize = sys::Hardware::GetPageSize(),
         PrivateSize = PageSize,
         RequiredStackSize = N * PrivateSize;

  // Stack too small, expand it.
  if(StackSize < RequiredStackSize) {
    sys::Free(Stack);
    Stack = sys::PageAlignedAlloc(RequiredStackSize);
    StackSize = RequiredStackSize;
  }

  #ifndef NDEBUG
  // Add guard pages between work-item stacks. Add a guard page at the end of
  // the stack.
  size_t DebugPrivateSize = PrivateSize + PageSize;
  size_t DebugStackSize = N * DebugPrivateSize + PageSize;

  // If needed, redo memory allocation, in order to get space for guard page.
  if(StackSize < DebugStackSize) {
    sys::Free(Stack);
    Stack = sys::PageAlignedAlloc(DebugStackSize);
    StackSize = DebugStackSize;
  }

  // Remove page protection, reset all pages to R/W.
  sys::MarkPagesReadWrite(Stack, StackSize);

  // Zero the memory.
  std::memset(Stack, 0, StackSize);
  #endif // NDEBUG

  uintptr_t StackAddr = reinterpret_cast<uintptr_t>(Stack),
            Addr = StackAddr,
            PrevAddr = 0;

  // Skip first page.
  #ifndef NDEBUG
  Addr += PageSize;
  #endif // NDEBUG

  // Initialize all work-item stacks.
  for(unsigned I = 0; I < N; ++I) {
    // Mark the guard page.
    #ifndef NDEBUG
    sys::MarkPageReadOnly(reinterpret_cast<void *>(Addr - PageSize));
    #endif // NDEBUG

    InitWorkItemStack(reinterpret_cast<void *>(Addr), PrivateSize, Entry, Args);

    if(PrevAddr)
      SetWorkItemStackLink(reinterpret_cast<void *>(Addr),
                           reinterpret_cast<void *>(PrevAddr),
                           PrivateSize);

    PrevAddr = Addr;

    #ifndef NDEBUG
    Addr += DebugPrivateSize;
    #else
    Addr += PrivateSize;
    #endif // NDEBUG
  }

  // Mark last guard page.
  #ifndef NDEBUG
  sys::MarkPageReadOnly(reinterpret_cast<void *>(Addr - PageSize));
  #endif // NDEBUG

  Addr = StackAddr;

  // Skip first page.
  #ifndef NDEBUG
  Addr += PageSize;
  #endif // NDEBUG

  // Link last stack with the first.
  SetWorkItemStackLink(reinterpret_cast<void *>(Addr),
                       reinterpret_cast<void *>(PrevAddr),
                       PrivateSize);
}

void ExecutionStack::Run() {
  uintptr_t StackAddr = reinterpret_cast<uintptr_t>(Stack);
  size_t PageSize = sys::Hardware::GetPageSize();

  // Skip first page.
  #ifndef NDEBUG
  StackAddr += PageSize;
  #endif // NDEBUG

  RunWorkItems(reinterpret_cast<void *>(StackAddr), PageSize);
}

void ExecutionStack::SwitchToNextWorkItem() {
  SwitchWorkItem();
}

void ExecutionStack::Dump() const {
  llvm::raw_ostream &OS = llvm::errs();

  uintptr_t StartAddr = reinterpret_cast<uintptr_t>(Stack),
            EndAddr = StartAddr + StackSize;
  size_t PageSize = sys::Hardware::GetPageSize();

  OS << llvm::format("Stack [%p, %p]", StartAddr, EndAddr)
     << llvm::format(" => %zd bytes, %u pages", StackSize, StackSize / PageSize)
     << ":";
  Dump(reinterpret_cast<void *>(StartAddr), reinterpret_cast<void *>(EndAddr));
}

void ExecutionStack::Dump(unsigned I) const {
  llvm::raw_ostream &OS = llvm::errs();

  uintptr_t StartAddr = reinterpret_cast<uintptr_t>(Stack),
            EndAddr;
  size_t PageSize = sys::Hardware::GetPageSize();

  #ifndef NDEBUG
  StartAddr += PageSize + I * 2 * PageSize;
  #else
  StartAddr += StartAddr + I * PageSize;
  #endif // NDEBUG
  
  EndAddr = StartAddr + PageSize;

  OS << llvm::format("Work-item stack [%p, %p]", StartAddr, EndAddr)
     << ":";
  Dump(reinterpret_cast<void *>(StartAddr), reinterpret_cast<void *>(EndAddr));
}

void ExecutionStack::Dump(void *StartAddr, void *EndAddr) const {
  llvm::raw_ostream &OS = llvm::errs();

  size_t PageSize = sys::Hardware::GetPageSize();

  for(uintptr_t Addr = reinterpret_cast<uintptr_t>(StartAddr);
                Addr < reinterpret_cast<uintptr_t>(EndAddr);
                Addr += 16) {
    if((Addr % PageSize) == 0)
      OS << "\n";

    OS.indent(2) << llvm::format("%p:", Addr);
    for(unsigned I = 0; I < 16; ++I) {
      uint8_t *Byte = reinterpret_cast<uint8_t *>(Addr) + I;
      OS << llvm::format(" %02x", *(Byte));
    }
    OS << "\n";
  }
}

//
// CurrentWorkItemStack implementation.
//

namespace {

extern "C" {

llvm::sys::ThreadLocal<const void> CurrentWorkItemStack;

void SetCurrentWorkItemStack(void *Stack) {
  CurrentWorkItemStack.set(Stack);
}

void *GetCurrentWorkItemStack() {
  return const_cast<void *>(CurrentWorkItemStack.get());
}

}

} // End anonymous namespace.

//
// CPUThread implementation.
//

CPUThread::CPUThread(Multiprocessor &MP, const sys::HardwareCPU &CPU) :
  Thread(CPU),
  Mode(FullyOperational),
  MP(MP),
  Stack(CPU.GetFirstLevelMemory()) {
  Start();
}

CPUThread::~CPUThread() {
  // Send a command to stop the device thread. Do not employ sys:FastRendevouz
  // because potentially we can wait for a long time.
  CPUCommand *Cmd = new StopDeviceCPUCommand();
  Submit(Cmd);

  // Use the Thread::Join method instead, that suspend the current thread until
  // the joined thread exits.
  Join();
}

bool CPUThread::Submit(CPUCommand *Cmd) {
  bool Enqueued;

  sys::ScopedMonitor Mnt(ThisMnt);

  if(Mode & NoNewJobs)
    return false;

  if(CPUServiceCommand *Enq = llvm::dyn_cast<CPUServiceCommand>(Cmd))
    Enqueued = Submit(Enq);
  else if(CPUExecCommand *Enq = llvm::dyn_cast<CPUExecCommand>(Cmd))
    Enqueued = Submit(Enq);
  else
    Enqueued = false;

  if(Enqueued) {
    Commands.push_back(Cmd);
    Mnt.Signal();
  }

  return Enqueued;
}

void CPUThread::Run() {
  SetCurrentThread(*this);

  while(Mode & ExecJobs) {
    ThisMnt.Enter();

    while(Commands.empty())
      ThisMnt.Wait();

    CPUCommand *Cmd = Commands.front();
    Commands.pop_front();

    ThisMnt.Exit();

    Execute(Cmd);
  }

  ResetCurrentThread();
}

float CPUThread::GetLoadIndicator() {
  sys::ScopedMonitor Mnt(ThisMnt);

  return Commands.size();
}

void CPUThread::SwitchToNextWorkItem() {
  if(++Cur == End)
    Cur = Begin;

  Stack.SwitchToNextWorkItem();
}

bool CPUThread::Submit(CPUServiceCommand *Cmd) {
  if(RunStaticConstructorsCPUCommand *Ctor =
       llvm::dyn_cast<RunStaticConstructorsCPUCommand>(Cmd))
    return Submit(Ctor);

  else if(RunStaticDestructorsCPUCommand *Dtor =
            llvm::dyn_cast<RunStaticDestructorsCPUCommand>(Cmd))
    return Submit(Dtor);

  else if(StopDeviceCPUCommand *Enq = llvm::dyn_cast<StopDeviceCPUCommand>(Cmd))
    return Submit(Enq);

  else
    llvm::report_fatal_error("unknown command submitted");

  return false;
}

bool CPUThread::Submit(CPUExecCommand *Cmd) {
  if(ReadBufferCPUCommand *Read = llvm::dyn_cast<ReadBufferCPUCommand>(Cmd))
    return Submit(Read);

  else if(WriteBufferCPUCommand *Write =
            llvm::dyn_cast<WriteBufferCPUCommand>(Cmd))
    return Submit(Write);

  else if(NDRangeKernelBlockCPUCommand *NDBlock =
            llvm::dyn_cast<NDRangeKernelBlockCPUCommand>(Cmd))
    return Submit(NDBlock);

  else if(NativeKernelCPUCommand *Native =
            llvm::dyn_cast<NativeKernelCPUCommand>(Cmd))
    return Submit(Native);

  else
    llvm::report_fatal_error("unknown command submitted");

  return false;
}

void CPUThread::Execute(CPUCommand *Cmd) {
  if(CPUServiceCommand *OnFly = llvm::dyn_cast<CPUServiceCommand>(Cmd))
    Execute(OnFly);
  else if(CPUExecCommand *OnFly = llvm::dyn_cast<CPUExecCommand>(Cmd))
    Execute(OnFly);
}

void CPUThread::Execute(CPUServiceCommand *Cmd) {
  if(RunStaticConstructorsCPUCommand *OnFly =
       llvm::dyn_cast<RunStaticConstructorsCPUCommand>(Cmd))
    Execute(OnFly);

  else if(RunStaticDestructorsCPUCommand *OnFly =
            llvm::dyn_cast<RunStaticDestructorsCPUCommand>(Cmd))
    Execute(OnFly);

  else if(StopDeviceCPUCommand *OnFly =
            llvm::dyn_cast<StopDeviceCPUCommand>(Cmd))
    Execute(OnFly);

  MP.NotifyDone(Cmd);
}

void CPUThread::Execute(RunStaticConstructorsCPUCommand *Cmd) {
  RunStaticConstructorsCPUCommand::ConstructorsInvoker &Invoker =
    Cmd->GetInvoker();

  Invoker();
}

void CPUThread::Execute(RunStaticDestructorsCPUCommand *Cmd) {
  RunStaticDestructorsCPUCommand::DestructorsInvoker &Invoker =
    Cmd->GetInvoker();

  Invoker();
}

void CPUThread::Execute(CPUExecCommand *Cmd) {
  InternalEvent &Ev = Cmd->GetQueueCommand().GetNotifyEvent();
  unsigned Counters = Cmd->IsProfiled() ? Profiler::Time : Profiler::None;
  int ExitStatus;

  // Command started.
  if(Cmd->RegisterStarted()) {
    ProfileSample *Sample = GetProfilerSample(MP,
                                              Counters,
                                              ProfileSample::CommandRunning);
    Ev.MarkRunning(Sample);
  }

  // This command is part of a large OpenCL command. Register partial execution.
  if(CPUMultiExecCommand *MultiCmd = llvm::dyn_cast<CPUMultiExecCommand>(Cmd)) {
    ProfileSample *Sample = GetProfilerSample(MP,
                                              Counters,
                                              ProfileSample::CommandRunning,
                                              MultiCmd->GetId());
    Ev.MarkSubRunning(Sample);
  }

  if(ReadBufferCPUCommand *OnFly = llvm::dyn_cast<ReadBufferCPUCommand>(Cmd))
    ExitStatus = Execute(*OnFly);

  else if(WriteBufferCPUCommand *OnFly =
            llvm::dyn_cast<WriteBufferCPUCommand>(Cmd))
    ExitStatus = Execute(*OnFly);

  else if(NDRangeKernelBlockCPUCommand *OnFly =
            llvm::dyn_cast<NDRangeKernelBlockCPUCommand>(Cmd))
    ExitStatus = Execute(*OnFly);

  else if(NativeKernelCPUCommand *OnFly =
            llvm::dyn_cast<NativeKernelCPUCommand>(Cmd))
    ExitStatus = Execute(*OnFly);
  
  else
    ExitStatus = CPUCommand::Unsupported;
  
  MP.NotifyDone(Cmd, ExitStatus);
}

int CPUThread::Execute(ReadBufferCPUCommand &Cmd) {
  std::memcpy(Cmd.GetTarget(), Cmd.GetSource(), Cmd.GetSize());

  return CPUCommand::NoError;
}

int CPUThread::Execute(WriteBufferCPUCommand &Cmd) {
  std::memcpy(Cmd.GetTarget(), Cmd.GetSource(), Cmd.GetSize());

  return CPUCommand::NoError;
}

int CPUThread::Execute(NDRangeKernelBlockCPUCommand &Cmd) {
  NDRangeKernelBlockCPUCommand::Signature Func = Cmd.GetFunction();
  void **Args = Cmd.GetArgumentsPointer();

  // Save first and last work item to execute.
  Begin = Cmd.index_begin();
  End = Cmd.index_end();

  // Get the iteration space.
  DimensionInfo &DimInfo = Cmd.GetDimensionInfo();

  // Reset the stack and set initial work-item.
  Stack.Reset(Func, Args, DimInfo.GetLocalWorkItems());
  Cur = Begin;

  // Start the first work-item.
  Stack.Run();

  return CPUCommand::NoError;
}

int CPUThread::Execute(NativeKernelCPUCommand &Cmd) {
  NativeKernelCPUCommand::Signature Func = Cmd.GetFunction();
  void *Args = Cmd.GetArgumentsPointer();

  Func(Args);

  return CPUCommand::NoError;
}

//
// GetCurrentThread implementation.
//

namespace {

llvm::sys::ThreadLocal<const CPUThread> CurThread;

void SetCurrentThread(CPUThread &Thr) { CurThread.set(&Thr); }
void ResetCurrentThread() { CurThread.erase(); }

} // End anonymous namespace.

CPUThread &opencrun::cpu::GetCurrentThread() {
  return *const_cast<CPUThread *>(CurThread.get());
}
