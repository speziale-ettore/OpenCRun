
//===- DimensionInfo.h - Information about an OpenCL iteration space ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This files define an helper class to store information about the geometry of
// an OpenCL iteration space.
//
//===----------------------------------------------------------------------===//

#ifndef OPENCRUN_UTIL_DIMENSIONINFO_H
#define OPENCRUN_UTIL_DIMENSIONINFO_H

#include "llvm/ADT/SmallVector.h"

#include <algorithm>

namespace opencrun {

//===----------------------------------------------------------------------===//
/// DimensionInfo - Stores information about an OpenCL iteration space. Stored
///  information includes the iteration space origin, the number of work items
///  in each dimension, and the number of work items in each group of each
///  dimension.
//===----------------------------------------------------------------------===//
class DimensionInfo {
public:
  //===--------------------------------------------------------------------===//
  /// InfoWrapper - Store information about a dimension in the OpenCL
  ///  iteration space.
  //===--------------------------------------------------------------------===//
  class InfoWrapper {
  public:
    InfoWrapper(size_t Offset, size_t GlobalSize, size_t LocalSize) :
      Offset(Offset),
      GlobalSize(GlobalSize),
      LocalSize(LocalSize) { }

  public:
    size_t GetOffset() const { return Offset; }
    size_t GetGlobalSize() const { return GlobalSize; }
    size_t GetLocalSize() const { return LocalSize; }

  private:
    size_t Offset;
    size_t GlobalSize;
    size_t LocalSize;
  };

  typedef llvm::SmallVector<InfoWrapper, 4> InfoContainer;

public:
  DimensionInfo(llvm::SmallVector<size_t, 4> &GlobalWorkOffsets,
                llvm::SmallVector<size_t, 4> &GlobalWorkSizes,
                llvm::SmallVector<size_t, 4> &LocalWorkSizes) {
    unsigned E;

    E = std::min(GlobalWorkOffsets.size(), GlobalWorkSizes.size());
    E = std::min(E, LocalWorkSizes.size());

    for(unsigned I = 0; I < E; ++I)
      Info.push_back(InfoWrapper(GlobalWorkOffsets[I],
                                 GlobalWorkSizes[I],
                                 LocalWorkSizes[I]));
  }

public:
  unsigned GetWorkGroupsCount() const {
    return GetGlobalWorkItems() / GetLocalWorkItems();
  }

  unsigned GetGlobalWorkItems() const {
    InfoContainer::const_iterator I = Info.begin(), E = Info.end();

    if(I == E)
      return 0;

    unsigned Size = 1;
    for(; I != E; ++I)
      Size *= I->GetGlobalSize();

    return Size;
  }

  unsigned GetLocalWorkItems() const {
    InfoContainer::const_iterator I = Info.begin(), E = Info.end();

    if(I == E)
      return 0;

    unsigned Size = 1;
    for(; I != E; ++I)
      Size *= I->GetLocalSize();

    return Size;
  }

public:
  bool IsLocalWorkGroupSizeSpecified() const { return GetLocalWorkItems(); }

private:
  InfoContainer Info;
};

} // End namespace opencrun.

#endif // OPENCRUN_UTIL_DIMENSIONINFO_H
