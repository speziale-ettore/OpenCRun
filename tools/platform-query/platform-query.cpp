
#include <algorithm>
#include <iostream>
#include <vector>

#include <cstdlib>

#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>

#define BUF_LENGHT \
  1024
#define BUF_SIZE \
  (BUF_LENGHT * sizeof(char))

void ShowPlatforms();
void ShowPlatform(const cl::Platform &Plat);
void ShowDevice(const cl::Device &Dev);

void NativeKernelHello();
void SayHelloFromAllNativeDevices(const cl::Platform &Plat);

void NDRangeHello();
void SayNDHelloFromAllDevices(const cl::Platform &Plat);

void SayHello(void *Args);
void GetGreeter(void *Args);

const char *NDRangeHelloSrc = "kernel void NDRangeHello(global char *buf) {  "
                              "  unsigned i = get_global_id(0);              "
                              "  buf[i] = \"Hello, From NDRange Device\"[i]; "
                              "}                                             ";

void DebugPrinter(const char *Err,
                  const void *PrivInfo,
                  size_t PrivInfoSize,
                  void *UserData);

int main(int argc, char* argv[]) {
  std::cout << "OpenCL Test Program -- Start" << std::endl;

  ShowPlatforms();
  NativeKernelHello();
  NDRangeHello();

  std::cout << "OpenCL Test Program -- End" << std::endl;

  return EXIT_SUCCESS;
}

void ShowPlatforms() {
  std::vector<cl::Platform> Plats;
  cl::Platform::get(&Plats);

  std::cout << "## Found " << Plats.size() << " Platform[s] ##" << std::endl;
  std::for_each(Plats.begin(), Plats.end(), ShowPlatform);
  std::cout << "## Done Platforms ##" << std::endl;
}

void ShowPlatform(const cl::Platform &Plat) {
  std::cout << "  Platform: " << Plat.getInfo<CL_PLATFORM_NAME>()
            << std::endl
            << "  Profile: " << Plat.getInfo<CL_PLATFORM_PROFILE>()
            << std::endl
            << "  Version: " << Plat.getInfo<CL_PLATFORM_VERSION>()
            << std::endl
            << "  Vendor: " << Plat.getInfo<CL_PLATFORM_VENDOR>()
            << std::endl
            << "  Extensions: " << Plat.getInfo<CL_PLATFORM_EXTENSIONS>()
            << std::endl;

  std::vector<cl::Device> Devs;
  Plat.getDevices(CL_DEVICE_TYPE_ALL, &Devs);

  std::cout << "  ## Found " << Devs.size() << " Device[s] ##" << std::endl;
  std::for_each(Devs.begin(), Devs.end(), ShowDevice);
  std::cout << "  ## Done Devices ##" << std::endl;
}

void ShowDevice(const cl::Device &Dev) {
  std::cout << "    Device: " << Dev.getInfo<CL_DEVICE_NAME>()
            << std::endl

            << "    Type: " << Dev.getInfo<CL_DEVICE_TYPE>()
            << std::endl

            << "    Vendor ID: " << Dev.getInfo<CL_DEVICE_VENDOR_ID>()
            << std::endl
            << "    Max Compute Units: "
            << Dev.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>()
            << std::endl
            << "    Max Work Item Dimensions: "
            << Dev.getInfo<CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS>()
            << std::endl;

  std::vector<size_t> MaxWISizes = Dev.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
  std::cout << "    Max Work Item Sizes:";
  for(std::vector<size_t>::iterator I = MaxWISizes.begin(),
                                    E = MaxWISizes.end();
                                    I != E;
                                    ++I)
    std::cout << " " << *I;
  std::cout << std::endl;

  std::cout << "    Max Work Group Size: "
            << Dev.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>()
            << std::endl
            << "    Preferred Char Vector Width: "
            << Dev.getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR>()
            << std::endl
            << "    Preferred Short Vector Width: "
            << Dev.getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT>()
            << std::endl
            << "    Preferred Int Vector Width: "
            << Dev.getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT>()
            << std::endl
            << "    Preferred Long Vector Width: "
            << Dev.getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG>()
            << std::endl
            << "    Preferred Float Vector Width: "
            << Dev.getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT>()
            << std::endl
            << "    Preferred Half Vector Width: "
            << Dev.getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF>()
            << std::endl
            << "    Native Char Vector Width: "
            << Dev.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR>()
            << std::endl
            << "    Native Short Vector Width: "
            << Dev.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT>()
            << std::endl
            << "    Native Int Vector Width: "
            << Dev.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_INT>()
            << std::endl
            << "    Native Long Vector Width: "
            << Dev.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG>()
            << std::endl
            << "    Native Float Vector Width: "
            << Dev.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT>()
            << std::endl
            << "    Native Half Vector Width: "
            << Dev.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF>()
            << std::endl
            << "    Max Clock Frequency: "
            << Dev.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>()
            << std::endl
            << "    Address Bits: " << Dev.getInfo<CL_DEVICE_ADDRESS_BITS>()
            << std::endl

            << "    Max Memory Allocation Size: "
            << Dev.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>()
            << std::endl

            << "    Image Support: " << Dev.getInfo<CL_DEVICE_IMAGE_SUPPORT>()
            << std::endl
            << "    Max Read Image Args: "
            << Dev.getInfo<CL_DEVICE_MAX_READ_IMAGE_ARGS>()
            << std::endl
            << "    Max Write Image Args: "
            << Dev.getInfo<CL_DEVICE_MAX_WRITE_IMAGE_ARGS>()
            << std::endl
            << "    Image2D Max Width: "
            << Dev.getInfo<CL_DEVICE_IMAGE2D_MAX_WIDTH>()
            << std::endl
            << "    Image2D Max Height: "
            << Dev.getInfo<CL_DEVICE_IMAGE2D_MAX_HEIGHT>()
            << std::endl
            << "    Image3D Max Width: "
            << Dev.getInfo<CL_DEVICE_IMAGE3D_MAX_WIDTH>()
            << std::endl
            << "    Image3D Max Depth: "
            << Dev.getInfo<CL_DEVICE_IMAGE3D_MAX_DEPTH>()
            << std::endl
            << "    Max Samplers: " << Dev.getInfo<CL_DEVICE_MAX_SAMPLERS>()
            << std::endl

            << "    Max Parameter Size: "
            << Dev.getInfo<CL_DEVICE_MAX_PARAMETER_SIZE>()
            << std::endl

            << "    Memory Base Address Alignment: "
            << Dev.getInfo<CL_DEVICE_MEM_BASE_ADDR_ALIGN>()
            << std::endl
            << "    Min Data Type Alignment Size: "
            << Dev.getInfo<CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE>()
            << std::endl

            << "    Single Precision FP Config: "
            << Dev.getInfo<CL_DEVICE_SINGLE_FP_CONFIG>()
            << std::endl
            
            << "    Global Memory Cache Type: "
            << Dev.getInfo<CL_DEVICE_GLOBAL_MEM_CACHE_TYPE>()
            << std::endl
            << "    Global Memory Cache Line Size: "
            << Dev.getInfo<CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE>()
            << std::endl
            << "    Global Memory Cache Size: "
            << Dev.getInfo<CL_DEVICE_GLOBAL_MEM_CACHE_SIZE>()
            << std::endl
            << "    Global Memory Size: "
            << Dev.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>()
            << std::endl

            << "    Max Constant Buffer Size: "
            << Dev.getInfo<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>()
            << std::endl
            << "    Max Constant Arguments: "
            << Dev.getInfo<CL_DEVICE_MAX_CONSTANT_ARGS>()
            << std::endl

            << "    Local Memory Type: "
            << Dev.getInfo<CL_DEVICE_LOCAL_MEM_TYPE>()
            << std::endl
            << "    Local Memory Size: "
            << Dev.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>()
            << std::endl
            << "    Error Correction Support: "
            << Dev.getInfo<CL_DEVICE_ERROR_CORRECTION_SUPPORT>()
            << std::endl

            << "    Host Unified Memory: "
            << Dev.getInfo<CL_DEVICE_HOST_UNIFIED_MEMORY>()
            << std::endl

            << "    Profiling Timer Resolution: "
            << Dev.getInfo<CL_DEVICE_PROFILING_TIMER_RESOLUTION>()
            << std::endl

            << "    Little Endian: " << Dev.getInfo<CL_DEVICE_ENDIAN_LITTLE>()
            << std::endl
            << "    Available: " << Dev.getInfo<CL_DEVICE_AVAILABLE>()
            << std::endl

            << "    Compiler Available: "
            << Dev.getInfo<CL_DEVICE_COMPILER_AVAILABLE>()
            << std::endl

            << "    Execution Capabilities: "
            << Dev.getInfo<CL_DEVICE_EXECUTION_CAPABILITIES>()
            << std::endl

            << "    Queue Properties: "
            << Dev.getInfo<CL_DEVICE_QUEUE_PROPERTIES>()
            << std::endl

            << "    Platform: "
            << Dev.getInfo<CL_DEVICE_PLATFORM>()
            << std::endl;
}

void NativeKernelHello() {
  std::vector<cl::Platform> Plats;
  cl::Platform::get(&Plats);

  std::for_each(Plats.begin(), Plats.end(), SayHelloFromAllNativeDevices);
}

void SayHelloFromAllNativeDevices(const cl::Platform &Plat) {
  std::vector<cl::Device> Devs;
  Plat.getDevices(CL_DEVICE_TYPE_ALL, &Devs);

  std::cout << "## Invoking Native Kernels ##" << std::endl;

  for(std::vector<cl::Device>::iterator I = Devs.begin(),
                                        E = Devs.end();
                                        I != E;
                                        ++I) {
    if(!(I->getInfo<CL_DEVICE_EXECUTION_CAPABILITIES>() &
         CL_EXEC_NATIVE_KERNEL))
      continue;

    cl_context_properties Props[] =
      { CL_CONTEXT_PLATFORM,
        reinterpret_cast<cl_context_properties>(Plat()),
        0
      };
    cl::Context Ctx(std::vector<cl::Device>(1, *I), Props, DebugPrinter);
    cl::CommandQueue Queue(Ctx, *I);

    char HostBufA[BUF_LENGHT] = { '1', '2', '3', '\0' };
    char HostBufB[BUF_LENGHT] = { '4', '5', '6', '\0' };
    char HostBufC[BUF_LENGHT];

    // Simple native kernel invocation.
    cl::Buffer BufA(Ctx,
                    CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                    BUF_SIZE,
                    HostBufA);
    void *RawArgs[] = { (void *) BufA(), (void *) HostBufB };

    std::pair<void *, size_t> Args;
    Args.first = RawArgs;
    Args.second = sizeof(void *) * 2;

    std::vector<cl::Memory> Bufs(1, BufA);
    std::vector<const void *> BufMappings(1, &RawArgs[0]);

    Queue.enqueueNativeKernel(SayHello, Args, &Bufs, &BufMappings);

    // Try sending a buffer before invoking the native kernel.
    cl::Buffer BufB(Ctx, CL_MEM_READ_ONLY, BUF_SIZE);
    RawArgs[1] = (void *) BufB();

    Bufs.push_back(BufB);
    BufMappings.push_back(&RawArgs[1]);

    Queue.enqueueWriteBuffer(BufB, true, 0, BUF_SIZE, HostBufB);
    Queue.enqueueNativeKernel(SayHello, Args, &Bufs, &BufMappings);

    // Try reading from the device after native kernel invocation.
    cl::Buffer BufC(Ctx, CL_MEM_WRITE_ONLY, BUF_SIZE);
    RawArgs[1] = (void *) BufC();

    Bufs[1] = BufC;

    Queue.enqueueNativeKernel(GetGreeter, Args, &Bufs, &BufMappings);
    Queue.enqueueReadBuffer(BufC, true, 0, BUF_SIZE, HostBufC);

    std::cout << "Hello (" << HostBufC << "), From Host" << std::endl;

    Queue.finish();
  }

  std::cout << "## Done Invoking Native Kernels ##" << std::endl;
}

void NDRangeHello() {
  std::vector<cl::Platform> Plats;
  cl::Platform::get(&Plats);

  std::for_each(Plats.begin(), Plats.end(), SayNDHelloFromAllDevices);
}

void SayNDHelloFromAllDevices(const cl::Platform &Plat) {
  std::vector<cl::Device> Devs;
  Plat.getDevices(CL_DEVICE_TYPE_ALL, &Devs);

  std::cout << "## Invoking NDRange Kernels ##" << std::endl;

  cl_context_properties Props[] =
    { CL_CONTEXT_PLATFORM,
      reinterpret_cast<cl_context_properties>(Plat()),
      0
    };
  cl::Context Ctx(Devs, Props, DebugPrinter);

  cl::Program::Sources Srcs;
  Srcs.push_back(std::make_pair(NDRangeHelloSrc, 0));

  cl::Program Prog(Ctx, Srcs);
  Prog.build(std::vector<cl::Device>());

  char HostBufA[BUF_LENGHT];
  cl::Buffer BufA(Ctx, CL_MEM_WRITE_ONLY, BUF_SIZE);

  cl::Kernel Kernel(Prog, "NDRangeHello");
  Kernel.setArg(0, BufA);

  for(std::vector<cl::Device>::iterator I = Devs.begin(),
                                        E = Devs.end();
                                        I != E;
                                        ++I) {
    cl::CommandQueue Queue(Ctx, *I);

    Queue.enqueueNDRangeKernel(Kernel,
                               cl::NullRange,
                               cl::NDRange(BUF_SIZE),
                               cl::NDRange(1));
    Queue.enqueueReadBuffer(BufA, true, 0, BUF_SIZE, HostBufA);

    std::cout << "Hello (" << HostBufA << "), From Device" << std::endl;
  }

  std::cout << "## Done Invoking NDRange Kernels ##" << std::endl;
}

void SayHello(void *Args) {
  char **CastedArgs = static_cast<char **>(Args);
  char *FromHost = CastedArgs[0];
  char *FromBuf = CastedArgs[1];

  std::cout << "Hello (" << FromHost << ", " << FromBuf << "), "
            << "From Device" << std::endl;
}

void GetGreeter(void *Args) {
  char **CastedArgs = static_cast<char **>(Args);
  char *FromHost = CastedArgs[0];
  char *ToHost = CastedArgs[1];

  std::strcpy(ToHost, FromHost);
}

void DebugPrinter(const char *Err,
                  const void *PrivInfo,
                  size_t PrivInfoSize,
                  void *UserData) {
  std::cerr << "OpenCL Debug: " << Err << std::endl;
}
