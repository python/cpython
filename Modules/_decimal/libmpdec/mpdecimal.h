/*
 * Copyright (c) 2008-2016 Stefan Krah. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#ifndef MPDECIMAL_H
#define MPDECIMAL_H


#ifdef __cplusplus
extern "C" {
  #ifndef __STDC_LIMIT_MACROS
    #define __STDC_LIMIT_MACROS
    #define MPD_CLEAR_STDC_LIMIT_MACROS
  #endif
#endif


#ifndef _MSC_VER
  #include "pyconfig.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>

#ifdef _MSC_VER
  #include "vccompat.h"
  #ifndef UNUSED
    #define UNUSED
  #endif
  #define MPD_PRAGMA(x)
  #define MPD_HIDE_SYMBOLS_START
  #define MPD_HIDE_SYMBOLS_END
  #define EXTINLINE extern inline
#else
  #ifndef __GNUC_STDC_INLINE__
    #define __GNUC_STDC_INLINE__ 1
  #endif
  #if defined(__GNUC__) && !defined(__INTEL_COMPILER)
    #define UNUSED __attribute__((unused))
  #else
    #define UNUSED
  #endif
  #if (defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)) && \
      defined(__GNUC__) && __GNUC__ >= 4 && !defined(__INTEL_COMPILER)
    #define MPD_PRAGMA(x) _Pragma(x)
    #define MPD_HIDE_SYMBOLS_START "GCC visibility push(hidden)"
    #define MPD_HIDE_SYMBOLS_END "GCC visibility pop"
  #else
    #define MPD_PRAGMA(x)
    #define MPD_HIDE_SYMBOLS_START
    #define MPD_HIDE_SYMBOLS_END
  #endif
  #define EXTINLINE
#endif


/* This header file is internal for the purpose of building _decimal.so.
 * All symbols should have local scope in the DSO. */
MPD_PRAGMA(MPD_HIDE_SYMBOLS_START)


#if !defined(LEGACY_COMPILER)
  #if !defined(UINT64_MAX)
    /* The following #error is just a warning. If the compiler indeed does
     * not have uint64_t, it is perfectly safe to comment out the #error. */
    #error "Warning: Compiler without uint64_t. Comment out this line."
    #define LEGACY_COMPILER
  #endif
#endif


/******************************************************************************/
/*                                  Version                                   */
/******************************************************************************/

#define MPD_MAJOR_VERSION 2
#define MPD_MINOR_VERSION 4
#define MPD_MICRO_VERSION 2

#define MPD_VERSION "2.4.2"

#define MPD_VERSION_HEX ((MPD_MAJOR_VERSION << 24) | \
                         (MPD_MINOR_VERSION << 16) | \
                         (MPD_MICRO_VERSION <<  8))

const char *mpd_version(void);


/******************************************************************************/
/*                              Configuration                                 */
/******************************************************************************/

#if defined(UNIVERSAL)
  #if defined(CONFIG_64) || defined(CONFIG_32)
    #error "cannot use CONFIG_64 or CONFIG_32 with UNIVERSAL."
  #endif
  #if defined(__ppc__)
    #define CONFIG_32
    #define ANSI
  #elif defined(__ppc64__)
    #define CONFIG_64
    #define ANSI
  #elif defined(__i386__)
    #define CONFIG_32
    #define ANSI
  #elif defined(__x86_64__)
    #define CONFIG_64
    #define ASM
  #else
    #error "unknown architecture for universal build."
  #endif
#endif


/* BEGIN CONFIG_64 */
#if defined(CONFIG_64)
/* types for modular and base arithmetic */
#define MPD_UINT_MAX UINT64_MAX
#define MPD_BITS_PER_UINT 64
typedef uint64_t mpd_uint_t;  /* unsigned mod type */

#define MPD_SIZE_MAX SIZE_MAX
typedef size_t mpd_size_t; /* unsigned size type */

/* type for exp, digits, len, prec */
#define MPD_SSIZE_MAX INT64_MAX
#define MPD_SSIZE_MIN INT64_MIN
typedef int64_t mpd_ssize_t;
#define _mpd_strtossize strtoll

/* decimal arithmetic */
#define MPD_RADIX 10000000000000000000ULL  /* 10**19 */
#define MPD_RDIGITS 19
#define MPD_MAX_POW10 19
#define MPD_EXPDIGITS 19  /* MPD_EXPDIGITS <= MPD_RDIGITS+1 */

#define MPD_MAXTRANSFORM_2N 4294967296ULL      /* 2**32 */
#define MPD_MAX_PREC 999999999999999999LL
#define MPD_MAX_PREC_LOG2 64
#define MPD_ELIMIT  1000000000000000000LL
#define MPD_MAX_EMAX   999999999999999999LL    /* ELIMIT-1 */
#define MPD_MIN_EMIN  (-999999999999999999LL)  /* -EMAX */
#define MPD_MIN_ETINY (MPD_MIN_EMIN-(MPD_MAX_PREC-1))
#define MPD_EXP_INF 2000000000000000001LL
#define MPD_EXP_CLAMP (-4000000000000000001LL)
#define MPD_MAXIMPORT 105263157894736842L /* ceil((2*MPD_MAX_PREC)/MPD_RDIGITS) */

/* conversion specifiers */
#define PRI_mpd_uint_t PRIu64
#define PRI_mpd_ssize_t PRIi64
/* END CONFIG_64 */


/* BEGIN CONFIG_32 */
#elif defined(CONFIG_32)
/* types for modular and base arithmetic */
#define MPD_UINT_MAX UINT32_MAX
#define MPD_BITS_PER_UINT 32
typedef uint32_t mpd_uint_t;  /* unsigned mod type */

#ifndef LEGACY_COMPILER
#define MPD_UUINT_MAX UINT64_MAX
typedef uint64_t mpd_uuint_t; /* double width unsigned mod type */
#endif

#define MPD_SIZE_MAX SIZE_MAX
typedef size_t mpd_size_t; /* unsigned size type */

/* type for dec->len, dec->exp, ctx->prec */
#define MPD_SSIZE_MAX INT32_MAX
#define MPD_SSIZE_MIN INT32_MIN
typedef int32_t mpd_ssize_t;
#define _mpd_strtossize strtol

/* decimal arithmetic */
#define MPD_RADIX 1000000000UL  /* 10**9 */
#define MPD_RDIGITS 9
#define MPD_MAX_POW10 9
#define MPD_EXPDIGITS 10 /* MPD_EXPDIGITS <= MPD_RDIGITS+1 */

#define MPD_MAXTRANSFORM_2N 33554432UL /* 2**25 */
#define MPD_MAX_PREC 425000000L
#define MPD_MAX_PREC_LOG2 32
#define MPD_ELIMIT 425000001L
#define MPD_MAX_EMAX 425000000L        /* ELIMIT-1 */
#define MPD_MIN_EMIN (-425000000L)     /* -EMAX */
#define MPD_MIN_ETINY (MPD_MIN_EMIN-(MPD_MAX_PREC-1))
#define MPD_EXP_INF 1000000001L      /* allows for emax=999999999 in the tests */
#define MPD_EXP_CLAMP (-2000000001L) /* allows for emin=-999999999 in the tests */
#define MPD_MAXIMPORT 94444445L      /* ceil((2*MPD_MAX_PREC)/MPD_RDIGITS) */

/* conversion specifiers */
#define PRI_mpd_uint_t PRIu32
#define PRI_mpd_ssize_t PRIi32
/* END CONFIG_32 */

#else
  #error "define CONFIG_64 or CONFIG_32"
#endif
/* END CONFIG */


#if MPD_SIZE_MAX != MPD_UINT_MAX
  #error "unsupported platform: need mpd_size_t == mpd_uint_t"
#endif


/******************************************************************************/
/*                                Context                                     */
/******************************************************************************/

enum {
    MPD_ROUND_UP,          /* round away from 0               */
    MPD_ROUND_DOWN,        /* round toward 0 (truncate)       */
    MPD_ROUND_CEILING,     /* round toward +infinity          */
    MPD_ROUND_FLOOR,       /* round toward -infinity          */
    MPD_ROUND_HALF_UP,     /* 0.5 is rounded up               */
    MPD_ROUND_HALF_DOWN,   /* 0.5 is rounded down             */
    MPD_ROUND_HALF_EVEN,   /* 0.5 is rounded to even          */
    MPD_ROUND_05UP,        /* round zero or five away from 0  */
    MPD_ROUND_TRUNC,       /* truncate, but set infinity      */
    MPD_ROUND_GUARD
};

enum { MPD_CLAMP_DEFAULT, MPD_CLAMP_IEEE_754, MPD_CLAMP_GUARD };

extern const char *mpd_round_string[MPD_ROUND_GUARD];
extern const char *mpd_clamp_string[MPD_CLAMP_GUARD];


typedef struct mpd_context_t {
    mpd_ssize_t prec;   /* precision */
    mpd_ssize_t emax;   /* max positive exp */
    mpd_ssize_t emin;   /* min negative exp */
    uint32_t traps;     /* status events that should be trapped */
    uint32_t status;    /* status flags */
    uint32_t newtrap;   /* set by mpd_addstatus_raise() */
    int      round;     /* rounding mode */
    int      clamp;     /* clamp mode */
    int      allcr;     /* all functions correctly rounded */
} mpd_context_t;


/* Status flags */
#define MPD_Clamped             0x00000001U
#define MPD_Conversion_syntax   0x00000002U
#define MPD_Division_by_zero    0x00000004U
#define MPD_Division_impossible 0x00000008U
#define MPD_Division_undefined  0x00000010U
#define MPD_Fpu_error           0x00000020U
#define MPD_Inexact             0x00000040U
#define MPD_Invalid_context     0x00000080U
#define MPD_Invalid_operation   0x00000100U
#define MPD_Malloc_error        0x00000200U
#define MPD_Not_implemented     0x00000400U
#define MPD_Overflow            0x00000800U
#define MPD_Rounded             0x00001000U
#define MPD_Subnormal           0x00002000U
#define MPD_Underflow           0x00004000U
#define MPD_Max_status         (0x00008000U-1U)

/* Conditions that result in an IEEE 754 exception */
#define MPD_IEEE_Invalid_operation (MPD_Conversion_syntax |   \
                                    MPD_Division_impossible | \
                                    MPD_Division_undefined |  \
                                    MPD_Fpu_error |           \
                                    MPD_Invalid_context |     \
                                    MPD_Invalid_operation |   \
                                    MPD_Malloc_error)         \

/* Errors that require the result of an operation to be set to NaN */
#define MPD_Errors (MPD_IEEE_Invalid_operation | \
                    MPD_Division_by_zero)

/* Default traps */
#define MPD_Traps (MPD_IEEE_Invalid_operation | \
                   MPD_Division_by_zero |       \
                   MPD_Overflow |               \
                   MPD_Underflow)

/* Official name */
#define MPD_Insufficient_storage MPD_Malloc_error

/* IEEE 754 interchange format contexts */
#define MPD_IEEE_CONTEXT_MAX_BITS 512 /* 16*(log2(MPD_MAX_EMAX / 3)-3) */
#define MPD_DECIMAL32 32
#define MPD_DECIMAL64 64
#define MPD_DECIMAL128 128


#define MPD_MINALLOC_MIN 2
#define MPD_MINALLOC_MAX 64
extern mpd_ssize_t MPD_MINALLOC;
extern void (* mpd_traphandler)(mpd_context_t *);
void mpd_dflt_traphandler(mpd_context_t *);

void mpd_setminalloc(mpd_ssize_t n);
void mpd_init(mpd_context_t *ctx, mpd_ssize_t prec);

void mpd_maxcontext(mpd_context_t *ctx);
void mpd_defaultcontext(mpd_context_t *ctx);
void mpd_basiccontext(mpd_context_t *ctx);
int mpd_ieee_context(mpd_context_t *ctx, int bits);

mpd_ssize_t mpd_getprec(const mpd_context_t *ctx);
mpd_ssize_t mpd_getemax(const mpd_context_t *ctx);
mpd_ssize_t mpd_getemin(const mpd_context_t *ctx);
int mpd_getround(const mpd_context_t *ctx);
uint32_t mpd_gettraps(const mpd_context_t *ctx);
uint32_t mpd_getstatus(const mpd_context_t *ctx);
int mpd_getclamp(const mpd_context_t *ctx);
int mpd_getcr(const mpd_context_t *ctx);

int mpd_qsetprec(mpd_context_t *ctx, mpd_ssize_t prec);
int mpd_qsetemax(mpd_context_t *ctx, mpd_ssize_t emax);
int mpd_qsetemin(mpd_context_t *ctx, mpd_ssize_t emin);
int mpd_qsetround(mpd_context_t *ctx, int newround);
int mpd_qsettraps(mpd_context_t *ctx, uint32_t flags);
int mpd_qsetstatus(mpd_context_t *ctx, uint32_t flags);
int mpd_qsetclamp(mpd_context_t *ctx, int c);
int mpd_qsetcr(mpd_context_t *ctx, int c);
void mpd_addstatus_raise(mpd_context_t *ctx, uint32_t flags);


/******************************************************************************/
/*                           Decimal Arithmetic                               */
/******************************************************************************/

/* mpd_t flags */
#define MPD_POS                 ((uint8_t)0)
#define MPD_NEG                 ((uint8_t)1)
#define MPD_INF                 ((uint8_t)2)
#define MPD_NAN                 ((uint8_t)4)
#define MPD_SNAN                ((uint8_t)8)
#define MPD_SPECIAL (MPD_INF|MPD_NAN|MPD_SNAN)
#define MPD_STATIC              ((uint8_t)16)
#define MPD_STATIC_DATA         ((uint8_t)32)
#define MPD_SHARED_DATA         ((uint8_t)64)
#define MPD_CONST_DATA          ((uint8_t)128)
#define MPD_DATAFLAGS (MPD_STATIC_DATA|MPD_SHARED_DATA|MPD_CONST_DATA)

/* mpd_t */
typedef struct mpd_t {
    uint8_t flags;
    mpd_ssize_t exp;
    mpd_ssize_t digits;
    mpd_ssize_t len;
    mpd_ssize_t alloc;
    mpd_uint_t *data;
} mpd_t;


typedef unsigned char uchar;


/******************************************************************************/
/*                       Quiet, thread-safe functions                         */
/******************************************************************************/

/* format specification */
typedef struct mpd_spec_t {
    mpd_ssize_t min_width; /* minimum field width */
    mpd_ssize_t prec;      /* fraction digits or significant digits */
    char type;             /* conversion specifier */
    char align;            /* alignment */
    char sign;             /* sign printing/alignment */
    char fill[5];          /* fill character */
    const char *dot;       /* decimal point */
    const char *sep;       /* thousands separator */
    const char *grouping;  /* grouping of digits */
} mpd_spec_t;

/* output to a string */
char *mpd_to_sci(const mpd_t *dec, int fmt);
char *mpd_to_eng(const mpd_t *dec, int fmt);
mpd_ssize_t mpd_to_sci_size(char **res, const mpd_t *dec, int fmt);
mpd_ssize_t mpd_to_eng_size(char **res, const mpd_t *dec, int fmt);
int mpd_validate_lconv(mpd_spec_t *spec);
int mpd_parse_fmt_str(mpd_spec_t *spec, const char *fmt, int caps);
char *mpd_qformat_spec(const mpd_t *dec, const mpd_spec_t *spec, const mpd_context_t *ctx, uint32_t *status);
char *mpd_qformat(const mpd_t *dec, const char *fmt, const mpd_context_t *ctx, uint32_t *status);

#define MPD_NUM_FLAGS 15
#define MPD_MAX_FLAG_STRING 208
#define MPD_MAX_FLAG_LIST (MPD_MAX_FLAG_STRING+18)
#define MPD_MAX_SIGNAL_LIST 121
int mpd_snprint_flags(char *dest, int nmemb, uint32_t flags);
int mpd_lsnprint_flags(char *dest, int nmemb, uint32_t flags, const char *flag_string[]);
int mpd_lsnprint_signals(char *dest, int nmemb, uint32_t flags, const char *signal_string[]);

/* output to a file */
void mpd_fprint(FILE *file, const mpd_t *dec);
void mpd_print(const mpd_t *dec);

/* assignment from a string */
void mpd_qset_string(mpd_t *dec, const char *s, const mpd_context_t *ctx, uint32_t *status);

/* set to NaN with error flags */
void mpd_seterror(mpd_t *result, uint32_t flags, uint32_t *status);
/* set a special with sign and type */
void mpd_setspecial(mpd_t *dec, uint8_t sign, uint8_t type);
/* set coefficient to zero or all nines */
void mpd_zerocoeff(mpd_t *result);
void mpd_qmaxcoeff(mpd_t *result, const mpd_context_t *ctx, uint32_t *status);

/* quietly assign a C integer type to an mpd_t */
void mpd_qset_ssize(mpd_t *result, mpd_ssize_t a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qset_i32(mpd_t *result, int32_t a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qset_uint(mpd_t *result, mpd_uint_t a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qset_u32(mpd_t *result, uint32_t a, const mpd_context_t *ctx, uint32_t *status);
#ifndef LEGACY_COMPILER
void mpd_qset_i64(mpd_t *result, int64_t a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qset_u64(mpd_t *result, uint64_t a, const mpd_context_t *ctx, uint32_t *status);
#endif

/* quietly assign a C integer type to an mpd_t with a static coefficient */
void mpd_qsset_ssize(mpd_t *result, mpd_ssize_t a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qsset_i32(mpd_t *result, int32_t a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qsset_uint(mpd_t *result, mpd_uint_t a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qsset_u32(mpd_t *result, uint32_t a, const mpd_context_t *ctx, uint32_t *status);

/* quietly get a C integer type from an mpd_t */
mpd_ssize_t mpd_qget_ssize(const mpd_t *dec, uint32_t *status);
mpd_uint_t mpd_qget_uint(const mpd_t *dec, uint32_t *status);
mpd_uint_t mpd_qabs_uint(const mpd_t *dec, uint32_t *status);

int32_t mpd_qget_i32(const mpd_t *dec, uint32_t *status);
uint32_t mpd_qget_u32(const mpd_t *dec, uint32_t *status);
#ifndef LEGACY_COMPILER
int64_t mpd_qget_i64(const mpd_t *dec, uint32_t *status);
uint64_t mpd_qget_u64(const mpd_t *dec, uint32_t *status);
#endif

/* quiet functions */
int mpd_qcheck_nan(mpd_t *nanresult, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);
int mpd_qcheck_nans(mpd_t *nanresult, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qfinalize(mpd_t *result, const mpd_context_t *ctx, uint32_t *status);

const char *mpd_class(const mpd_t *a, const mpd_context_t *ctx);

int mpd_qcopy(mpd_t *result, const mpd_t *a,  uint32_t *status);
mpd_t *mpd_qncopy(const mpd_t *a);
int mpd_qcopy_abs(mpd_t *result, const mpd_t *a, uint32_t *status);
int mpd_qcopy_negate(mpd_t *result, const mpd_t *a, uint32_t *status);
int mpd_qcopy_sign(mpd_t *result, const mpd_t *a, const mpd_t *b, uint32_t *status);

void mpd_qand(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qinvert(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qlogb(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qor(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qscaleb(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qxor(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
int mpd_same_quantum(const mpd_t *a, const mpd_t *b);

void mpd_qrotate(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
int mpd_qshiftl(mpd_t *result, const mpd_t *a, mpd_ssize_t n, uint32_t *status);
mpd_uint_t mpd_qshiftr(mpd_t *result, const mpd_t *a, mpd_ssize_t n, uint32_t *status);
mpd_uint_t mpd_qshiftr_inplace(mpd_t *result, mpd_ssize_t n);
void mpd_qshift(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qshiftn(mpd_t *result, const mpd_t *a, mpd_ssize_t n, const mpd_context_t *ctx, uint32_t *status);

int mpd_qcmp(const mpd_t *a, const mpd_t *b, uint32_t *status);
int mpd_qcompare(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
int mpd_qcompare_signal(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
int mpd_cmp_total(const mpd_t *a, const mpd_t *b);
int mpd_cmp_total_mag(const mpd_t *a, const mpd_t *b);
int mpd_compare_total(mpd_t *result, const mpd_t *a, const mpd_t *b);
int mpd_compare_total_mag(mpd_t *result, const mpd_t *a, const mpd_t *b);

void mpd_qround_to_intx(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qround_to_int(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qtrunc(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qfloor(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qceil(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);

void mpd_qabs(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qmax(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qmax_mag(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qmin(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qmin_mag(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qminus(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qplus(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qnext_minus(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qnext_plus(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qnext_toward(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qquantize(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qrescale(mpd_t *result, const mpd_t *a, mpd_ssize_t exp, const mpd_context_t *ctx, uint32_t *status);
void mpd_qrescale_fmt(mpd_t *result, const mpd_t *a, mpd_ssize_t exp, const mpd_context_t *ctx, uint32_t *status);
void mpd_qreduce(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qadd(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qadd_ssize(mpd_t *result, const mpd_t *a, mpd_ssize_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qadd_i32(mpd_t *result, const mpd_t *a, int32_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qadd_uint(mpd_t *result, const mpd_t *a, mpd_uint_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qadd_u32(mpd_t *result, const mpd_t *a, uint32_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qsub(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qsub_ssize(mpd_t *result, const mpd_t *a, mpd_ssize_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qsub_i32(mpd_t *result, const mpd_t *a, int32_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qsub_uint(mpd_t *result, const mpd_t *a, mpd_uint_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qsub_u32(mpd_t *result, const mpd_t *a, uint32_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qmul(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qmul_ssize(mpd_t *result, const mpd_t *a, mpd_ssize_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qmul_i32(mpd_t *result, const mpd_t *a, int32_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qmul_uint(mpd_t *result, const mpd_t *a, mpd_uint_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qmul_u32(mpd_t *result, const mpd_t *a, uint32_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qfma(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_t *c, const mpd_context_t *ctx, uint32_t *status);
void mpd_qdiv(mpd_t *q, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qdiv_ssize(mpd_t *result, const mpd_t *a, mpd_ssize_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qdiv_i32(mpd_t *result, const mpd_t *a, int32_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qdiv_uint(mpd_t *result, const mpd_t *a, mpd_uint_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qdiv_u32(mpd_t *result, const mpd_t *a, uint32_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qdivint(mpd_t *q, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qrem(mpd_t *r, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qrem_near(mpd_t *r, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qdivmod(mpd_t *q, mpd_t *r, const mpd_t *a, const mpd_t *b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qpow(mpd_t *result, const mpd_t *base, const mpd_t *exp, const mpd_context_t *ctx, uint32_t *status);
void mpd_qpowmod(mpd_t *result, const mpd_t *base, const mpd_t *exp, const mpd_t *mod, const mpd_context_t *ctx, uint32_t *status);
void mpd_qexp(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qln10(mpd_t *result, mpd_ssize_t prec, uint32_t *status);
void mpd_qln(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qlog10(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qsqrt(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qinvroot(mpd_t *result, const mpd_t *a, const mpd_context_t *ctx, uint32_t *status);

#ifndef LEGACY_COMPILER
void mpd_qadd_i64(mpd_t *result, const mpd_t *a, int64_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qadd_u64(mpd_t *result, const mpd_t *a, uint64_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qsub_i64(mpd_t *result, const mpd_t *a, int64_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qsub_u64(mpd_t *result, const mpd_t *a, uint64_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qmul_i64(mpd_t *result, const mpd_t *a, int64_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qmul_u64(mpd_t *result, const mpd_t *a, uint64_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qdiv_i64(mpd_t *result, const mpd_t *a, int64_t b, const mpd_context_t *ctx, uint32_t *status);
void mpd_qdiv_u64(mpd_t *result, const mpd_t *a, uint64_t b, const mpd_context_t *ctx, uint32_t *status);
#endif


size_t mpd_sizeinbase(const mpd_t *a, uint32_t base);
void mpd_qimport_u16(mpd_t *result, const uint16_t *srcdata, size_t srclen,
                     uint8_t srcsign, uint32_t srcbase,
                     const mpd_context_t *ctx, uint32_t *status);
void mpd_qimport_u32(mpd_t *result, const uint32_t *srcdata, size_t srclen,
                     uint8_t srcsign, uint32_t srcbase,
                     const mpd_context_t *ctx, uint32_t *status);
size_t mpd_qexport_u16(uint16_t **rdata, size_t rlen, uint32_t base,
                       const mpd_t *src, uint32_t *status);
size_t mpd_qexport_u32(uint32_t **rdata, size_t rlen, uint32_t base,
                       const mpd_t *src, uint32_t *status);


/******************************************************************************/
/*                           Signalling functions                             */
/******************************************************************************/

char *mpd_format(const mpd_t *dec, const char *fmt, mpd_context_t *ctx);
void mpd_import_u16(mpd_t *result, const uint16_t *srcdata, size_t srclen, uint8_t srcsign, uint32_t base, mpd_context_t *ctx);
void mpd_import_u32(mpd_t *result, const uint32_t *srcdata, size_t srclen, uint8_t srcsign, uint32_t base, mpd_context_t *ctx);
size_t mpd_export_u16(uint16_t **rdata, size_t rlen, uint32_t base, const mpd_t *src, mpd_context_t *ctx);
size_t mpd_export_u32(uint32_t **rdata, size_t rlen, uint32_t base, const mpd_t *src, mpd_context_t *ctx);
void mpd_finalize(mpd_t *result, mpd_context_t *ctx);
int mpd_check_nan(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
int mpd_check_nans(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_set_string(mpd_t *result, const char *s, mpd_context_t *ctx);
void mpd_maxcoeff(mpd_t *result, mpd_context_t *ctx);
void mpd_sset_ssize(mpd_t *result, mpd_ssize_t a, mpd_context_t *ctx);
void mpd_sset_i32(mpd_t *result, int32_t a, mpd_context_t *ctx);
void mpd_sset_uint(mpd_t *result, mpd_uint_t a, mpd_context_t *ctx);
void mpd_sset_u32(mpd_t *result, uint32_t a, mpd_context_t *ctx);
void mpd_set_ssize(mpd_t *result, mpd_ssize_t a, mpd_context_t *ctx);
void mpd_set_i32(mpd_t *result, int32_t a, mpd_context_t *ctx);
void mpd_set_uint(mpd_t *result, mpd_uint_t a, mpd_context_t *ctx);
void mpd_set_u32(mpd_t *result, uint32_t a, mpd_context_t *ctx);
#ifndef LEGACY_COMPILER
void mpd_set_i64(mpd_t *result, int64_t a, mpd_context_t *ctx);
void mpd_set_u64(mpd_t *result, uint64_t a, mpd_context_t *ctx);
#endif
mpd_ssize_t mpd_get_ssize(const mpd_t *a, mpd_context_t *ctx);
mpd_uint_t mpd_get_uint(const mpd_t *a, mpd_context_t *ctx);
mpd_uint_t mpd_abs_uint(const mpd_t *a, mpd_context_t *ctx);
int32_t mpd_get_i32(const mpd_t *a, mpd_context_t *ctx);
uint32_t mpd_get_u32(const mpd_t *a, mpd_context_t *ctx);
#ifndef LEGACY_COMPILER
int64_t mpd_get_i64(const mpd_t *a, mpd_context_t *ctx);
uint64_t mpd_get_u64(const mpd_t *a, mpd_context_t *ctx);
#endif
void mpd_and(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_copy(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_canonical(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_copy_abs(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_copy_negate(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_copy_sign(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_invert(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_logb(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_or(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_rotate(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_scaleb(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_shiftl(mpd_t *result, const mpd_t *a, mpd_ssize_t n, mpd_context_t *ctx);
mpd_uint_t mpd_shiftr(mpd_t *result, const mpd_t *a, mpd_ssize_t n, mpd_context_t *ctx);
void mpd_shiftn(mpd_t *result, const mpd_t *a, mpd_ssize_t n, mpd_context_t *ctx);
void mpd_shift(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_xor(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_abs(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
int mpd_cmp(const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
int mpd_compare(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
int mpd_compare_signal(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_add(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_add_ssize(mpd_t *result, const mpd_t *a, mpd_ssize_t b, mpd_context_t *ctx);
void mpd_add_i32(mpd_t *result, const mpd_t *a, int32_t b, mpd_context_t *ctx);
void mpd_add_uint(mpd_t *result, const mpd_t *a, mpd_uint_t b, mpd_context_t *ctx);
void mpd_add_u32(mpd_t *result, const mpd_t *a, uint32_t b, mpd_context_t *ctx);
void mpd_sub(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_sub_ssize(mpd_t *result, const mpd_t *a, mpd_ssize_t b, mpd_context_t *ctx);
void mpd_sub_i32(mpd_t *result, const mpd_t *a, int32_t b, mpd_context_t *ctx);
void mpd_sub_uint(mpd_t *result, const mpd_t *a, mpd_uint_t b, mpd_context_t *ctx);
void mpd_sub_u32(mpd_t *result, const mpd_t *a, uint32_t b, mpd_context_t *ctx);
void mpd_div(mpd_t *q, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_div_ssize(mpd_t *result, const mpd_t *a, mpd_ssize_t b, mpd_context_t *ctx);
void mpd_div_i32(mpd_t *result, const mpd_t *a, int32_t b, mpd_context_t *ctx);
void mpd_div_uint(mpd_t *result, const mpd_t *a, mpd_uint_t b, mpd_context_t *ctx);
void mpd_div_u32(mpd_t *result, const mpd_t *a, uint32_t b, mpd_context_t *ctx);
void mpd_divmod(mpd_t *q, mpd_t *r, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_divint(mpd_t *q, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_exp(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_fma(mpd_t *result, const mpd_t *a, const mpd_t *b, const mpd_t *c, mpd_context_t *ctx);
void mpd_ln(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_log10(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_max(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_max_mag(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_min(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_min_mag(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_minus(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_mul(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_mul_ssize(mpd_t *result, const mpd_t *a, mpd_ssize_t b, mpd_context_t *ctx);
void mpd_mul_i32(mpd_t *result, const mpd_t *a, int32_t b, mpd_context_t *ctx);
void mpd_mul_uint(mpd_t *result, const mpd_t *a, mpd_uint_t b, mpd_context_t *ctx);
void mpd_mul_u32(mpd_t *result, const mpd_t *a, uint32_t b, mpd_context_t *ctx);
void mpd_next_minus(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_next_plus(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_next_toward(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_plus(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_pow(mpd_t *result, const mpd_t *base, const mpd_t *exp, mpd_context_t *ctx);
void mpd_powmod(mpd_t *result, const mpd_t *base, const mpd_t *exp, const mpd_t *mod, mpd_context_t *ctx);
void mpd_quantize(mpd_t *result, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_rescale(mpd_t *result, const mpd_t *a, mpd_ssize_t exp, mpd_context_t *ctx);
void mpd_reduce(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_rem(mpd_t *r, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_rem_near(mpd_t *r, const mpd_t *a, const mpd_t *b, mpd_context_t *ctx);
void mpd_round_to_intx(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_round_to_int(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_trunc(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_floor(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_ceil(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_sqrt(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);
void mpd_invroot(mpd_t *result, const mpd_t *a, mpd_context_t *ctx);

#ifndef LEGACY_COMPILER
void mpd_add_i64(mpd_t *result, const mpd_t *a, int64_t b, mpd_context_t *ctx);
void mpd_add_u64(mpd_t *result, const mpd_t *a, uint64_t b, mpd_context_t *ctx);
void mpd_sub_i64(mpd_t *result, const mpd_t *a, int64_t b, mpd_context_t *ctx);
void mpd_sub_u64(mpd_t *result, const mpd_t *a, uint64_t b, mpd_context_t *ctx);
void mpd_div_i64(mpd_t *result, const mpd_t *a, int64_t b, mpd_context_t *ctx);
void mpd_div_u64(mpd_t *result, const mpd_t *a, uint64_t b, mpd_context_t *ctx);
void mpd_mul_i64(mpd_t *result, const mpd_t *a, int64_t b, mpd_context_t *ctx);
void mpd_mul_u64(mpd_t *result, const mpd_t *a, uint64_t b, mpd_context_t *ctx);
#endif


/******************************************************************************/
/*                          Configuration specific                            */
/******************************************************************************/

#ifdef CONFIG_64
void mpd_qsset_i64(mpd_t *result, int64_t a, const mpd_context_t *ctx, uint32_t *status);
void mpd_qsset_u64(mpd_t *result, uint64_t a, const mpd_context_t *ctx, uint32_t *status);
void mpd_sset_i64(mpd_t *result, int64_t a, mpd_context_t *ctx);
void mpd_sset_u64(mpd_t *result, uint64_t a, mpd_context_t *ctx);
#endif


/******************************************************************************/
/*                       Get attributes of a decimal                          */
/******************************************************************************/

EXTINLINE mpd_ssize_t mpd_adjexp(const mpd_t *dec);
EXTINLINE mpd_ssize_t mpd_etiny(const mpd_context_t *ctx);
EXTINLINE mpd_ssize_t mpd_etop(const mpd_context_t *ctx);
EXTINLINE mpd_uint_t mpd_msword(const mpd_t *dec);
EXTINLINE int mpd_word_digits(mpd_uint_t word);
/* most significant digit of a word */
EXTINLINE mpd_uint_t mpd_msd(mpd_uint_t word);
/* least significant digit of a word */
EXTINLINE mpd_uint_t mpd_lsd(mpd_uint_t word);
/* coefficient size needed to store 'digits' */
EXTINLINE mpd_ssize_t mpd_digits_to_size(mpd_ssize_t digits);
/* number of digits in the exponent, undefined for MPD_SSIZE_MIN */
EXTINLINE int mpd_exp_digits(mpd_ssize_t exp);
EXTINLINE int mpd_iscanonical(const mpd_t *dec UNUSED);
EXTINLINE int mpd_isfinite(const mpd_t *dec);
EXTINLINE int mpd_isinfinite(const mpd_t *dec);
EXTINLINE int mpd_isinteger(const mpd_t *dec);
EXTINLINE int mpd_isnan(const mpd_t *dec);
EXTINLINE int mpd_isnegative(const mpd_t *dec);
EXTINLINE int mpd_ispositive(const mpd_t *dec);
EXTINLINE int mpd_isqnan(const mpd_t *dec);
EXTINLINE int mpd_issigned(const mpd_t *dec);
EXTINLINE int mpd_issnan(const mpd_t *dec);
EXTINLINE int mpd_isspecial(const mpd_t *dec);
EXTINLINE int mpd_iszero(const mpd_t *dec);
/* undefined for special numbers */
EXTINLINE int mpd_iszerocoeff(const mpd_t *dec);
EXTINLINE int mpd_isnormal(const mpd_t *dec, const mpd_context_t *ctx);
EXTINLINE int mpd_issubnormal(const mpd_t *dec, const mpd_context_t *ctx);
/* odd word */
EXTINLINE int mpd_isoddword(mpd_uint_t word);
/* odd coefficient */
EXTINLINE int mpd_isoddcoeff(const mpd_t *dec);
/* odd decimal, only defined for integers */
int mpd_isodd(const mpd_t *dec);
/* even decimal, only defined for integers */
int mpd_iseven(const mpd_t *dec);
/* 0 if dec is positive, 1 if dec is negative */
EXTINLINE uint8_t mpd_sign(const mpd_t *dec);
/* 1 if dec is positive, -1 if dec is negative */
EXTINLINE int mpd_arith_sign(const mpd_t *dec);
EXTINLINE long mpd_radix(void);
EXTINLINE int mpd_isdynamic(const mpd_t *dec);
EXTINLINE int mpd_isstatic(const mpd_t *dec);
EXTINLINE int mpd_isdynamic_data(const mpd_t *dec);
EXTINLINE int mpd_isstatic_data(const mpd_t *dec);
EXTINLINE int mpd_isshared_data(const mpd_t *dec);
EXTINLINE int mpd_isconst_data(const mpd_t *dec);
EXTINLINE mpd_ssize_t mpd_trail_zeros(const mpd_t *dec);


/******************************************************************************/
/*                       Set attributes of a decimal                          */
/******************************************************************************/

/* set number of decimal digits in the coefficient */
EXTINLINE void mpd_setdigits(mpd_t *result);
EXTINLINE void mpd_set_sign(mpd_t *result, uint8_t sign);
/* copy sign from another decimal */
EXTINLINE void mpd_signcpy(mpd_t *result, const mpd_t *a);
EXTINLINE void mpd_set_infinity(mpd_t *result);
EXTINLINE void mpd_set_qnan(mpd_t *result);
EXTINLINE void mpd_set_snan(mpd_t *result);
EXTINLINE void mpd_set_negative(mpd_t *result);
EXTINLINE void mpd_set_positive(mpd_t *result);
EXTINLINE void mpd_set_dynamic(mpd_t *result);
EXTINLINE void mpd_set_static(mpd_t *result);
EXTINLINE void mpd_set_dynamic_data(mpd_t *result);
EXTINLINE void mpd_set_static_data(mpd_t *result);
EXTINLINE void mpd_set_shared_data(mpd_t *result);
EXTINLINE void mpd_set_const_data(mpd_t *result);
EXTINLINE void mpd_clear_flags(mpd_t *result);
EXTINLINE void mpd_set_flags(mpd_t *result, uint8_t flags);
EXTINLINE void mpd_copy_flags(mpd_t *result, const mpd_t *a);


/******************************************************************************/
/*                              Error Macros                                  */
/******************************************************************************/

#define mpd_err_fatal(...) \
    do {fprintf(stderr, "%s:%d: error: ", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__);  fputc('\n', stderr);    \
        abort();                                               \
    } while (0)
#define mpd_err_warn(...) \
    do {fprintf(stderr, "%s:%d: warning: ", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__); fputc('\n', stderr);       \
    } while (0)


/******************************************************************************/
/*                            Memory handling                                 */
/******************************************************************************/

extern void *(* mpd_mallocfunc)(size_t size);
extern void *(* mpd_callocfunc)(size_t nmemb, size_t size);
extern void *(* mpd_reallocfunc)(void *ptr, size_t size);
extern void (* mpd_free)(void *ptr);

void *mpd_callocfunc_em(size_t nmemb, size_t size);

void *mpd_alloc(mpd_size_t nmemb, mpd_size_t size);
void *mpd_calloc(mpd_size_t nmemb, mpd_size_t size);
void *mpd_realloc(void *ptr, mpd_size_t nmemb, mpd_size_t size, uint8_t *err);
void *mpd_sh_alloc(mpd_size_t struct_size, mpd_size_t nmemb, mpd_size_t size);

mpd_t *mpd_qnew(void);
mpd_t *mpd_new(mpd_context_t *ctx);
mpd_t *mpd_qnew_size(mpd_ssize_t size);
void mpd_del(mpd_t *dec);

void mpd_uint_zero(mpd_uint_t *dest, mpd_size_t len);
int mpd_qresize(mpd_t *result, mpd_ssize_t size, uint32_t *status);
int mpd_qresize_zero(mpd_t *result, mpd_ssize_t size, uint32_t *status);
void mpd_minalloc(mpd_t *result);

int mpd_resize(mpd_t *result, mpd_ssize_t size, mpd_context_t *ctx);
int mpd_resize_zero(mpd_t *result, mpd_ssize_t size, mpd_context_t *ctx);


MPD_PRAGMA(MPD_HIDE_SYMBOLS_END) /* restore previous scope rules */


#ifdef __cplusplus
  #ifdef MPD_CLEAR_STDC_LIMIT_MACROS
    #undef MPD_CLEAR_STDC_LIMIT_MACROS
    #undef __STDC_LIMIT_MACROS
  #endif
} /* END extern "C" */
#endif


#endif /* MPDECIMAL_H */



