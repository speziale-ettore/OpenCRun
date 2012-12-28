
#ifndef OCLLIB_BUILTIN_H
#define OCLLIB_BUILTIN_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/TableGen/Error.h"
#include "llvm/TableGen/Record.h"

#include <map>
#include <vector>

namespace opencrun {

class OCLType {
public:
  enum Type {
    Scalar,
    Vect,
    Gen
  };

public:
  virtual ~OCLType() { }

protected:
  OCLType(Type TypeTy, llvm::StringRef Name) : TypeTy(TypeTy),
                                               Name(Name) { }

public:
  Type GetType() const { return TypeTy; }
  std::string GetName() const { return Name; }

private:
  Type TypeTy;
  std::string Name;
};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const OCLType &Ty) {
  return OS << Ty.GetName();
}

class OCLScalarType : public OCLType {
public:
  static bool classof(const OCLType *Ty) {
    return Ty->GetType() == OCLType::Scalar;
  }

public:
  OCLScalarType(llvm::StringRef Name) : OCLType(OCLType::Scalar,
                                                BuildName(Name)) { }

private:
  std::string BuildName(llvm::StringRef Name);
};

class OCLVectType : public OCLType {
public:
  static bool classof(const OCLType *Ty) {
    return Ty->GetType() == OCLType::Vect;
  }

public:
  OCLVectType(OCLScalarType &BaseType,
              int64_t Width) : OCLType(OCLType::Vect,
                                       BuildName(BaseType, Width)),
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
  static bool classof(const OCLType *Ty) {
    return Ty->GetType() == OCLType::Gen;
  }

public:
  OCLGenType(int64_t N) : OCLType(OCLType::Gen, BuildName(N)),
                          N(N) { }

public:
  int64_t getN() { return N; }

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

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                     const OCLAttribute &Attr) {
  return OS << Attr.GetName();
}

class OCLLibBuiltin {
public:
  typedef std::vector<OCLType *> GenTypeSubsEntry;
  typedef std::map<OCLGenType *, GenTypeSubsEntry> GenTypeSubsContainer;

public:
  OCLLibBuiltin(llvm::Record &R);

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
      llvm::PrintFatalError("Out of range parameter index: " + llvm::utostr(I));

    return *ParamTys[I];
  }

  OCLType &GetParameterType(unsigned I, unsigned SpecID) {
    return GetSpecializedType(GetParameterType(I), SpecID);
  }

  OCLAttribute &GetAttribute(unsigned I) {
    if(I >= GetAttributesCount())
      llvm::PrintFatalError("Out of range attribute index: " + llvm::utostr(I));

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
  llvm::SmallVector<OCLType *, 4> ParamTys;
  GenTypeSubsContainer GenTypeSubs;

  llvm::SmallVector<OCLAttribute *, 2> Attributes;

  std::string BaseImpl;
};

typedef std::vector<OCLLibBuiltin> OCLBuiltinContainer;
typedef std::vector<OCLVectType *> OCLVectTypeContainer;

OCLBuiltinContainer LoadOCLBuiltins(const llvm::RecordKeeper &R);
OCLVectTypeContainer LoadOCLVectTypes(const llvm::RecordKeeper &R);

} // End namespace opencrun.

#endif // OCLLIB_BUILTIN_H
