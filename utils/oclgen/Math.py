
from create_gen_lib import *

with gentype_func("gentype acos(gentype x)") as f:
  f.body  = "return __builtin_acos$cty(x);"
  f.types = ["float"] + vectors("float")

with gentype_func("gentype acosh(gentype x)") as f:
  f.body = "return __builtin_acosh$cty(x);"
  f.types = ["float"] + vectors("float")

with gentype_func("gentype acospi(gentype x)") as f:
  f.body = "return acos(x) / M_PI;"
  f.types = ["float"] + vectors("float")
