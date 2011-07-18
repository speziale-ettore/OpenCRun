
#include "LibraryFixture.h"

template <typename DevTy>
class WorkItemFunctionsTest : public LibraryFixture<DevTy> { };

#define ALL_DEVICE_TYPES \
  CPUDev

typedef testing::Types<ALL_DEVICE_TYPES> OCLDevicesType;

TYPED_TEST_CASE_P(WorkItemFunctionsTest);

TYPED_TEST_P(WorkItemFunctionsTest, get_work_dim) {
  cl_uint WorkDim;

  this->Invoke("get_work_dim", WorkDim);
  EXPECT_EQ(1u, WorkDim);

  this->Invoke("get_work_dim", WorkDim, cl::NDRange(1, 1, 1));
  EXPECT_EQ(3u, WorkDim);
}

TYPED_TEST_P(WorkItemFunctionsTest, get_global_size) {
  typename DeviceTraits<TypeParam>::SizeType GlobalSize;

  this->Invoke("get_global_size", GlobalSize, (cl_uint) 0);
  EXPECT_EQ(1u, GlobalSize);

  cl::NDRange Cube(1, 1, 1);
  for(cl_uint I = 0; I < 3; ++I) {
    this->Invoke("get_global_size", GlobalSize, I, Cube);
    EXPECT_EQ(1u, GlobalSize);
  }
}

TYPED_TEST_P(WorkItemFunctionsTest, get_global_id) {
  typename DeviceTraits<TypeParam>::SizeType GlobalID;

  this->Invoke("get_global_id", GlobalID, (cl_uint) 0);
  EXPECT_EQ(0u, GlobalID);

  cl::NDRange Cube(1, 1, 1);
  for(cl_uint I = 0; I < 3; ++I) {
    this->Invoke("get_global_id", GlobalID, I, Cube);
    EXPECT_EQ(0u, GlobalID);
  }
}

TYPED_TEST_P(WorkItemFunctionsTest, get_local_size) {
  typename DeviceTraits<TypeParam>::SizeType LocalSize;

  this->Invoke("get_local_size", LocalSize, (cl_uint) 0);
  EXPECT_EQ(1u, LocalSize);

  cl::NDRange Cube(1, 1, 1);
  for(cl_uint I = 0; I < 3; ++I) {
    this->Invoke("get_local_size", LocalSize, I, Cube);
    EXPECT_EQ(1u, LocalSize);
  }
}

TYPED_TEST_P(WorkItemFunctionsTest, get_local_id) {
  typename DeviceTraits<TypeParam>::SizeType LocalID;

  this->Invoke("get_local_id", LocalID, (cl_uint) 0);
  EXPECT_EQ(0u, LocalID);

  cl::NDRange Cube(1, 1, 1);
  for(cl_uint I = 0; I < 3; ++I) {
    this->Invoke("get_local_id", LocalID, I, Cube);
    EXPECT_EQ(0u, LocalID);
  }
}

TYPED_TEST_P(WorkItemFunctionsTest, get_num_groups) {
  typename DeviceTraits<TypeParam>::SizeType NumGroups;

  this->Invoke("get_num_groups", NumGroups, (cl_uint) 0);
  EXPECT_EQ(1u, NumGroups);

  cl::NDRange Cube(1, 1, 1);
  for(cl_uint I = 0; I < 3; ++I) {
    this->Invoke("get_num_groups", NumGroups, I, Cube);
    EXPECT_EQ(1u, NumGroups);
  }
}

TYPED_TEST_P(WorkItemFunctionsTest, get_group_id) {
  typename DeviceTraits<TypeParam>::SizeType GroupID;

  this->Invoke("get_group_id", GroupID, (cl_uint) 0);
  EXPECT_EQ(0u, GroupID);

  cl::NDRange Cube(1, 1, 1);
  for(cl_uint I = 0; I < 3; ++I) {
    this->Invoke("get_group_id", GroupID, I, Cube);
    EXPECT_EQ(0u, GroupID);
  }
}

TYPED_TEST_P(WorkItemFunctionsTest, get_global_offset) {
  typename DeviceTraits<TypeParam>::SizeType GlobalOffset;

  this->Invoke("get_global_offset", GlobalOffset, (cl_uint) 0);
  EXPECT_EQ(0u, GlobalOffset);

  cl::NDRange Cube(1, 1, 1);
  for(cl_uint I = 0; I < 3; ++I) {
    this->Invoke("get_global_offset", GlobalOffset, I, Cube);
    EXPECT_EQ(0u, GlobalOffset);
  }
}

REGISTER_TYPED_TEST_CASE_P(WorkItemFunctionsTest, get_work_dim,
                                                  get_global_size,
                                                  get_global_id,
                                                  get_local_size,
                                                  get_local_id,
                                                  get_num_groups,
                                                  get_group_id,
                                                  get_global_offset);

INSTANTIATE_TYPED_TEST_CASE_P(OCLDev, WorkItemFunctionsTest, OCLDevicesType);
