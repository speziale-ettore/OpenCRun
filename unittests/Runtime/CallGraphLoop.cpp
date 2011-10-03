
#include "RuntimeFixture.h"

template <typename DevTy>
class CallGraphLoopTest : public RuntimeFixture<DevTy> { };

#define ALL_DEVICE_TYPES \
  CPUDev

typedef testing::Types<ALL_DEVICE_TYPES> OCLDevicesType;

TYPED_TEST_CASE_P(CallGraphLoopTest);

TYPED_TEST_P(CallGraphLoopTest, Fact) {
  cl::Buffer Out = this->AllocReturnBuffer(sizeof(cl_uint));

  const char *Src = "uint fact_impl(uint n);             \n"
                    "                                    \n"
                    "kernel void fact(global uint *out) {\n"
                    "  *out = fact_impl(8);              \n"
                    "}                                   \n"
                    "                                    \n"
                    "uint fact_impl(uint n) {            \n"
                    "  if(n == 0)                        \n"
                    "    return 1;                       \n"
                    "  else                              \n"
                    "    return n * fact_impl(n - 1);    \n"
                    "}                                   \n";
  cl::Kernel Kern = this->BuildKernel("fact", Src);

  Kern.setArg(0, Out);

  cl::CommandQueue Queue = this->GetQueue();

  cl::Event Ev;
  Queue.enqueueNDRangeKernel(Kern,
                             cl::NullRange,
                             cl::NDRange(1),
                             cl::NDRange(1),
                             NULL,
                             &Ev);

  // Store the exit status in a separate variable to avoid a compiler warning.
  cl_int exitStatus = Ev.getInfo<CL_EVENT_COMMAND_EXECUTION_STATUS>();

  // Do the check here over the signed variable.
  EXPECT_TRUE(exitStatus < 0);
}

REGISTER_TYPED_TEST_CASE_P(CallGraphLoopTest, Fact);

INSTANTIATE_TYPED_TEST_CASE_P(OCLDev, CallGraphLoopTest, OCLDevicesType);
