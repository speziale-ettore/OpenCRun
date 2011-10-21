
#include "opencrun/Passes/AllPasses.h"
#include "opencrun/Passes/FootprintEstimate.h"
#include "opencrun/Util/OpenCLMetadataHandler.h"

#define DEBUG_TYPE "footprint-estimate"

#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/InstIterator.h"

using namespace opencrun;

bool FootprintEstimate::runOnFunction(llvm::Function &Fun) {
  OpenCLMetadataHandler MDHandler(*Fun.getParent());

  if(MDHandler.IsKernel(Fun) && (Kernel == Fun.getName() || Kernel == "")) {
    for(llvm::inst_iterator I = inst_begin(Fun), E = inst_end(Fun); I != E; ++I)
      AnalyzeMemoryUsage(*I);
  }

  return false;
}

void FootprintEstimate::print(llvm::raw_ostream &OS,
                               const llvm::Module *Mod) const {
  // Private memory usage.

  size_t PrivateMemoryUsage = GetPrivateMemoryUsage();
  float PrivateMemoryUsageAcc = GetPrivateMemoryUsageAccuracy();

  OS << "Private memory: " << PrivateMemoryUsage
                           << " bytes (min), "
     << llvm::format("%1.5f", PrivateMemoryUsageAcc)
                           << " (accuracy)\n";

  // Some estimates of maximum work-group size.

  // First of all, computed the maximum field width.
  size_t WIField = std::ceil(std::log10(GetMaxWorkGroupSize(64 * 1024)));

  // Now print all estimates.
  OS << "Work-group size: ";
  for(size_t Size = 16; Size <= 64; Size *= 2) {
    if(Size != 16)
      OS.indent(19); // 19 = strlen("Work-group size: ") + 2

    OS << llvm::format("%*zu", WIField, GetMaxWorkGroupSize(Size * 1024))
       << " (max), "
       << llvm::format("%zuKbytes", Size)
       << " (available private memory)\n";
  }
}

void FootprintEstimate::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

void FootprintEstimate::AnalyzeMemoryUsage(llvm::Instruction &I) {
  // There are restrictions on the functions we can call.
  if(llvm::CallInst *Call = llvm::dyn_cast<llvm::CallInst>(&I)) {
    llvm::Function *Fun = Call->getCalledFunction();

    // Indirect call not allowed in OpenCL. Moreover, we expect normalized
    // kernel, that is every function have been in-lined. The only allowed
    // functions to call are external functions -- typically internal
    // calls/runtime builtins.
    if(!Fun || !Fun->hasExternalLinkage())
      return;

    // Add the function return value.
    AddTempMemoryUsage(GetSizeLowerBound(*Call->getType()));
  }

  // Alloca instruction will be mapped on private memory.
  else if(llvm::AllocaInst *Alloca = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
    // Get the number of allocated values, one if not an array.
    llvm::Value *ElsVal = Alloca->getArraySize();

    // Automatic variable declaration.
    if(llvm::ConstantInt *Els = llvm::dyn_cast<llvm::ConstantInt>(ElsVal)) {
      llvm::Type *Ty = Alloca->getAllocatedType();

      AddPrivateMemoryUsage(Els->getZExtValue() * GetSizeLowerBound(*Ty));
    }

  // Other instructions can produce a value. It is temporary.
  } else
    AddTempMemoryUsage(GetSizeLowerBound(*I.getType()));
}

size_t FootprintEstimate::GetSizeLowerBound(llvm::Type &Ty) {
  // This pass operates on the intermediate code, e.g. we have not access to
  // target information, so we have to estimate the actual value using
  // the size of the LLVM types.

  if(!Ty.isSized())
    return 0;

  // Getting primitive types size is straightforward.
  if(Ty.isPrimitiveType())
    return Ty.getPrimitiveSizeInBits() / 8;

  // Non primitive types required custom inspection.

  // Integer: ceiling to byte.
  else if(llvm::IntegerType *IntTy = llvm::dyn_cast<llvm::IntegerType>(&Ty)) {
    size_t BitWidth = IntTy->getBitWidth(),
           Size = BitWidth / 8;

    if(BitWidth % 8)
      ++Size;

    return Size;

  // Struct: cycle over elements and sum their values.
  } else if(llvm::StructType *StructTy =
              llvm::dyn_cast<llvm::StructType>(&Ty)) {
    size_t Size = 0;

    for(llvm::StructType::subtype_iterator I = StructTy->subtype_begin(),
                                           E = StructTy->subtype_end();
                                           I != E;
                                           ++I)
      Size += GetSizeLowerBound(**I);

    return Size;
  }

  // Array: base type * number of elements.
  else if(llvm::ArrayType *ArrayTy = llvm::dyn_cast<llvm::ArrayType>(&Ty)) {
    llvm::Type *ElsTy = ArrayTy->getElementType();

    return ArrayTy->getNumElements() * GetSizeLowerBound(*ElsTy);

  // Vector: like Array.
  } else if(llvm::VectorType *VectorTy =
              llvm::dyn_cast<llvm::VectorType>(&Ty)) {
    llvm::Type *ElsTy = VectorTy->getElementType();

    return VectorTy->getNumElements() * GetSizeLowerBound(*ElsTy);
  }

  // Pointer: in OpenCL a pointer can be 32 or 64 bit wide, since we have to
  // provide a lower bound, estimate pointer size equal to 32 bits.
  else if(llvm::isa<llvm::PointerType>(&Ty))
    return 4;

  return 0;
}

char FootprintEstimate::ID = 0;

FootprintEstimate *
opencrun::CreateFootprintEstimatePass(llvm::StringRef Kernel) {
  return new FootprintEstimate(Kernel);
}

using namespace llvm;

INITIALIZE_PASS_BEGIN(FootprintEstimate,
                      "footprint-estimate",
                      "Estimate kernel footprint",
                      false,
                      false)
INITIALIZE_PASS_END(FootprintEstimate,
                    "footprint-estimate",
                    "Estimate kernel footprint",
                    false,
                    false)
