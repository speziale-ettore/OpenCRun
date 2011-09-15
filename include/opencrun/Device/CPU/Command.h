
#ifndef OPENCRUN_DEVICE_CPU_COMMAND_H
#define OPENCRUN_DEVICE_CPU_COMMAND_H

#include "opencrun/Core/Command.h"
#include "opencrun/Core/Context.h"
#include "opencrun/Device/CPU/Memory.h"
#include "opencrun/System/FastRendevouz.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"

namespace opencrun {
namespace cpu {

class CPUCommand {
public:
  enum Type {
    RunStaticConstructors,
    RunStaticDestructors,
    StopDevice,
    LastCPUServiceCommand,
    ReadBuffer,
    WriteBuffer,
    NativeKernel,
    LastCPUSingleExecCommand,
    NDRangeKernelBlock
  };

  enum {
    NoError     = CL_COMPLETE,
    Unsupported = CL_INVALID_OPERATION
  };

public:
  class ResultRecorder : public MTRefCountedBase<ResultRecorder> {
  public:
    typedef llvm::SmallVector<int, 32> StatusContainer;

  public:
    ResultRecorder(unsigned N = 1) : Started(false),
                                     ToWait(N),
                                     ExitStatus(N, CPUCommand::NoError) { }

  public:
    bool SetStarted() {
      // Common case, command started -- skip atomic operations.
      if(Started)
        return false;

      // Infrequent/contented case -- use CAS.
      return !llvm::sys::CompareAndSwap(&Started, true, false);
    }

    bool SetExitStatus(unsigned ExitStatus) {
      // Decrement the number of acknowledgment to wait.
      unsigned I = llvm::sys::AtomicDecrement(&ToWait);

      // The returned number is the caller Id inside the exit status array.
      this->ExitStatus[I] = ExitStatus;

      // Return true if this is the last acknowledgment.
      return I == 0;
    }

    int GetExitStatus() {
      int Status = CPUCommand::NoError;

      for(StatusContainer::iterator I = ExitStatus.begin(),
                                    E = ExitStatus.end();
                                    I != E && Status == CPUCommand::NoError;
                                    ++I)
        Status = *I;

      return Status;
    }

  private:
    volatile llvm::sys::cas_flag Started;
    volatile llvm::sys::cas_flag ToWait;
    StatusContainer ExitStatus;
  };

protected:
  CPUCommand(Type CommandTy) : CommandTy(CommandTy),
                               Result(new CPUCommand::ResultRecorder()) { }

  CPUCommand(Type CommandTy, CPUCommand::ResultRecorder &Result) :
    CommandTy(CommandTy),
    Result(&Result) { }

public:
  virtual ~CPUCommand() { }

public:
  Type GetType() const { return CommandTy; }

  virtual bool IsProfiled() const = 0;

  int GetExitStatus() { return Result->GetExitStatus(); }

public:
  bool RegisterStarted() {
    return Result->SetStarted();
  }

  bool RegisterCompleted(int ExitStatus) {
    return Result->SetExitStatus(ExitStatus);
  }

private:
  Type CommandTy;
  llvm::IntrusiveRefCntPtr<ResultRecorder> Result;
};

class CPUServiceCommand : public CPUCommand {
public:
  static bool classof(const CPUCommand *Cmd) {
    return Cmd->GetType() < CPUCommand::LastCPUServiceCommand;
  }

protected:
  CPUServiceCommand(CPUCommand::Type CommandTy) : CPUCommand(CommandTy),
                                                  Synch(NULL) { }

  CPUServiceCommand(CPUCommand::Type CommandTy,
                    sys::FastRendevouz &Synch) : CPUCommand(CommandTy),
                                                 Synch(&Synch) { }

public:
  virtual ~CPUServiceCommand() { if(Synch) Synch->Signal(); }

public:
  virtual bool IsProfiled() const { return false; }

private:
  sys::FastRendevouz *Synch;
};

class CPUExecCommand : public CPUCommand {
public:
  static bool classof(const CPUCommand *Cmd) {
    return Cmd->GetType() > CPUCommand::LastCPUServiceCommand;
  }

protected:
  CPUExecCommand(CPUCommand::Type CommandTy, Command &Cmd) :
    CPUCommand(CommandTy),
    Cmd(Cmd) { }

  CPUExecCommand(CPUCommand::Type CommandTy,
                 Command &Cmd,
                 CPUCommand::ResultRecorder &Result) :
    CPUCommand(CommandTy, Result),
    Cmd(Cmd) { }

public:
  Command &GetQueueCommand() const { return Cmd; }
  template <typename Ty>
  Ty &GetQueueCommandAs() const { return llvm::cast<Ty>(Cmd); }

  virtual bool IsProfiled() const { return Cmd.IsProfiled(); }

private:
  Command &Cmd;
};

class CPUSingleExecCommand : public CPUExecCommand {
public:
  static bool classof(const CPUCommand *Cmd) {
    return CPUExecCommand::classof(Cmd) &&
           Cmd->GetType() < CPUCommand::LastCPUSingleExecCommand;
  }

protected:
  CPUSingleExecCommand(CPUCommand::Type CommandTy, Command &Cmd)
    : CPUExecCommand(CommandTy, Cmd) { }
};

class CPUMultiExecCommand : public CPUExecCommand {
public:
  static bool classof(const CPUCommand *Cmd) {
    return CPUExecCommand::classof(Cmd) &&
           Cmd->GetType() > CPUCommand::LastCPUSingleExecCommand;
  }

protected:
  CPUMultiExecCommand(CPUCommand::Type CommandTy,
                      Command &Cmd,
                      CPUCommand::ResultRecorder &Result,
                      unsigned Id) :
    CPUExecCommand(CommandTy, Cmd, Result),
    Id(Id) { }

public:
  unsigned GetId() const { return Id; }

private:
  unsigned Id;
};

class RunStaticConstructorsCPUCommand : public CPUServiceCommand {
public:
  static bool classof(const CPUCommand *Cmd) {
    return Cmd->GetType() == CPUCommand::RunStaticConstructors;
  }

public:
  class ConstructorsInvoker {
  public:
    ConstructorsInvoker(llvm::Module &Mod,
                        llvm::ExecutionEngine &Engine) : Mod(Mod),
                                                         Engine(Engine) { }

  public:
    void operator()() {
      // Do not lock the execution engine. This because the JIT lock is held by
      // the thread that has sent the command associated to this object and that
      // thread is spin-waiting for static constructors invocation.
      Engine.runStaticConstructorsDestructors(&Mod, false);
    }

  private:
    llvm::Module &Mod;
    llvm::ExecutionEngine &Engine;
  };

public:
  RunStaticConstructorsCPUCommand(ConstructorsInvoker Invoker,
                                  sys::FastRendevouz &Synch) :
    CPUServiceCommand(CPUCommand::RunStaticConstructors, Synch),
    Invoker(Invoker) { }

public:
  ConstructorsInvoker &GetInvoker() { return Invoker; }

private:
  ConstructorsInvoker Invoker;
};

class RunStaticDestructorsCPUCommand : public CPUServiceCommand {
public:
  static bool classof(const CPUCommand *Cmd) {
    return Cmd->GetType() == CPUCommand::RunStaticDestructors;
  }

public:
  class DestructorsInvoker {
  public:
    DestructorsInvoker(llvm::Module &Mod,
                       llvm::ExecutionEngine &Engine) : Mod(Mod),
                                                        Engine(Engine) { }

  public:
    void operator()() {
      // Do not lock the execution engine. This because the JIT lock is held by
      // the thread that has sent the command associated to this object and that
      // thread is spin-waiting for static constructors invocation.
      Engine.runStaticConstructorsDestructors(&Mod, true);
    }

  private:
    llvm::Module &Mod;
    llvm::ExecutionEngine &Engine;
  };

public:
  RunStaticDestructorsCPUCommand(DestructorsInvoker Invoker,
                                 sys::FastRendevouz &Synch) :
    CPUServiceCommand(CPUCommand::RunStaticDestructors, Synch),
    Invoker(Invoker) { }

public:
  DestructorsInvoker &GetInvoker() { return Invoker; }

private:
  DestructorsInvoker Invoker;
};

class StopDeviceCPUCommand : public CPUServiceCommand {
public:
  static bool classof(const CPUCommand *Cmd) {
    return Cmd->GetType() == CPUCommand::StopDevice;
  }

public:
  StopDeviceCPUCommand() : CPUServiceCommand(CPUCommand::StopDevice) { }
};

class ReadBufferCPUCommand : public CPUExecCommand {
public:
  static bool classof(const CPUCommand *Cmd) {
    return Cmd->GetType() == CPUCommand::ReadBuffer;
  }

public:
  ReadBufferCPUCommand(EnqueueReadBuffer &Cmd, const void *Src)
    : CPUExecCommand(CPUCommand::ReadBuffer, Cmd),
      Src(Src) { }

  void *GetTarget() {
    return GetQueueCommandAs<EnqueueReadBuffer>().GetTarget();
  }

  const void *GetSource() {
    uintptr_t Base = reinterpret_cast<uintptr_t>(Src);
    EnqueueReadBuffer &Cmd = GetQueueCommandAs<EnqueueReadBuffer>();

    return reinterpret_cast<const void *>(Base + Cmd.GetOffset());
  }

  size_t GetSize() {
    return GetQueueCommandAs<EnqueueReadBuffer>().GetSize();
  }

private:
  const void *Src;
};

class WriteBufferCPUCommand : public CPUExecCommand {
public:
  static bool classof(const CPUCommand *Cmd) {
    return Cmd->GetType() == CPUCommand::WriteBuffer;
  }

public:
  WriteBufferCPUCommand(EnqueueWriteBuffer &Cmd, void *Target)
    : CPUExecCommand(CPUCommand::WriteBuffer, Cmd),
      Target(Target) { }

public:
  void *GetTarget() {
    uintptr_t Base = reinterpret_cast<uintptr_t>(Target);
    EnqueueWriteBuffer &Cmd = GetQueueCommandAs<EnqueueWriteBuffer>();
    
    return reinterpret_cast<void *>(Base + Cmd.GetOffset());
  }

  const void *GetSource() {
    return GetQueueCommandAs<EnqueueWriteBuffer>().GetSource();
  }

  size_t GetSize() {
    return GetQueueCommandAs<EnqueueWriteBuffer>().GetSize();
  }

private:
  void *Target;
};

class NDRangeKernelBlockCPUCommand : public CPUMultiExecCommand {
public:
  static bool classof(const CPUCommand *Cmd) {
    return Cmd->GetType() == CPUCommand::NDRangeKernelBlock;
  }

public:
  typedef void (*Signature) (void **);

  typedef llvm::DenseMap<unsigned, void *> ArgsMappings;

public:
  DimensionInfo::iterator index_begin() { return Start; }
  DimensionInfo::iterator index_end() { return End; }

public:
  NDRangeKernelBlockCPUCommand(EnqueueNDRangeKernel &Cmd,
                               Signature Entry,
                               ArgsMappings &GlobalArgs,
                               DimensionInfo::iterator I,
                               DimensionInfo::iterator E,
                               CPUCommand::ResultRecorder &Result);

  ~NDRangeKernelBlockCPUCommand();

public:
  void SetLocalParams(LocalMemory &Local);

public:
  Signature &GetFunction() { return Entry; }
  void **GetArgumentsPointer() { return Args; }

  Kernel &GetKernel() {
    return GetQueueCommandAs<EnqueueNDRangeKernel>().GetKernel();
  }

  DimensionInfo &GetDimensionInfo() {
    return GetQueueCommandAs<EnqueueNDRangeKernel>().GetDimensionInfo();
  }

private:
  Signature Entry;
  void **Args;

  DimensionInfo::iterator Start;
  DimensionInfo::iterator End;
};

class NativeKernelCPUCommand : public CPUExecCommand {
public:
  static bool classof(const CPUCommand *Cmd) {
    return Cmd->GetType() == CPUCommand::NativeKernel;
  }

public:
  typedef EnqueueNativeKernel::Signature Signature;

public:
  NativeKernelCPUCommand(EnqueueNativeKernel &Cmd)
    : CPUExecCommand(CPUCommand::NativeKernel, Cmd) { }

public:
  Signature &GetFunction() const {
    return GetQueueCommandAs<EnqueueNativeKernel>().GetFunction();
  }

  void *GetArgumentsPointer() const {
    return GetQueueCommandAs<EnqueueNativeKernel>().GetArgumentsPointer();
  }
};

} // End namespace cpu.
} // End namespace opencrun.

#endif // OPENCRUN_DEVICE_CPU_COMMAND_H
