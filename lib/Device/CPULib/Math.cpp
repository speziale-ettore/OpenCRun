
#include <ocldef.h>

//
// acos implementation.
//

#define VEC_ACOS_N(T, N)            \
  __attribute__((overloadable))     \
  T ## N acos(T ## N x) {           \
    T ## N Res;                     \
                                    \
    for(unsigned I = 0; I < N; ++I) \
      Res[I] = acos(x[I]);          \
                                    \
    return Res;                     \
  }


#define ACOS_FLOAT_IMPL()       \
  __attribute__((overloadable)) \
  float acos(float x) {         \
    return __builtin_acosf(x);  \
  }                             \
                                \
  VEC_ACOS_N(float,  2)         \
  VEC_ACOS_N(float,  3)         \
  VEC_ACOS_N(float,  4)         \
  VEC_ACOS_N(float,  8)         \
  VEC_ACOS_N(float, 16)

ACOS_FLOAT_IMPL()

#undef VEC_ACOS_N
#undef ACOS_FLOAT_IMPL
