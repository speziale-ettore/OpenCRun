
#include "CL/cl.hpp"

#include "gtest/gtest.h"

class ContextTest : public testing::Test {
public:
  void SetUp() {
    // Get all platforms.
    std::vector<cl::Platform> Plats;
    cl::Platform::get(&Plats);

    // Since we link with OpenCRun, the only platform available is OpenCRun.
    Plat = Plats.front();

    // Get all devices.
    Plat.getDevices(CL_DEVICE_TYPE_ALL, &Devs);

    // Fill a standard cl_context_properties.
    Props[0] = CL_CONTEXT_PLATFORM;
    Props[1] = reinterpret_cast<cl_context_properties>(Plat());
    Props[2] = 0;
  }

  void TearDown() {
    // OpenCL resources released automatically by the destructor, via
    // cl::{Platform,Device} destructors.
  }

public:
  cl::Device GetDevice(llvm::StringRef Name = "CPU") {
    cl::Device Dev;

    for(std::vector<cl::Device>::iterator I = Devs.begin(),
                                          E = Devs.end();
                                          I != E && !Dev();
                                          ++I)
      if(Name == I->getInfo<CL_DEVICE_NAME>())
        Dev = cl::Device(*I);

    return Dev;
  }

  cl_context_properties *GetProps() { return Props; }

private:
  cl::Platform Plat;
  std::vector<cl::Device> Devs;
  cl_context_properties Props[3];
};

TEST_F(ContextTest, FromDeviceList) {
  cl::Device Dev = GetDevice();
  cl_context_properties *DefaultProps = GetProps();

  cl::Context Ctx(std::vector<cl::Device>(1, Dev), DefaultProps);

  std::vector<cl::Device> Devs = Ctx.getInfo<CL_CONTEXT_DEVICES>();

  std::vector<cl_context_properties> Props;
  Props = Ctx.getInfo<CL_CONTEXT_PROPERTIES>();

  EXPECT_EQ(1u, Ctx.getInfo<CL_CONTEXT_NUM_DEVICES>());
  EXPECT_EQ(1u, Devs.size());

  // Note: the following check make sense only into OpenCRun.
  EXPECT_EQ(Dev(), Devs[0]());

  EXPECT_EQ(3u, Props.size());

  // Unroll the checking loop by hand, to provide better error reporting.
  EXPECT_EQ(DefaultProps[0], Props[0]);
  EXPECT_EQ(DefaultProps[1], Props[1]);
  EXPECT_EQ(DefaultProps[2], Props[2]);
}
