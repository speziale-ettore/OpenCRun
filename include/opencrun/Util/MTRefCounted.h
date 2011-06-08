
//===- MTRefCounted.h - Multi-threaded Reference Counted Pointer ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This files implements classes to manage reference counted pointers shared
// among multiple threads. With respect to classes defined in
// "llvm/ADT/IntrusiveRefCntPtr.h", here we perform reference counter updates
// using atomic read-modify-write operations. Moreover, the reference counter is
// exposed -- required by the OpenCL specifications for debugging purposes --
// and it it possible to updating it directly, via the Release/Retain member
// functions -- OpenCL objects must support this features by standard.
//
// For usage internal of the library, please use the llvm::IntrusiveRefCntPtr
// class to handle pointers to multi-threaded reference counted objects.
//
//===----------------------------------------------------------------------===//

#ifndef OPENCRUN_UTIL_MTREFCOUNTED_H
#define OPENCRUN_UTIL_MTREFCOUNTED_H

#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/Support/Atomic.h"

namespace opencrun {

//===----------------------------------------------------------------------===//
/// MTRefCountedBase - Multi-threaded reference counted for static allocations.
///  See llvm::RefCountedBase for details.
//===----------------------------------------------------------------------===//
template <typename Derived>
class MTRefCountedBase {
protected:
  MTRefCountedBase() : Refs(0) { }

public:
  void Retain() {
    llvm::sys::AtomicIncrement(&Refs);
  }

  void Release() {
    if(llvm::sys::AtomicDecrement(&Refs) == 0)
      delete static_cast<Derived *>(this);
  }

  unsigned GetReferenceCount() const { return Refs; }

private:
  volatile llvm::sys::cas_flag Refs;

  friend class llvm::IntrusiveRefCntPtr<Derived>;
};

//===----------------------------------------------------------------------===//
/// MTRefCountedBaseVPTR - Multi-threaded reference counted for object having a
///  a virtual destructor. See llvm::RefCountedBaseVPTR for details.
//===----------------------------------------------------------------------===//
template <typename Derived>
class MTRefCountedBaseVPTR {
protected:
  MTRefCountedBaseVPTR() : Refs(0) { }
  virtual ~MTRefCountedBaseVPTR() { }

public:
  void Retain() {
    llvm::sys::AtomicIncrement(&Refs);
  }

  void Release() {
    if(llvm::sys::AtomicDecrement(&Refs) == 0)
      delete this;
  }

  unsigned GetReferenceCount() const { return Refs; }

private:
  volatile llvm::sys::cas_flag Refs;

  friend class llvm::IntrusiveRefCntPtr<Derived>;
};

} // End namespace opencrun.

#endif // OPENCRUN_UTIL_MTREFCOUNTED_H
