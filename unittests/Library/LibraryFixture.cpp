
#include "LibraryFixture.h"

//
// DeviceTraits specializations.
//

#define SPECIALIZE_DEVICE_TRAITS(D, N) \
  llvm::StringRef DeviceTraits<D>::Name = N;

SPECIALIZE_DEVICE_TRAITS(CPUDev, "CPU")

#undef SPECIALIZE_DEVICE_TRAITS

//
// OCLTypeTraits specializations.
//

#define SPECIALIZE_OCL_TYPE_TRAITS(T, N)          \
  template <>                                     \
  llvm::StringRef OCLTypeTraits<T>::OCLCName = N; \
                                                  \
  template <>                                     \
  void OCLTypeTraits<T>::AssertEq(T A, T B) {     \
    ASSERT_EQ(A, B);                              \
  }

#define SPECIALIZE_OCL_TYPE_TRAITS_SIZED_VEC(T, N, S)        \
  template <>                                                \
  llvm::StringRef OCLTypeTraits<T ## S>::OCLCName = N #S;    \
                                                             \
  template <>                                                \
  void OCLTypeTraits<T ## S>::AssertEq(T ## S A, T ## S B) { \
    T RawA[S], RawB[S];                                      \
                                                             \
    std::memcpy(RawA, &A, sizeof(T ## S));                   \
    std::memcpy(RawB, &B, sizeof(T ## S));                   \
                                                             \
    for(unsigned I = 0; I < S; ++I)                          \
      OCLTypeTraits<T>::AssertEq(RawA[I], RawB[I]);          \
  }

#define SPECIALIZE_OCL_TYPE_TRAITS_VEC(T, N)     \
  SPECIALIZE_OCL_TYPE_TRAITS_SIZED_VEC(T, N,  2) \
  SPECIALIZE_OCL_TYPE_TRAITS_SIZED_VEC(T, N,  4) \
  SPECIALIZE_OCL_TYPE_TRAITS_SIZED_VEC(T, N,  8) \
  SPECIALIZE_OCL_TYPE_TRAITS_SIZED_VEC(T, N, 16)

#define SPECIALIZE_OCL_TYPE_TRAITS_CREATE(T, V) \
  template <> template <>                       \
  T OCLTypeTraits<T>::Create(V Val) {           \
    return static_cast<T>(Val);                 \
  }

#define SPECIALIZE_OCL_TYPE_TRAITS_SIZED_VEC_CREATE(T, S, V) \
  template <> template <>                                    \
  T ## S OCLTypeTraits<T ## S>::Create(V Val) {              \
    T Raw[S];                                                \
                                                             \
    for(unsigned I = 0; I < S; ++I)                          \
      Raw[I] = OCLTypeTraits<T>::Create<V>(Val);             \
                                                             \
    T ## S Vect;                                             \
    std::memcpy(&Vect, Raw, sizeof(T ## S));                 \
                                                             \
    return Vect;                                             \
  }

#define SPECIALIZE_OCL_TYPE_TRAITS_VEC_CREATE(T, V)     \
  SPECIALIZE_OCL_TYPE_TRAITS_SIZED_VEC_CREATE(T,  2, V) \
  SPECIALIZE_OCL_TYPE_TRAITS_SIZED_VEC_CREATE(T,  4, V) \
  SPECIALIZE_OCL_TYPE_TRAITS_SIZED_VEC_CREATE(T,  8, V) \
  SPECIALIZE_OCL_TYPE_TRAITS_SIZED_VEC_CREATE(T, 16, V)

// Fixed width integer specializations.

SPECIALIZE_OCL_TYPE_TRAITS(uint32_t, "unsigned int")
SPECIALIZE_OCL_TYPE_TRAITS(uint64_t, "unsigned long")

// Single precision floating point specializations.

SPECIALIZE_OCL_TYPE_TRAITS(float, "float")
SPECIALIZE_OCL_TYPE_TRAITS_VEC(cl_float, "float")
SPECIALIZE_OCL_TYPE_TRAITS_CREATE(float, int32_t)
SPECIALIZE_OCL_TYPE_TRAITS_CREATE(float, double)
SPECIALIZE_OCL_TYPE_TRAITS_VEC_CREATE(cl_float, int32_t)
SPECIALIZE_OCL_TYPE_TRAITS_VEC_CREATE(cl_float, double)

#undef SPECIALIZE_OCL_TYPE_TRAITS
#undef SPECIALIZE_OCL_TYPE_TRAITS_SIZED_VEC
#undef SPECIALIZE_OCL_TYPE_TRAITS_VEC
#undef SPECIALIZE_OCL_TYPE_TRAITS_CREATE
#undef SPECIALIZE_OCL_TYPE_TRAITS_SIZED_VEC_CREATE
#undef SPECIALIZE_OCL_TYPE_TRAITS_VEC_CREATE

//
// Specialization of googletest templates.
//

namespace testing {
namespace internal {

#define SPECIALIZE_TYPE_NAME(T) \
  template <>                   \
  String GetTypeName<T>() {     \
    return #T;                  \
  }

#define SPECIALIZE_COMPOUND_TYPE(D, T)                                  \
  template <>                                                           \
  String GetTypeName< DeviceTypePair<D, T> >() {                        \
    String DevName = GetTypeName<D>();                                  \
    String TyName = GetTypeName<T>();                                   \
                                                                        \
    return String::Format("(%s, %s)", DevName.c_str(), TyName.c_str()); \
  }

#define SPECIALIZE_TYPE(T)            \
  SPECIALIZE_TYPE_NAME(T)             \
  SPECIALIZE_COMPOUND_TYPE(CPUDev, T)

// Do not specialize vector the vector with 3 elements. OpenCL uses the same
// storage class as for 4 element vector.

#define SPECIALIZE_VEC_TYPE(T) \
  SPECIALIZE_TYPE(T ##  2)     \
  SPECIALIZE_TYPE(T ##  4)     \
  SPECIALIZE_TYPE(T ##  8)     \
  SPECIALIZE_TYPE(T ## 16)

// Device type specializations.

SPECIALIZE_TYPE_NAME(CPUDev)

// Fixed width integer specializations.

SPECIALIZE_TYPE(uint32_t)
SPECIALIZE_TYPE(uint64_t)

// Single precision floating point specializations.

SPECIALIZE_TYPE(float)
SPECIALIZE_VEC_TYPE(cl_float)

#undef SPECIALIZE_TYPE_NAME
#undef SPECIALIZE_COMPOUND_TYPE
#undef SPECIALIZE_TYPE
#undef SPECIALIZE_VECT_TYPE

} // End namespace internal.
} // End namespace testing.
