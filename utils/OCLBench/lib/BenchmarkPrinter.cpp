
#include "oclbench/BenchmarkPrinter.h"
#include "oclbench/Benchmark.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/CommandLine.h"

using namespace oclbench;

//
// BenchmarkConsolePrinter options.
//

static llvm::cl::opt<bool> Verbose("verbose",
                                   llvm::cl::init(false),
                                   llvm::cl::Hidden,
                                   llvm::cl::desc("Enable verbose output"));

static llvm::cl::opt<bool> DisableColors("disable-colors",
                                         llvm::cl::init(false),
                                         llvm::cl::Hidden,
                                         llvm::cl::desc("Whether to disable "
                                                        "colored output"));

//
// BenchmarkConsolePrinter implementation.
//

BenchmarkConsolePrinter::BenchmarkConsolePrinter() : OS(llvm::outs()),
                                                     ES(llvm::errs()),
                                                     FirstBenchmark(true) { }

void BenchmarkConsolePrinter::BenchmarkStart(llvm::StringRef Bench) {
  if(!FirstBenchmark)
    OS << "\n";

  if(Verbose)
    PrintPrefix(Bench, OS) << "Benchmark Start\n";

  FirstBenchmark = false;
}

void BenchmarkConsolePrinter::BenchmarkEnd(llvm::StringRef Bench) {
  if(Verbose)
    PrintPrefix(Bench, OS) << "Benchmark End\n";
}

void BenchmarkConsolePrinter::BenchmarkResults(
  llvm::StringRef Bench,
  bool Success,
  std::vector<BenchmarkResult *> &Results) {
  PrintPrefix(Bench, OS) << "Benchmark Results\n";

  PrintPrefix(Bench, OS);
  OS.indent(2) << "Runs: " << llvm::utostr(Results.size()) << "/"
                           << llvm::utostr(Results.capacity())
                           << " (actual/planned)\n";

  PrintPrefix(Bench, OS);
  OS.indent(2);

  PrintSuccess(Success, OS);

  if(!Verbose)
    return;

  for(unsigned I = 0, E = Results.size(); I != E; ++I) {
    PrintPrefix(Bench, OS) << "  * Iteration " << llvm::utostr(I) << "\n";

    PrintPrefix(Bench, OS);
    OS.indent(4);
    PrintSuccess(Results[I]->IsSuccess(), OS);
  }
}

void BenchmarkConsolePrinter::IterationStart(llvm::StringRef Bench,
                                             unsigned I) {
  if(Verbose)
    PrintPrefix(Bench, OS) << "Iteration " << llvm::utostr(I) << " Start\n";
}

void BenchmarkConsolePrinter::IterationEnd(llvm::StringRef Bench, unsigned I) {
  if(Verbose)
    PrintPrefix(Bench, OS) << "Iteration " << llvm::utostr(I) << " End\n";
}

void BenchmarkConsolePrinter::Error(llvm::StringRef Bench,
                                    llvm::StringRef Kind,
                                    llvm::StringRef Msg) {
  PrintPrefix(Bench, ES);

  if(!DisableColors)
    ES.changeColor(llvm::raw_ostream::RED, true);

  if(!Kind.empty())
    ES << Kind << " ";

  ES << "error";

  if(!Msg.empty())
    ES << ": ";

  if(!DisableColors)
    ES.resetColor();

  ES << Msg << "\n";
}

llvm::raw_ostream &
BenchmarkConsolePrinter::PrintPrefix(llvm::StringRef BenchName,
                                     llvm::raw_ostream &OS) {
  if(!DisableColors)
    OS.changeColor(llvm::raw_ostream::CYAN, true);

  OS << BenchName << ": ";

  if(!DisableColors)
    OS.resetColor();

  return OS;
}

llvm::raw_ostream &
BenchmarkConsolePrinter::PrintSuccess(bool Success, llvm::raw_ostream &OS) {
  OS << "Success: ";

  if(!DisableColors) {
    llvm::raw_ostream::Colors Color;
    Color = Success ? llvm::raw_ostream::GREEN : llvm::raw_ostream::RED;

    OS.changeColor(Color, true);
  }

  OS << (Success ? "Yes" : "No") << "\n";

  if(!DisableColors)
    OS.resetColor();

  return OS;
}
