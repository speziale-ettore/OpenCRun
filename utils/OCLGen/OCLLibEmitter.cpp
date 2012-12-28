
#include "llvm/TableGen/TableGenBackend.h"

#include "OCLLibEmitter.h"

using namespace opencrun;

namespace {

void EmitHeader(llvm::raw_ostream &OS, OCLLibBuiltin &Blt, unsigned SpecID);

void EmitIncludes(llvm::raw_ostream &OS);
void EmitTypes(llvm::raw_ostream &OS, llvm::RecordKeeper &R);
void EmitWorkItemDecls(llvm::raw_ostream &OS);
void EmitSynchronizationDecls(llvm::raw_ostream &OS);
void EmitMacros(llvm::raw_ostream &OS);
void EmitWorkItemRewritingMacros(llvm::raw_ostream &OS);
void EmitSynchronizationRewritingMacros(llvm::raw_ostream &OS);
void EmitBuiltinRewritingMacro(llvm::raw_ostream &OS, OCLLibBuiltin &Blt);

void EmitImplementation(llvm::raw_ostream &OS,
                        OCLLibBuiltin &Blt,
                        unsigned SpecID);
void EmitAutoDecl(llvm::raw_ostream &OS, OCLType &Ty, llvm::StringRef Name);
void EmitAutoArrayDecl(llvm::raw_ostream &OS,
                       OCLScalarType &BaseTy,
                       int64_t N,
                       llvm::StringRef Name);

} // End anonymous namespace.

//
// EmitOCLDef implementation.
//

void opencrun::EmitOCLDef(llvm::raw_ostream &OS, llvm::RecordKeeper &R) {
  OCLBuiltinContainer Blts = LoadOCLBuiltins(R);

  llvm::emitSourceFileHeader("OpenCL C library definitions.", OS);

  // Emit opening guard.
  OS << "#ifndef __OPENCRUN_OCLDEF_H\n"
     << "#define __OPENCRUN_OCLDEF_H\n";

  EmitIncludes(OS);

  // Emit C++ opening guard.
  OS << "\n"
     << "#ifdef __cplusplus\n"
     << "extern \"C\" {\n"
     << "#endif\n";

  EmitTypes(OS, R);
  EmitWorkItemDecls(OS);
  EmitSynchronizationDecls(OS);

  EmitMacros(OS);

  EmitWorkItemRewritingMacros(OS);
  EmitSynchronizationRewritingMacros(OS);

  // Emit generic definitions.
  OS << "\n/* BEGIN AUTO-GENERATED PROTOTYPES */\n";
  for(OCLBuiltinContainer::iterator I = Blts.begin(),
                                    E = Blts.end();
                                    I != E;
                                    ++I) {
    for(unsigned J = 0, F = I->GetSpecializationsCount(); J != F; ++J) {
        OS << "\n";
      EmitHeader(OS, *I, J);
      OS << ";";
    }
    OS << "\n";
  }
  OS << "\n/* END AUTO-GENERATED PROTOTYPES */\n";

  // Emit builtin rewriting macros.
  OS << "\n/* BEGIN AUTO-GENERATED BUILTIN MACROS */\n"
     << "\n";
  for(OCLBuiltinContainer::iterator I = Blts.begin(),
                                    E = Blts.end();
                                    I != E;
                                    ++I)
    EmitBuiltinRewritingMacro(OS, *I);
  OS << "\n/* END AUTO-GENERATED BUILTIN MACROS */\n";

  // Emit C++ closing guard.
  OS << "\n"
     << "#ifdef __cplusplus\n"
     << "}\n"
     << "#endif\n";

  // Emit closing guard.
  OS << "\n#endif /* __CLANG_OCLDEF_H */\n";
}

//
// EmitOCLLibImpl implementation.
//

void opencrun::EmitOCLLibImpl(llvm::raw_ostream &OS, llvm::RecordKeeper &R) {
  OCLBuiltinContainer Blts = LoadOCLBuiltins(R);

  llvm::emitSourceFileHeader("OpenCL C generic library implementation.", OS);

  for(OCLBuiltinContainer::iterator I = Blts.begin(),
                                    E = Blts.end();
                                    I != E;
                                    ++I)
    for(unsigned J = 0, F = I->GetSpecializationsCount(); J != F; ++J) {
      if(J != 0)
        OS << "\n";
      EmitImplementation(OS, *I, J);
    }
}

namespace {

void EmitHeader(llvm::raw_ostream &OS, OCLLibBuiltin &Blt, unsigned SpecID) {
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
  OS << "__builtin_ocl_" << Blt.GetName();

  // Emit arguments.
  OS << "(";
  for(unsigned I = 0, E = Blt.GetParametersCount(); I != E; ++I) {
    if(I != 0)
      OS << ", ";
    OS << Blt.GetParameterType(I, SpecID) << " x" << I;
  }
  OS << ")";
}

void EmitIncludes(llvm::raw_ostream &OS) {
  OS << "\n"
     << "#include <float.h>\n"
     << "#include <stddef.h>\n"
     << "#include <stdint.h>\n";
}

void EmitTypes(llvm::raw_ostream &OS, llvm::RecordKeeper &R) {
  // In OpenCL C unsigned types have a shortcut.
  OS << "\n"
     << "/* Built-in Scalar Data Types */\n"
     << "\n"
     << "typedef unsigned char uchar;\n"
     << "typedef unsigned short ushort;\n"
     << "typedef unsigned int uint;\n"
     << "typedef unsigned long ulong;\n";

  // Emit vectors using definitions found in the .td files.
  OCLVectTypeContainer Vectors = LoadOCLVectTypes(R);

  OS << "\n"
     << "/* Built-in Vector Data Types */\n"
     << "\n";

  for(OCLVectTypeContainer::iterator I = Vectors.begin(),
                                     E = Vectors.end();
                                     I != E;
                                     ++I) {
    OCLVectType &VectTy = **I;
    OS << "typedef __attribute__(("
       << "ext_vector_type(" << (*I)->GetWidth() << ")"
       << ")) "
       << VectTy.GetBaseType() << " " << VectTy << ";\n";
  }

  // Other types.
  OS << "\n"
     << "/* Other derived types */\n"
     << "\n"
     << "typedef ulong cl_mem_fence_flags;\n";
}

void EmitWorkItemDecls(llvm::raw_ostream &OS) {
  OS << "\n"
     << "/* Work-Item Functions */\n"
     << "\n"
     << "uint __builtin_ocl_get_work_dim();\n"
     << "size_t __builtin_ocl_get_global_size(uint dimindx);\n"
     << "size_t __builtin_ocl_get_global_id(uint dimindx);\n"
     << "size_t __builtin_ocl_get_local_size(uint dimindx);\n"
     << "size_t __builtin_ocl_get_local_id(uint dimindx);\n"
     << "size_t __builtin_ocl_get_num_groups(uint dimindx);\n"
     << "size_t __builtin_ocl_get_group_id(uint dimindx);\n"
     << "size_t __builtin_ocl_get_global_offset(uint dimindx);\n";
}

void EmitSynchronizationDecls(llvm::raw_ostream &OS) {
  OS << "\n"
     << "/* Synchronization Functions */\n"
     << "\n"
     << "void __builtin_ocl_barrier(cl_mem_fence_flags flags);\n";
}

void EmitMacros(llvm::raw_ostream &OS) {
  // Math macros.
  OS << "\n"
     << "/*\n"
     << " * These macros are defined in math.h, but because we\n"
     << " * cannot include it, define them here. Definitions picked-up\n"
     << " * from GNU math.h.\n"
     << " */\n"
     << "\n"
     << "#define M_E            2.71828182845904523540f  /* e          */\n"
     << "#define M_LOG2E        1.44269504088896340740f  /* log_2 e    */\n"
     << "#define M_LOG10E       0.43429448190325182765f  /* log_10 e   */\n"
     << "#define M_LN2          0.69314718055994530942f  /* log_e 2    */\n"
     << "#define M_LN10         2.30258509299404568402f  /* log_e 10   */\n"
     << "#define M_PI           3.14159265358979323846f  /* pi         */\n"
     << "#define M_PI_2         1.57079632679489661923f  /* pi/2       */\n"
     << "#define M_PI_4         0.78539816339744830962f  /* pi/4       */\n"
     << "#define M_1_PI         0.31830988618379067154f  /* 1/pi       */\n"
     << "#define M_2_PI         0.63661977236758134308f  /* 2/pi       */\n"
     << "#define M_2_SQRTPI     1.12837916709551257390f  /* 2/sqrt(pi) */\n"
     << "#define M_SQRT2        1.41421356237309504880f  /* sqrt(2)    */\n"
     << "#define M_SQRT1_2      0.70710678118654752440f  /* 1/sqrt(2)  */\n";

  // Synchronization macros.
  OS << "\n"
     << "/* Synchronization Macros*/\n"
     << "#define CLK_LOCAL_MEM_FENCE  0\n"
     << "#define CLK_GLOBAL_MEM_FENCE 1\n";
}

void EmitWorkItemRewritingMacros(llvm::raw_ostream &OS) {
  OS << "\n"
     << "/* Work-Item Rewriting Macros */\n"
     << "\n"
     << "#define get_work_dim __builtin_ocl_get_work_dim\n"
     << "#define get_global_size __builtin_ocl_get_global_size\n"
     << "#define get_global_id __builtin_ocl_get_global_id\n"
     << "#define get_local_size __builtin_ocl_get_local_size\n"
     << "#define get_local_id __builtin_ocl_get_local_id\n"
     << "#define get_num_groups __builtin_ocl_get_num_groups\n"
     << "#define get_group_id __builtin_ocl_get_group_id\n"
     << "#define get_global_offset __builtin_ocl_get_global_offset\n";
}

void EmitSynchronizationRewritingMacros(llvm::raw_ostream &OS) {
  OS << "\n"
     << "/* Synchronization Rewriting Macros */\n"
     << "\n"
     << "#define barrier __builtin_ocl_barrier\n";
}

void EmitBuiltinRewritingMacro(llvm::raw_ostream &OS, OCLLibBuiltin &Blt) {
  OS << "#define " << Blt.GetName() << " "
                   << "__builtin_ocl_" << Blt.GetName()
                   << "\n";
}

void EmitImplementation(llvm::raw_ostream &OS,
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
    RetTy = llvm::dyn_cast<OCLVectType>(&Blt.GetReturnType(SpecID));
    if(!RetTy)
      llvm_unreachable("Not yet implemented");

    // This variable holds return value in non-vector form.
    OS.indent(2);
    EmitAutoArrayDecl(OS, RetTy->GetBaseType(), RetTy->GetWidth(), "RawRetValue");
    OS << ";\n\n";

    // Call base implementation for each vector element.
    for(unsigned I = 0, E = RetTy->GetWidth(); I != E; ++I) {
      // The i-th call writes the i-th element of the return value.
      OS.indent(2);
      OS << "RawRetValue[" << I << "] = " << "__builtin_ocl_" + Blt.GetName();
      OS << "(";

      // Push arguments.
      for(unsigned J = 0, F = Blt.GetParametersCount(); J != F; ++J) {
        if(J != 0)
          OS << ", ";

        // Non-gentype arguments are passed directly to callee.
        OS << "x" << J;

        // Gentype arguments are passed "element-wise".
        if(llvm::isa<OCLGenType>(&Blt.GetParameterType(J)))
          OS << "[" << I << "]";
      }

      OS << ");\n";
    }

    OS << "\n";

    // Declare a return value in vector form.
    OS.indent(2);
    EmitAutoDecl(OS, *RetTy, "RetValue");
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

void EmitAutoDecl(llvm::raw_ostream &OS, OCLType &Ty, llvm::StringRef Name) {
  OS << Ty << " " << Name;
}

void EmitAutoArrayDecl(llvm::raw_ostream &OS,
                       OCLScalarType &BaseTy,
                       int64_t N,
                       llvm::StringRef Name) {
  OS << BaseTy << " " << Name << "[" << N << "]";
}

} // End anonymous namespace.
