/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

#ifndef TOMMATH_PRIV_H_
#define TOMMATH_PRIV_H_

#include <tommath.h>
#include "tommath_class.h"

/*
 * Private symbols
 * ---------------
 *
 * On Unix symbols can be marked as hidden if libtommath is compiled
 * as a shared object. By default, symbols are visible.
 * As of now, this feature is opt-in via the MP_PRIVATE_SYMBOLS define.
 *
 * On Win32 a .def file must be used to specify the exported symbols.
 */
#if defined (MP_PRIVATE_SYMBOLS) && defined(__GNUC__) && __GNUC__ >= 4
#   define MP_PRIVATE __attribute__ ((visibility ("hidden")))
#else
#   define MP_PRIVATE
#endif

/* Hardening libtommath
 * --------------------
 *
 * By default memory is zeroed before calling
 * MP_FREE to avoid leaking data. This is good
 * practice in cryptographical applications.
 *
 * Note however that memory allocators used
 * in cryptographical applications can often
 * be configured by itself to clear memory,
 * rendering the clearing in tommath unnecessary.
 * See for example https://github.com/GrapheneOS/hardened_malloc
 * and the option CONFIG_ZERO_ON_FREE.
 *
 * Furthermore there are applications which
 * value performance more and want this
 * feature to be disabled. For such applications
 * define MP_NO_ZERO_ON_FREE during compilation.
 */
#ifdef MP_NO_ZERO_ON_FREE
#  define MP_FREE_BUFFER(mem, size)   MP_FREE((mem), (size))
#  define MP_FREE_DIGITS(mem, digits) MP_FREE((mem), sizeof (mp_digit) * (size_t)(digits))
#else
#  define MP_FREE_BUFFER(mem, size)                     \
do {                                                    \
   size_t fs_ = (size);                                 \
   void* fm_ = (mem);                                   \
   if (fm_ != NULL) {                                   \
      MP_ZERO_BUFFER(fm_, fs_);                         \
      MP_FREE(fm_, fs_);                                \
   }                                                    \
} while (0)
#  define MP_FREE_DIGITS(mem, digits)                   \
do {                                                    \
   int fd_ = (digits);                                  \
   void* fm_ = (mem);                                   \
   if (fm_ != NULL) {                                   \
      size_t fs_ = sizeof (mp_digit) * (size_t)fd_;     \
      MP_ZERO_BUFFER(fm_, fs_);                         \
      MP_FREE(fm_, fs_);                                \
   }                                                    \
} while (0)
#endif

#ifdef MP_USE_MEMSET
#  include <string.h>
#  define MP_ZERO_BUFFER(mem, size)   memset((mem), 0, (size))
#  define MP_ZERO_DIGITS(mem, digits)                   \
do {                                                    \
   int zd_ = (digits);                                  \
   if (zd_ > 0) {                                       \
      memset((mem), 0, sizeof(mp_digit) * (size_t)zd_); \
   }                                                    \
} while (0)
#else
#  define MP_ZERO_BUFFER(mem, size)                     \
do {                                                    \
   size_t zs_ = (size);                                 \
   char* zm_ = (char*)(mem);                            \
   while (zs_-- > 0u) {                                 \
      *zm_++ = '\0';                                    \
   }                                                    \
} while (0)
#  define MP_ZERO_DIGITS(mem, digits)                   \
do {                                                    \
   int zd_ = (digits);                                  \
   mp_digit* zm_ = (mem);                               \
   while (zd_-- > 0) {                                  \
      *zm_++ = 0;                                       \
   }                                                    \
} while (0)
#endif

/* Tunable cutoffs
 * ---------------
 *
 *  - In the default settings, a cutoff X can be modified at runtime
 *    by adjusting the corresponding X_CUTOFF variable.
 *
 *  - Tunability of the library can be disabled at compile time
 *    by defining the MP_FIXED_CUTOFFS macro.
 *
 *  - There is an additional file tommath_cutoffs.h, which defines
 *    the default cutoffs. These can be adjusted manually or by the
 *    autotuner.
 *
 */

#ifdef MP_FIXED_CUTOFFS
#  include "tommath_cutoffs.h"
#  define MP_KARATSUBA_MUL_CUTOFF MP_DEFAULT_KARATSUBA_MUL_CUTOFF
#  define MP_KARATSUBA_SQR_CUTOFF MP_DEFAULT_KARATSUBA_SQR_CUTOFF
#  define MP_TOOM_MUL_CUTOFF      MP_DEFAULT_TOOM_MUL_CUTOFF
#  define MP_TOOM_SQR_CUTOFF      MP_DEFAULT_TOOM_SQR_CUTOFF
#else
#  define MP_KARATSUBA_MUL_CUTOFF KARATSUBA_MUL_CUTOFF
#  define MP_KARATSUBA_SQR_CUTOFF KARATSUBA_SQR_CUTOFF
#  define MP_TOOM_MUL_CUTOFF      TOOM_MUL_CUTOFF
#  define MP_TOOM_SQR_CUTOFF      TOOM_SQR_CUTOFF
#endif

/* define heap macros */
#ifndef MP_MALLOC
/* default to libc stuff */
#   include <stdlib.h>
#   define MP_MALLOC(size)                   malloc(size)
#   define MP_REALLOC(mem, oldsize, newsize) realloc((mem), (newsize))
#   define MP_CALLOC(nmemb, size)            calloc((nmemb), (size))
#   define MP_FREE(mem, size)                free(mem)
#elif 0
/* prototypes for our heap functions */
extern void *MP_MALLOC(size_t size);
extern void *MP_REALLOC(void *mem, size_t oldsize, size_t newsize);
extern void *MP_CALLOC(size_t nmemb, size_t size);
extern void MP_FREE(void *mem, size_t size);
#endif

/* feature detection macro */
#ifdef _MSC_VER
/* Prevent false positive: not enough arguments for function-like macro invocation */
#pragma warning(disable: 4003)
#endif
#define MP_STRINGIZE(x)  MP__STRINGIZE(x)
#define MP__STRINGIZE(x) ""#x""
#define MP_HAS(x)        (sizeof(MP_STRINGIZE(BN_##x##_C)) == 1u)

/* TODO: Remove private_mp_word as soon as deprecated mp_word is removed from tommath. */
#undef mp_word
typedef private_mp_word mp_word;

#define MP_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MP_MAX(x, y) (((x) > (y)) ? (x) : (y))

/* Static assertion */
#define MP_STATIC_ASSERT(msg, cond) typedef char mp_static_assert_##msg[(cond) ? 1 : -1];

/* ---> Basic Manipulations <--- */
#define MP_IS_ZERO(a) ((a)->used == 0)
#define MP_IS_EVEN(a) (((a)->used == 0) || (((a)->dp[0] & 1u) == 0u))
#define MP_IS_ODD(a)  (((a)->used > 0) && (((a)->dp[0] & 1u) == 1u))

#define MP_SIZEOF_BITS(type)    ((size_t)CHAR_BIT * sizeof(type))
#define MP_MAXFAST              (int)(1uL << (MP_SIZEOF_BITS(mp_word) - (2u * (size_t)MP_DIGIT_BIT)))

/* TODO: Remove PRIVATE_MP_WARRAY as soon as deprecated MP_WARRAY is removed from tommath.h */
#undef MP_WARRAY
#define MP_WARRAY PRIVATE_MP_WARRAY

/* TODO: Remove PRIVATE_MP_PREC as soon as deprecated MP_PREC is removed from tommath.h */
#ifdef PRIVATE_MP_PREC
#   undef MP_PREC
#   define MP_PREC PRIVATE_MP_PREC
#endif

/* Minimum number of available digits in mp_int, MP_PREC >= MP_MIN_PREC */
#define MP_MIN_PREC ((((int)MP_SIZEOF_BITS(Tcl_WideInt) + MP_DIGIT_BIT) - 1) / MP_DIGIT_BIT)

MP_STATIC_ASSERT(prec_geq_min_prec, MP_PREC >= MP_MIN_PREC)

/* random number source */
extern MP_PRIVATE mp_err(*s_mp_rand_source)(void *out, size_t size);

/* lowlevel functions, do not call! */
MP_PRIVATE mp_bool s_mp_get_bit(const mp_int *a, unsigned int b);
MP_PRIVATE mp_err s_mp_add(const mp_int *a, const mp_int *b, mp_int *c) MP_WUR;
MP_PRIVATE mp_err s_mp_sub(const mp_int *a, const mp_int *b, mp_int *c) MP_WUR;
MP_PRIVATE mp_err s_mp_mul_digs_fast(const mp_int *a, const mp_int *b, mp_int *c, int digs) MP_WUR;
MP_PRIVATE mp_err s_mp_mul_digs(const mp_int *a, const mp_int *b, mp_int *c, int digs) MP_WUR;
MP_PRIVATE mp_err s_mp_mul_high_digs_fast(const mp_int *a, const mp_int *b, mp_int *c, int digs) MP_WUR;
MP_PRIVATE mp_err s_mp_mul_high_digs(const mp_int *a, const mp_int *b, mp_int *c, int digs) MP_WUR;
MP_PRIVATE mp_err s_mp_sqr_fast(const mp_int *a, mp_int *b) MP_WUR;
MP_PRIVATE mp_err s_mp_sqr(const mp_int *a, mp_int *b) MP_WUR;
MP_PRIVATE mp_err s_mp_balance_mul(const mp_int *a, const mp_int *b, mp_int *c) MP_WUR;
MP_PRIVATE mp_err s_mp_karatsuba_mul(const mp_int *a, const mp_int *b, mp_int *c) MP_WUR;
MP_PRIVATE mp_err s_mp_toom_mul(const mp_int *a, const mp_int *b, mp_int *c) MP_WUR;
MP_PRIVATE mp_err s_mp_karatsuba_sqr(const mp_int *a, mp_int *b) MP_WUR;
MP_PRIVATE mp_err s_mp_toom_sqr(const mp_int *a, mp_int *b) MP_WUR;
MP_PRIVATE mp_err s_mp_invmod_fast(const mp_int *a, const mp_int *b, mp_int *c) MP_WUR;
MP_PRIVATE mp_err s_mp_invmod_slow(const mp_int *a, const mp_int *b, mp_int *c) MP_WUR;
MP_PRIVATE mp_err s_mp_montgomery_reduce_fast(mp_int *x, const mp_int *n, mp_digit rho) MP_WUR;
MP_PRIVATE mp_err s_mp_exptmod_fast(const mp_int *G, const mp_int *X, const mp_int *P, mp_int *Y, int redmode) MP_WUR;
MP_PRIVATE mp_err s_mp_exptmod(const mp_int *G, const mp_int *X, const mp_int *P, mp_int *Y, int redmode) MP_WUR;
MP_PRIVATE mp_err s_mp_rand_platform(void *p, size_t n) MP_WUR;
MP_PRIVATE mp_err s_mp_prime_random_ex(mp_int *a, int t, int size, int flags, private_mp_prime_callback cb, void *dat);
MP_PRIVATE void s_mp_reverse(unsigned char *s, size_t len);
MP_PRIVATE mp_err s_mp_prime_is_divisible(const mp_int *a, mp_bool *result);

/* TODO: jenkins prng is not thread safe as of now */
MP_PRIVATE mp_err s_mp_rand_jenkins(void *p, size_t n) MP_WUR;
#ifndef MP_NO_STDINT
MP_PRIVATE void s_mp_rand_jenkins_init(uint64_t seed);
#endif

extern MP_PRIVATE const char *const mp_s_rmap;
extern MP_PRIVATE const unsigned char mp_s_rmap_reverse[];
extern MP_PRIVATE const size_t mp_s_rmap_reverse_sz;
extern MP_PRIVATE const mp_digit *s_mp_prime_tab;

/* deprecated functions */
#if 0
MP_DEPRECATED(s_mp_invmod_fast) mp_err fast_mp_invmod(const mp_int *a, const mp_int *b, mp_int *c);
MP_DEPRECATED(s_mp_montgomery_reduce_fast) mp_err fast_mp_montgomery_reduce(mp_int *x, const mp_int *n,
      mp_digit rho);
MP_DEPRECATED(s_mp_mul_digs_fast) mp_err fast_s_mp_mul_digs(const mp_int *a, const mp_int *b, mp_int *c,
      int digs);
MP_DEPRECATED(s_mp_mul_high_digs_fast) mp_err fast_s_mp_mul_high_digs(const mp_int *a, const mp_int *b,
      mp_int *c,
      int digs);
MP_DEPRECATED(s_mp_sqr_fast) mp_err fast_s_mp_sqr(const mp_int *a, mp_int *b);
MP_DEPRECATED(s_mp_balance_mul) mp_err mp_balance_mul(const mp_int *a, const mp_int *b, mp_int *c);
MP_DEPRECATED(s_mp_exptmod_fast) mp_err mp_exptmod_fast(const mp_int *G, const mp_int *X, const mp_int *P,
      mp_int *Y,
      int redmode);
MP_DEPRECATED(s_mp_invmod_slow) mp_err mp_invmod_slow(const mp_int *a, const mp_int *b, mp_int *c);
MP_DEPRECATED(s_mp_karatsuba_mul) mp_err mp_karatsuba_mul(const mp_int *a, const mp_int *b, mp_int *c);
MP_DEPRECATED(s_mp_karatsuba_sqr) mp_err mp_karatsuba_sqr(const mp_int *a, mp_int *b);
MP_DEPRECATED(s_mp_toom_mul) mp_err mp_toom_mul(const mp_int *a, const mp_int *b, mp_int *c);
MP_DEPRECATED(s_mp_toom_sqr) mp_err mp_toom_sqr(const mp_int *a, mp_int *b);
MP_DEPRECATED(s_mp_reverse) void bn_reverse(unsigned char *s, int len);
#endif

#define MP_GET_ENDIANNESS(x) \
   do{\
      int16_t n = 0x1;                                          \
      char *p = (char *)&n;                                     \
      x = (p[0] == '\x01') ? MP_LITTLE_ENDIAN : MP_BIG_ENDIAN;  \
   } while (0)

/* code-generating macros */
#define MP_SET_UNSIGNED(name, type)                                                    \
    void name(mp_int * a, type b)                                                      \
    {                                                                                  \
        int i = 0;                                                                     \
        while (b != 0u) {                                                              \
            a->dp[i++] = ((mp_digit)b & MP_MASK);                                      \
            if (MP_SIZEOF_BITS(type) <= MP_DIGIT_BIT) { break; }                       \
            b >>= ((MP_SIZEOF_BITS(type) <= MP_DIGIT_BIT) ? 0 : MP_DIGIT_BIT);         \
        }                                                                              \
        a->used = i;                                                                   \
        a->sign = MP_ZPOS;                                                             \
        MP_ZERO_DIGITS(a->dp + a->used, a->alloc - a->used);                           \
    }

#define MP_SET_SIGNED(name, uname, type, utype)          \
    void name(mp_int * a, type b)                        \
    {                                                    \
        uname(a, (b < 0) ? -(utype)b : (utype)b);        \
        if (b < 0) { a->sign = MP_NEG; }                 \
    }

#define MP_INIT_INT(name , set, type)                    \
    mp_err name(mp_int * a, type b)                      \
    {                                                    \
        mp_err err;                                      \
        if ((err = mp_init(a)) != MP_OKAY) {             \
            return err;                                  \
        }                                                \
        set(a, b);                                       \
        return MP_OKAY;                                  \
    }

#define MP_GET_MAG(name, type)                                                         \
    type name(const mp_int* a)                                                         \
    {                                                                                  \
        unsigned i = MP_MIN((unsigned)a->used, (unsigned)((MP_SIZEOF_BITS(type) + MP_DIGIT_BIT - 1) / MP_DIGIT_BIT)); \
        type res = 0u;                                                                 \
        while (i --> 0u) {                                                             \
            res <<= ((MP_SIZEOF_BITS(type) <= MP_DIGIT_BIT) ? 0 : MP_DIGIT_BIT);       \
            res |= (type)a->dp[i];                                                     \
            if (MP_SIZEOF_BITS(type) <= MP_DIGIT_BIT) { break; }                       \
        }                                                                              \
        return res;                                                                    \
    }

#define MP_GET_SIGNED(name, mag, type, utype)                 \
    type name(const mp_int* a)                                \
    {                                                         \
        utype res = mag(a);                                   \
        return (a->sign == MP_NEG) ? (type)-res : (type)res;  \
    }

#undef mp_isodd
#define mp_isodd TclBN_mp_isodd

#endif
