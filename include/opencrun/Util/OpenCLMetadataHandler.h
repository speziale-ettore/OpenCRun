
//===- OpenCLMetadataHandler.h - Ease access to OpenCL-specific meta ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines an helper class that can be used to ease access to
// OpenCL-specific meta-data. Format of meta-data follows:
//
// opencl.kernels : list of MDNode, each item describe a kernel. Support for
//                  this meta-data available in main-stream Clang, patch by
//                  Intel.
// <kernel> : kernel description. First item is the kernel itself, second
//            item contains mappings from kernel arguments to OpenCL address
//            spaces.
//
// <address_spaces> : list of kernel argument/address space mappings. If the
//                    i-th kernel argument is a pointer, the i-th element will
//                    be the OpenCL address space pointed to.
//                    Otherwise, an invalid address space is used. Address space
//                    values comes frome clang::LangAS::opencl_*. Invalid
//                    address space is clang::LangA::Last.
//
//===----------------------------------------------------------------------===//

#ifndef OPENCRUN_UTIL_OPENCLMETADATAHANDLER_H
#define OPENCRUN_UTIL_OPENCLMETADATAHANDLER_H

#include "clang/Basic/AddressSpaces.h"
#include "llvm/Module.h"

namespace opencrun {

//===----------------------------------------------------------------------===//
/// OpenCLMetadataHandler - Define utility methods to access OpenCL-specific
///  meta-data stored inside an LLVM module. This class is designed to be used
///  exploiting the RAII idiom.
//===----------------------------------------------------------------------===//
class OpenCLMetadataHandler {
public:
  // Defines meta-data IDs. Keep this order, in order to match meta-data layout
  // generated by LLVM.
  enum {
    KernelSignatureMID,
    KernelArgAddressSpacesMD
  };

public:
  //===--------------------------------------------------------------------===//
  /// KernelIterator - Iterate over kernels defined in a module.
  //===--------------------------------------------------------------------===//
  class KernelIterator {
  public:
    KernelIterator(llvm::NamedMDNode *Kernels, bool Head) :
      Kernels(Kernels),
      Cur(Kernels && Head ? Kernels->getNumOperands() : 0) { }

  public:
    bool operator==(const KernelIterator &That) const {
      return Kernels == That.Kernels && Cur == That.Cur;
    }

    bool operator!=(const KernelIterator &That) const {
      return !(*this == That);
    }

    llvm::Function &operator*() const {
      llvm::Value *Val = Kernels->getOperand(Cur)->getOperand(0);

      return *llvm::cast<llvm::Function>(Val);
    }

    llvm::Function *operator->() const {
      llvm::Value *Val = Kernels->getOperand(Cur)->getOperand(0);

      return llvm::cast<llvm::Function>(Val);
    }

    // Pre-increment.
    KernelIterator &operator++() {
      Advance(); return *this;
    }

    // Post-increment.
    KernelIterator operator++(int Ign) {
      KernelIterator That = *this; ++*this; return That;
    }

  private:
    void Advance() {
      if(Kernels && Cur < Kernels->getNumOperands())
        ++Cur;
    }

  private:
    llvm::NamedMDNode *Kernels;
    unsigned Cur;
  };

public:
  typedef KernelIterator kernel_iterator;

public:
  kernel_iterator kernel_begin() {
    return KernelIterator(Mod.getNamedMetadata("opencl.kernels"), false);
  }

  kernel_iterator kernel_end() {
    return KernelIterator(Mod.getNamedMetadata("opencl.kernels"), true);
  }

public:
  OpenCLMetadataHandler(const llvm::Module &Mod) : Mod(Mod) { }
  OpenCLMetadataHandler(const OpenCLMetadataHandler &That); // Do not implement.
  void operator=(const OpenCLMetadataHandler &That); // Do not implement.

public:
  llvm::Function *GetKernel(llvm::StringRef KernName) const;
  clang::LangAS::ID GetArgAddressSpace(llvm::Function &Kern, unsigned I);

private:
  const llvm::Module &Mod;
};

} // End namespace opencrun.

#endif // OPENCRUN_UTIL_OPENCLMETADATAHANDLER_H
