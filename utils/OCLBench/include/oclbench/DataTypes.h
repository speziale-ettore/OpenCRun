
#ifndef OCLBENCH_DATATYPES_H
#define OCLBENCH_DATATYPES_H

#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/raw_ostream.h"

#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>

#include <stdexcept>

namespace oclbench {

template <typename Ty>
class ManagedMatrix {
protected:
  ManagedMatrix(size_t Sz) : Mem(new Ty[Sz]) {
    Sizes.push_back(Sz);
  }

  ManagedMatrix(size_t Sz0, size_t Sz1) : Mem(new Ty[Sz0 * Sz1]) {
    Sizes.push_back(Sz0);
    Sizes.push_back(Sz1);
  }

  ManagedMatrix(const ManagedMatrix<Ty> &That) : Sizes(That.Sizes) {
    size_t NVolume = GetNVolume();

    Mem.reset(new Ty[NVolume]);
    std::memcpy(Mem.get(), That.GetMem(), NVolume * sizeof(Ty));
  }

  const ManagedMatrix &operator=(const ManagedMatrix<Ty> &That) {
    if(this != &That) {
      Sizes = That.Sizes;

      size_t NVolume = GetNVolume();

      Mem.reset(new Ty[NVolume]);
      std::memcpy(Mem.get(), That.GetMem(), NVolume * sizeof(Ty));
    }

    return *this;
  }

public:
  bool operator==(const ManagedMatrix<Ty> &That) const {
    if(GetDimensions() != That.GetDimensions())
      return false;

    for(unsigned I = 0, E = GetDimensions(); I != E; ++I)
      if(GetSize(I) != That.GetSize(I))
        return false;

    return std::memcmp(Mem.get(), That.GetMem(), GetMemSize()) == 0;
  }

  Ty operator[](unsigned I) const {
    return GetMem()[I];
  }

  Ty &operator[](unsigned I) {
    return GetMem()[I];
  }

public:
  void Randomize() {
    for(unsigned I = 0, E = GetNVolume(); I != E; ++I)
      Mem[I] = random();
  }

  void Dump() { Dump(llvm::errs()); }

  void Dump(llvm::raw_ostream &OS) {
    unsigned NewLineAt;

    if(GetDimensions() == 2)
      NewLineAt = Sizes[1];
    else
      NewLineAt = GetNVolume();

    for(unsigned I = 0, E = GetNVolume(); I != E; ++I) {
      if(I != 0) {
        if(I % NewLineAt == 0)
          OS << "\n";
        else
          OS << " ";
      }
      OS << Mem[I];
    }
    OS << "\n";
  }

public:
  Ty *GetMem() const { return Mem.get(); }

  unsigned GetDimensions() const { return Sizes.size(); }

  size_t GetSize(unsigned I) const {
    if(I >= Sizes.size())
      throw std::out_of_range("ManagedMatrix " + llvm::utostr(I));

    return Sizes[I];
  }

  size_t GetMemSize() const {
    return GetNVolume() * sizeof(Ty);
  }

  size_t GetNVolume() const {
    size_t NVolume = 1;

    for(unsigned I = 0, E = Sizes.size(); I != E; ++I)
      NVolume *= Sizes[I];

    return NVolume;
  }

private:
  llvm::SmallVector<size_t, 4> Sizes;
  llvm::OwningArrayPtr<Ty> Mem;
};

// Default implementation is a bitwise comparison. Provide specialization for
// single precision floating point type. Code borrowed from AMD OpenCL SDK.
template <> bool
ManagedMatrix<cl_float>::operator==(const ManagedMatrix<cl_float> &That) const;

template <typename Ty>
class BiManagedMatrix : public ManagedMatrix<Ty> {
public:
  BiManagedMatrix(size_t Sz0, size_t Sz1) : ManagedMatrix<Ty>(Sz0, Sz1) { }

public:
  Ty operator()(unsigned I, unsigned J) const {
    return this->GetMem()[I * this->GetSize(I) + J];
  }

  Ty &operator()(unsigned I, unsigned J) {
    return this->GetMem()[I * this->GetSize(0) + J];
  }
};

} // End namespace oclbench.

#endif // OCLBENCH_DATATYPES_H
