
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
#include "llvm/Support/raw_ostream.h"

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

  public:
    void SetOffset(size_t Delta) { Offset = Delta; }
    void SetGlobalSize(size_t Size) { GlobalSize = Size; }
    void SetLocalSize(size_t Size) { LocalSize = Size; }

  private:
    size_t Offset;
    size_t GlobalSize;
    size_t LocalSize;
  };

  typedef llvm::SmallVector<InfoWrapper, 4> InfoContainer;

public:
  class DimensionInfoIterator {
  public:
    typedef llvm::SmallVector<size_t, 4> IndexesContainer;

    typedef std::pair<IndexesContainer, IndexesContainer> IndexesPair;

  public:
    DimensionInfoIterator(DimensionInfo &DimInfo,
                          IndexesContainer &Locals,
                          IndexesContainer &WorkGroups) :
      DimInfo(DimInfo),
      Indexes(Locals, WorkGroups) { }

  public:
    bool operator==(const DimensionInfoIterator &That) const {
      return Indexes == That.Indexes;
    }

    bool operator!=(const DimensionInfoIterator &That) const {
      return !(*this == That);
    }

    // Pre-increment.
    DimensionInfoIterator &operator++() {
      Advance(1); return *this;
    }

    // Post-increment.
    DimensionInfoIterator operator++(int Ign) {
      DimensionInfoIterator That = *this; ++*this; return That;
    }

    IndexesPair &operator*() { return Indexes; }
    IndexesPair *operator->() { return &Indexes; }

  private:
    void Advance(unsigned N);
    bool AdvanceToNextWorkGroupOrigin();

  private:
    DimensionInfo &DimInfo;
    IndexesPair Indexes;
  };

  typedef DimensionInfoIterator iterator;

public:
  iterator begin() {
    DimensionInfoIterator::IndexesContainer Locals(Info.size());
    DimensionInfoIterator::IndexesContainer WorkGroups(Info.size());

    return DimensionInfoIterator(*this, Locals, WorkGroups);
  }

  iterator end() {
    DimensionInfoIterator::IndexesContainer Locals(Info.size());
    DimensionInfoIterator::IndexesContainer WorkGroups(Info.size());
    WorkGroups[0] = GetWorkGroupsCount(0);
  
    return DimensionInfoIterator(*this, Locals, WorkGroups);
  }

public:
  DimensionInfo(llvm::SmallVector<size_t, 4> &GlobalWorkOffsets,
                llvm::SmallVector<size_t, 4> &GlobalWorkSizes,
                llvm::SmallVector<size_t, 4> &LocalWorkSizes) {
    size_t E;

    E = std::min(GlobalWorkOffsets.size(), GlobalWorkSizes.size());
    E = std::min(E, LocalWorkSizes.size());

    for(size_t I = 0; I < E; ++I)
      Info.push_back(InfoWrapper(GlobalWorkOffsets[I],
                                 GlobalWorkSizes[I],
                                 LocalWorkSizes[I]));
  }

public:
  unsigned GetDimensions() const { return Info.size(); }

  size_t GetWorkGroupSize(unsigned I) const {
    if(I >= Info.size())
      return 0;

    return Info[I].GetLocalSize();
  }

  unsigned GetWorkGroupsCount() const {
    return GetGlobalWorkItems() / GetLocalWorkItems();
  }

  unsigned GetWorkGroupsCount(unsigned I) const {
    if(I >= Info.size())
      return 0;

    size_t GlobalSize = Info[I].GetGlobalSize();
    size_t LocalSize = Info[I].GetLocalSize();

    // Do not rely on floating point arithmentic, paranoia-mode.
    unsigned Count = GlobalSize / LocalSize;
    if(GlobalSize % LocalSize)
      ++Count;

    return Count;
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

  unsigned GetGlobalWorkItems(unsigned I) const {
    if(I >= Info.size())
      return 0;

    return Info[I].GetGlobalSize();
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

  bool SetWorkGroupsSize(llvm::SmallVector<size_t, 4> Sizes) {
    if(Sizes.size() != Info.size())
      return false;

    // Checking bounds.
    for(unsigned I = 0; I < Info.size(); ++I)
      if(Info[I].GetGlobalSize() % Sizes[I])
        return false;

    // Setting the sizes.
    for(unsigned I = 0; I < Info.size(); ++I)
      Info[I].SetLocalSize(Sizes[I]);

    return true;
  }

public:
  bool IsLocalWorkGroupSizeSpecified() const { return GetLocalWorkItems(); }

private:
  InfoContainer Info;
};

} // End namespace opencrun.

namespace llvm {

using namespace opencrun;

llvm::raw_ostream&
operator<<(llvm::raw_ostream &OS,
           DimensionInfo::DimensionInfoIterator::IndexesPair &Indexes);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, DimensionInfo &DimInfo);

} // End namespace llvm.

#endif // OPENCRUN_UTIL_DIMENSIONINFO_H
