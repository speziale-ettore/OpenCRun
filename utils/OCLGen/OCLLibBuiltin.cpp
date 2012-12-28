
#include "OCLLibBuiltin.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"

using namespace opencrun;

namespace {

//
// OCLTypeTable implementation.
//

class OCLTypeTable {
public:
  typedef std::map<llvm::Record *, OCLType *> TypesContainer;

public:
  ~OCLTypeTable() { DeleteContainerSeconds(Types); }

public:
  OCLType *GetType(llvm::Record &R) {
    if(!Types.count(&R))
      BuildType(R);

    return Types[&R];
  }

  OCLType *GetType(llvm::StringRef Name) {
    TypesContainer::iterator I, E;

    for(I = Types.begin(), E = Types.end(); I != E; ++I) {
      OCLType *Type = I->second;
      if(Type->GetName() == Name)
        return Type;
    }

    llvm::PrintFatalError("Type not found: " + Name.str());
  }

private:
  void BuildType(llvm::Record &R) {
    OCLType *Type;

    // OpenCL C scalar types.
    if(R.isSubClassOf("OCLScalarType"))
      Type = new OCLScalarType(R.getName());

    // OpenCL C vector types.
    else if(R.isSubClassOf("OCLVectType")) {
      OCLType *RawBaseType = GetType(*R.getValueAsDef("BaseType"));
      int64_t Width = R.getValueAsInt("Width");

      OCLScalarType *BaseType = llvm::dyn_cast<OCLScalarType>(RawBaseType);
      if(!BaseType)
        llvm::PrintFatalError("Cannot construct a vector for "
                              "a non-scalar type: " + RawBaseType->GetName());

      Type = new OCLVectType(*BaseType, Width);
    }

    // Generic types.
    else if(R.isSubClassOf("OCLGenType"))
      Type = new OCLGenType(R.getValueAsInt("N"));

    // Unknown types.
    else
      llvm::PrintFatalError("Cannot construct a type for " + R.getName());

    Types[&R] = Type;
  }

private:
  TypesContainer Types;
};

class OCLAttributeTable {
public:
  typedef std::map<llvm::Record *, OCLAttribute *> AttributeContainer;

public:
  ~OCLAttributeTable() { DeleteContainerSeconds(Attrs); }

public:
  OCLAttribute *GetAttribute(llvm::Record &R) {
    if(!Attrs.count(&R))
      Attrs[&R] = new OCLAttribute(R.getName());

    return Attrs[&R];
  }

private:
  AttributeContainer Attrs;
};

OCLTypeTable TypeTable;
OCLAttributeTable AttributeTable;

} // End anonymous namespace.

//
// OCLScalarType implementation.
//

std::string OCLScalarType::BuildName(llvm::StringRef Name) {
  if(!Name.startswith("ocl_"))
    llvm::PrintFatalError("Scalar type names must begin with ocl_: " +
                          Name.str());

  llvm::StringRef StrippedName = Name.substr(4);

  if(StrippedName.empty())
    llvm::PrintFatalError("Scalar type names must have a non-empty suffix: " +
                          Name.str());

  // TODO: perform more accurate checks.

  return StrippedName.str();
}

//
// OCLGenType implementation.
//

std::string OCLGenType::BuildName(int64_t N) {
  return "gentype" + llvm::utostr(N);
}

//
// OCLVectType implementation.
//

std::string OCLVectType::BuildName(OCLScalarType &BaseType, int64_t Width) {
  return BaseType.GetName() + llvm::utostr(Width);
}

//
// OCLLibBuiltin implementation.
//

OCLLibBuiltin::OCLLibBuiltin(llvm::Record &R) {
  // Parse name.
  llvm::StringRef RawName = R.getName();
  if(!RawName.startswith("blt_"))
    llvm::PrintFatalError("Builtin name must starts with blt_: " +
                          RawName.str());
  Name.assign(RawName.begin() + 4, RawName.end());

  // Parse return type.
  llvm::Record *Return = R.getValueAsDef("RetType");
  ReturnTy = TypeTable.GetType(*Return);

  // Parse parameter list.
  std::vector<llvm::Record *> Params = R.getValueAsListOfDefs("ParamTypes");
  for(unsigned I = 0, E = Params.size(); I != E; ++I)
    ParamTys.push_back(TypeTable.GetType(*Params[I]));

  // Parse gentype mappings.
  llvm::ListInit *Mappings = R.getValueAsListInit("GentypeSubs");
  for(unsigned I = 0, E = Mappings->size(); I != E; ++I) {
    llvm::Init *CurElem = Mappings->getElement(I);
    llvm::ListInit *CurMappings = llvm::dyn_cast<llvm::ListInit>(CurElem);

    if(!CurMappings)
      llvm::PrintFatalError("Expected gentype mappings list. Found: " +
                            CurElem->getAsString());

    // Get the generic type for this index.
    OCLType *CurType = TypeTable.GetType("gentype" + llvm::utostr(I + 1));
    OCLGenType *GenType = llvm::cast<OCLGenType>(CurType);

    // Map each type specialization to the gentype. We cannot use a multimap
    // because we need to preserve the order in which specialization have been
    // declared by the user.
    for(unsigned J = 0, F = CurMappings->size(); J != F; ++J) {
      llvm::Record *CurSub = CurMappings->getElementAsRecord(J);
      GenTypeSubs[GenType].push_back(TypeTable.GetType(*CurSub));
    }
  }

  // Parse attributes.
  std::vector<llvm::Record *> Attrs = R.getValueAsListOfDefs("Attributes");
  for(unsigned I = 0, E = Attrs.size(); I != E; ++I)
    Attributes.push_back(AttributeTable.GetAttribute(*Attrs[I]));

  // Parse basic built-in implementation.
  BaseImpl = R.getValueAsString("BaseImpl");
}

OCLType &OCLLibBuiltin::GetSpecializedType(OCLType &Ty, unsigned SpecID) {
  if(OCLGenType *GenTy = llvm::dyn_cast<OCLGenType>(&Ty)) {
    if(!GenTypeSubs.count(GenTy))
      llvm::PrintFatalError("Type is not specialized: " + Ty.GetName());

    GenTypeSubsEntry &Subs = GenTypeSubs[GenTy];
    if(SpecID >= Subs.size())
      llvm::PrintFatalError("Not enough specializations");

    return *Subs[SpecID];
  }

  return Ty;
}

//
// LoadOCLBuiltins implementation.
//

OCLBuiltinContainer opencrun::LoadOCLBuiltins(const llvm::RecordKeeper &R) {
  std::vector<llvm::Record *> RawBlts;
  OCLBuiltinContainer Blts;

  RawBlts = R.getAllDerivedDefinitions("GenBuiltin");

  for(unsigned I = 0, E = RawBlts.size(); I < E; ++I)
    Blts.push_back(OCLLibBuiltin(*RawBlts[I]));

  return Blts;
}

//
// LoadOCLVectTypes implementation.
//

OCLVectTypeContainer opencrun::LoadOCLVectTypes(const llvm::RecordKeeper &R) {
  std::vector<llvm::Record *> RawVects;
  OCLVectTypeContainer Vects;

  RawVects = R.getAllDerivedDefinitions("OCLVectType");

  for(unsigned I = 0, E = RawVects.size(); I < E; ++I) {
    OCLType *Ty = TypeTable.GetType(*RawVects[I]);
    Vects.push_back(llvm::cast<OCLVectType>(Ty));
  }

  return Vects;
}
