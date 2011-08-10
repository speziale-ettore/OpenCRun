
#include "RuntimeFixture.h"

//
// DeviceTraits specializations.
//

#define SPECIALIZE_DEVICE_TRAITS(D, N)       \
  llvm::StringRef DeviceTraits<D>::Name = N;

SPECIALIZE_DEVICE_TRAITS(CPUDev, "CPU")

#undef SPECIALIZE_DEVICE_TRAITS

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

SPECIALIZE_TYPE_NAME(CPUDev)

#undef SPECIALIZE_TYPE_NAME

} // End namespace internal.
} // End namespace testing.
