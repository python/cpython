/* Copyright (c) INRIA and Microsoft Corporation. All rights reserved.
   Licensed under the Apache 2.0 License. */

/******************************************************************************/
/* Machine integers (128-bit arithmetic)                                      */
/******************************************************************************/

/* This header contains two things.
 *
 * First, an implementation of 128-bit arithmetic suitable for 64-bit GCC and
 * Clang, i.e. all the operations from FStar.UInt128.
 *
 * Second, 128-bit operations from C.Endianness (or LowStar.Endianness),
 * suitable for any compiler and platform (via a series of ifdefs). This second
 * part is unfortunate, and should be fixed by moving {load,store}128_{be,le} to
 * FStar.UInt128 to avoid a maze of preprocessor guards and hand-written code.
 * */

/* This file is used for both the minimal and generic krmllib distributions. As
 * such, it assumes that the machine integers have been bundled the exact same
 * way in both cases. */

#ifndef FSTAR_UINT128_GCC64
#define FSTAR_UINT128_GCC64

#include "FStar_UInt128.h"
#include "FStar_UInt_8_16_32_64.h"
#include "LowStar_Endianness.h"

/* GCC + using native unsigned __int128 support */

inline static uint128_t load128_le(uint8_t *b) {
  uint128_t l = (uint128_t)load64_le(b);
  uint128_t h = (uint128_t)load64_le(b + 8);
  return (h << 64 | l);
}

inline static void store128_le(uint8_t *b, uint128_t n) {
  store64_le(b, (uint64_t)n);
  store64_le(b + 8, (uint64_t)(n >> 64));
}

inline static uint128_t load128_be(uint8_t *b) {
  uint128_t h = (uint128_t)load64_be(b);
  uint128_t l = (uint128_t)load64_be(b + 8);
  return (h << 64 | l);
}

inline static void store128_be(uint8_t *b, uint128_t n) {
  store64_be(b, (uint64_t)(n >> 64));
  store64_be(b + 8, (uint64_t)n);
}

inline static uint128_t FStar_UInt128_add(uint128_t x, uint128_t y) {
  return x + y;
}

inline static uint128_t FStar_UInt128_mul(uint128_t x, uint128_t y) {
  return x * y;
}

inline static uint128_t FStar_UInt128_add_mod(uint128_t x, uint128_t y) {
  return x + y;
}

inline static uint128_t FStar_UInt128_sub(uint128_t x, uint128_t y) {
  return x - y;
}

inline static uint128_t FStar_UInt128_sub_mod(uint128_t x, uint128_t y) {
  return x - y;
}

inline static uint128_t FStar_UInt128_logand(uint128_t x, uint128_t y) {
  return x & y;
}

inline static uint128_t FStar_UInt128_logor(uint128_t x, uint128_t y) {
  return x | y;
}

inline static uint128_t FStar_UInt128_logxor(uint128_t x, uint128_t y) {
  return x ^ y;
}

inline static uint128_t FStar_UInt128_lognot(uint128_t x) {
  return ~x;
}

inline static uint128_t FStar_UInt128_shift_left(uint128_t x, uint32_t y) {
  return x << y;
}

inline static uint128_t FStar_UInt128_shift_right(uint128_t x, uint32_t y) {
  return x >> y;
}

inline static uint128_t FStar_UInt128_uint64_to_uint128(uint64_t x) {
  return (uint128_t)x;
}

inline static uint64_t FStar_UInt128_uint128_to_uint64(uint128_t x) {
  return (uint64_t)x;
}

inline static uint128_t FStar_UInt128_mul_wide(uint64_t x, uint64_t y) {
  return ((uint128_t) x) * y;
}

inline static uint128_t FStar_UInt128_eq_mask(uint128_t x, uint128_t y) {
  uint64_t mask =
      FStar_UInt64_eq_mask((uint64_t)(x >> 64), (uint64_t)(y >> 64)) &
      FStar_UInt64_eq_mask(x, y);
  return ((uint128_t)mask) << 64 | mask;
}

inline static uint128_t FStar_UInt128_gte_mask(uint128_t x, uint128_t y) {
  uint64_t mask =
      (FStar_UInt64_gte_mask(x >> 64, y >> 64) &
       ~(FStar_UInt64_eq_mask(x >> 64, y >> 64))) |
      (FStar_UInt64_eq_mask(x >> 64, y >> 64) & FStar_UInt64_gte_mask(x, y));
  return ((uint128_t)mask) << 64 | mask;
}

inline static uint64_t FStar_UInt128___proj__Mkuint128__item__low(uint128_t x) {
  return (uint64_t) x;
}

inline static uint64_t FStar_UInt128___proj__Mkuint128__item__high(uint128_t x) {
  return (uint64_t) (x >> 64);
}

inline static uint128_t FStar_UInt128_add_underspec(uint128_t x, uint128_t y) {
  return x + y;
}

inline static uint128_t FStar_UInt128_sub_underspec(uint128_t x, uint128_t y) {
  return x - y;
}

inline static bool FStar_UInt128_eq(uint128_t x, uint128_t y) {
  return x == y;
}

inline static bool FStar_UInt128_gt(uint128_t x, uint128_t y) {
  return x > y;
}

inline static bool FStar_UInt128_lt(uint128_t x, uint128_t y) {
  return x < y;
}

inline static bool FStar_UInt128_gte(uint128_t x, uint128_t y) {
  return x >= y;
}

inline static bool FStar_UInt128_lte(uint128_t x, uint128_t y) {
  return x <= y;
}

inline static uint128_t FStar_UInt128_mul32(uint64_t x, uint32_t y) {
  return (uint128_t) x * (uint128_t) y;
}

#endif
