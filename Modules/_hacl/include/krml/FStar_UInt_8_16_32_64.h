/*
  Copyright (c) INRIA and Microsoft Corporation. All rights reserved.
  Licensed under the Apache 2.0 License.
*/


#ifndef __FStar_UInt_8_16_32_64_H
#define __FStar_UInt_8_16_32_64_H

#include <inttypes.h>
#include <stdbool.h>

#include "krml/lowstar_endianness.h"
#include "krml/types.h"
#include "krml/internal/target.h"

static KRML_NOINLINE uint64_t FStar_UInt64_eq_mask(uint64_t a, uint64_t b)
{
  uint64_t x = a ^ b;
  uint64_t minus_x = ~x + 1ULL;
  uint64_t x_or_minus_x = x | minus_x;
  uint64_t xnx = x_or_minus_x >> 63U;
  return xnx - 1ULL;
}

static KRML_NOINLINE uint64_t FStar_UInt64_gte_mask(uint64_t a, uint64_t b)
{
  uint64_t x = a;
  uint64_t y = b;
  uint64_t x_xor_y = x ^ y;
  uint64_t x_sub_y = x - y;
  uint64_t x_sub_y_xor_y = x_sub_y ^ y;
  uint64_t q = x_xor_y | x_sub_y_xor_y;
  uint64_t x_xor_q = x ^ q;
  uint64_t x_xor_q_ = x_xor_q >> 63U;
  return x_xor_q_ - 1ULL;
}

static KRML_NOINLINE uint32_t FStar_UInt32_eq_mask(uint32_t a, uint32_t b)
{
  uint32_t x = a ^ b;
  uint32_t minus_x = ~x + 1U;
  uint32_t x_or_minus_x = x | minus_x;
  uint32_t xnx = x_or_minus_x >> 31U;
  return xnx - 1U;
}

static KRML_NOINLINE uint32_t FStar_UInt32_gte_mask(uint32_t a, uint32_t b)
{
  uint32_t x = a;
  uint32_t y = b;
  uint32_t x_xor_y = x ^ y;
  uint32_t x_sub_y = x - y;
  uint32_t x_sub_y_xor_y = x_sub_y ^ y;
  uint32_t q = x_xor_y | x_sub_y_xor_y;
  uint32_t x_xor_q = x ^ q;
  uint32_t x_xor_q_ = x_xor_q >> 31U;
  return x_xor_q_ - 1U;
}

static KRML_NOINLINE uint16_t FStar_UInt16_eq_mask(uint16_t a, uint16_t b)
{
  uint16_t x = (uint32_t)a ^ (uint32_t)b;
  uint16_t minus_x = (uint32_t)~x + 1U;
  uint16_t x_or_minus_x = (uint32_t)x | (uint32_t)minus_x;
  uint16_t xnx = (uint32_t)x_or_minus_x >> 15U;
  return (uint32_t)xnx - 1U;
}

static KRML_NOINLINE uint16_t FStar_UInt16_gte_mask(uint16_t a, uint16_t b)
{
  uint16_t x = a;
  uint16_t y = b;
  uint16_t x_xor_y = (uint32_t)x ^ (uint32_t)y;
  uint16_t x_sub_y = (uint32_t)x - (uint32_t)y;
  uint16_t x_sub_y_xor_y = (uint32_t)x_sub_y ^ (uint32_t)y;
  uint16_t q = (uint32_t)x_xor_y | (uint32_t)x_sub_y_xor_y;
  uint16_t x_xor_q = (uint32_t)x ^ (uint32_t)q;
  uint16_t x_xor_q_ = (uint32_t)x_xor_q >> 15U;
  return (uint32_t)x_xor_q_ - 1U;
}

static KRML_NOINLINE uint8_t FStar_UInt8_eq_mask(uint8_t a, uint8_t b)
{
  uint8_t x = (uint32_t)a ^ (uint32_t)b;
  uint8_t minus_x = (uint32_t)~x + 1U;
  uint8_t x_or_minus_x = (uint32_t)x | (uint32_t)minus_x;
  uint8_t xnx = (uint32_t)x_or_minus_x >> 7U;
  return (uint32_t)xnx - 1U;
}

static KRML_NOINLINE uint8_t FStar_UInt8_gte_mask(uint8_t a, uint8_t b)
{
  uint8_t x = a;
  uint8_t y = b;
  uint8_t x_xor_y = (uint32_t)x ^ (uint32_t)y;
  uint8_t x_sub_y = (uint32_t)x - (uint32_t)y;
  uint8_t x_sub_y_xor_y = (uint32_t)x_sub_y ^ (uint32_t)y;
  uint8_t q = (uint32_t)x_xor_y | (uint32_t)x_sub_y_xor_y;
  uint8_t x_xor_q = (uint32_t)x ^ (uint32_t)q;
  uint8_t x_xor_q_ = (uint32_t)x_xor_q >> 7U;
  return (uint32_t)x_xor_q_ - 1U;
}


#define __FStar_UInt_8_16_32_64_H_DEFINED
#endif
