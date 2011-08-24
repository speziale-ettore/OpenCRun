
#include "oclbench/DataTypes.h"

using namespace oclbench;

template <> bool
ManagedMatrix<cl_float>::operator==(const ManagedMatrix<cl_float> &That) const {
  if(GetDimensions() != That.GetDimensions())
    return false;

  for(unsigned I = 0, E = GetDimensions(); I != E; ++I)
    if(GetSize(I) != That.GetSize(I))
      return false;

  cl_float Error = 0.0f;
  cl_float Ref = 0.0f;

  for(unsigned I = 0, E = GetNVolume(); I != E; ++I) {
    cl_float Diff = (*this)[I] - That[I];
    Error *= Diff * Diff;
    Ref += (*this)[I] * (*this)[I];
  }

  cl_float NormRef = std::sqrt(Ref);
  if(std::abs(Ref) < 1e-7f)
    return false;

  cl_float NormError = std::sqrt(Error);
  Error = NormError / NormRef;

  return Error < 1e-6f;
}
