
#include "opencrun/Device/CPU/CPUDevice.h"
#include "opencrun/Device/CPU/InternalCalls.h"
#include "opencrun/Core/Event.h"
#include "opencrun/Device/CPUPasses/AllPasses.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/PassManager.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ThreadLocal.h"
#include "llvm/Target/TargetSelect.h"

using namespace opencrun;

namespace {

void SignalJITCallStart(CPUDevice &Dev);
void SignalJITCallEnd();

} // End anonymous namespace.

//
// CPUDevice implementation.
//

CPUDevice::CPUDevice(sys::HardwareNode &Node) :
  Device("CPU", llvm::sys::getHostTriple()),
  Global(Node.GetMemorySize()) {
  InitDeviceInfo(Node);
  InitMultiprocessors(Node);
  InitJIT();
  InitInternalCalls();
}

CPUDevice::~CPUDevice() {
  DestroyMultiprocessors();
  DestroyJIT();
}

bool CPUDevice::CreateHostBuffer(HostBuffer &Buf) {
  return false;
}

bool CPUDevice::CreateHostAccessibleBuffer(HostAccessibleBuffer &Buf) {
  return false;
}

bool CPUDevice::CreateDeviceBuffer(DeviceBuffer &Buf) {
  return Global.Alloc(Buf);
}

void CPUDevice::DestroyMemoryObj(MemoryObj &MemObj) {
  Global.Free(MemObj);
}

bool CPUDevice::Submit(Command &Cmd) {
  bool Submitted = false;
  llvm::OwningPtr<ProfileSample> Sample;

  // Take the profiling information here, in order to force this sample
  // happening before the subsequents samples.
  unsigned Counters = Cmd.IsProfiled() ? Profiler::Time : Profiler::None;
  Sample.reset(GetProfilerSample(*this,
                                 Counters,
                                 ProfileSample::CommandSubmitted));

  if(EnqueueReadBuffer *Read = llvm::dyn_cast<EnqueueReadBuffer>(&Cmd))
    Submitted = Submit(*Read);

  else if(EnqueueWriteBuffer *Write = llvm::dyn_cast<EnqueueWriteBuffer>(&Cmd))
    Submitted = Submit(*Write);

  else if(EnqueueNDRangeKernel *NDRange =
            llvm::dyn_cast<EnqueueNDRangeKernel>(&Cmd))
    Submitted = Submit(*NDRange);

  else if(EnqueueNativeKernel *Native =
            llvm::dyn_cast<EnqueueNativeKernel>(&Cmd))
    Submitted = Submit(*Native);

  else
    llvm::report_fatal_error("unknown command submitted");

  // The command has been submitted, register the sample. On failure, the
  // llvm::OwningPtr destructor will reclaim the sample.
  if(Submitted) {
    InternalEvent &Ev = Cmd.GetNotifyEvent();
    Ev.MarkSubmitted(Sample.take());
  }

  return Submitted;
}

void CPUDevice::UnregisterKernel(Kernel &Kern) {
  llvm::sys::ScopedLock Lock(ThisLock);

  // Remove module from the JIT.
  llvm::Module &Mod = *Kern.GetModule(*this);
  JIT->removeModule(&Mod);

  // Erase kernel from the cache.
  BlockParallelEntryPoints::iterator I, E;
  I = BlockParallelEntriesCache.find(&Kern);
  E = BlockParallelEntriesCache.end();
  if(I != E)
    BlockParallelEntriesCache.erase(I);

  // Build a command to invoke static destructors.
  RunStaticDestructorsCPUCommand::DestructorsInvoker Invoker(Mod, *JIT);
  sys::FastRendevouz Synch;
  RunStaticDestructorsCPUCommand *Cmd;

  Cmd = new RunStaticDestructorsCPUCommand(Invoker, Synch);

  Multiprocessor &MP = **Multiprocessors.begin();

  // None available for executing the command: it is critical, run directly.
  if(!MP.Submit(Cmd)) {
    Invoker();
    delete Cmd;

  // Wait for other thread invoking static destructors.
  } else
    Synch.Wait();
}

void CPUDevice::NotifyDone(CPUExecCommand *Cmd, int ExitStatus) {
  // Get counters to profile.
  Command &QueueCmd = Cmd->GetQueueCommand();
  InternalEvent &Ev = QueueCmd.GetNotifyEvent();
  unsigned Counters = Cmd->IsProfiled() ? Profiler::Time : Profiler::None;

  // This command does not directly translate to an OpenCL command. Register
  // partial acknowledgment.
  if(CPUMultiExecCommand *MultiCmd = llvm::dyn_cast<CPUMultiExecCommand>(Cmd)) {
    ProfileSample *Sample = GetProfilerSample(*this,
                                              Counters,
                                              ProfileSample::CommandCompleted,
                                              MultiCmd->GetId());
    Ev.MarkSubCompleted(Sample);
  }

  // All acknowledgment received.
  if(Cmd->RegisterCompleted(ExitStatus)) {
    ProfileSample *Sample = GetProfilerSample(*this,
                                              Counters,
                                              ProfileSample::CommandCompleted);
    Ev.MarkCompleted(Cmd->GetExitStatus(), Sample);
  }

  delete Cmd;
}

void CPUDevice::InitDeviceInfo(sys::HardwareNode &Node) {
  // TODO: define device geometry and set all properties!

  VendorID = 0;
  MaxComputeUnits = Node.CPUsCount();
  MaxWorkItemDimensions = 3;
  MaxWorkItemSizes.assign(3, 1024);
  MaxWorkGroupSize = 1024;

  // TODO: Preferred* should be gathered by the compiler.
  // TODO: Native* should be gathered by the compiler.
  // TODO: set MaxClockFrequency.
  // TODO: set AddressBits.

  MaxMemoryAllocSize = Node.GetMemorySize();

  // TODO: set image properties.

  // TODO: set MaxParameterSize.

  // TODO: set MemoryBaseAddressAlignment.
  // TODO: set MinimumDataTypeAlignment.

  // TODO: set SinglePrecisionFPCapabilities.

  GlobalMemoryCacheType = DeviceInfo::ReadWriteCache;

  // Assuming symmetric systems.
  if(Node.llc_begin() != Node.llc_end()) {
    const sys::HardwareCache &Cache = Node.llc_front();

    GlobalMemoryCachelineSize = Cache.GetLineSize();
    GlobalMemoryCacheSize = Cache.GetSize();
  }

  GlobalMemorySize = Node.GetMemorySize();

  // TODO: set MaxConstantBufferSize.
  // TODO: set MaxConstantArguments.

  LocalMemoryMapping = DeviceInfo::SharedLocal;
  if(GlobalMemoryCacheSize)
    LocalMemorySize = GlobalMemoryCacheSize;
  SupportErrorCorrection = true;

  HostUnifiedMemory = true;

  // TODO: set ProfilingTimerResolution.
  
  // TODO: set LittleEndian, by the compiler?
  // Available is a virtual property.

  CompilerAvailable = false;

  ExecutionCapabilities = DeviceInfo::CanExecKernel |
                          DeviceInfo::CanExecNativeKernel;

  // TODO: set SizeTypeMax, by the compiler?
  PrivateMemorySize = LocalMemorySize;
}

void CPUDevice::InitJIT() {
  // Init the native target.
  llvm::InitializeNativeTarget();

  // Create the JIT.
  llvm::EngineBuilder Bld(&*BitCodeLibrary);
  llvm::ExecutionEngine *Engine = Bld.setEngineKind(llvm::EngineKind::JIT)
                                     .setOptLevel(llvm::CodeGenOpt::None)
                                     .create();

  // Configure the JIT.
  Engine->InstallLazyFunctionCreator(LibLinker);

  // Force compilation of all functions.
  for(llvm::Module::iterator I = BitCodeLibrary->begin(),
                             E = BitCodeLibrary->end();
                             I != E;
                             ++I)
    Engine->runJITOnFunction(&*I);

  // Save pointer.
  JIT.reset(Engine);
}

void CPUDevice::InitInternalCalls() {
  opencrun::OpenCLMetadataHandler MDHandler(*BitCodeLibrary);

  intptr_t AddrInt;
  void *Addr;

  llvm::Function *Func;

  #define INTERNAL_CALL(N, F)                 \
    AddrInt = reinterpret_cast<intptr_t>(F);  \
    Addr = reinterpret_cast<void *>(AddrInt); \
    Func = MDHandler.GetBuiltin(#N);          \
    JIT->addGlobalMapping(Func, Addr);
  #include "InternalCalls.def"
  #undef INTERNAL_CALL
}

void CPUDevice::InitMultiprocessors(sys::HardwareNode &Node) {
  for(sys::HardwareNode::const_llc_iterator I = Node.llc_begin(),
                                            E = Node.llc_end();
                                            I != E;
                                            ++I)
    Multiprocessors.insert(new Multiprocessor(*this, *I));
}

void CPUDevice::DestroyJIT() {
  JIT->removeModule(&*BitCodeLibrary);
}

void CPUDevice::DestroyMultiprocessors() {
  llvm::DeleteContainerPointers(Multiprocessors);
}

bool CPUDevice::Submit(EnqueueReadBuffer &Cmd) {
  // TODO: implement a smarter selection policy.
  Multiprocessor &MP = **Multiprocessors.begin();

  return MP.Submit(new ReadBufferCPUCommand(Cmd, Global[Cmd.GetSource()]));
}

bool CPUDevice::Submit(EnqueueWriteBuffer &Cmd) {
  // TODO: implement a smarter selection policy.
  Multiprocessor &MP = **Multiprocessors.begin();

  return MP.Submit(new WriteBufferCPUCommand(Cmd, Global[Cmd.GetTarget()]));
}

bool CPUDevice::Submit(EnqueueNDRangeKernel &Cmd) {
  // Get global and constant buffer mappings.
  GlobalArgMappingsContainer GlobalArgs;
  LocateMemoryObjArgAddresses(Cmd.GetKernel(), GlobalArgs);

  // TODO: analyze kernel and decide the scheduling policy to use.
  return BlockParallelSubmit(Cmd, GlobalArgs);
}

bool CPUDevice::Submit(EnqueueNativeKernel &Cmd) {
  // TODO: implement a smarter selection policy.
  Multiprocessor &MP = **Multiprocessors.begin();

  Memory::MappingsContainer Mappings;
  Global.GetMappings(Mappings);

  Cmd.RemapMemoryObjAddresses(Mappings);

  return MP.Submit(new NativeKernelCPUCommand(Cmd));
}

bool CPUDevice::BlockParallelSubmit(EnqueueNDRangeKernel &Cmd,
                                    GlobalArgMappingsContainer &GlobalArgs) {
  // Native launcher address.
  BlockParallelEntryPoint Entry = GetBlockParallelEntryPoint(Cmd.GetKernel());

  // Index space.
  DimensionInfo &DimInfo = Cmd.GetDimensionInfo();

  // Decide the work group size.
  if(!Cmd.IsLocalWorkGroupSizeSpecified()) {
    llvm::SmallVector<size_t, 4> Sizes;

    for(unsigned I = 0; I < DimInfo.GetDimensions(); ++I)
      Sizes.push_back(DimInfo.GetGlobalWorkItems(I));

    DimInfo.SetLocalWorkItems(Sizes);
  }

  // Holds data about kernel result.
  llvm::IntrusiveRefCntPtr<CPUCommand::ResultRecorder> Result;
  Result = new CPUCommand::ResultRecorder(Cmd.GetWorkGroupsCount());

  MultiprocessorsContainer::iterator S = Multiprocessors.begin(),
                                     F = Multiprocessors.end(),
                                     J = S;
  size_t WorkGroupSize = DimInfo.GetLocalWorkItems();
  bool AllSent = true;

  // Perfect load balancing.
  for(DimensionInfo::iterator I = DimInfo.begin(),
                              E = DimInfo.end();
                              I != E;
                              I += WorkGroupSize) {
    NDRangeKernelBlockCPUCommand *NDRangeCmd;
    NDRangeCmd = new NDRangeKernelBlockCPUCommand(Cmd,
                                                  Entry,
                                                  GlobalArgs,
                                                  I,
                                                  I + WorkGroupSize,
                                                  *Result);

    // Submit command.
    AllSent = AllSent && (*J)->Submit(NDRangeCmd);

    // Reset counter, round robin.
    if(++J == F)
      J = S;
  }

  return AllSent;
}

CPUDevice::BlockParallelEntryPoint
CPUDevice::GetBlockParallelEntryPoint(Kernel &Kern) {
  llvm::sys::ScopedLock Lock(ThisLock);

  // Cache hit.
  if(BlockParallelEntriesCache.count(&Kern))
    return BlockParallelEntriesCache[&Kern];

  // Cache miss.
  llvm::Module &Mod = *Kern.GetModule(*this);
  llvm::StringRef KernName = Kern.GetName();

  // Build the entry point.
  llvm::PassManager PM;
  PM.add(CreateGroupParallelStubPass(KernName));
  PM.run(Mod);

  // Retrieve it.
  std::string EntryName = MangleBlockParallelKernelName(KernName);
  llvm::Function *EntryFn = Mod.getFunction(EntryName);

  // Force translation to native code.
  SignalJITCallStart(*this);
  JIT->addModule(&Mod);
  void *EntryPtr = JIT->getPointerToFunction(EntryFn);
  SignalJITCallEnd();

  // Build a command to invoke static constructors.
  RunStaticConstructorsCPUCommand::ConstructorsInvoker Invoker(Mod, *JIT);
  RunStaticConstructorsCPUCommand *Cmd;
  sys::FastRendevouz Synch;

  Cmd = new RunStaticConstructorsCPUCommand(Invoker, Synch);

  Multiprocessor &MP = **Multiprocessors.begin();

  // None available for executing the command: it is critical, run directly.
  if(!MP.Submit(Cmd)) {
    Invoker();
    delete Cmd;

  // Wait for other thread invoking static constructors.
  } else
    Synch.Wait();

  // Cache it. In order to correctly cast the EntryPtr to a function pointer we
  // must pass through a uintptr_t. Otherwise, a warning is issued by the
  // compiler.
  uintptr_t EntryPtrInt = reinterpret_cast<uintptr_t>(EntryPtr);
  CPUDevice::BlockParallelEntryPoint Entry;
  Entry = reinterpret_cast<CPUDevice::BlockParallelEntryPoint>(EntryPtrInt);
  BlockParallelEntriesCache[&Kern] = Entry;

  return Entry;
}

void *CPUDevice::LinkLibFunction(const std::string &Name) {
  // Bit-code function.
  if(llvm::Function *Func = JIT->FindFunctionNamed(Name.c_str()))
    return JIT->getPointerToFunction(Func);

  return NULL;
}

void CPUDevice::LocateMemoryObjArgAddresses(
                  Kernel &Kern,
                  GlobalArgMappingsContainer &GlobalArgs) {
  for(Kernel::arg_iterator I = Kern.arg_begin(),
                           E = Kern.arg_end();
                           I != E;
                           ++I)
    if(BufferKernelArg *Arg = llvm::dyn_cast<BufferKernelArg>(*I)) {
      // Local mappings handled by Multiprocessor.
      if(Arg->OnLocalAddressSpace())
        continue;

      unsigned I = Arg->GetPosition();

      if(Buffer *Buf = Arg->GetBuffer())
        GlobalArgs[I] = Global[*Buf];
      else
        GlobalArgs[I] = NULL;
    }
}

//
// LibLinker implementation
//

namespace {

llvm::sys::ThreadLocal<const CPUDevice> CurDevice;

} // End anonymous namespace.

void *opencrun::LibLinker(const std::string &Name) {
  void *Func = NULL;

  if(CPUDevice *Dev = const_cast<CPUDevice *>(CurDevice.get()))
    Func = Dev->LinkLibFunction(Name);

  return Func;
}

namespace {

void SignalJITCallStart(CPUDevice &Dev) {
  // Save current device, it can be used by the JIT trampoline later.
  CurDevice.set(&Dev);
}

void SignalJITCallEnd() {
  // JIT is done, remove current device.
  CurDevice.erase();
}

} // End anonymous namespace.
