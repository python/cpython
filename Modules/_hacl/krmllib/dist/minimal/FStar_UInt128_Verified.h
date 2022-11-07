/*
  Copyright (c) INRIA and Microsoft Corporation. All rights reserved.
  Licensed under the Apache 2.0 License.
*/


#ifndef __FStar_UInt128_Verified_H
#define __FStar_UInt128_Verified_H

#include "FStar_UInt_8_16_32_64.h"
#include <inttypes.h>
#include <stdbool.h>
#include "krml/internal/types.h"
#include "krml/internal/target.h"

static inline uint64_t FStar_UInt128_constant_time_carry(uint64_t a, uint64_t b)
{
  return (a ^ ((a ^ b) | ((a - b) ^ b))) >> (uint32_t)63U;
}

static inline uint64_t FStar_UInt128_carry(uint64_t a, uint64_t b)
{
  return FStar_UInt128_constant_time_carry(a, b);
}

static inline FStar_UInt128_uint128
FStar_UInt128_add(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b)
{
  FStar_UInt128_uint128 lit;
  lit.low = a.low + b.low;
  lit.high = a.high + b.high + FStar_UInt128_carry(a.low + b.low, b.low);
  return lit;
}

static inline FStar_UInt128_uint128
FStar_UInt128_add_underspec(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b)
{
  FStar_UInt128_uint128 lit;
  lit.low = a.low + b.low;
  lit.high = a.high + b.high + FStar_UInt128_carry(a.low + b.low, b.low);
  return lit;
}

static inline FStar_UInt128_uint128
FStar_UInt128_add_mod(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b)
{
  FStar_UInt128_uint128 lit;
  lit.low = a.low + b.low;
  lit.high = a.high + b.high + FStar_UInt128_carry(a.low + b.low, b.low);
  return lit;
}

static inline FStar_UInt128_uint128
FStar_UInt128_sub(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b)
{
  FStar_UInt128_uint128 lit;
  lit.low = a.low - b.low;
  lit.high = a.high - b.high - FStar_UInt128_carry(a.low, a.low - b.low);
  return lit;
}

static inline FStar_UInt128_uint128
FStar_UInt128_sub_underspec(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b)
{
  FStar_UInt128_uint128 lit;
  lit.low = a.low - b.low;
  lit.high = a.high - b.high - FStar_UInt128_carry(a.low, a.low - b.low);
  return lit;
}

static inline FStar_UInt128_uint128
FStar_UInt128_sub_mod_impl(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b)
{
  FStar_UInt128_uint128 lit;
  lit.low = a.low - b.low;
  lit.high = a.high - b.high - FStar_UInt128_carry(a.low, a.low - b.low);
  return lit;
}

static inline FStar_UInt128_uint128
FStar_UInt128_sub_mod(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b)
{
  return FStar_UInt128_sub_mod_impl(a, b);
}

static inline FStar_UInt128_uint128
FStar_UInt128_logand(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b)
{
  FStar_UInt128_uint128 lit;
  lit.low = a.low & b.low;
  lit.high = a.high & b.high;
  return lit;
}

static inline FStar_UInt128_uint128
FStar_UInt128_logxor(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b)
{
  FStar_UInt128_uint128 lit;
  lit.low = a.low ^ b.low;
  lit.high = a.high ^ b.high;
  return lit;
}

static inline FStar_UInt128_uint128
FStar_UInt128_logor(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b)
{
  FStar_UInt128_uint128 lit;
  lit.low = a.low | b.low;
  lit.high = a.high | b.high;
  return lit;
}

static inline FStar_UInt128_uint128 FStar_UInt128_lognot(FStar_UInt128_uint128 a)
{
  FStar_UInt128_uint128 lit;
  lit.low = ~a.low;
  lit.high = ~a.high;
  return lit;
}

static uint32_t FStar_UInt128_u32_64 = (uint32_t)64U;

static inline uint64_t FStar_UInt128_add_u64_shift_left(uint64_t hi, uint64_t lo, uint32_t s)
{
  return (hi << s) + (lo >> (FStar_UInt128_u32_64 - s));
}

static inline uint64_t
FStar_UInt128_add_u64_shift_left_respec(uint64_t hi, uint64_t lo, uint32_t s)
{
  return FStar_UInt128_add_u64_shift_left(hi, lo, s);
}

static inline FStar_UInt128_uint128
FStar_UInt128_shift_left_small(FStar_UInt128_uint128 a, uint32_t s)
{
  if (s == (uint32_t)0U)
  {
    return a;
  }
  else
  {
    FStar_UInt128_uint128 lit;
    lit.low = a.low << s;
    lit.high = FStar_UInt128_add_u64_shift_left_respec(a.high, a.low, s);
    return lit;
  }
}

static inline FStar_UInt128_uint128
FStar_UInt128_shift_left_large(FStar_UInt128_uint128 a, uint32_t s)
{
  FStar_UInt128_uint128 lit;
  lit.low = (uint64_t)0U;
  lit.high = a.low << (s - FStar_UInt128_u32_64);
  return lit;
}

static inline FStar_UInt128_uint128
FStar_UInt128_shift_left(FStar_UInt128_uint128 a, uint32_t s)
{
  if (s < FStar_UInt128_u32_64)
  {
    return FStar_UInt128_shift_left_small(a, s);
  }
  else
  {
    return FStar_UInt128_shift_left_large(a, s);
  }
}

static inline uint64_t FStar_UInt128_add_u64_shift_right(uint64_t hi, uint64_t lo, uint32_t s)
{
  return (lo >> s) + (hi << (FStar_UInt128_u32_64 - s));
}

static inline uint64_t
FStar_UInt128_add_u64_shift_right_respec(uint64_t hi, uint64_t lo, uint32_t s)
{
  return FStar_UInt128_add_u64_shift_right(hi, lo, s);
}

static inline FStar_UInt128_uint128
FStar_UInt128_shift_right_small(FStar_UInt128_uint128 a, uint32_t s)
{
  if (s == (uint32_t)0U)
  {
    return a;
  }
  else
  {
    FStar_UInt128_uint128 lit;
    lit.low = FStar_UInt128_add_u64_shift_right_respec(a.high, a.low, s);
    lit.high = a.high >> s;
    return lit;
  }
}

static inline FStar_UInt128_uint128
FStar_UInt128_shift_right_large(FStar_UInt128_uint128 a, uint32_t s)
{
  FStar_UInt128_uint128 lit;
  lit.low = a.high >> (s - FStar_UInt128_u32_64);
  lit.high = (uint64_t)0U;
  return lit;
}

static inline FStar_UInt128_uint128
FStar_UInt128_shift_right(FStar_UInt128_uint128 a, uint32_t s)
{
  if (s < FStar_UInt128_u32_64)
  {
    return FStar_UInt128_shift_right_small(a, s);
  }
  else
  {
    return FStar_UInt128_shift_right_large(a, s);
  }
}

static inline bool FStar_UInt128_eq(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b)
{
  return a.low == b.low && a.high == b.high;
}

static inline bool FStar_UInt128_gt(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b)
{
  return a.high > b.high || (a.high == b.high && a.low > b.low);
}

static inline bool FStar_UInt128_lt(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b)
{
  return a.high < b.high || (a.high == b.high && a.low < b.low);
}

static inline bool FStar_UInt128_gte(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b)
{
  return a.high > b.high || (a.high == b.high && a.low >= b.low);
}

static inline bool FStar_UInt128_lte(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b)
{
  return a.high < b.high || (a.high == b.high && a.low <= b.low);
}

static inline FStar_UInt128_uint128
FStar_UInt128_eq_mask(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b)
{
  FStar_UInt128_uint128 lit;
  lit.low = FStar_UInt64_eq_mask(a.low, b.low) & FStar_UInt64_eq_mask(a.high, b.high);
  lit.high = FStar_UInt64_eq_mask(a.low, b.low) & FStar_UInt64_eq_mask(a.high, b.high);
  return lit;
}

static inline FStar_UInt128_uint128
FStar_UInt128_gte_mask(FStar_UInt128_uint128 a, FStar_UInt128_uint128 b)
{
  FStar_UInt128_uint128 lit;
  lit.low =
    (FStar_UInt64_gte_mask(a.high, b.high) & ~FStar_UInt64_eq_mask(a.high, b.high))
    | (FStar_UInt64_eq_mask(a.high, b.high) & FStar_UInt64_gte_mask(a.low, b.low));
  lit.high =
    (FStar_UInt64_gte_mask(a.high, b.high) & ~FStar_UInt64_eq_mask(a.high, b.high))
    | (FStar_UInt64_eq_mask(a.high, b.high) & FStar_UInt64_gte_mask(a.low, b.low));
  return lit;
}

static inline FStar_UInt128_uint128 FStar_UInt128_uint64_to_uint128(uint64_t a)
{
  FStar_UInt128_uint128 lit;
  lit.low = a;
  lit.high = (uint64_t)0U;
  return lit;
}

static inline uint64_t FStar_UInt128_uint128_to_uint64(FStar_UInt128_uint128 a)
{
  return a.low;
}

static inline uint64_t FStar_UInt128_u64_mod_32(uint64_t a)
{
  return a & (uint64_t)0xffffffffU;
}

static uint32_t FStar_UInt128_u32_32 = (uint32_t)32U;

static inline uint64_t FStar_UInt128_u32_combine(uint64_t hi, uint64_t lo)
{
  return lo + (hi << FStar_UInt128_u32_32);
}

static inline FStar_UInt128_uint128 FStar_UInt128_mul32(uint64_t x, uint32_t y)
{
  FStar_UInt128_uint128 lit;
  lit.low =
    FStar_UInt128_u32_combine((x >> FStar_UInt128_u32_32)
      * (uint64_t)y
      + (FStar_UInt128_u64_mod_32(x) * (uint64_t)y >> FStar_UInt128_u32_32),
      FStar_UInt128_u64_mod_32(FStar_UInt128_u64_mod_32(x) * (uint64_t)y));
  lit.high =
    ((x >> FStar_UInt128_u32_32)
    * (uint64_t)y
    + (FStar_UInt128_u64_mod_32(x) * (uint64_t)y >> FStar_UInt128_u32_32))
    >> FStar_UInt128_u32_32;
  return lit;
}

static inline uint64_t FStar_UInt128_u32_combine_(uint64_t hi, uint64_t lo)
{
  return lo + (hi << FStar_UInt128_u32_32);
}

static inline FStar_UInt128_uint128 FStar_UInt128_mul_wide(uint64_t x, uint64_t y)
{
  FStar_UInt128_uint128 lit;
  lit.low =
    FStar_UInt128_u32_combine_(FStar_UInt128_u64_mod_32(x)
      * (y >> FStar_UInt128_u32_32)
      +
        FStar_UInt128_u64_mod_32((x >> FStar_UInt128_u32_32)
          * FStar_UInt128_u64_mod_32(y)
          + (FStar_UInt128_u64_mod_32(x) * FStar_UInt128_u64_mod_32(y) >> FStar_UInt128_u32_32)),
      FStar_UInt128_u64_mod_32(FStar_UInt128_u64_mod_32(x) * FStar_UInt128_u64_mod_32(y)));
  lit.high =
    (x >> FStar_UInt128_u32_32)
    * (y >> FStar_UInt128_u32_32)
    +
      (((x >> FStar_UInt128_u32_32)
      * FStar_UInt128_u64_mod_32(y)
      + (FStar_UInt128_u64_mod_32(x) * FStar_UInt128_u64_mod_32(y) >> FStar_UInt128_u32_32))
      >> FStar_UInt128_u32_32)
    +
      ((FStar_UInt128_u64_mod_32(x)
      * (y >> FStar_UInt128_u32_32)
      +
        FStar_UInt128_u64_mod_32((x >> FStar_UInt128_u32_32)
          * FStar_UInt128_u64_mod_32(y)
          + (FStar_UInt128_u64_mod_32(x) * FStar_UInt128_u64_mod_32(y) >> FStar_UInt128_u32_32)))
      >> FStar_UInt128_u32_32);
  return lit;
}


/* Hand-written implementation of endianness-related uint128 functions
 * for the extracted uint128 implementation */

/* Access 64-bit fields within the int128. */
#define HIGH64_OF(x) ((x)->high)
#define LOW64_OF(x)  ((x)->low)

/* A series of definitions written using pointers. */

inline static void load128_le_(uint8_t *b, uint128_t *r) {
  LOW64_OF(r) = load64_le(b);
  HIGH64_OF(r) = load64_le(b + 8);
}

inline static void store128_le_(uint8_t *b, uint128_t *n) {
  store64_le(b, LOW64_OF(n));
  store64_le(b + 8, HIGH64_OF(n));
}

inline static void load128_be_(uint8_t *b, uint128_t *r) {
  HIGH64_OF(r) = load64_be(b);
  LOW64_OF(r) = load64_be(b + 8);
}

inline static void store128_be_(uint8_t *b, uint128_t *n) {
  store64_be(b, HIGH64_OF(n));
  store64_be(b + 8, LOW64_OF(n));
}

#ifndef KRML_NOSTRUCT_PASSING

inline static uint128_t load128_le(uint8_t *b) {
  uint128_t r;
  load128_le_(b, &r);
  return r;
}

inline static void store128_le(uint8_t *b, uint128_t n) {
  store128_le_(b, &n);
}

inline static uint128_t load128_be(uint8_t *b) {
  uint128_t r;
  load128_be_(b, &r);
  return r;
}

inline static void store128_be(uint8_t *b, uint128_t n) {
  store128_be_(b, &n);
}

#else /* !defined(KRML_STRUCT_PASSING) */

#  define print128 print128_
#  define load128_le load128_le_
#  define store128_le store128_le_
#  define load128_be load128_be_
#  define store128_be store128_be_

#endif /* KRML_STRUCT_PASSING */

#define __FStar_UInt128_Verified_H_DEFINED
#endif
