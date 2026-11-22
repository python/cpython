/* Compile the HACL* streaming HMAC implementation with explicit SIMD defaults. */
#include "internal/Hacl_Streaming_HMAC.h"

#ifndef HACL_CAN_COMPILE_VEC128
#  define HACL_CAN_COMPILE_VEC128 0
#endif
#ifndef HACL_CAN_COMPILE_VEC256
#  define HACL_CAN_COMPILE_VEC256 0
#endif

#include "Hacl_Streaming_HMAC.c"
