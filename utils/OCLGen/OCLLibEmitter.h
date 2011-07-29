
#ifndef OCLLIB_EMITTER_H
#define OCLLIB_EMITTER_H

#include "TableGenBackend.h"

#include "OCLLibBuiltin.h"

namespace llvm {

class OCLLibEmitter : public TableGenBackend {
protected:
  OCLLibEmitter(RecordKeeper &R) : Records(R) { }

protected:
  void EmitHeader(raw_ostream &OS, OCLLibBuiltin &Blt, unsigned SpecID);

protected:
  RecordKeeper &Records;
};

class OCLLibImplEmitter : public OCLLibEmitter {
public:
  OCLLibImplEmitter(RecordKeeper &R) : OCLLibEmitter(R) { }

public:
  virtual void run(raw_ostream &OS);

private:
  void EmitImplementation(raw_ostream &OS, OCLLibBuiltin &Blt, unsigned SpecID);
  void EmitDecl(raw_ostream &OS, OCLType &Ty, llvm::StringRef Name);
  void EmitArrayDecl(raw_ostream &OS,
                     OCLScalarType &BaseTy,
                     int64_t N,
                     llvm::StringRef Name);
};

class OCLDefEmitter : public OCLLibEmitter {
public:
  OCLDefEmitter(RecordKeeper &R) : OCLLibEmitter(R) { }

public:
  virtual void run(raw_ostream &OS);

private:
  void EmitIncludes(raw_ostream &OS);
  void EmitTypes(raw_ostream &OS);
  void EmitWorkItemDecls(raw_ostream &OS);
  void EmitSynchronizationDecls(raw_ostream &OS);
  void EmitMacros(raw_ostream &OS);
  void EmitWorkItemRewritingMacros(raw_ostream &OS);
  void EmitSynchronizationRewritingMacros(raw_ostream &OS);
  void EmitBuiltinRewritingMacro(raw_ostream &OS, OCLLibBuiltin &Blt);
};

} // End namespace llvm.

#endif // OCLLIB_EMITTER_H
