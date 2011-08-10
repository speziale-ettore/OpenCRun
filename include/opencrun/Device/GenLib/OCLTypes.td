
//===----------------------------------------------------------------------===//
/// OCLType - Root of the OpenCL C type system.
//===----------------------------------------------------------------------===//
class OCLType { }

//===----------------------------------------------------------------------===//
/// OCLScalareType - An OpenCL C scalar type.
//===----------------------------------------------------------------------===//
class OCLScalarType : OCLType { }

//===----------------------------------------------------------------------===//
/// OCLVectType - An OpenCL C vector type. The vectors contatins width elements
///  of base_type.
//===----------------------------------------------------------------------===//
class OCLVectType <OCLScalarType base_type, int width> : OCLType {
  OCLScalarType BaseType = base_type;
  int Width = width;
}

//===----------------------------------------------------------------------===//
/// OCLGenType - A generic OpenCL C type. This type is used to mark generic
///  arguments in builtin definitions.
//===----------------------------------------------------------------------===//
class OCLGenType <int n> : OCLType {
  int N = n;
}

//===----------------------------------------------------------------------===//
/// OCLVectorizableType - Declare standard OpenCL C vectors for the given
///  base_type.
//===----------------------------------------------------------------------===//
multiclass OCLVectorizableType <OCLScalarType base_type> {
  def  _2 : OCLVectType<base_type,  2>;
  def  _3 : OCLVectType<base_type,  3>;
  def  _4 : OCLVectType<base_type,  4>;
  def  _8 : OCLVectType<base_type,  8>;
  def _16 : OCLVectType<base_type, 16>;
}

//===----------------------------------------------------------------------===//
/// OCLGenericType - Declare a bunch of OCLGenType.
//===----------------------------------------------------------------------===//
multiclass OCLGenericType {
  def _1 : OCLGenType<1>;
}

// Integer scalar types.
def ocl_short : OCLScalarType;
def ocl_int   : OCLScalarType;
def ocl_long  : OCLScalarType;

// Integer vector types.
defm ocl_short_v : OCLVectorizableType<ocl_short>;
defm ocl_int_v   : OCLVectorizableType<ocl_int>;
defm ocl_long_v  : OCLVectorizableType<ocl_long>;

// Floating point scalar types.
def ocl_float : OCLScalarType;

// Floating point vector types.
defm ocl_float_v : OCLVectorizableType<ocl_float>;

// Generic types.
defm ocl_gentype : OCLGenericType;

//===----------------------------------------------------------------------===//
/// OCLAttribute - An attribute that can be attached to GenBuiltin. Support a
///  restricted syntax with respect to OpenCL C attributes.
//===----------------------------------------------------------------------===//
class OCLAttribute <string name> {
  string Name = name;
}

// Declare attributes.
def overloadable : OCLAttribute<"overloadable">;
def pure         : OCLAttribute<"pure">;

//===----------------------------------------------------------------------===//
/// GenBuiltin - An OpenCL C built-in that can take generic arguments. The
///  ret_type argument is the built-in return type, while param_types is the
///  built-in parameters list. The base_impl argument contains the built-in
///  implementation for the scalar version; gentype_subs contains generic type
///  mappings. If generic_type_i is used in [ret_type] + param_types, the i-th
///  entry of gentype_subs contains a list of types to expand to.
//===----------------------------------------------------------------------===//
class GenBuiltin <OCLType ret_type,
                  list<OCLType> param_types = [],
                  code base_impl = [{}],
                  list<list<OCLType>> gentype_subs = []> {
  OCLType RetType = ret_type;
  list<OCLType> ParamTypes = param_types;

  code BaseImpl = base_impl;

  list<list<OCLType>> GentypeSubs = gentype_subs;

  list<OCLAttribute> Attributes = [overloadable];
}
