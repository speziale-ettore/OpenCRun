
#include "opencrun/Util/DimensionInfo.h"

using namespace opencrun;

//
// DimensionInfo::DimensionInfoIterator implementation.
//

size_t DimensionInfo::DimensionInfoIterator::GetWorkGroup() const {
  const IndexesContainer &WorkGroups = Indexes.second;

  size_t WorkGroup = WorkGroups.back();

  for(unsigned I = 0, E = WorkGroups.size() - 1; I != E; ++I)
    WorkGroup += DimInfo.GetLocalWorkItems(I) * WorkGroups[I];

  return WorkGroup;
}

void DimensionInfo::DimensionInfoIterator::Advance(unsigned N) {
  IndexesContainer &Locals = Indexes.first;

  unsigned I = 0;
  size_t Carry = N;

  while(Carry) {
    // We have to switch to the next work-group.
    if(I == Locals.size()) {
      // No next work-group, overflow.
      if(!AdvanceToNextWorkGroupOrigin())
        break;

      // Next work-group exists, rebase from its origin.
      else
        I = 0;
    }

    size_t WorkGroupSize = DimInfo.GetLocalWorkItems(I);

    size_t NewId = Locals[I] + N;
    Carry = NewId / WorkGroupSize - Locals[I] / WorkGroupSize;

    // Move along current dimension.
    Locals[I++] = NewId % WorkGroupSize;
  }
}

bool DimensionInfo::DimensionInfoIterator::AdvanceToNextWorkGroupOrigin() {
  IndexesContainer &Locals = Indexes.first;
  IndexesContainer &WorkGroups = Indexes.second;

  unsigned I = 0;
  bool Carry = true;

  // Reset local origin.
  Locals.assign(Locals.size(), 0);

  while(Carry) {
    // No more dimensions, overflow.
    if(I == WorkGroups.size()) {
      WorkGroups.assign(WorkGroups.size(), 0);
      WorkGroups[0] = DimInfo.GetWorkGroupsCount(0);
      break;
    }

    unsigned WorkGroupsCount = DimInfo.GetWorkGroupsCount(I);

    size_t NewId = WorkGroups[I] + 1;
    Carry = NewId / WorkGroupsCount - WorkGroups[I] / WorkGroupsCount;

    // Move along current dimension.
    WorkGroups[I++] = NewId % WorkGroupsCount;
  }

  return !Carry;
}

//
// DimensionInfo implementation.
//

namespace llvm {

llvm::raw_ostream &
operator<<(llvm::raw_ostream &OS,
            DimensionInfo::DimensionInfoIterator::IndexesPair &Indexes) {
  DimensionInfo::DimensionInfoIterator::IndexesContainer &Locals =
    Indexes.first;
  DimensionInfo::DimensionInfoIterator::IndexesContainer &WorkGroups =
    Indexes.second;

  OS << "[";

  DimensionInfo::DimensionInfoIterator::IndexesContainer::iterator I, E;

  I = Locals.begin();
  E = Locals.end();

  if(I != E)
    OS << *I;

  for(++I; I != E; ++I)
    OS << ", " << *I;

  I = WorkGroups.begin();
  E = WorkGroups.end();

  if(I != E)
    OS << "|" << *I;

  for(++I; I != E; ++I)
    OS << ", " << *I;

  OS << "]";

  return OS;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, DimensionInfo &DimInfo) {
  OS << "(";

  DimensionInfo::iterator I = DimInfo.begin(), E = DimInfo.end();

  if(I != E)
    OS << *I;

  for(++I; I != E; ++I)
    OS << ", " << *I;

  OS << ")";

  return OS;
}

} // End namespace llvm.
