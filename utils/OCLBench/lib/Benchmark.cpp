
#include "oclbench/Benchmark.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/system_error.h"

using namespace oclbench;

//
// OpenCL options.
//

static llvm::cl::opt<std::string> PlatformName("platform",
                                               llvm::cl::init("OpenCRun"),
                                               llvm::cl::desc("Platform "
                                                              "to use"),
                                               llvm::cl::value_desc("name"));

static llvm::cl::opt<std::string> DeviceName("device",
                                             llvm::cl::init("CPU"),
                                             llvm::cl::desc("Device to use"),
                                             llvm::cl::value_desc("name"));

//
// Benchmark options.
//

static llvm::cl::opt<unsigned> BenchLoops("bench-loops",
                                          llvm::cl::init(1),
                                          llvm::cl::desc("How many times "
                                                         "exec the benchmark"),
                                          llvm::cl::value_desc("N"));

static llvm::cl::opt<Benchmark::Class>
BenchClass("bench-class",
           llvm::cl::desc("Choose benchmark class"),
           llvm::cl::init(Benchmark::ClassA),
           llvm::cl::values(clEnumValN(Benchmark::ClassA,
                                       "a",
                                       "Smallest size (default)"),
                            clEnumValN(Benchmark::ClassB,
                                       "b",
                                       "Medium size"),
                            clEnumValN(Benchmark::ClassC,
                                       "c",
                                       "Bigger size"),
                            clEnumValEnd));

//
// Benchmark implementation.
//

Benchmark::Benchmark(int argc, char *argv[]) {
  // Parse command line.
  llvm::cl::ParseCommandLineOptions(argc, argv);

  // Get the file name.
  BenchName = llvm::sys::path::filename(argv[0]);

  // Always add current directory to .cl files search path.
  BCLPath.push_back(llvm::SmallString<32>("."));

  // Add other directories from environment.
  // TODO: support benchmarking on windows -- i.e. use ";" instead of ": as
  // separator.
  if(const char *EnvVar = std::getenv("BCL_PATH")) {
    llvm::StringRef Raw = EnvVar;
    llvm::SmallVector<llvm::StringRef, 4> Paths;

    Raw.split(Paths, ":");
    BCLPath.append(Paths.begin(), Paths.end());
  }

  // Instantiate a printer.
  Printer.reset(new BenchmarkConsolePrinter());
}

Benchmark::~Benchmark() {
  llvm::DeleteContainerPointers(Results);
  llvm::DeleteContainerSeconds(Sources);
}

bool Benchmark::Exec() {
  bool Success = true;

  Results.reserve(BenchLoops);

  Printer->BenchmarkStart(BenchName);

  for(unsigned I = 0; I < BenchLoops && Success; ++I) {
    Printer->IterationStart(BenchName, I);

    try {
      BenchmarkResult *Result;

      PreSetUp();
      SetUp();
      Result = Run(BenchClass);
      TearDown();
      PostTearDown();

      // A result have been computed.
      if(Result) {
        Results.push_back(Result);
        Success = Result->IsSuccess();

      // Catastrophic error.
      } else
        Success = false;

    } catch(Error ex) {
      Printer->FrameworkError(BenchName, ex.what());
      Success = false;

    } catch(cl::Error ex) {
      Printer->OpenCLError(BenchName, ex.what());
      Success = false;

    } catch(std::exception ex) {
      Printer->UnknownError(BenchName, ex.what());
      Success = false;
    }

    Printer->IterationEnd(BenchName, I);
  }

  Printer->BenchmarkEnd(BenchName);

  Printer->BenchmarkResults(BenchName, Success, Results);

  return Success;
}

void Benchmark::PreSetUp() {
  LookupPlatform();
  LookupDevice();
  SetupContext();
  SetupCommandQueue();
}

void Benchmark::PostTearDown() {
  // OpenCL resources are automatically released by the C++ wrapper library.
}

void Benchmark::LookupPlatform() {
  std::vector<cl::Platform> Plats;
  cl::Platform::get(&Plats);

  for(std::vector<cl::Platform>::iterator I = Plats.begin(),
                                          E = Plats.end();
                                          I != E && !Plat();
                                          ++I)
    if(I->getInfo<CL_PLATFORM_NAME>() == PlatformName)
      Plat = *I;

  if(!Plat())
    throw Error("OpenCL platform \"" + PlatformName + "\" not available");
}

void Benchmark::LookupDevice() {
  std::vector<cl::Device> Devs;
  Plat.getDevices(CL_DEVICE_TYPE_ALL, &Devs);

  for(std::vector<cl::Device>::iterator I = Devs.begin(),
                                        E = Devs.end();
                                        I != E && !Dev();
                                        ++I)
    if(I->getInfo<CL_DEVICE_NAME>() == DeviceName)
      Dev = *I;

  if(!Dev())
    throw Error("OpenCL device \"" + DeviceName + "\" not available");
}

void Benchmark::SetupContext() {
  cl_context_properties Props[] =
    { CL_CONTEXT_PLATFORM,
      reinterpret_cast<cl_context_properties>(Plat()),
      0
    };
  Ctx = cl::Context(std::vector<cl::Device>(1, Dev), Props);
}

void Benchmark::SetupCommandQueue() {
  Queue = cl::CommandQueue(Ctx, Dev);
}

cl::Program Benchmark::LoadProgramFromFile(llvm::StringRef File) {
  // Load file if not yet done.
  if(!Sources.count(File)) {
    llvm::OwningPtr<llvm::MemoryBuffer> BufPtr;

    // Try loading the file.
    for(BCLPathContainer::iterator I = BCLPath.begin(),
                                   E = BCLPath.end();
                                   I != E && !BufPtr;
                                   ++I) {
      llvm::SmallString<32> Path = *I;
      llvm::sys::path::append(Path, File);

      llvm::MemoryBuffer::getFile(Path, BufPtr);
    }

    // File not found.
    if(!BufPtr)
      throw Error("Cannot open " + File.str());

    Sources[File] = BufPtr.take();
  }

  llvm::MemoryBuffer *Buf = Sources[File];

  // Collect sources.
  cl::Program::Sources Srcs;
  Srcs.push_back(std::make_pair(Buf->getBufferStart(), 0));

  // Create the program.
  cl::Program Prog(Ctx, Srcs);
  Prog.build(std::vector<cl::Device>());

  return Prog;
}
