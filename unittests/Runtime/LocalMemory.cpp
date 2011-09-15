
#include "RuntimeFixture.h"

template <typename DevTy>
class LocalMemoryTest : public RuntimeFixture<DevTy> { };

#define ALL_DEVICE_TYPES \
  CPUDev

typedef testing::Types<ALL_DEVICE_TYPES> OCLDevicesType;

TYPED_TEST_CASE_P(LocalMemoryTest);

TYPED_TEST_P(LocalMemoryTest, ByAPI) {
  // This syntax force zeroing all elements of HostOut.
  cl_uint HostOut[4] = { 0 };

  cl::Buffer Out = this->AllocReturnBuffer(4 * sizeof(cl_uint));

  const char *Src = "kernel void butterfly(global uint *out,      \n"
                    "                      local uint *tmp) {     \n"
                    "  uint id = get_global_id(0);                \n"
                    "                                             \n"
                    "  tmp[id] = id;                              \n"
                    "  barrier(0);                                \n"
                    "  out[id] = tmp[get_global_size(0) - 1 - id];\n"
                    "}                                            \n";
  cl::Kernel Kern = this->BuildKernel("butterfly", Src);

  Kern.setArg(0, Out);
  Kern.setArg(1, cl::__local(4 * sizeof(cl_uint)));

  cl::CommandQueue Queue = this->GetQueue();

  Queue.enqueueNDRangeKernel(Kern,
                             cl::NullRange,
                             cl::NDRange(4),
                             cl::NDRange(4));
  Queue.enqueueReadBuffer(Out, true, 0, 4 * sizeof(cl_uint), &HostOut);

  for(unsigned I = 0; I < 4; ++I)
    EXPECT_EQ(3 - I, HostOut[I]);
}

REGISTER_TYPED_TEST_CASE_P(LocalMemoryTest, ByAPI);

INSTANTIATE_TYPED_TEST_CASE_P(OCLDev, LocalMemoryTest, OCLDevicesType);
