
#include "opencrun/Util/OpenCLMetadataHandler.h"

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/Constants.h"
#include "llvm/Support/ErrorHandling.h"

using namespace opencrun;

//
// ParserState implementation.
//

namespace {

class SignatureBuilder {
public:
  typedef llvm::SmallVector<llvm::Type *, 4> TypesContainer;

public:
  SignatureBuilder(const llvm::StringRef Name,
                   llvm::Module &Mod) : Name(Name),
                                        Mod(Mod) {
    ResetState();
    InitTargetInfo();
  }

public:
  void SetUnsignedPrefix() { Unsigned = true; }
  void SetLongPrefix() { Long = true; }

  void SetInteger() { Integer = true; }
  void SetSizeT() { SizeT = true; }
  void SetVoid() { Void = true; }

  void SetTypeDone() {
    llvm::Type *Ty;

    if(Integer && Unsigned && !Long)
      Ty = GetUIntTy();
    else if(Integer && Unsigned && Long)
      Ty = GetULongTy();
    else if(SizeT)
      Ty = GetSizeTTy();
    else if(Void)
      Ty = GetVoidTy();
    else
      llvm_unreachable("Type not specified");

    Types.push_back(Ty);

    ResetState();
  }

  llvm::Function *CreateFunction() {
    if(Types.size() < 1)
      llvm_unreachable("Not enough arguments");

    llvm::Type *RetTy = Types.front();
    Types.erase(Types.begin());

    llvm::FunctionType *FunTy = llvm::FunctionType::get(RetTy, Types, false);

    llvm::Function *Fun;
    Fun = llvm::Function::Create(FunTy,
                                 llvm::Function::ExternalLinkage,
                                 Name,
                                 &Mod);

    return Fun;
  }

private:
  void ResetState() {
    Unsigned = Long = false;
    Integer = SizeT = Void = false;
  }

  void InitTargetInfo() {
    clang::TargetOptions TargetOpts;
    TargetOpts.Triple = Mod.getTargetTriple();

    llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagIDs;
    DiagIDs = new clang::DiagnosticIDs();

    clang::DiagnosticClient *DiagClient = new clang::DiagnosticClient();
    clang::Diagnostic *Diag = new clang::Diagnostic(DiagIDs, DiagClient);

    TargetInfo.reset(clang::TargetInfo::CreateTargetInfo(*Diag, TargetOpts));

    delete Diag;
  }

  llvm::Type *GetUIntTy() {
    return GetIntTy(clang::TargetInfo::UnsignedInt);
  }

  llvm::Type *GetULongTy() {
    return GetIntTy(clang::TargetInfo::UnsignedLong);
  }

  llvm::Type *GetSizeTTy() {
    return GetIntTy(TargetInfo->getSizeType());
  }

  llvm::Type *GetIntTy(clang::TargetInfo::IntType Ty) {
    unsigned Width = TargetInfo->getTypeWidth(Ty);

    return llvm::Type::getIntNTy(Mod.getContext(), Width);
  }

  llvm::Type *GetVoidTy() {
    return llvm::Type::getVoidTy(Mod.getContext());
  }

private:
  const llvm::StringRef Name;
  llvm::Module &Mod;

  // Prefixes.
  bool Unsigned, Long;

  // Types.
  bool Integer, SizeT, Void;

  TypesContainer Types;

  llvm::OwningPtr<clang::TargetInfo> TargetInfo;
};

} // End anonymous namespace.

//
// OpenCLMetadataHandler implementation.
//

OpenCLMetadataHandler::OpenCLMetadataHandler(llvm::Module &Mod) : Mod(Mod) {
  #define BUILTIN(N, S) \
    Builtins[N] = S;
  #include "opencrun/Util/Builtins.def"
  #undef BUILTIN
}

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

llvm::Function *OpenCLMetadataHandler::GetBuiltin(llvm::StringRef Name) {
  // Function is not a built-in.
  if(!Builtins.count(Name)) {
    std::string ErrMsg = "Unknown builtin: ";
    ErrMsg += Name;

    llvm_unreachable(ErrMsg.c_str());
  }

  llvm::Function *Func = Mod.getFunction(Name);

  // Built-in declaration found inside the module.
  if(Func) {
    if(!HasRightSignature(Func, Builtins[Name])) {
      std::string ErrMsg = "Malformed builtin declaration: ";
      ErrMsg += Name;

      llvm_unreachable(ErrMsg.c_str());
    }
  }

  // Built-in not found in the module, create it.
  else
    Func = BuildBuiltin(Name, Builtins[Name]);

  return Func;
}

bool OpenCLMetadataHandler::HasRightSignature(
                              const llvm::Function *Func,
                              const llvm::StringRef Signature) const {
  // TODO: do the check. Ask Magni about name mangling.
  return true;
}

llvm::Function *
OpenCLMetadataHandler::BuildBuiltin(const llvm::StringRef Name,
                                    const llvm::StringRef Signature) {
  llvm::StringRef::iterator I = Signature.begin(), E = Signature.end();

  SignatureBuilder Bld(Name, Mod);

  while(I != E) {
    // Scan prefix.

    if(*I == 'U') {
      I++;
      Bld.SetUnsignedPrefix();
    }

    if(*I == 'L') {
      I++;
      Bld.SetLongPrefix();
    }

    // Scan type.

    switch(*I++) {
    case 'i':
      Bld.SetInteger();
      break;

    case 'z':
      Bld.SetSizeT();
      break;

    case 'v':
      Bld.SetVoid();
      break;

    default:
      llvm_unreachable("Unknown type");
    }

    // TODO: scan suffix.

    // Build type.
    Bld.SetTypeDone();
  }

  return Bld.CreateFunction();
}
