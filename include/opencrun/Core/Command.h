
#ifndef OPENCRUN_CORE_COMMAND_H
#define OPENCRUN_CORE_COMMAND_H

#include "CL/opencl.h"

#include "opencrun/Core/Kernel.h"
#include "opencrun/Util/DimensionInfo.h"

#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/SmallVector.h"

#include <map>

namespace opencrun {

class Context;
class Buffer;
class Device;
class Event;
class InternalEvent;
class MemoryObj;

class Command {
public:
  enum Type {
    ReadBuffer,
    WriteBuffer,
    NDRangeKernel,
    NativeKernel
  };

public:
  typedef llvm::SmallVector<Event *, 8> EventsContainer;

  typedef EventsContainer::iterator event_iterator;
  typedef EventsContainer::const_iterator const_event_iterator;

public:
  event_iterator wait_begin() { return WaitList.begin(); }
  event_iterator wait_end() { return WaitList.end(); }

  const_event_iterator wait_begin() const { return WaitList.begin(); }
  const_event_iterator wait_end() const { return WaitList.end(); }

public:
  virtual ~Command() { }

protected:
  Command(Type CommandTy,
          EventsContainer& WaitList,
          bool Blocking = false) : CommandTy(CommandTy),
                                   WaitList(WaitList),
                                   Blocking(Blocking) { }

public:
  void SetNotifyEvent(InternalEvent &Ev) { NotifyEv = &Ev; }
  InternalEvent &GetNotifyEvent() { return *NotifyEv; }

  Type GetType() const { return CommandTy; }
  Context &GetContext() const;

public:
  bool CanRun() const;
  bool IsProfiled() const;
  bool IsBlocking() const { return Blocking; }

protected:
  Type CommandTy;
  EventsContainer WaitList;
  bool Blocking;

  InternalEvent *NotifyEv;
};

class EnqueueReadBuffer : public Command {
public:
  static bool classof(const Command *Cmd) {
    return Cmd->GetType() == Command::ReadBuffer;
  }

private:
  EnqueueReadBuffer(void *Target,
                    Buffer &Src,
                    bool Blocking,
                    size_t Offset,
                    size_t Size,
                    EventsContainer &WaitList);

public:
  void *GetTarget() { return Target; }
  Buffer &GetSource() { return *Src; }
  size_t GetOffset() { return Offset; }
  size_t GetSize() { return Size; }

private:
  void *Target;
  llvm::IntrusiveRefCntPtr<Buffer> Src;
  size_t Offset;
  size_t Size;

  friend class EnqueueReadBufferBuilder;
};

class EnqueueWriteBuffer : public Command {
public:
  static bool classof(const Command *Cmd) {
    return Cmd->GetType() == Command::WriteBuffer;
  }

private:
  EnqueueWriteBuffer(Buffer &Target,
                     const void *Src,
                     bool Blocking,
                     size_t Offset,
                     size_t Size,
                     EventsContainer &WaitList);

public:
  Buffer &GetTarget() { return *Target; }
  const void *GetSource() { return Src; }
  size_t GetOffset() { return Offset; }
  size_t GetSize() { return Size; }

private:
  llvm::IntrusiveRefCntPtr<Buffer> Target;
  const void *Src;
  size_t Offset;
  size_t Size;

  friend class EnqueueWriteBufferBuilder;
};

class EnqueueNDRangeKernel : public Command {
public:
  static bool classof(const Command *Cmd) {
    return Cmd->GetType() == Command::NDRangeKernel;
  }

private:
  EnqueueNDRangeKernel(Kernel &Kern,
                       DimensionInfo &DimInfo,
                       EventsContainer &WaitList);

public:
  Kernel &GetKernel() { return *Kern; }
  DimensionInfo &GetDimensionInfo() { return DimInfo; }

  unsigned GetWorkGroupsCount() const { return DimInfo.GetWorkGroupsCount(); }

public:
  bool IsLocalWorkGroupSizeSpecified() const {
    return DimInfo.IsLocalWorkGroupSizeSpecified();
  }

private:
  llvm::IntrusiveRefCntPtr<Kernel> Kern;
  DimensionInfo DimInfo;

  friend class EnqueueNDRangeKernelBuilder;
};

class EnqueueNativeKernel : public Command {
public:
  static bool classof(const Command *Cmd) {
    return Cmd->GetType() == Command::NativeKernel;
  }

public:
  typedef void (*Signature)(void*);
  typedef std::pair<void *, size_t> Arguments;
  typedef std::map<MemoryObj *, void *> MappingsContainer;

private:
  EnqueueNativeKernel(Signature &Func,
                      Arguments &RawArgs,
                      MappingsContainer &Mappings,
                      EventsContainer &WaitList);
  ~EnqueueNativeKernel();

public:
  void RemapMemoryObjAddresses(const MappingsContainer &GlobalMappings);

public:
  Signature &GetFunction() { return Func; }
  void *GetArgumentsPointer() { return RawArgs.first; }

private:
  Signature Func;
  Arguments RawArgs;
  MappingsContainer Mappings;

  friend class EnqueueNativeKernelBuilder;
};

class CommandBuilder {
public:
  enum Type {
    EnqueueReadBufferBuilder,
    EnqueueWriteBufferBuilder,
    EnqueueNDRangeKernelBuilder,
    EnqueueNativeKernelBuilder
  };

protected:
  CommandBuilder(Type BldTy, Context &Ctx) : Ctx(Ctx),
                                             ErrCode(CL_SUCCESS),
                                             BldTy(BldTy) { }

public:
  CommandBuilder &SetWaitList(unsigned N, const cl_event *Evs);

public:
  Type GetType() const { return BldTy; }

  bool IsWaitListInconsistent() const;

protected:
  CommandBuilder &NotifyError(cl_int ErrCode, const char *Msg = "");

protected:
  Context &Ctx;
  Command::EventsContainer WaitList;

  cl_int ErrCode;

private:
  Type BldTy;
};

class EnqueueReadBufferBuilder : public CommandBuilder {
public:
  static bool classof(const CommandBuilder *Bld) {
    return Bld->GetType() == CommandBuilder::EnqueueReadBufferBuilder;
  }

public:
  EnqueueReadBufferBuilder(Context &Ctx, cl_mem Buf, void *Target);

public:
  EnqueueReadBufferBuilder &SetBlocking(bool Blocking = true);
  EnqueueReadBufferBuilder &SetCopyArea(size_t Offset, size_t Size);
  EnqueueReadBufferBuilder &SetWaitList(unsigned N, const cl_event *Evs);

  EnqueueReadBuffer *Create(cl_int *ErrCode);

private:
  EnqueueReadBufferBuilder &NotifyError(cl_int ErrCode,
                                        const char *Msg = "") {
    CommandBuilder::NotifyError(ErrCode, Msg);
    return *this;
  }

private:
  Buffer *Src;
  void *Target;
  bool Blocking;
  size_t Offset;
  size_t Size;
};

class EnqueueWriteBufferBuilder : public CommandBuilder {
public:
  static bool classof(const CommandBuilder *Bld) {
    return Bld->GetType() == CommandBuilder::EnqueueWriteBufferBuilder;
  }

public:
  EnqueueWriteBufferBuilder(Context &Ctx, cl_mem Buf, const void *Src);

public:
  EnqueueWriteBufferBuilder &SetBlocking(bool Blocking = true);
  EnqueueWriteBufferBuilder &SetCopyArea(size_t Offset, size_t Size);
  EnqueueWriteBufferBuilder &SetWaitList(unsigned N, const cl_event *Evs);

  EnqueueWriteBuffer *Create(cl_int *ErrCode);

private:
  EnqueueWriteBufferBuilder &NotifyError(cl_int ErrCode,
                                         const char *Msg = "") {
    CommandBuilder::NotifyError(ErrCode, Msg);
    return *this;
  }

private:
  Buffer *Target;
  const void *Src;
  bool Blocking;
  size_t Offset;
  size_t Size;
};

class EnqueueNDRangeKernelBuilder : public CommandBuilder {
public:
  static bool classof(const CommandBuilder *Bld) {
    return Bld->GetType() == CommandBuilder::EnqueueNDRangeKernelBuilder;
  }

public:
  EnqueueNDRangeKernelBuilder(Context &Ctx,
                              Device &Dev,
                              cl_kernel Kern,
                              unsigned WorkDimensions,
                              const size_t *GlobalWorkSizes);

public:
  EnqueueNDRangeKernelBuilder &
  SetGlobalWorkOffset(const size_t *GlobalWorkOffsets);

  EnqueueNDRangeKernelBuilder &SetLocalWorkSize(const size_t *LocalWorkSizes);
  EnqueueNDRangeKernelBuilder &SetWaitList(unsigned N, const cl_event *Evs);

  EnqueueNDRangeKernel *Create(cl_int *ErrCode);

private:
  EnqueueNDRangeKernelBuilder &NotifyError(cl_int ErrCode,
                                           const char *Msg = "") {
    CommandBuilder::NotifyError(ErrCode, Msg);
    return *this;
  }

private:
  Device &Dev;
  Kernel *Kern;

  unsigned WorkDimensions;
  llvm::SmallVector<size_t, 4> GlobalWorkSizes;
  llvm::SmallVector<size_t, 4> GlobalWorkOffsets;
  llvm::SmallVector<size_t, 4> LocalWorkSizes;
};

class EnqueueNativeKernelBuilder : public CommandBuilder {
public:
  static bool classof(const CommandBuilder *Bld) {
    return Bld->GetType() == CommandBuilder::EnqueueNativeKernelBuilder;
  }

public:
  EnqueueNativeKernelBuilder(Context &Ctx,
                             EnqueueNativeKernel::Signature Func,
                             EnqueueNativeKernel::Arguments &RawArgs);

public:
  EnqueueNativeKernelBuilder &SetMemoryMappings(unsigned N,
                                                const cl_mem *MemObjs,
                                                const void **MemLocs);
  EnqueueNativeKernelBuilder &SetWaitList(unsigned N, const cl_event *Evs);

  EnqueueNativeKernel *Create(cl_int *ErrCode);

private:
  EnqueueNativeKernelBuilder &NotifyError(cl_int ErrCode,
                                          const char *Msg = "") {
    CommandBuilder::NotifyError(ErrCode, Msg);
    return *this;
  }

private:
  EnqueueNativeKernel::Signature Func;
  EnqueueNativeKernel::Arguments RawArgs;
  EnqueueNativeKernel::MappingsContainer Mappings;
};

} // End namespace opencrun.

#endif // OPENCRUN_CORE_COMMAND_H
