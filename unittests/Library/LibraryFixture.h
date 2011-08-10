
//===- LibraryFixture.h - Fixtures to automate library basic testing ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines test fixtures that automatically setup and OpenCL context
// where perform basic unit testing of library functions.
//
//===----------------------------------------------------------------------===//

#ifndef UNITTESTS_LIBRARY_LIBRARYFIXTURE_H
#define UNITTESTS_LIBRARY_LIBRARYFIXTURE_H

#include "CL/cl.hpp"

#include "gtest/gtest.h"

//===----------------------------------------------------------------------===//
/// DeviceTraits - Traits of OpenCL devices. The LibraryFixture requires a
///  template type parameter describing the OpenCL device where the test will be
///  run. This class provides needed information. Please provide a SizeType
///  type definition in template instantiation. This because the device size_t
///  type has not a corresponding type in the host, but its size must be known
///  in order to allocate storage on the host.
//===----------------------------------------------------------------------===//
template <typename Ty>
class DeviceTraits {
public:
  static llvm::StringRef Name;
};

//===----------------------------------------------------------------------===//
/// OCLTypeTraits - Traits of OpenCL types. Usually tests operate on
///  user-provided data. Storage must be allocated on both the device and the
///  host. This class provides information on how to translate each host type to
///  the corresponding device type. Pay attention in template instantiation in
///  order to avoid template redefinitions.
//===----------------------------------------------------------------------===//
template <typename Ty>
class OCLTypeTraits {
public:
  template <typename ValTy>
  static Ty Create(ValTy Val);

  static void AssertEq(Ty A, Ty B);
  static void AssertIsNaN(Ty A);

public:
  static llvm::StringRef OCLCName;
};

//===----------------------------------------------------------------------===//
/// DeviceTypePair - A couple device type, tested type. This class allows to
///  pack two types into one type, providing specialized type names to access to
///  the device type and to the tested type type. It is needed to exploit
///  googletest typed test framework.
//===----------------------------------------------------------------------===//
template <typename DevTy, typename Ty>
class DeviceTypePair {
public:
  typedef DevTy Dev;
  typedef Ty Type;
};

//===----------------------------------------------------------------------===//
/// GENTTYPE_DECLARE - Declare a variable for generic type-based tests.
//===----------------------------------------------------------------------===//
#define GENTYPE_DECLARE(V) \
  typename TypeParam::Type V

//===----------------------------------------------------------------------===//
/// GENTYPE_CREATE - Value creator for generic type-based tests.
//===----------------------------------------------------------------------===//
#define GENTYPE_CREATE(V) \
  (OCLTypeTraits<typename TypeParam::Type>::Create(V))

//===----------------------------------------------------------------------===//
/// ASSERT_GENTYPE_EQ - Equality check for generic type-based tests.
//===----------------------------------------------------------------------===//
#define ASSERT_GENTYPE_EQ(A, B) \
  (OCLTypeTraits<typename TypeParam::Type>::AssertEq(A, B))

#define ASSERT_GENTYPE_IS_NAN(A) \
  (OCLTypeTraits<typename TypeParam::Type>::AssertIsNaN(A))

//===----------------------------------------------------------------------===//
/// RuntimeFailed - OpenCL callback to report runtime errors.
//===----------------------------------------------------------------------===//
template <typename DevTy>
void RuntimeFailed(const char *Err,
                   const void *PrivInfo,
                   size_t PrivInfoSize,
                   void *UserData);

//===----------------------------------------------------------------------===//
/// LibraryFixture - Library testing automation fixture. This fixture
///  automatically setup an OpenCL context to run library unit testing for the
///  device specified by the template parameter. In order to execute the test,
///  simply call the Invoke method from inside a TYPED_TEST_P().
//===----------------------------------------------------------------------===//
template <typename DevTy>
class LibraryFixture : public testing::Test {
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

    // Get a queue for the device.
    cl_context_properties Props[] =
      { CL_CONTEXT_PLATFORM,
        reinterpret_cast<cl_context_properties>(Plat()),
        0
      };
    Ctx = cl::Context(std::vector<cl::Device>(1, Dev),
                      Props,
                      ::RuntimeFailed<DevTy>,
                      this);
    Queue = cl::CommandQueue(Ctx, Dev);
  }

  void TearDown() {
    // OpenCL resources released automatically by the destructor, via
    // cl:{Platform,Device,Context,CommandQueue} destructors.
  }

  template <typename RetTy>
  void Invoke(llvm::StringRef Fun,
              RetTy &R,
              cl::NDRange Space = cl::NDRange(1)) {
    cl::Buffer RetBuf = AllocReturnBuffer<RetTy>();

    cl::Kernel Kern = BuildKernel<RetTy>(Fun);
    Kern.setArg(0, RetBuf);

    Queue.enqueueNDRangeKernel(Kern, cl::NullRange, Space, Space);
    Queue.enqueueReadBuffer(RetBuf, true, 0, sizeof(RetTy), &R);
  }

  template <typename RetTy, typename A1Ty>
  void Invoke(llvm::StringRef Fun,
              RetTy &R,
              A1Ty A1,
              cl::NDRange Space = cl::NDRange(1)) {
    cl::Buffer RetBuf = AllocReturnBuffer<RetTy>();
    cl::Buffer A1Buf = AllocArgBuffer<A1Ty>();

    cl::Kernel Kern = BuildKernel<RetTy, A1Ty>(Fun);
    Kern.setArg(0, RetBuf);
    Kern.setArg(1, A1Buf);

    Queue.enqueueWriteBuffer(A1Buf, true, 0, sizeof(A1Ty), &A1);
    Queue.enqueueNDRangeKernel(Kern, cl::NullRange, Space, Space);
    Queue.enqueueReadBuffer(RetBuf, true, 0, sizeof(RetTy), &R);
  }

protected:
  template <typename RetTy>
  cl::Kernel BuildKernel(llvm::StringRef Fun) {
    std::string KernName;
    BuildKernelName(Fun, KernName);

    std::ostringstream SrcStream;
    SrcStream << "kernel void " << KernName
              << "(\n"
              << "  global " << OCLTypeTraits<RetTy>::OCLCName.str() << " *r\n"
              << ")\n"
              << "{\n"
              << "  *r = " << Fun.str() << "();\n"
              << "}\n";
    std::string Src = SrcStream.str();

    return GetKernel(KernName, Src);
  }

  template <typename RetTy, typename A1Ty>
  cl::Kernel BuildKernel(llvm::StringRef Fun) {
    std::string KernName;
    BuildKernelName(Fun, KernName);

    std::ostringstream SrcStream;

    SrcStream << "kernel void " << KernName
              << "(\n"
              << "  global " << OCLTypeTraits<RetTy>::OCLCName.str() << " *r,\n"
              << "  global " << OCLTypeTraits<A1Ty>::OCLCName.str() << " *a1\n"
              << ")\n"
              << "{\n"
              << "  *r = " << Fun.str() << "(*a1);\n"
              << "}\n";
    std::string Src = SrcStream.str();

    return GetKernel(KernName, Src);
  }

  template <typename Ty>
  cl::Buffer AllocReturnBuffer(Ty T = Ty()) {
    return cl::Buffer(Ctx, CL_MEM_WRITE_ONLY, sizeof(Ty));
  }

  template <typename Ty>
  cl::Buffer AllocArgBuffer() {
    return cl::Buffer(Ctx, CL_MEM_READ_ONLY, sizeof(Ty));
  }

  void BuildKernelName(llvm::StringRef Fun, std::string &KernName) {
    KernName = Fun.str() + "_test";
  }

  cl::Kernel GetKernel(std::string &Name, std::string &Src) {
    cl::Program::Sources Srcs;
    Srcs.push_back(std::make_pair(Src.c_str(), 0));

    cl::Program Prog(Ctx, Srcs);
    Prog.build(std::vector<cl::Device>());

    return cl::Kernel(Prog, Name.c_str());
  }

private:
  void RuntimeFailed(llvm::StringRef Err) {
    FAIL() << Err.str() << "\n";
  }

protected:
  cl::Platform Plat;
  cl::Device Dev;
  cl::Context Ctx;
  cl::CommandQueue Queue;

  friend void ::RuntimeFailed<DevTy>(const char *,
                                     const void *,
                                     size_t,
                                     void *);
};

template <typename DevTy, typename Ty>
class LibraryGenTypeFixture : public LibraryFixture<DevTy> { };

class CPUDev { };

template <>
class DeviceTraits<CPUDev> {
public:
  #ifdef __x86_64__
  typedef uint64_t SizeType;
  #else
  #error "architecture not supported"
  #endif

public:
  static llvm::StringRef Name;
};

template <typename DevTy>
void RuntimeFailed(const char *Err,
                   const void *PrivInfo,
                   size_t PrivInfoSize,
                   void *UserData) {
  LibraryFixture<DevTy> *Fix = static_cast<LibraryFixture<DevTy> *>(UserData);

  Fix->RuntimeFailed(Err);
}

#endif // UNITTESTS_LIBRARY_LIBRARYFIXTURE_H
