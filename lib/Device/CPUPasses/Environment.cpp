
#include "PassUtils.h"

#include "llvm/Support/CommandLine.h"

using namespace opencrun;

static llvm::cl::opt<std::string> Kernel("kernel",
                                         llvm::cl::init(""),
                                         llvm::cl::Hidden,
                                         llvm::cl::desc("Kernel to be "
                                                        "transformed"));

static llvm::cl::opt<bool> VerifyCPUPass("verify-cpu-pass",
                                         llvm::cl::init(false),
                                         llvm::cl::Hidden,
                                         llvm::cl::desc("Verify OpenCL "
                                                        "CPU pass"));

std::string opencrun::GetKernelOption(llvm::StringRef Default) {
  return Kernel != "" ? Kernel : Default;
}

bool opencrun::GetVerifyCPUPassOption() {
  return VerifyCPUPass;
}
