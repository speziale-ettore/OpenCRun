
#include "opencrun/Util/PassOptions.h"

#include "llvm/Support/CommandLine.h"

using namespace opencrun;

static llvm::cl::opt<std::string> Kernel("kernel",
                                         llvm::cl::init(""),
                                         llvm::cl::Hidden,
                                         llvm::cl::desc("Kernel to be "
                                                        "transformed"));

std::string opencrun::GetKernelOption(llvm::StringRef Default) {
  return Kernel != "" ? Kernel : Default;
}
