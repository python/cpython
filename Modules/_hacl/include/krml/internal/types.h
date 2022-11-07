/* Copyright (c) INRIA and Microsoft Corporation. All rights reserved.
   Licensed under the Apache 2.0 License. */

#ifndef KRML_TYPES_H
#define KRML_TYPES_H

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if (defined(_MSC_VER) && defined(_M_X64) && !defined(__clang__))
#define IS_MSVC64 1
#endif

/* This code makes a number of assumptions and should be refined. In particular,
 * it assumes that: any non-MSVC amd64 compiler supports int128. Maybe it would
 * be easier to just test for defined(__SIZEOF_INT128__) only? */
#if (defined(__x86_64__) || \
    defined(__x86_64) || \
    defined(__aarch64__) || \
    (defined(__powerpc64__) && defined(__LITTLE_ENDIAN__)) || \
    defined(__s390x__) || \
    (defined(_MSC_VER) && defined(_M_X64) && defined(__clang__)) || \
    (defined(__mips__) && defined(__LP64__)) || \
    (defined(__riscv) && __riscv_xlen == 64) || \
    defined(__SIZEOF_INT128__))
#define HAS_INT128 1
#endif

/* The uint128 type is a special case since we offer several implementations of
 * it, depending on the compiler and whether the user wants the verified
 * implementation or not. */
#if !defined(KRML_VERIFIED_UINT128) && defined(IS_MSVC64)
#  include <emmintrin.h>
typedef __m128i FStar_UInt128_uint128;
#elif !defined(KRML_VERIFIED_UINT128) && defined(HAS_INT128)
typedef unsigned __int128 FStar_UInt128_uint128;
#else
typedef struct FStar_UInt128_uint128_s {
  uint64_t low;
  uint64_t high;
} FStar_UInt128_uint128;
#endif

/* The former is defined once, here (otherwise, conflicts for test-c89. The
 * latter is for internal use. */
typedef FStar_UInt128_uint128 FStar_UInt128_t, uint128_t;

#include "krml/lowstar_endianness.h"

#endif

/* Avoid a circular loop: if this header is included via FStar_UInt8_16_32_64,
 * then don't bring the uint128 definitions into scope. */
#ifndef __FStar_UInt_8_16_32_64_H

#if !defined(KRML_VERIFIED_UINT128) && defined(IS_MSVC64)
#include "fstar_uint128_msvc.h"
#elif !defined(KRML_VERIFIED_UINT128) && defined(HAS_INT128)
#include "fstar_uint128_gcc64.h"
#else
#include "FStar_UInt128_Verified.h"
#endif

#endif
