#pragma once

#include <inttypes.h>

typedef struct FStar_UInt128_uint128_s {
  uint64_t low;
  uint64_t high;
} FStar_UInt128_uint128, uint128_t;

#define KRML_VERIFIED_UINT128

#include "krml/lowstar_endianness.h"
#include "krml/fstar_uint128_struct_endianness.h"
#include "krml/FStar_UInt128_Verified.h"
