
#include "OCLLibBuiltin.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"

#include <map>

using namespace llvm;

namespace {

//
// OCLTypeTable implementation.
//

class OCLTypeTable {
public:
  typedef std::map<Record *, OCLType *> TypesContainer;

public:
  ~OCLTypeTable() { DeleteContainerSeconds(Types); }

public:
  OCLType *GetType(Record &R) {
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

    throw "Type not found: " + Name.str();
  }

private:
  void BuildType(Record &R) {
    OCLType *Type;

    // OpenCL C scalar types.
    if(R.isSubClassOf("OCLScalarType"))
      Type = new OCLScalarType(R.getName());

    // OpenCL C vector types.
    else if(R.isSubClassOf("OCLVectType")) {
      OCLType *RawBaseType = GetType(*R.getValueAsDef("BaseType"));
      int64_t Width = R.getValueAsInt("Width");

      OCLScalarType *BaseType = dynamic_cast<OCLScalarType *>(RawBaseType);
      if(!BaseType)
        throw "Cannot construct a vector for a non-scalar type: " +
              RawBaseType->GetName();

      Type = new OCLVectType(*BaseType, Width);
    }

    // Generic types.
    else if(R.isSubClassOf("OCLGenType"))
      Type = new OCLGenType(R.getValueAsInt("N"));

    // Unknown types.
    else
      throw "Cannot construct a type for " + R.getName();

    Types[&R] = Type;
  }

private:
  TypesContainer Types;
};

class OCLAttributeTable {
public:
  typedef std::map<Record *, OCLAttribute *> AttributeContainer;

public:
  ~OCLAttributeTable() { DeleteContainerSeconds(Attrs); }

public:
  OCLAttribute *GetAttribute(Record &R) {
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
    throw "Scalar type names must begin with ocl_: " + Name.str();

  llvm::StringRef StrippedName = Name.substr(4);

  if(StrippedName.empty())
    throw "Scalar type names must have a non-empty suffix: " + Name.str();

  // TODO: perform more accurate checks.

  return StrippedName.str();
}

//
// OCLGenType implementation.
//

std::string OCLGenType::BuildName(int64_t N) {
  return "gentype" + utostr(N);
}

//
// OCLVectType implementation.
//

std::string OCLVectType::BuildName(OCLScalarType &BaseType, int64_t Width) {
  return BaseType.GetName() + utostr(Width);
}

//
// OCLLibBuiltin implementation.
//

OCLLibBuiltin::OCLLibBuiltin(Record &R) {
  // Parse name.
  llvm::StringRef RawName = R.getName();
  if(!RawName.startswith("blt_"))
    throw "Builtin name must starts with blt_: " + RawName.str();
  Name.assign(RawName.begin() + 4, RawName.end());

  // Parse return type.
  Record *Return = R.getValueAsDef("RetType");
  ReturnTy = TypeTable.GetType(*Return);

  // Parse parameter list.
  std::vector<Record *> Params = R.getValueAsListOfDefs("ParamTypes");
  for(unsigned I = 0, E = Params.size(); I != E; ++I)
    ParamTys.push_back(TypeTable.GetType(*Params[I]));

  // Parse gentype mappings.
  ListInit *Mappings = R.getValueAsListInit("GentypeSubs");
  for(unsigned I = 0, E = Mappings->size(); I != E; ++I) {
    Init *CurElem = Mappings->getElement(I);
    ListInit *CurMappings = dynamic_cast<ListInit *>(CurElem);

    if(!CurMappings)
      throw "Expected gentype mappings list. Found: " + CurElem->getAsString();

    // Get the generic type for this index.
    OCLType *CurType = TypeTable.GetType("gentype" + utostr(I + 1));
    OCLGenType *GenType = static_cast<OCLGenType *>(CurType);

    // Map each type specialization to the gentype. We cannot use a multimap
    // because we need to preserve the order in which specialization have been
    // declared by the user.
    for(unsigned J = 0, F = CurMappings->size(); J != F; ++J) {
      Record *CurSub = CurMappings->getElementAsRecord(J);
      GenTypeSubs[GenType].push_back(TypeTable.GetType(*CurSub));
    }
  }

  // Parse attributes.
  std::vector<Record *> Attrs = R.getValueAsListOfDefs("Attributes");
  for(unsigned I = 0, E = Attrs.size(); I != E; ++I)
    Attributes.push_back(AttributeTable.GetAttribute(*Attrs[I]));

  // Parse basic built-in implementation.
  BaseImpl = R.getValueAsCode("BaseImpl");
}

OCLType &OCLLibBuiltin::GetSpecializedType(OCLType &Ty, unsigned SpecID) {
  if(OCLGenType *GenTy = dynamic_cast<OCLGenType *>(&Ty)) {
    if(!GenTypeSubs.count(GenTy))
      throw "Type is not specialized: " + Ty.GetName();

    GenTypeSubsEntry &Subs = GenTypeSubs[GenTy];
    if(SpecID >= Subs.size())
      throw "Not enough specializations";

    return *Subs[SpecID];
  }

  return Ty;
}

//
// LoadOCLBuiltins implementation.
//

llvm::OCLBuiltinContainer llvm::LoadOCLBuiltins(const RecordKeeper &RC) {
  std::vector<Record *> RawBlts = RC.getAllDerivedDefinitions("GenBuiltin");
  OCLBuiltinContainer Blts;

  for(unsigned I = 0, E = RawBlts.size(); I < E; ++I)
    Blts.push_back(OCLLibBuiltin(*RawBlts[I]));

  return Blts;
}
