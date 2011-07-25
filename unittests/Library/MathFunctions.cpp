
#include "LibraryFixture.h"

template <typename TyTy>
class MathFunctionsTest : public LibraryGenTypeFixture<typename TyTy::Dev,
                                                       typename TyTy::Type> { };

#define LAST_ALL_TYPES(D)       \
  DeviceTypePair<D, cl_float>,  \
  DeviceTypePair<D, cl_float2>, \
  DeviceTypePair<D, cl_float3>, \
  DeviceTypePair<D, cl_float4>, \
  DeviceTypePair<D, cl_float8>, \
  DeviceTypePair<D, cl_float16>

#define ALL_DEVICE_TYPES \
  LAST_ALL_TYPES(CPUDev)

typedef testing::Types<ALL_DEVICE_TYPES> OCLDevicesTypes;

TYPED_TEST_CASE_P(MathFunctionsTest);

TYPED_TEST_P(MathFunctionsTest, acos) {
  GENTYPE_DECLARE(Input);
  GENTYPE_DECLARE(Expected);
  GENTYPE_DECLARE(Output);

  Input = GENTYPE_CREATE(-1);
  Expected = GENTYPE_CREATE(M_PI);
  this->Invoke("acos", Output, Input);
  ASSERT_GENTYPE_EQ(Expected, Output);

  Input = GENTYPE_CREATE(0);
  Expected = GENTYPE_CREATE(M_PI_2);
  this->Invoke("acos", Output, Input);
  ASSERT_GENTYPE_EQ(Expected, Output);

  Input = GENTYPE_CREATE(1);
  Expected = GENTYPE_CREATE(0);
  this->Invoke("acos", Output, Input);
  ASSERT_GENTYPE_EQ(Expected, Output);
}

TYPED_TEST_P(MathFunctionsTest, acosh) {
  GENTYPE_DECLARE(Input);
  GENTYPE_DECLARE(Expected);
  GENTYPE_DECLARE(Output);

  Input = GENTYPE_CREATE(0);
  this->Invoke("acosh", Output, Input);
  ASSERT_GENTYPE_IS_NAN(Output);

  Input = GENTYPE_CREATE(1);
  Expected = GENTYPE_CREATE(0);
  this->Invoke("acosh", Output, Input);
  ASSERT_GENTYPE_EQ(Expected, Output);
}

TYPED_TEST_P(MathFunctionsTest, acospi) {
  GENTYPE_DECLARE(Input);
  GENTYPE_DECLARE(Expected);
  GENTYPE_DECLARE(Output);

  Input = GENTYPE_CREATE(-1);
  Expected = GENTYPE_CREATE(1);
  this->Invoke("acospi", Output, Input);
  ASSERT_GENTYPE_EQ(Expected, Output);

  Input = GENTYPE_CREATE(0);
  Expected = GENTYPE_CREATE(0.5);
  this->Invoke("acospi", Output, Input);
  ASSERT_GENTYPE_EQ(Expected, Output);

  Input = GENTYPE_CREATE(1);
  Expected = GENTYPE_CREATE(0);
  this->Invoke("acospi", Output, Input);
  ASSERT_GENTYPE_EQ(Expected, Output);
}

TYPED_TEST_P(MathFunctionsTest, cos) {
  GENTYPE_DECLARE(Input);
  GENTYPE_DECLARE(Expected);
  GENTYPE_DECLARE(Output);

  Input = GENTYPE_CREATE(0);
  Expected = GENTYPE_CREATE(1);
  this->Invoke("cos", Output, Input);
  ASSERT_GENTYPE_EQ(Expected, Output);

  // Do no perform the check near 1/2 * pi, the FPU loss precision here!

  Input = GENTYPE_CREATE(M_PI);
  Expected = GENTYPE_CREATE(-1);
  this->Invoke("cos", Output, Input);
  ASSERT_GENTYPE_EQ(Expected, Output);

  // Do no perform the check near 3/4 * pi, the FPU loss precision here!

  Input = GENTYPE_CREATE(2 * M_PI);
  Expected = GENTYPE_CREATE(1);
  this->Invoke("cos", Output, Input);
  ASSERT_GENTYPE_EQ(Expected, Output);
}

REGISTER_TYPED_TEST_CASE_P(MathFunctionsTest, acos,
                                              acosh,
                                              acospi,
// TODO: fill the gap.
                                              cos);

INSTANTIATE_TYPED_TEST_CASE_P(OCLDev, MathFunctionsTest, OCLDevicesTypes);
