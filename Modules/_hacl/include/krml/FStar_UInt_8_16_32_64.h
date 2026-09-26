/*
  Copyright (c) INRIA and Microsoft Corporation. All rights reserved.
  Licensed under the Apache 2.0 and MIT Licenses.
*/


#ifndef FStar_UInt_8_16_32_64_H
#define FStar_UInt_8_16_32_64_H

#include <inttypes.h>
#include <stdbool.h>
#include "krml/internal/compat.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/types.h"
#include "krml/internal/target.h"

extern krml_checked_int_t FStar_UInt64_n;

extern bool FStar_UInt64_uu___is_Mk(uint64_t projectee);

extern krml_checked_int_t FStar_UInt64___proj__Mk__item__v(uint64_t projectee);

extern krml_checked_int_t FStar_UInt64_v(uint64_t x);

typedef void *FStar_UInt64_fits;

extern uint64_t FStar_UInt64_uint_to_t(krml_checked_int_t x);

extern uint64_t FStar_UInt64_zero;

extern uint64_t FStar_UInt64_one;

extern bool FStar_UInt64_ne(uint64_t a, uint64_t b);

extern uint64_t FStar_UInt64_minus(uint64_t a);

extern uint32_t FStar_UInt64_n_minus_one;

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

extern Prims_string FStar_UInt64_to_string(uint64_t uu___);

extern Prims_string FStar_UInt64_to_string_hex(uint64_t uu___);

extern Prims_string FStar_UInt64_to_string_hex_pad(uint64_t uu___);

extern uint64_t FStar_UInt64_of_string(Prims_string uu___);

extern krml_checked_int_t FStar_UInt32_n;

extern bool FStar_UInt32_uu___is_Mk(uint32_t projectee);

extern krml_checked_int_t FStar_UInt32___proj__Mk__item__v(uint32_t projectee);

extern krml_checked_int_t FStar_UInt32_v(uint32_t x);

typedef void *FStar_UInt32_fits;

extern uint32_t FStar_UInt32_uint_to_t(krml_checked_int_t x);

extern uint32_t FStar_UInt32_zero;

extern uint32_t FStar_UInt32_one;

extern bool FStar_UInt32_ne(uint32_t a, uint32_t b);

extern uint32_t FStar_UInt32_minus(uint32_t a);

extern uint32_t FStar_UInt32_n_minus_one;

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

extern Prims_string FStar_UInt32_to_string(uint32_t uu___);

extern Prims_string FStar_UInt32_to_string_hex(uint32_t uu___);

extern Prims_string FStar_UInt32_to_string_hex_pad(uint32_t uu___);

extern uint32_t FStar_UInt32_of_string(Prims_string uu___);

extern krml_checked_int_t FStar_UInt16_n;

extern bool FStar_UInt16_uu___is_Mk(uint16_t projectee);

extern krml_checked_int_t FStar_UInt16___proj__Mk__item__v(uint16_t projectee);

extern krml_checked_int_t FStar_UInt16_v(uint16_t x);

typedef void *FStar_UInt16_fits;

extern uint16_t FStar_UInt16_uint_to_t(krml_checked_int_t x);

extern uint16_t FStar_UInt16_zero;

extern uint16_t FStar_UInt16_one;

extern bool FStar_UInt16_ne(uint16_t a, uint16_t b);

extern uint16_t FStar_UInt16_minus(uint16_t a);

extern uint32_t FStar_UInt16_n_minus_one;

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

extern Prims_string FStar_UInt16_to_string(uint16_t uu___);

extern Prims_string FStar_UInt16_to_string_hex(uint16_t uu___);

extern Prims_string FStar_UInt16_to_string_hex_pad(uint16_t uu___);

extern uint16_t FStar_UInt16_of_string(Prims_string uu___);

extern krml_checked_int_t FStar_UInt8_n;

extern bool FStar_UInt8_uu___is_Mk(uint8_t projectee);

extern krml_checked_int_t FStar_UInt8___proj__Mk__item__v(uint8_t projectee);

extern krml_checked_int_t FStar_UInt8_v(uint8_t x);

typedef void *FStar_UInt8_fits;

extern uint8_t FStar_UInt8_uint_to_t(krml_checked_int_t x);

extern uint8_t FStar_UInt8_zero;

extern uint8_t FStar_UInt8_one;

extern bool FStar_UInt8_ne(uint8_t a, uint8_t b);

extern uint8_t FStar_UInt8_minus(uint8_t a);

extern uint32_t FStar_UInt8_n_minus_one;

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

extern Prims_string FStar_UInt8_to_string(uint8_t uu___);

extern Prims_string FStar_UInt8_to_string_hex(uint8_t uu___);

extern Prims_string FStar_UInt8_to_string_hex_pad(uint8_t uu___);

extern uint8_t FStar_UInt8_of_string(Prims_string uu___);

typedef uint8_t FStar_UInt8_byte;


#define FStar_UInt_8_16_32_64_H_DEFINED
#endif /* FStar_UInt_8_16_32_64_H */
