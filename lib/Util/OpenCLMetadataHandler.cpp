
#include "opencrun/Util/OpenCLMetadataHandler.h"

#include "llvm/Constants.h"

using namespace opencrun;

llvm::Function *
OpenCLMetadataHandler::GetKernel(llvm::StringRef KernName) const {
  llvm::Function *Kern = Mod.getFunction(KernName);

  if(!Kern)
    return NULL;

  llvm::NamedMDNode *KernsMD = Mod.getNamedMetadata("opencl.kernels");

  if(!KernsMD)
    return NULL;

  bool IsKernel = false;

  for(unsigned I = 0, E = KernsMD->getNumOperands(); I != E && !IsKernel; ++I) {
    llvm::MDNode &KernMD = *KernsMD->getOperand(I);

    IsKernel = KernMD.getOperand(KernelSignatureMID) == Kern;
  }

  return IsKernel ? Kern : NULL;
}

clang::LangAS::ID
OpenCLMetadataHandler::GetArgAddressSpace(llvm::Function &Kern, unsigned I) {
  llvm::NamedMDNode *OpenCLMetadata = Mod.getNamedMetadata("opencl.kernels");
  if(!OpenCLMetadata)
    return clang::LangAS::Last;

  llvm::MDNode *AddrSpacesMD = NULL;

  for(unsigned K = 0, E = OpenCLMetadata->getNumOperands(); K != E; ++K) {
    llvm::MDNode *KernMD = OpenCLMetadata->getOperand(K);

    // Not enough metadata.
    if(KernMD->getNumOperands() < KernelArgAddressSpacesMD + 1)
      continue;

    // Kernel found.
    if(KernMD->getOperand(KernelSignatureMID) == &Kern) {
      llvm::Value *RawVal;
      RawVal = KernMD->getOperand(KernelArgAddressSpacesMD);

      AddrSpacesMD = llvm::dyn_cast<llvm::MDNode>(RawVal);
      break;
    }
  }

  if(!AddrSpacesMD || AddrSpacesMD->getNumOperands() < I + 1)
    return clang::LangAS::Last;

  llvm::Value *RawConst = AddrSpacesMD->getOperand(I);
  if(llvm::ConstantInt *AddrSpace = llvm::dyn_cast<llvm::ConstantInt>(RawConst))
    return static_cast<clang::LangAS::ID>(AddrSpace->getZExtValue());
  else
    return clang::LangAS::Last;
}
