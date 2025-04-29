/* Copyright (c) INRIA and Microsoft Corporation. All rights reserved.
   Licensed under the Apache 2.0 and MIT Licenses. */

#ifndef FSTAR_UINT128_STRUCT_ENDIANNESS_H
#define FSTAR_UINT128_STRUCT_ENDIANNESS_H

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

#endif
