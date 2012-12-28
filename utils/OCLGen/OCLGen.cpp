//===- TableGen.cpp - Top-Level TableGen implementation -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Simple TableGen tool to automate library building in OpenCRun.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/TableGen/Main.h"

#include "OCLLibEmitter.h"

using namespace opencrun;

enum ActionType {
  GenOCLDef,
  GenOCLLibImpl
};

namespace {

llvm::cl::opt<ActionType>
Action(llvm::cl::desc("Action to perform:"),
       llvm::cl::values(clEnumValN(GenOCLDef, "gen-ocldef",
                                   "Generate ocldef.h"),
                        clEnumValN(GenOCLLibImpl, "gen-ocl-lib-impl",
                                   "Generate OpenCL C library implementation"),
                        clEnumValEnd));

bool OCLGenMain(llvm::raw_ostream &OS, llvm::RecordKeeper &R);

} // End anonymous namespace.

int main(int argc, char *argv[]) {
  llvm::PrettyStackTraceProgram X(argc, argv);

  llvm::sys::PrintStackTraceOnErrorSignal();
  llvm::cl::ParseCommandLineOptions(argc, argv);

  return llvm::TableGenMain(argv[0], OCLGenMain);
}

namespace {

bool OCLGenMain(llvm::raw_ostream &OS, llvm::RecordKeeper &R) {
  switch (Action) {
  case GenOCLDef:
    EmitOCLDef(OS, R);
    break;

  case GenOCLLibImpl:
    EmitOCLLibImpl(OS, R);
    break;
  }

  return false;
}

} // End anonymous namespace.
