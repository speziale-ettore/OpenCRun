
#ifndef OCLBENCH_BENCHMARKPRINTER_H
#define OCLBENCH_BENCHMARKPRINTER_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <vector>

namespace oclbench {

class BenchmarkResult;

class BenchmarkPrinter {
public:
  virtual ~BenchmarkPrinter() { }

public:
  virtual void BenchmarkStart(llvm::StringRef Bench) = 0;
  virtual void BenchmarkEnd(llvm::StringRef Bench) = 0;

  virtual void BenchmarkResults(llvm::StringRef Bench,
                                bool Success,
                                std::vector<BenchmarkResult *> &Results) = 0;

  virtual void IterationStart(llvm::StringRef Bench, unsigned I) = 0;
  virtual void IterationEnd(llvm::StringRef Bench, unsigned I) = 0;

  virtual void Error(llvm::StringRef Bench,
                     llvm::StringRef Kind = "",
                     llvm::StringRef Msg = "") = 0;

  void FrameworkError(llvm::StringRef Bench, llvm::StringRef Msg = "") {
    Error(Bench, "Framework", Msg);
  }

  void OpenCLError(llvm::StringRef Bench, llvm::StringRef Msg = "") {
    Error(Bench, "OpenCL", Msg);
  }

  void UnknownError(llvm::StringRef Bench, llvm::StringRef Msg = "") {
    Error(Bench, "Unknown", Msg);
  }
};

class BenchmarkConsolePrinter : public BenchmarkPrinter {
public:
  BenchmarkConsolePrinter();

  // Do not implement.
  BenchmarkConsolePrinter(const BenchmarkConsolePrinter &That);

  // Do not implement.
  const BenchmarkConsolePrinter &operator=(const BenchmarkConsolePrinter &That);

public:
  virtual void BenchmarkStart(llvm::StringRef Bench);
  virtual void BenchmarkEnd(llvm::StringRef Bench);

  virtual void BenchmarkResults(llvm::StringRef Bench,
                                bool Success,
                                std::vector<BenchmarkResult *> &Results);

  virtual void IterationStart(llvm::StringRef Bench, unsigned I);
  virtual void IterationEnd(llvm::StringRef Bench, unsigned I);

  virtual void Error(llvm::StringRef Bench,
                     llvm::StringRef Kind = "",
                     llvm::StringRef Msg = "");

private:
  llvm::raw_ostream &PrintPrefix(llvm::StringRef BenchName,
                                 llvm::raw_ostream &OS);
  llvm::raw_ostream &PrintSuccess(bool Success, llvm::raw_ostream &OS);

private:
  llvm::raw_ostream &OS;
  llvm::raw_ostream &ES;

  bool FirstBenchmark;
};

} // End namespace oclbench.

#endif // OCLBENCH_BENCHMARKPRINTER_H
