
include "opencrun/Device/GenLib/OCLTypes.td"

class MathBuiltin <OCLType ret_type,
                   list<OCLType> param_types = [],
                   code base_impl> :
  GenBuiltin<ret_type,
             param_types,
             base_impl,
             [[ ocl_float
              , ocl_float_v_2
              , ocl_float_v_4
              , ocl_float_v_8
              , ocl_float_v_16
              ]
             ]> {
  list<OCLAttribute> Attributes = [overloadable,pure];
}

def blt_acos   : MathBuiltin<ocl_gentype_1,
                             [ocl_gentype_1],
                             [{return __builtin_acos(x0);}]>;
def blt_acosh  : MathBuiltin<ocl_gentype_1,
                             [ocl_gentype_1],
                             [{return __builtin_acosh(x0);}]>;
def blt_acospi : MathBuiltin<ocl_gentype_1,
                             [ocl_gentype_1],
                             [{return acos(x0) / M_PI; }]>;
// TODO: fill the gap.
def blt_cos    : MathBuiltin<ocl_gentype_1,
                             [ocl_gentype_1],
                             [{return __builtin_cosf(x0);}]>;
// TODO: file the gap.
def blt_exp    : MathBuiltin<ocl_gentype_1,
                             [ocl_gentype_1],
                             [{return __builtin_expf(x0);}]>;
