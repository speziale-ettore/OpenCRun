//===- TableGen.cpp - Top-Level TableGen implementation -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// TableGen is a tool which can be used to build up a description of something,
// then invoke one or more "tablegen backends" to emit information about the
// description in some predefined format.  In practice, this is used by the LLVM
// code generators to automate generation of a code generator through a
// high-level description of the target.
//
// Since TableGen does not support dynamic back-end loading, we have customized
// it in order to add some OpenCL specific generators.
//
//===----------------------------------------------------------------------===//

#include "Error.h"
#include "Record.h"
#include "SetTheory.h"
#include "TGParser.h"
#include "OCLLibEmitter.h"

#include "llvm/ADT/OwningPtr.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/system_error.h"

#include <algorithm>
#include <cstdio>

using namespace llvm;

enum ActionType {
  PrintRecords,
  GenOCLLibImpl,
  PrintEnums,
  PrintSets
};

namespace {
  cl::opt<ActionType>
  Action(cl::desc("Action to perform:"),
         cl::values(clEnumValN(PrintRecords, "print-records",
                               "Print all records to stdout (default)"),
                    clEnumValN(GenOCLLibImpl, "gen-ocl-lib-impl",
                               "Generate OpenCL C library implementation"),
                    clEnumValN(PrintEnums, "print-enums",
                               "Print enum values for a class"),
                    clEnumValN(PrintSets, "print-sets",
                               "Print expanded sets for testing DAG exprs"),
                    clEnumValEnd));

  cl::opt<std::string>
  Class("class", cl::desc("Print Enum list for this class"),
        cl::value_desc("class name"));

  cl::opt<std::string>
  OutputFilename("o", cl::desc("Output filename"), cl::value_desc("filename"),
                 cl::init("-"));

  cl::opt<std::string>
  DependFilename("d", cl::desc("Dependency filename"), cl::value_desc("filename"),
                 cl::init(""));

  cl::opt<std::string>
  InputFilename(cl::Positional, cl::desc("<input file>"), cl::init("-"));

  cl::list<std::string>
  IncludeDirs("I", cl::desc("Directory of include files"),
              cl::value_desc("directory"), cl::Prefix);
}

int main(int argc, char *argv[]) {
  RecordKeeper Records;

  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);
  cl::ParseCommandLineOptions(argc, argv);

  try {
    // Parse the input file.
    OwningPtr<MemoryBuffer> File;
    if (error_code ec = MemoryBuffer::getFileOrSTDIN(InputFilename.c_str(), File)) {
      errs() << "Could not open input file '" << InputFilename << "': "
             << ec.message() << "\n";
      return 1;
    }
    MemoryBuffer *F = File.take();

    // Tell SrcMgr about this buffer, which is what TGParser will pick up.
    SrcMgr.AddNewSourceBuffer(F, SMLoc());

    // Record the location of the include directory so that the lexer can find
    // it later.
    SrcMgr.setIncludeDirs(IncludeDirs);

    TGParser Parser(SrcMgr, Records);

    if (Parser.ParseFile())
      return 1;

    std::string Error;
    tool_output_file Out(OutputFilename.c_str(), Error);
    if (!Error.empty()) {
      errs() << argv[0] << ": error opening " << OutputFilename
        << ":" << Error << "\n";
      return 1;
    }
    if (!DependFilename.empty()) {
      if (OutputFilename == "-") {
        errs() << argv[0] << ": the option -d must be used together with -o\n";
        return 1;
      }
      tool_output_file DepOut(DependFilename.c_str(), Error);
      if (!Error.empty()) {
        errs() << argv[0] << ": error opening " << DependFilename
          << ":" << Error << "\n";
        return 1;
      }
      DepOut.os() << DependFilename << ":";
      const std::vector<std::string> &Dependencies = Parser.getDependencies();
      for (std::vector<std::string>::const_iterator I = Dependencies.begin(),
                                                    E = Dependencies.end();
                                                    I != E; ++I) {
        DepOut.os() << " " << (*I);
      }
      DepOut.os() << "\n";
      DepOut.keep();
    }

    switch (Action) {
    case PrintRecords:
      // No argument, dump all contents.
      Out.os() << Records;
      break;
    case GenOCLLibImpl:
      OCLLibImplEmitter(Records).run(Out.os());
      break;
    case PrintEnums:
    {
      std::vector<Record*> Recs = Records.getAllDerivedDefinitions(Class);
      for (unsigned i = 0, e = Recs.size(); i != e; ++i)
        Out.os() << Recs[i]->getName() << ", ";
      Out.os() << "\n";
      break;
    }
    case PrintSets:
    {
      SetTheory Sets;
      Sets.addFieldExpander("Set", "Elements");
      std::vector<Record*> Recs = Records.getAllDerivedDefinitions("Set");
      for (unsigned i = 0, e = Recs.size(); i != e; ++i) {
        Out.os() << Recs[i]->getName() << " = [";
        const std::vector<Record*> *Elts = Sets.expand(Recs[i]);
        assert(Elts && "Couldn't expand Set instance");
        for (unsigned ei = 0, ee = Elts->size(); ei != ee; ++ei)
          Out.os() << ' ' << (*Elts)[ei]->getName();
        Out.os() << " ]\n";
      }
      break;
    }
    default:
      assert(1 && "Invalid Action");
      return 1;
    }

    // Declare success.
    Out.keep();
    return 0;

  } catch (const TGError &Error) {
    PrintError(Error);
  } catch (const std::string &Error) {
    PrintError(Error);
  } catch (const char *Error) {
    PrintError(Error);
  } catch (...) {
    errs() << argv[0] << ": Unknown unexpected exception occurred.\n";
  }

  return 1;
}
