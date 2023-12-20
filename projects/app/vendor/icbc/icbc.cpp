#define ICBC_IMPLEMENTATION

#ifdef AM_IMAGE_USE_AVX2
#define ICBC_SIMD 4
#else
#define ICBC_SIMD 1
#endif

#include "icbc.h"
