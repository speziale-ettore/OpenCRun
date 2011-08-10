
//===- RuntimeFixture.h - Fixtures to help developing runtime tests -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines test fixtures that automatically setup and OpenCL context
// where tests the runtime. With respect to LibraryFixture, more low-level
// methods are exposed. This allows to customize the interaction with the
// runtime. Obviously, less automated tests are supported.
//
//===----------------------------------------------------------------------===//

#ifndef UNITTESTS_RUNTIME_RUNTIMEFIXTURE_H
#define UNITTESTS_RUNTIME_RUNTIMEFIXTURE_H

#include "CL/cl.hpp"

#include "gtest/gtest.h"

//===----------------------------------------------------------------------===//
/// DeviceTraits - Traits of OpenCL devices. We do not need much power here.
///  Knowing the name of the device is enough.
//===----------------------------------------------------------------------===//
template <typename Ty>
class DeviceTraits {
public:
  static llvm::StringRef Name;
};

//===----------------------------------------------------------------------===//
/// RuntimeFailed - OpenCL callback to report runtime errors.
//===----------------------------------------------------------------------===//
template <typename DevTy>
void RuntimeFailed(const char *Err,
                   const void *PrivInfo,
                   size_t PrivInfoSize,
                   void *UserData);

//===----------------------------------------------------------------------===//
/// RuntimeFixture - Runtime testing helper. This class setup an OpenCL context
///  for a device. It exposes methods to performs operations on that context,
///  such as getting command queues, allocating buffers, ... .
//===----------------------------------------------------------------------===//
template <typename DevTy>
class RuntimeFixture : public testing::Test {
public:
  void SetUp() {
    // Get all platforms.
    std::vector<cl::Platform> Plats;
    cl::Platform::get(&Plats);

    // Since we link with OpenCRun, the only platform available is OpenCRun.
    Plat = Plats.front();

    // Get all devices.
    std::vector<cl::Device> Devs;
    Plat.getDevices(CL_DEVICE_TYPE_ALL, &Devs);

    // Look for a specific device.
    for(std::vector<cl::Device>::iterator I = Devs.begin(),
                                          E = Devs.end();
                                          I != E && !Dev();
                                          ++I)
      if(DeviceTraits<DevTy>::Name == I->getInfo<CL_DEVICE_NAME>())
        Dev = cl::Device(*I);

    ASSERT_TRUE(Dev());

    // Build a context where working.
    cl_context_properties Props[] =
      { CL_CONTEXT_PLATFORM,
        reinterpret_cast<cl_context_properties>(Plat()),
        0
      };
    Ctx = cl::Context(std::vector<cl::Device>(1, Dev),
                      Props,
                      ::RuntimeFailed<DevTy>,
                      this);
  }

  void TearDown() {
    // OpenCL resources released automatically by the destructor, via
    // cl:{Platform,Device,Context} destructors.
  }

public:
  cl::Buffer AllocReturnBuffer(size_t Size) {
    return cl::Buffer(Ctx, CL_MEM_WRITE_ONLY, Size);
  }

  cl::Buffer AllocArgBuffer(size_t Size) {
    return cl::Buffer(Ctx, CL_MEM_READ_ONLY, Size);
  }

  cl::Kernel BuildKernel(llvm::StringRef Name, llvm::StringRef Src) {
    std::string NameStr = Name.str();

    cl::Program::Sources Srcs;
    Srcs.push_back(std::make_pair(Src.begin(), Src.size()));

    cl::Program Prog(Ctx, Srcs);
    Prog.build(std::vector<cl::Device>());

    return cl::Kernel(Prog, NameStr.c_str());
  }

  cl::CommandQueue GetQueue() { return cl::CommandQueue(Ctx, Dev); }

private:
  void RuntimeFailed(llvm::StringRef Err) {
    FAIL() << Err.str() << "\n";
  }

private:
  cl::Platform Plat;
  cl::Device Dev;
  cl::Context Ctx;

  friend void ::RuntimeFailed<DevTy>(const char *,
                                     const void *,
                                     size_t,
                                     void *);
};

class CPUDev { };

template <>
class DeviceTraits<CPUDev> {
public:
  static llvm::StringRef Name;
};

template <typename DevTy>
void RuntimeFailed(const char *Err,
                   const void *PrivInfo,
                   size_t PrivInfoSize,
                   void *UserData) {
  RuntimeFixture<DevTy> *Fix = static_cast<RuntimeFixture<DevTy> *>(UserData);

  Fix->RuntimeFailed(Err);
}

#endif // UNITTESTS_RUNTIME_RUNTIMEFIXTURE_H
