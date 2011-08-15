
#include "CL/cl.hpp"

#include "gtest/gtest.h"

class KernelTest : public testing::Test {
public:
  void SetUp() {
    // Get all platforms.
    std::vector<cl::Platform> Plats;
    cl::Platform::get(&Plats);

    // Since we link with OpenCRun, the only platform available is OpenCRun.
    Plat = Plats.front();

    // Get all CPU devices.
    std::vector<cl::Device> Devs;
    Plat.getDevices(CL_DEVICE_TYPE_CPU, &Devs);

    // We expect only the CPU device.
    Dev = Devs.front();

    // Create a context for the CPU device.
    cl_context_properties Props[] =
      { CL_CONTEXT_PLATFORM,
        reinterpret_cast<cl_context_properties>(Plat()),
        0
      };
    Ctx = cl::Context(std::vector<cl::Device>(1, Dev), Props);

    // Get a command queue for the CPU device.
    Queue = cl::CommandQueue(Ctx, Dev);
  }

  void TearDown() {
    // OpenCL resources released automatically by the destructor, via
    // cl::{Platform,Device,Context,CommandQueue} destructors.
  }

public:
  cl::Buffer AllocReturnBuffer(size_t Size) {
    return cl::Buffer(Ctx, CL_MEM_WRITE_ONLY, Size);
  }

  cl::Buffer AllocArgBuffer(size_t Size) {
    return cl::Buffer(Ctx, CL_MEM_READ_ONLY, Size);
  }

  cl::Program BuildProgram(llvm::StringRef Src) {
    cl::Program::Sources Srcs;
    Srcs.push_back(std::make_pair(Src.begin(), Src.size()));

    cl::Program Prog(Ctx, Srcs);
    Prog.build(std::vector<cl::Device>());

    return Prog;
  }

private:
  cl::Platform Plat;
  cl::Device Dev;
  cl::Context Ctx;
  cl::CommandQueue Queue;
};

TEST_F(KernelTest, VoidKernel) {
  const char *Src = "kernel void foo() { }";
  cl::Program Prog = BuildProgram(Src);

  cl::Kernel Kern(Prog, "foo");

  EXPECT_EQ("foo", Kern.getInfo<CL_KERNEL_FUNCTION_NAME>());
  EXPECT_EQ(0u, Kern.getInfo<CL_KERNEL_NUM_ARGS>());

  // The following check make sense only in OpenCRun.
  EXPECT_EQ(Prog(), Kern.getInfo<CL_KERNEL_PROGRAM>()());
}

TEST_F(KernelTest, ArgsKernel) {
  const char *Src = "kernel void foo(uint *out, uint *in, uint size) { }";
  cl::Program Prog = BuildProgram(Src);

  cl::Kernel Kern(Prog, "foo");

  EXPECT_EQ("foo", Kern.getInfo<CL_KERNEL_FUNCTION_NAME>());
  EXPECT_EQ(3u, Kern.getInfo<CL_KERNEL_NUM_ARGS>());

  // The following check make sense only in OpenCRun.
  EXPECT_EQ(Prog(), Kern.getInfo<CL_KERNEL_PROGRAM>()());
}
