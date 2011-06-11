
#include "opencrun/Device/CPU/CPUDevice.h"
#include "opencrun/Core/Event.h"
#include "opencrun/Device/CPUPasses/AllPasses.h"
#include "opencrun/System/OS.h"

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
// Memory implementation.
//

Memory::~Memory() {
  for(MappingsContainer::iterator I = Mappings.begin(),
                                  E = Mappings.end();
                                  I != E;
                                  ++I)
    sys::Free(I->second);
}

void *Memory::Alloc(MemoryObj &MemObj) {
  size_t RequestedSize = MemObj.GetSize();

  llvm::sys::ScopedLock Lock(ThisLock);
  
  // TODO: more accurate check.
  if(Available < RequestedSize) 
    return NULL;

  size_t ActualSize;
  void *Addr = sys::CacheAlignedAlloc(MemObj.GetSize(), ActualSize);

  if(Addr) {
    Mappings[&MemObj] = Addr;
    Available -= ActualSize;
  }

  return Addr;
}

void *Memory::Alloc(HostBuffer &Buf) {
  return NULL;
}

void *Memory::Alloc(HostAccessibleBuffer &Buf) {
  return NULL;
}

void *Memory::Alloc(DeviceBuffer &Buf) {
  void *Addr = Alloc(llvm::cast<MemoryObj>(Buf));

  if(Buf.HasInitializationData())
    std::memcpy(Addr, Buf.GetInitializationData(), Buf.GetSize());

  return Addr;
}

void Memory::Free(MemoryObj &MemObj) {
  llvm::sys::ScopedLock Lock(ThisLock);

  MappingsContainer::iterator I = Mappings.find(&MemObj);
  if(I == Mappings.end())
    return;

  // TODO: remove the round-up error.
  Available += MemObj.GetSize();

  sys::Free(I->second);
  Mappings.erase(I);
}

//
// CPUDevice implementation.
//

CPUDevice::CPUDevice(sys::HardwareNode &Node) :
  Device("CPU", llvm::sys::getHostTriple()),
  Global(Node.GetMemorySize()) {
  InitDeviceInfo(Node);
  InitMultiprocessors(Node);
  InitJIT();
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

  if(Submitted) {
    unsigned Counters = Cmd.IsProfiled() ? Profiler::Time : Profiler::None;
    InternalEvent &Ev = Cmd.GetNotifyEvent();

    ProfileSample *Sample = GetProfilerSample(*this,
                                              Counters,
                                              ProfileSample::CommandSubmitted);
    Ev.MarkSubmitted(Sample);
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

  // Invoke static destructors in the device, waiting for completion.
  RunStaticDestructorsCPUCommand::DestructorsInvoker Invoker(Mod, *JIT);
  sys::FastRendevouz Synch;
  Multiprocessor &MP = **Multiprocessors.begin();
  MP.Submit(new RunStaticDestructorsCPUCommand(Invoker, Synch));
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
  Engine->DisableSymbolSearching();
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

  // TODO: remove critical race on mapping accesses.
  Cmd.RemapMemoryObjAddresses(Global.GetMappings());

  return MP.Submit(new NativeKernelCPUCommand(Cmd));
}

bool CPUDevice::BlockParallelSubmit(EnqueueNDRangeKernel &Cmd,
                                    GlobalArgMappingsContainer &GlobalArgs) {
  // Native launcher address.
  BlockParallelEntryPoint Entry = GetBlockParallelEntryPoint(Cmd.GetKernel());

  // Decide the work group size.
  // TODO: map to L1 size.
  if(!Cmd.IsLocalWorkGroupSizeSpecified()) {
    DimensionInfo &DimInfo = Cmd.GetDimensionInfo();
    llvm::SmallVector<size_t, 4> Sizes;

    for(unsigned I = 0; I < DimInfo.GetDimensions(); ++I)
      Sizes.push_back(DimInfo.GetGlobalWorkItems(I));

    DimInfo.SetWorkGroupsSize(Sizes);
  }

  // TODO: refactor.
  unsigned WorkGroups = Cmd.GetWorkGroupsCount();

  // Holds data about kernel result.
  llvm::IntrusiveRefCntPtr<CPUCommand::ResultRecorder> Result;
  Result = new CPUCommand::ResultRecorder(WorkGroups);

  MultiprocessorsContainer::iterator S = Multiprocessors.begin(),
                                     F = Multiprocessors.end(),
                                     J = S;
  bool AllSent = true;

  // Perfect load balancing.
  for(unsigned I = 0, E = WorkGroups; I != E; ++I) {
    NDRangeKernelBlockCPUCommand *NDRangeCmd;
    NDRangeCmd = new NDRangeKernelBlockCPUCommand(Cmd,
                                                  Entry,
                                                  I,
                                                  GlobalArgs,
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

  // Invoke static constructors in the device, waiting for completion.
  RunStaticConstructorsCPUCommand::ConstructorsInvoker Invoker(Mod, *JIT);
  sys::FastRendevouz Synch;
  Multiprocessor &MP = **Multiprocessors.begin();
  MP.Submit(new RunStaticConstructorsCPUCommand(Invoker, Synch));
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
  llvm::Function *Func = JIT->FindFunctionNamed(Name.c_str());
  if(!Func)
    return NULL;

  return JIT->getPointerToFunction(Func);
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
