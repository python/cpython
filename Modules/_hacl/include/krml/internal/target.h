/* Copyright (c) INRIA and Microsoft Corporation. All rights reserved.
   Licensed under the Apache 2.0 and MIT Licenses. */

#ifndef __KRML_TARGET_H
#define __KRML_TARGET_H

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* Since KaRaMeL emits the inline keyword unconditionally, we follow the
 * guidelines at https://gcc.gnu.org/onlinedocs/gcc/Inline.html and make this
 * __inline__ to ensure the code compiles with -std=c90 and earlier. */
#ifdef __GNUC__
#  define inline __inline__
#endif

/* There is no support for aligned_alloc() in macOS before Catalina, so
 * let's make a macro to use _mm_malloc() and _mm_free() functions
 * from mm_malloc.h. */
#if defined(__APPLE__) && defined(__MACH__)
#  include <AvailabilityMacros.h>
#  if defined(MAC_OS_X_VERSION_MIN_REQUIRED) &&                                \
   (MAC_OS_X_VERSION_MIN_REQUIRED < 101500)
#    include <mm_malloc.h>
#    define LEGACY_MACOS
#  else
#    undef LEGACY_MACOS
#endif
#endif

/******************************************************************************/
/* Macros that KaRaMeL will generate.                                         */
/******************************************************************************/

/* For "bare" targets that do not have a C stdlib, the user might want to use
 * [-add-early-include '"mydefinitions.h"'] and override these. */
#ifndef KRML_HOST_PRINTF
#  define KRML_HOST_PRINTF printf
#endif

#if (                                                                          \
    (defined __STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) &&             \
    (!(defined KRML_HOST_EPRINTF)))
#  define KRML_HOST_EPRINTF(...) fprintf(stderr, __VA_ARGS__)
#elif !(defined KRML_HOST_EPRINTF) && defined(_MSC_VER)
#  define KRML_HOST_EPRINTF(...) fprintf(stderr, __VA_ARGS__)
#endif

#ifndef KRML_HOST_EXIT
#  define KRML_HOST_EXIT exit
#endif

#ifndef KRML_HOST_MALLOC
#  define KRML_HOST_MALLOC malloc
#endif

#ifndef KRML_HOST_CALLOC
#  define KRML_HOST_CALLOC calloc
#endif

#ifndef KRML_HOST_FREE
#  define KRML_HOST_FREE free
#endif

#ifndef KRML_HOST_IGNORE
#  define KRML_HOST_IGNORE(x) (void)(x)
#endif

#ifndef KRML_MAYBE_UNUSED_VAR
#  define KRML_MAYBE_UNUSED_VAR(x) KRML_HOST_IGNORE(x)
#endif

#ifndef KRML_MAYBE_UNUSED
#  if defined(__GNUC__) || defined(__clang__)
#    define KRML_MAYBE_UNUSED __attribute__((unused))
#  else
#    define KRML_MAYBE_UNUSED
#  endif
#endif

#ifndef KRML_ATTRIBUTE_TARGET
#  if defined(__GNUC__) || defined(__clang__)
#    define KRML_ATTRIBUTE_TARGET(x) __attribute__((target(x)))
#  else
#    define KRML_ATTRIBUTE_TARGET(x)
#  endif
#endif

#ifndef KRML_NOINLINE
#  if defined (__GNUC__) || defined (__clang__)
#    define KRML_NOINLINE __attribute__((noinline,unused))
#  elif defined(_MSC_VER)
#    define KRML_NOINLINE __declspec(noinline)
#  elif defined (__SUNPRO_C)
#    define KRML_NOINLINE __attribute__((noinline))
#  else
#    define KRML_NOINLINE
#    warning "The KRML_NOINLINE macro is not defined for this toolchain!"
#    warning "The compiler may defeat side-channel resistance with optimizations."
#    warning "Please locate target.h and try to fill it out with a suitable definition for this compiler."
#  endif
#endif

#ifndef KRML_MUSTINLINE
#  if defined(_MSC_VER)
#    define KRML_MUSTINLINE inline __forceinline
#  elif defined (__GNUC__)
#    define KRML_MUSTINLINE inline __attribute__((always_inline))
#  elif defined (__SUNPRO_C)
#    define KRML_MUSTINLINE inline __attribute__((always_inline))
#  else
#    define KRML_MUSTINLINE inline
#    warning "The KRML_MUSTINLINE macro defaults to plain inline for this toolchain!"
#    warning "Please locate target.h and try to fill it out with a suitable definition for this compiler."
#  endif
#endif

#ifndef KRML_PRE_ALIGN
#  ifdef _MSC_VER
#    define KRML_PRE_ALIGN(X) __declspec(align(X))
#  else
#    define KRML_PRE_ALIGN(X)
#  endif
#endif

#ifndef KRML_POST_ALIGN
#  ifdef _MSC_VER
#    define KRML_POST_ALIGN(X)
#  else
#    define KRML_POST_ALIGN(X) __attribute__((aligned(X)))
#  endif
#endif

/* MinGW-W64 does not support C11 aligned_alloc, but it supports
 * MSVC's _aligned_malloc.
 */
#ifndef KRML_ALIGNED_MALLOC
#  ifdef __MINGW32__
#    include <_mingw.h>
#  endif
#  if (                                                                        \
      defined(_MSC_VER) ||                                                     \
      (defined(__MINGW32__) && defined(__MINGW64_VERSION_MAJOR)))
#    define KRML_ALIGNED_MALLOC(X, Y) _aligned_malloc(Y, X)
#  elif defined(LEGACY_MACOS)
#    define KRML_ALIGNED_MALLOC(X, Y) _mm_malloc(Y, X)
#  else
#    define KRML_ALIGNED_MALLOC(X, Y) aligned_alloc(X, Y)
#  endif
#endif

/* Since aligned allocations with MinGW-W64 are done with
 * _aligned_malloc (see above), such pointers must be freed with
 * _aligned_free.
 */
#ifndef KRML_ALIGNED_FREE
#  ifdef __MINGW32__
#    include <_mingw.h>
#  endif
#  if (                                                                        \
      defined(_MSC_VER) ||                                                     \
      (defined(__MINGW32__) && defined(__MINGW64_VERSION_MAJOR)))
#    define KRML_ALIGNED_FREE(X) _aligned_free(X)
#  elif defined(LEGACY_MACOS)
#    define KRML_ALIGNED_FREE(X) _mm_free(X)
#  else
#    define KRML_ALIGNED_FREE(X) free(X)
#  endif
#endif

#ifndef KRML_HOST_TIME

#  include <time.h>

/* Prims_nat not yet in scope */
inline static int32_t krml_time(void) {
  return (int32_t)time(NULL);
}

#  define KRML_HOST_TIME krml_time
#endif

/* In statement position, exiting is easy. */
#define KRML_EXIT                                                              \
  do {                                                                         \
    KRML_HOST_PRINTF("Unimplemented function at %s:%d\n", __FILE__, __LINE__); \
    KRML_HOST_EXIT(254);                                                       \
  } while (0)

/* In expression position, use the comma-operator and a malloc to return an
 * expression of the right size. KaRaMeL passes t as the parameter to the macro.
 */
#define KRML_EABORT(t, msg)                                                    \
  (KRML_HOST_PRINTF("KaRaMeL abort at %s:%d\n%s\n", __FILE__, __LINE__, msg),  \
   KRML_HOST_EXIT(255), *((t *)KRML_HOST_MALLOC(sizeof(t))))

/* In FStar.Buffer.fst, the size of arrays is uint32_t, but it's a number of
 * *elements*. Do an ugly, run-time check (some of which KaRaMeL can eliminate).
 */
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 4))
#  define _KRML_CHECK_SIZE_PRAGMA                                              \
    _Pragma("GCC diagnostic ignored \"-Wtype-limits\"")
#else
#  define _KRML_CHECK_SIZE_PRAGMA
#endif

#define KRML_CHECK_SIZE(size_elt, sz)                                          \
  do {                                                                         \
    _KRML_CHECK_SIZE_PRAGMA                                                    \
    if (((size_t)(sz)) > ((size_t)(SIZE_MAX / (size_elt)))) {                  \
      KRML_HOST_PRINTF(                                                        \
          "Maximum allocatable size exceeded, aborting before overflow at "    \
          "%s:%d\n",                                                           \
          __FILE__, __LINE__);                                                 \
      KRML_HOST_EXIT(253);                                                     \
    }                                                                          \
  } while (0)

#if defined(_MSC_VER) && _MSC_VER < 1900
#  define KRML_HOST_SNPRINTF(buf, sz, fmt, arg)                                \
    _snprintf_s(buf, sz, _TRUNCATE, fmt, arg)
#else
#  define KRML_HOST_SNPRINTF(buf, sz, fmt, arg) snprintf(buf, sz, fmt, arg)
#endif

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 4))
#  define KRML_DEPRECATED(x) __attribute__((deprecated(x)))
#elif defined(__GNUC__)
/* deprecated attribute is not defined in GCC < 4.5. */
#  define KRML_DEPRECATED(x)
#elif defined(__SUNPRO_C)
#  define KRML_DEPRECATED(x) __attribute__((deprecated(x)))
#elif defined(_MSC_VER)
#  define KRML_DEPRECATED(x) __declspec(deprecated(x))
#endif

/* Macros for prettier unrolling of loops */
#define KRML_LOOP1(i, n, x) { \
  x \
  i += n; \
  (void) i; \
}

#define KRML_LOOP2(i, n, x)                                                    \
  KRML_LOOP1(i, n, x)                                                          \
  KRML_LOOP1(i, n, x)

#define KRML_LOOP3(i, n, x)                                                    \
  KRML_LOOP2(i, n, x)                                                          \
  KRML_LOOP1(i, n, x)

#define KRML_LOOP4(i, n, x)                                                    \
  KRML_LOOP2(i, n, x)                                                          \
  KRML_LOOP2(i, n, x)

#define KRML_LOOP5(i, n, x)                                                    \
  KRML_LOOP4(i, n, x)                                                          \
  KRML_LOOP1(i, n, x)

#define KRML_LOOP6(i, n, x)                                                    \
  KRML_LOOP4(i, n, x)                                                          \
  KRML_LOOP2(i, n, x)

#define KRML_LOOP7(i, n, x)                                                    \
  KRML_LOOP4(i, n, x)                                                          \
  KRML_LOOP3(i, n, x)

#define KRML_LOOP8(i, n, x)                                                    \
  KRML_LOOP4(i, n, x)                                                          \
  KRML_LOOP4(i, n, x)

#define KRML_LOOP9(i, n, x)                                                    \
  KRML_LOOP8(i, n, x)                                                          \
  KRML_LOOP1(i, n, x)

#define KRML_LOOP10(i, n, x)                                                   \
  KRML_LOOP8(i, n, x)                                                          \
  KRML_LOOP2(i, n, x)

#define KRML_LOOP11(i, n, x)                                                   \
  KRML_LOOP8(i, n, x)                                                          \
  KRML_LOOP3(i, n, x)

#define KRML_LOOP12(i, n, x)                                                   \
  KRML_LOOP8(i, n, x)                                                          \
  KRML_LOOP4(i, n, x)

#define KRML_LOOP13(i, n, x)                                                   \
  KRML_LOOP8(i, n, x)                                                          \
  KRML_LOOP5(i, n, x)

#define KRML_LOOP14(i, n, x)                                                   \
  KRML_LOOP8(i, n, x)                                                          \
  KRML_LOOP6(i, n, x)

#define KRML_LOOP15(i, n, x)                                                   \
  KRML_LOOP8(i, n, x)                                                          \
  KRML_LOOP7(i, n, x)

#define KRML_LOOP16(i, n, x)                                                   \
  KRML_LOOP8(i, n, x)                                                          \
  KRML_LOOP8(i, n, x)

#define KRML_UNROLL_FOR(i, z, n, k, x)                                         \
  do {                                                                         \
    uint32_t i = z;                                                            \
    KRML_LOOP##n(i, k, x)                                                      \
  } while (0)

#define KRML_ACTUAL_FOR(i, z, n, k, x)                                         \
  do {                                                                         \
    for (uint32_t i = z; i < n; i += k) {                                      \
      x                                                                        \
    }                                                                          \
  } while (0)

#ifndef KRML_UNROLL_MAX
#  define KRML_UNROLL_MAX 16
#endif

/* 1 is the number of loop iterations, i.e. (n - z)/k as evaluated by krml */
#if 0 <= KRML_UNROLL_MAX
#  define KRML_MAYBE_FOR0(i, z, n, k, x)
#else
#  define KRML_MAYBE_FOR0(i, z, n, k, x) KRML_ACTUAL_FOR(i, z, n, k, x)
#endif

#if 1 <= KRML_UNROLL_MAX
#  define KRML_MAYBE_FOR1(i, z, n, k, x) KRML_UNROLL_FOR(i, z, 1, k, x)
#else
#  define KRML_MAYBE_FOR1(i, z, n, k, x) KRML_ACTUAL_FOR(i, z, n, k, x)
#endif

#if 2 <= KRML_UNROLL_MAX
#  define KRML_MAYBE_FOR2(i, z, n, k, x) KRML_UNROLL_FOR(i, z, 2, k, x)
#else
#  define KRML_MAYBE_FOR2(i, z, n, k, x) KRML_ACTUAL_FOR(i, z, n, k, x)
#endif

#if 3 <= KRML_UNROLL_MAX
#  define KRML_MAYBE_FOR3(i, z, n, k, x) KRML_UNROLL_FOR(i, z, 3, k, x)
#else
#  define KRML_MAYBE_FOR3(i, z, n, k, x) KRML_ACTUAL_FOR(i, z, n, k, x)
#endif

#if 4 <= KRML_UNROLL_MAX
#  define KRML_MAYBE_FOR4(i, z, n, k, x) KRML_UNROLL_FOR(i, z, 4, k, x)
#else
#  define KRML_MAYBE_FOR4(i, z, n, k, x) KRML_ACTUAL_FOR(i, z, n, k, x)
#endif

#if 5 <= KRML_UNROLL_MAX
#  define KRML_MAYBE_FOR5(i, z, n, k, x) KRML_UNROLL_FOR(i, z, 5, k, x)
#else
#  define KRML_MAYBE_FOR5(i, z, n, k, x) KRML_ACTUAL_FOR(i, z, n, k, x)
#endif

#if 6 <= KRML_UNROLL_MAX
#  define KRML_MAYBE_FOR6(i, z, n, k, x) KRML_UNROLL_FOR(i, z, 6, k, x)
#else
#  define KRML_MAYBE_FOR6(i, z, n, k, x) KRML_ACTUAL_FOR(i, z, n, k, x)
#endif

#if 7 <= KRML_UNROLL_MAX
#  define KRML_MAYBE_FOR7(i, z, n, k, x) KRML_UNROLL_FOR(i, z, 7, k, x)
#else
#  define KRML_MAYBE_FOR7(i, z, n, k, x) KRML_ACTUAL_FOR(i, z, n, k, x)
#endif

#if 8 <= KRML_UNROLL_MAX
#  define KRML_MAYBE_FOR8(i, z, n, k, x) KRML_UNROLL_FOR(i, z, 8, k, x)
#else
#  define KRML_MAYBE_FOR8(i, z, n, k, x) KRML_ACTUAL_FOR(i, z, n, k, x)
#endif

#if 9 <= KRML_UNROLL_MAX
#  define KRML_MAYBE_FOR9(i, z, n, k, x) KRML_UNROLL_FOR(i, z, 9, k, x)
#else
#  define KRML_MAYBE_FOR9(i, z, n, k, x) KRML_ACTUAL_FOR(i, z, n, k, x)
#endif

#if 10 <= KRML_UNROLL_MAX
#  define KRML_MAYBE_FOR10(i, z, n, k, x) KRML_UNROLL_FOR(i, z, 10, k, x)
#else
#  define KRML_MAYBE_FOR10(i, z, n, k, x) KRML_ACTUAL_FOR(i, z, n, k, x)
#endif

#if 11 <= KRML_UNROLL_MAX
#  define KRML_MAYBE_FOR11(i, z, n, k, x) KRML_UNROLL_FOR(i, z, 11, k, x)
#else
#  define KRML_MAYBE_FOR11(i, z, n, k, x) KRML_ACTUAL_FOR(i, z, n, k, x)
#endif

#if 12 <= KRML_UNROLL_MAX
#  define KRML_MAYBE_FOR12(i, z, n, k, x) KRML_UNROLL_FOR(i, z, 12, k, x)
#else
#  define KRML_MAYBE_FOR12(i, z, n, k, x) KRML_ACTUAL_FOR(i, z, n, k, x)
#endif

#if 13 <= KRML_UNROLL_MAX
#  define KRML_MAYBE_FOR13(i, z, n, k, x) KRML_UNROLL_FOR(i, z, 13, k, x)
#else
#  define KRML_MAYBE_FOR13(i, z, n, k, x) KRML_ACTUAL_FOR(i, z, n, k, x)
#endif

#if 14 <= KRML_UNROLL_MAX
#  define KRML_MAYBE_FOR14(i, z, n, k, x) KRML_UNROLL_FOR(i, z, 14, k, x)
#else
#  define KRML_MAYBE_FOR14(i, z, n, k, x) KRML_ACTUAL_FOR(i, z, n, k, x)
#endif

#if 15 <= KRML_UNROLL_MAX
#  define KRML_MAYBE_FOR15(i, z, n, k, x) KRML_UNROLL_FOR(i, z, 15, k, x)
#else
#  define KRML_MAYBE_FOR15(i, z, n, k, x) KRML_ACTUAL_FOR(i, z, n, k, x)
#endif

#if 16 <= KRML_UNROLL_MAX
#  define KRML_MAYBE_FOR16(i, z, n, k, x) KRML_UNROLL_FOR(i, z, 16, k, x)
#else
#  define KRML_MAYBE_FOR16(i, z, n, k, x) KRML_ACTUAL_FOR(i, z, n, k, x)
#endif
#endif
