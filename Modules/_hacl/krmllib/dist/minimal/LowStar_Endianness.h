/*
  Copyright (c) INRIA and Microsoft Corporation. All rights reserved.
  Licensed under the Apache 2.0 License.
*/


#ifndef __LowStar_Endianness_H
#define __LowStar_Endianness_H



#include "FStar_UInt128.h"
#include <inttypes.h>
#include <stdbool.h>
#include "krml/lowstar_endianness.h"
#include "krml/internal/types.h"
#include "krml/internal/target.h"
static inline void store128_le(uint8_t *x0, FStar_UInt128_uint128 x1);

static inline FStar_UInt128_uint128 load128_le(uint8_t *x0);

static inline void store128_be(uint8_t *x0, FStar_UInt128_uint128 x1);

static inline FStar_UInt128_uint128 load128_be(uint8_t *x0);


#define __LowStar_Endianness_H_DEFINED
#endif
