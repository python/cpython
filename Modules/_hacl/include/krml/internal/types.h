/* Copyright (c) INRIA and Microsoft Corporation. All rights reserved.
   Licensed under the Apache 2.0 and MIT Licenses. */

#ifndef KRML_TYPES_H
#define KRML_TYPES_H
#define KRML_VERIFIED_UINT128

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Types which are either abstract, meaning that have to be implemented in C, or
 * which are models, meaning that they are swapped out at compile-time for
 * hand-written C types (in which case they're marked as noextract). */

typedef uint64_t FStar_UInt64_t, FStar_UInt64_t_;
typedef int64_t FStar_Int64_t, FStar_Int64_t_;
typedef uint32_t FStar_UInt32_t, FStar_UInt32_t_;
typedef int32_t FStar_Int32_t, FStar_Int32_t_;
typedef uint16_t FStar_UInt16_t, FStar_UInt16_t_;
typedef int16_t FStar_Int16_t, FStar_Int16_t_;
typedef uint8_t FStar_UInt8_t, FStar_UInt8_t_;
typedef int8_t FStar_Int8_t, FStar_Int8_t_;

/* Only useful when building krmllib, because it's in the dependency graph of
 * FStar.Int.Cast. */
typedef uint64_t FStar_UInt63_t, FStar_UInt63_t_;
typedef int64_t FStar_Int63_t, FStar_Int63_t_;

typedef double FStar_Float_float;
typedef uint32_t FStar_Char_char;
typedef FILE *FStar_IO_fd_read, *FStar_IO_fd_write;

typedef void *FStar_Dyn_dyn;

typedef const char *C_String_t, *C_String_t_, *C_Compat_String_t, *C_Compat_String_t_;

typedef int exit_code;
typedef FILE *channel;

typedef unsigned long long TestLib_cycles;

typedef uint64_t FStar_Date_dateTime, FStar_Date_timeSpan;

/* Now Prims.string is no longer illegal with the new model in LowStar.Printf;
 * it's operations that produce Prims_string which are illegal. Bring the
 * definition into scope by default. */
typedef const char *Prims_string;

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
#include "krml/FStar_UInt128_Verified.h"
#include "krml/fstar_uint128_struct_endianness.h"
#endif

#endif
