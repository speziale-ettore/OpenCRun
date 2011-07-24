
#include "OCLLibEmitter.h"

#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

//
// OCLLibEmitter implementation.
//

void OCLLibEmitter::EmitHeader(raw_ostream &OS, OCLLibBuiltin &Blt, unsigned SpecID) {
  // Emit attributes.
  OS << "__attribute__((";
  for(unsigned I = 0, E = Blt.GetAttributesCount(); I != E; ++I) {
    if(I != 0)
      OS << ",";
    OS << Blt.GetAttribute(I);
  }
  OS << "))\n";

  // Emit return type.
  OS << Blt.GetReturnType(SpecID) << " ";

  // Emit name.
  OS << Blt.GetName();

  // Emit arguments.
  OS << "(";
  for(unsigned I = 0, E = Blt.GetParametersCount(); I != E; ++I) {
    if(I != 0)
      OS << ", ";
    OS << Blt.GetParameterType(I, SpecID) << " x" << I;
  }
  OS << ")";
}

//
// OCLLibImplEmitter implementation.
//

void OCLLibImplEmitter::run(raw_ostream &OS) {
  OCLBuiltinContainer Blts = LoadOCLBuiltins(Records);

  EmitSourceFileHeader("OpenCL C generic library implementation.", OS);

  for(OCLBuiltinContainer::iterator I = Blts.begin(),
                                    E = Blts.end(); I != E; ++I)
    for(unsigned J = 0, F = I->GetSpecializationsCount(); J != F; ++J) {
      if(J != 0)
        OS << "\n";
      EmitImplementation(OS, *I, J);
    }
}

void OCLLibImplEmitter::EmitImplementation(raw_ostream &OS,
                                           OCLLibBuiltin &Blt,
                                           unsigned SpecID) {
  EmitHeader(OS, Blt, SpecID);

  OS << " {\n";

  // Base specialization has ID 0: emit user-given code.
  if(SpecID == 0)
    OS << "  " << Blt.GetBaseImplementation() << "\n";

  // Recursive case, call base.
  else {
    OCLVectType *RetTy;
    RetTy = dynamic_cast<OCLVectType *>(&Blt.GetReturnType(SpecID));
    if(!RetTy)
      llvm_unreachable("Not yet implemented");

    // This variable holds return value in non-vector form.
    OS.indent(2);
    EmitArrayDecl(OS, RetTy->GetBaseType(), RetTy->GetWidth(), "RawRetValue");
    OS << ";\n\n";

    // Call base implementation for each vector element.
    for(unsigned I = 0, E = RetTy->GetWidth(); I != E; ++I) {
      // The i-th call writes the i-th element of the return value.
      OS.indent(2);
      OS << "RawRetValue[" << I << "] = " << Blt.GetName();
      OS << "(";

      // Push arguments.
      for(unsigned J = 0, F = Blt.GetParametersCount(); J != F; ++J) {
        if(J != 0)
          OS << ", ";

        // Non-gentype arguments are passed directly to callee.
        OS << "x" << J;

        // Gentype arguments are passed "element-wise".
        if(dynamic_cast<OCLGenType *>(&Blt.GetParameterType(J)))
          OS << "[" << I << "]";
      }

      OS << ");\n";
    }

    OS << "\n";

    // Declare a return value in vector form.
    OS.indent(2);
    EmitDecl(OS, *RetTy, "RetValue");
    OS << " = {\n";

    // Emit the initializer.
    for(unsigned I = 0, E = RetTy->GetWidth(); I != E; ++I) {
      if(I != 0)
        OS << ",\n";
      OS.indent(4);
      OS << "RawRetValue[" << I << "]";
    }
    OS << "\n";

    OS.indent(2) << "};\n\n";

    // Return the vector.
    OS.indent(2) << "return RetValue;\n";
  }

  OS << "}\n";
}

void OCLLibImplEmitter::EmitDecl(raw_ostream &OS,
                                 OCLType &Ty,
                                 llvm::StringRef Name) {
  OS << Ty << " " << Name;
}

void OCLLibImplEmitter::EmitArrayDecl(raw_ostream &OS,
                                      OCLScalarType &BaseTy,
                                      int64_t N,
                                      llvm::StringRef Name) {
  OS << BaseTy << " " << Name << "[" << N << "]";
}
