
#ifndef OCLLIB_EMITTER_H
#define OCLLIB_EMITTER_H

#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Record.h"

#include "OCLLibBuiltin.h"

namespace opencrun {

void EmitOCLDef(llvm::raw_ostream &OS, llvm::RecordKeeper &R);
void EmitOCLLibImpl(llvm::raw_ostream &OS, llvm::RecordKeeper &R);

} // End namespace opencrun.

#endif // OCLLIB_EMITTER_H
