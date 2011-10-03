
#ifndef LIBOPENCRUN_UTILS_H
#define LIBOPENCRUN_UTILS_H

#include "opencrun/Core/CommandQueue.h"
#include "opencrun/Core/Context.h"
#include "opencrun/Core/Device.h"
#include "opencrun/Core/Platform.h"

#include "llvm/ADT/StringRef.h"

#include <cstring>

#define RETURN_WITH_ERROR(VAR, ERRCODE) \
  {                                     \
  if(VAR)                               \
    *VAR = ERRCODE;                     \
  return NULL;                          \
  }

#define RETURN_WITH_FLAG(VAR, ERRCODE) \
  {                                    \
  if(VAR)                              \
    *VAR = ERRCODE;                    \
  return false;                        \
  }

#define RETURN_WITH_EVENT(VAR, EV) \
  {                                \
  if(VAR)                          \
    *VAR = EV;                     \
  else                             \
    EV->Release();                 \
  return CL_SUCCESS;               \
  }

template <typename ParamTy, typename FunTy>
ParamTy clReadValue(FunTy Src) {
  return static_cast<ParamTy>(Src);
}

#define CL_OBJECT_READ_VALUE(PARAM_TY, FUN_TY)            \
  template <> inline                                      \
  PARAM_TY clReadValue<PARAM_TY, FUN_TY &>(FUN_TY &Src) { \
    return &Src;                                          \
  }

CL_OBJECT_READ_VALUE(cl_platform_id, opencrun::Platform)
CL_OBJECT_READ_VALUE(cl_device_id, opencrun::Device)
CL_OBJECT_READ_VALUE(cl_context, opencrun::Context)
CL_OBJECT_READ_VALUE(cl_command_queue, opencrun::CommandQueue)
CL_OBJECT_READ_VALUE(cl_program, opencrun::Program)
#undef CL_OBJECT_READ_VALUE

template <> inline
cl_command_type clReadValue<cl_command_type,
                            opencrun::Command &>(opencrun::Command &Cmd) {
  return Cmd.GetType();
}

template <typename ParamTy, typename FunTy>
cl_int clFillValue(ParamTy *Dst,
                   FunTy Src,
                   size_t DstSize,
                   size_t *RightSizeRet) {
  size_t RightSize = sizeof(ParamTy);

  if(RightSizeRet)
    *RightSizeRet = RightSize;

  if(Dst) {
    if(DstSize < RightSize)
      return CL_INVALID_VALUE;

    *Dst = clReadValue<ParamTy, FunTy>(Src);
  }

  return CL_SUCCESS;
}

template <> inline
cl_int clFillValue<char, llvm::StringRef>(char *Dst,
                                          llvm::StringRef Src,
                                          size_t DstSize,
                                          size_t *RightSizeRet) {
  size_t RightSize = sizeof(char) * (Src.size() + 1);

  if(RightSizeRet)
    *RightSizeRet = RightSize;

  if(Dst) {
    if(DstSize < RightSize)
      return CL_INVALID_VALUE;

    std::strncpy(Dst, Src.begin(), RightSize);
  }

  return CL_SUCCESS;
}

template <> inline
cl_int clFillValue<size_t, llvm::SmallVector<size_t, 4> &>(
  size_t *Dst,
  llvm::SmallVector<size_t, 4> &Src,
  size_t DstSize,
  size_t *RightSizeRet) {
  size_t RightSize = sizeof(size_t) * Src.size();

  if(RightSizeRet)
    *RightSizeRet = RightSize;

  if(Dst) {
    if(DstSize < RightSize)
      return CL_INVALID_VALUE;

    std::copy(Src.begin(), Src.end(), Dst);
  }

  return CL_SUCCESS;
}

template <typename ParamTy, typename Iter>
cl_int clFillValue(ParamTy *Dst,
                   Iter I,
                   Iter E,
                   size_t DstSize,
                   size_t *RightSizeRet) {
  size_t RightSize = (E - I) * sizeof(ParamTy);

  if(RightSizeRet)
    *RightSizeRet = RightSize;

  if(Dst) {
    if(DstSize < RightSize)
      return CL_INVALID_VALUE;

    for(; I != E; ++I)
      *(Dst++) = clReadValue<ParamTy, __typeof__(*I)>(*I);
  }

  return CL_SUCCESS;
}

#endif // LIBOPENCRUN_UTILS_H
