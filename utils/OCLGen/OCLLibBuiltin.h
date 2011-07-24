
#ifndef OCLLIB_BUILTIN_H
#define OCLLIB_BUILTIN_H

#include "Record.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"

#include <map>
#include <vector>

namespace llvm {

class OCLType {
public:
  virtual ~OCLType() { }

protected:
  OCLType(llvm::StringRef Name) : Name(Name) { }

public:
  std::string GetName() const { return Name; }

private:
  std::string Name;
};

inline raw_ostream &operator<<(raw_ostream &OS, const OCLType &Ty) {
  return OS << Ty.GetName();
}

class OCLScalarType : public OCLType {
public:
  OCLScalarType(llvm::StringRef Name) : OCLType(BuildName(Name)) { }

private:
  std::string BuildName(llvm::StringRef Name);
};

class OCLVectType : public OCLType {
public:
  OCLVectType(OCLScalarType &BaseType,
              int64_t Width) : OCLType(BuildName(BaseType, Width)),
                               BaseType(BaseType),
                               Width(Width) { }

public:
  OCLScalarType &GetBaseType() { return BaseType; }
  int64_t GetWidth() { return Width; }

private:
  std::string BuildName(OCLScalarType &BaseType, int64_t Width);

private:
  OCLScalarType &BaseType;
  int64_t Width;
};

class OCLGenType : public OCLType {
public:
  OCLGenType(int64_t N) : OCLType(BuildName(N)), N(N) { }

private:
  std::string BuildName(int64_t N);

private:
  int64_t N;
};

class OCLAttribute {
public:
  OCLAttribute(llvm::StringRef Name) : Name(Name) { }

public:
  std::string GetName() const { return Name; }

private:
  std::string Name;
};

inline raw_ostream &operator<<(raw_ostream &OS, const OCLAttribute &Attr) {
  return OS << Attr.GetName();
}

class OCLLibBuiltin {
public:
  typedef std::vector<OCLType *> GenTypeSubsEntry;
  typedef std::map<OCLGenType *, GenTypeSubsEntry> GenTypeSubsContainer;

public:
  OCLLibBuiltin(Record &R);

  OCLLibBuiltin() : ReturnTy(NULL) { }

  OCLLibBuiltin(const OCLLibBuiltin &That) : Name(That.Name),
                                             ReturnTy(That.ReturnTy),
                                             ParamTys(That.ParamTys),
                                             GenTypeSubs(That.GenTypeSubs),
                                             Attributes(That.Attributes),
                                             BaseImpl(That.BaseImpl)
                                             { }

  const OCLLibBuiltin &operator=(const OCLLibBuiltin &That) {
    if(this != &That) {
      Name = That.Name;
      ReturnTy = That.ReturnTy;
      ParamTys = That.ParamTys;
      GenTypeSubs = That.GenTypeSubs;
      Attributes = That.Attributes;
      BaseImpl = That.BaseImpl;
    }

    return *this;
  }

public:
  size_t GetParametersCount() const {
    return ParamTys.size();
  }

  size_t GetSpecializationsCount() const {
    // No specializations.
    if(GenTypeSubs.empty())
      return 0;

    // All GenTypeSubs elements have the same length by construction.
    GenTypeSubsContainer::const_iterator I = GenTypeSubs.begin();

    return I->second.size();
  }

  size_t GetAttributesCount() const {
    return Attributes.size();
  }

public:
  std::string &GetName() { return Name; }

  OCLType &GetReturnType() {
    return *ReturnTy;
  }

  OCLType &GetReturnType(unsigned SpecID) {
    return GetSpecializedType(GetReturnType(), SpecID);
  }

  OCLType &GetParameterType(unsigned I) {
    if(I >= GetParametersCount())
      throw "Out of range parameter index: " + utostr(I);

    return *ParamTys[I];
  }

  OCLType &GetParameterType(unsigned I, unsigned SpecID) {
    return GetSpecializedType(GetParameterType(I), SpecID);
  }

  OCLAttribute &GetAttribute(unsigned I) {
    if(I >= GetAttributesCount())
      throw "Out of range attribute index: " + utostr(I);

    return *Attributes[I];
  }

  std::string GetBaseImplementation() {
    return BaseImpl;
  }

private:
  OCLType &GetSpecializedType(OCLType &Ty, unsigned SpecID);

private:
  std::string Name;

  OCLType *ReturnTy;
  SmallVector<OCLType *, 4> ParamTys;
  GenTypeSubsContainer GenTypeSubs;

  SmallVector<OCLAttribute *, 2> Attributes;

  std::string BaseImpl;
};

typedef std::vector<OCLLibBuiltin> OCLBuiltinContainer;

OCLBuiltinContainer LoadOCLBuiltins(const RecordKeeper &RC);

} // End namespace llvm.

#endif // OCLLIB_BUILTIN_H
