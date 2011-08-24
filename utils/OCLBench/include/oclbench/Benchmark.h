
#ifndef OCLBENCH_BENCHMARK_H
#define OCLBENCH_BENCHMARK_H

#include "oclbench/BenchmarkPrinter.h"

#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/MemoryBuffer.h"

#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>

namespace oclbench {

class BenchmarkResult {
public:
  BenchmarkResult() : Success(false) { }

public:
  void SetSuccess(bool Success = true) { this->Success = Success; }

public:
  bool IsSuccess() const { return Success; }

private:
  bool Success;
};

class Benchmark {
public:
  enum Class {
    ClassA,
    ClassB,
    ClassC
  };

public:
  typedef llvm::SmallVector<llvm::SmallString<32>, 4> BCLPathContainer;
  typedef std::vector<BenchmarkResult *> ResultsContainer;

public:
  Benchmark(int argc, char *argv[]);
  virtual ~Benchmark();

public:
  virtual void SetUp() { }
  virtual BenchmarkResult *Run(Class Cls) = 0;
  virtual void TearDown() { }

  bool Exec();

private:
  void PreSetUp();
  void PostTearDown();

  void LookupPlatform();
  void LookupDevice();
  void SetupContext();
  void SetupCommandQueue();

protected:
  cl::Program LoadProgramFromFile(llvm::StringRef File);

protected:
  cl::Platform Plat;
  cl::Device Dev;
  cl::Context Ctx;
  cl::CommandQueue Queue;

private:
  std::string BenchName;

  BCLPathContainer BCLPath;
  llvm::StringMap<llvm::MemoryBuffer *> Sources;

  ResultsContainer Results;
  llvm::OwningPtr<BenchmarkPrinter> Printer;
};

class Error : public std::exception {
public:
  Error(llvm::StringRef Msg = "") throw() : Msg(Msg) { }
  virtual ~Error() throw() { }

public:
  virtual const char *what() const throw() {
    return Msg.c_str();
  }

private:
  std::string Msg;
};

#define PROVIDE_BENCHMARK_LAUNCHER(B) \
  int main(int argc, char *argv[]) {  \
    B Bench(argc, argv);              \
                                      \
    return !Bench.Exec();             \
  }

} // End namespace oclbench.

#endif // OCLBENCH_BENCHMARK_H
