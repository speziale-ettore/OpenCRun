
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

//
// acosh implementation.
//

#define VEC_ACOSH_N(T, N)           \
  __attribute__((overloadable))     \
  T ## N acosh(T ## N x) {          \
    T ## N Res;                     \
                                    \
    for(unsigned I = 0; I < N; ++I) \
      Res[I] = acosh(x[I]);         \
                                    \
    return Res;                     \
  }

#define ACOSH_FLOAT_IMPL()      \
  __attribute__((overloadable)) \
  float acosh(float x) {        \
    return __builtin_acoshf(x); \
  }                             \
                                \
  VEC_ACOSH_N(float,  2)        \
  VEC_ACOSH_N(float,  3)        \
  VEC_ACOSH_N(float,  4)        \
  VEC_ACOSH_N(float,  8)        \
  VEC_ACOSH_N(float, 16)

ACOSH_FLOAT_IMPL()

#undef VEC_ACOSH_N
#undef ACOSH_FLOAT_IMPL

//
// acospi implementation.
//

#define VEC_ACOSPI_N(T, N)          \
  __attribute__((overloadable))     \
  T ## N acospi(T ## N x) {         \
    T ## N Res;                     \
                                    \
    for(unsigned I = 0; I < N; ++I) \
      Res[I] = acospi(x[I]);        \
                                    \
    return Res;                     \
  }

#define ACOSPI_FLOAT_IMPL()     \
  __attribute__((overloadable)) \
  float acospi(float x) {       \
    return acos(x) / M_PI;      \
  }                             \
                                \
  VEC_ACOSPI_N(float,  2)       \
  VEC_ACOSPI_N(float,  4)       \
  VEC_ACOSPI_N(float,  8)       \
  VEC_ACOSPI_N(float, 16)

ACOSPI_FLOAT_IMPL()

#undef VEC_ACOSPI_N
#undef ACOSPI_FLOAT_IMPL
