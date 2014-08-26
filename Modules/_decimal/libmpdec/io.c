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


#include "mpdecimal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <assert.h>
#include <errno.h>
#include <locale.h>
#include "bits.h"
#include "constants.h"
#include "memory.h"
#include "typearith.h"
#include "io.h"


/* This file contains functions for decimal <-> string conversions, including
   PEP-3101 formatting for numeric types. */


/*
 * Work around the behavior of tolower() and strcasecmp() in certain
 * locales. For example, in tr_TR.utf8:
 *
 * tolower((unsigned char)'I') == 'I'
 *
 * u is the exact uppercase version of l; n is strlen(l) or strlen(l)+1
 */
static inline int
_mpd_strneq(const char *s, const char *l, const char *u, size_t n)
{
    while (--n != SIZE_MAX) {
        if (*s != *l && *s != *u) {
            return 0;
        }
        s++; u++; l++;
    }

    return 1;
}

static mpd_ssize_t
strtoexp(const char *s)
{
    char *end;
    mpd_ssize_t retval;

    errno = 0;
    retval = mpd_strtossize(s, &end, 10);
    if (errno == 0 && !(*s != '\0' && *end == '\0'))
        errno = EINVAL;

    return retval;
}

/*
 * Scan 'len' words. The most significant word contains 'r' digits,
 * the remaining words are full words. Skip dpoint. The string 's' must
 * consist of digits and an optional single decimal point at 'dpoint'.
 */
static void
string_to_coeff(mpd_uint_t *data, const char *s, const char *dpoint, int r,
                size_t len)
{
    int j;

    if (r > 0) {
        data[--len] = 0;
        for (j = 0; j < r; j++, s++) {
            if (s == dpoint) s++;
            data[len] = 10 * data[len] + (*s - '0');
        }
    }

    while (--len != SIZE_MAX) {
        data[len] = 0;
        for (j = 0; j < MPD_RDIGITS; j++, s++) {
            if (s == dpoint) s++;
            data[len] = 10 * data[len] + (*s - '0');
        }
    }
}

/*
 * Partially verify a numeric string of the form:
 *
 *     [cdigits][.][cdigits][eE][+-][edigits]
 *
 * If successful, return a pointer to the location of the first
 * relevant coefficient digit. This digit is either non-zero or
 * part of one of the following patterns:
 *
 *     ["0\x00", "0.\x00", "0.E", "0.e", "0E", "0e"]
 *
 * The locations of a single optional dot or indicator are stored
 * in 'dpoint' and 'exp'.
 *
 * The end of the string is stored in 'end'. If an indicator [eE]
 * occurs without trailing [edigits], the condition is caught
 * later by strtoexp().
 */
static const char *
scan_dpoint_exp(const char *s, const char **dpoint, const char **exp,
                const char **end)
{
    const char *coeff = NULL;

    *dpoint = NULL;
    *exp = NULL;
    for (; *s != '\0'; s++) {
        switch (*s) {
        case '.':
            if (*dpoint != NULL || *exp != NULL)
                return NULL;
            *dpoint = s;
            break;
        case 'E': case 'e':
            if (*exp != NULL)
                return NULL;
            *exp = s;
            if (*(s+1) == '+' || *(s+1) == '-')
                s++;
            break;
        default:
            if (!isdigit((uchar)*s))
                return NULL;
            if (coeff == NULL && *exp == NULL) {
                if (*s == '0') {
                    if (!isdigit((uchar)*(s+1)))
                        if (!(*(s+1) == '.' &&
                              isdigit((uchar)*(s+2))))
                            coeff = s;
                }
                else {
                    coeff = s;
                }
            }
            break;

        }
    }

    *end = s;
    return coeff;
}

/* scan the payload of a NaN */
static const char *
scan_payload(const char *s, const char **end)
{
    const char *coeff;

    while (*s == '0')
        s++;
    coeff = s;

    while (isdigit((uchar)*s))
        s++;
    *end = s;

    return (*s == '\0') ? coeff : NULL;
}

/* convert a character string to a decimal */
void
mpd_qset_string(mpd_t *dec, const char *s, const mpd_context_t *ctx,
                uint32_t *status)
{
    mpd_ssize_t q, r, len;
    const char *coeff, *end;
    const char *dpoint = NULL, *exp = NULL;
    size_t digits;
    uint8_t sign = MPD_POS;

    mpd_set_flags(dec, 0);
    dec->len = 0;
    dec->exp = 0;

    /* sign */
    if (*s == '+') {
        s++;
    }
    else if (*s == '-') {
        mpd_set_negative(dec);
        sign = MPD_NEG;
        s++;
    }

    if (_mpd_strneq(s, "nan", "NAN", 3)) { /* NaN */
        s += 3;
        mpd_setspecial(dec, sign, MPD_NAN);
        if (*s == '\0')
            return;
        /* validate payload: digits only */
        if ((coeff = scan_payload(s, &end)) == NULL)
            goto conversion_error;
        /* payload consists entirely of zeros */
        if (*coeff == '\0')
            return;
        digits = end - coeff;
        /* prec >= 1, clamp is 0 or 1 */
        if (digits > (size_t)(ctx->prec-ctx->clamp))
            goto conversion_error;
    } /* sNaN */
    else if (_mpd_strneq(s, "snan", "SNAN", 4)) {
        s += 4;
        mpd_setspecial(dec, sign, MPD_SNAN);
        if (*s == '\0')
            return;
        /* validate payload: digits only */
        if ((coeff = scan_payload(s, &end)) == NULL)
            goto conversion_error;
        /* payload consists entirely of zeros */
        if (*coeff == '\0')
            return;
        digits = end - coeff;
        if (digits > (size_t)(ctx->prec-ctx->clamp))
            goto conversion_error;
    }
    else if (_mpd_strneq(s, "inf", "INF", 3)) {
        s += 3;
        if (*s == '\0' || _mpd_strneq(s, "inity", "INITY", 6)) {
            /* numeric-value: infinity */
            mpd_setspecial(dec, sign, MPD_INF);
            return;
        }
        goto conversion_error;
    }
    else {
        /* scan for start of coefficient, decimal point, indicator, end */
        if ((coeff = scan_dpoint_exp(s, &dpoint, &exp, &end)) == NULL)
            goto conversion_error;

        /* numeric-value: [exponent-part] */
        if (exp) {
            /* exponent-part */
            end = exp; exp++;
            dec->exp = strtoexp(exp);
            if (errno) {
                if (!(errno == ERANGE &&
                     (dec->exp == MPD_SSIZE_MAX ||
                      dec->exp == MPD_SSIZE_MIN)))
                    goto conversion_error;
            }
        }

            digits = end - coeff;
        if (dpoint) {
            size_t fracdigits = end-dpoint-1;
            if (dpoint > coeff) digits--;

            if (fracdigits > MPD_MAX_PREC) {
                goto conversion_error;
            }
            if (dec->exp < MPD_SSIZE_MIN+(mpd_ssize_t)fracdigits) {
                dec->exp = MPD_SSIZE_MIN;
            }
            else {
                dec->exp -= (mpd_ssize_t)fracdigits;
            }
        }
        if (digits > MPD_MAX_PREC) {
            goto conversion_error;
        }
        if (dec->exp > MPD_EXP_INF) {
            dec->exp = MPD_EXP_INF;
        }
        if (dec->exp == MPD_SSIZE_MIN) {
            dec->exp = MPD_SSIZE_MIN+1;
        }
    }

    _mpd_idiv_word(&q, &r, (mpd_ssize_t)digits, MPD_RDIGITS);

    len = (r == 0) ? q : q+1;
    if (len == 0) {
        goto conversion_error; /* GCOV_NOT_REACHED */
    }
    if (!mpd_qresize(dec, len, status)) {
        mpd_seterror(dec, MPD_Malloc_error, status);
        return;
    }
    dec->len = len;

    string_to_coeff(dec->data, coeff, dpoint, (int)r, len);

    mpd_setdigits(dec);
    mpd_qfinalize(dec, ctx, status);
    return;

conversion_error:
    /* standard wants a positive NaN */
    mpd_seterror(dec, MPD_Conversion_syntax, status);
}

/* Print word x with n decimal digits to string s. dot is either NULL
   or the location of a decimal point. */
#define EXTRACT_DIGIT(s, x, d, dot) \
        if (s == dot) *s++ = '.'; *s++ = '0' + (char)(x / d); x %= d
static inline char *
word_to_string(char *s, mpd_uint_t x, int n, char *dot)
{
    switch(n) {
#ifdef CONFIG_64
    case 20: EXTRACT_DIGIT(s, x, 10000000000000000000ULL, dot); /* GCOV_NOT_REACHED */
    case 19: EXTRACT_DIGIT(s, x, 1000000000000000000ULL, dot);
    case 18: EXTRACT_DIGIT(s, x, 100000000000000000ULL, dot);
    case 17: EXTRACT_DIGIT(s, x, 10000000000000000ULL, dot);
    case 16: EXTRACT_DIGIT(s, x, 1000000000000000ULL, dot);
    case 15: EXTRACT_DIGIT(s, x, 100000000000000ULL, dot);
    case 14: EXTRACT_DIGIT(s, x, 10000000000000ULL, dot);
    case 13: EXTRACT_DIGIT(s, x, 1000000000000ULL, dot);
    case 12: EXTRACT_DIGIT(s, x, 100000000000ULL, dot);
    case 11: EXTRACT_DIGIT(s, x, 10000000000ULL, dot);
#endif
    case 10: EXTRACT_DIGIT(s, x, 1000000000UL, dot);
    case 9:  EXTRACT_DIGIT(s, x, 100000000UL, dot);
    case 8:  EXTRACT_DIGIT(s, x, 10000000UL, dot);
    case 7:  EXTRACT_DIGIT(s, x, 1000000UL, dot);
    case 6:  EXTRACT_DIGIT(s, x, 100000UL, dot);
    case 5:  EXTRACT_DIGIT(s, x, 10000UL, dot);
    case 4:  EXTRACT_DIGIT(s, x, 1000UL, dot);
    case 3:  EXTRACT_DIGIT(s, x, 100UL, dot);
    case 2:  EXTRACT_DIGIT(s, x, 10UL, dot);
    default: if (s == dot) *s++ = '.'; *s++ = '0' + (char)x;
    }

    *s = '\0';
    return s;
}

/* Print exponent x to string s. Undefined for MPD_SSIZE_MIN. */
static inline char *
exp_to_string(char *s, mpd_ssize_t x)
{
    char sign = '+';

    if (x < 0) {
        sign = '-';
        x = -x;
    }
    *s++ = sign;

    return word_to_string(s, x, mpd_word_digits(x), NULL);
}

/* Print the coefficient of dec to string s. len(dec) > 0. */
static inline char *
coeff_to_string(char *s, const mpd_t *dec)
{
    mpd_uint_t x;
    mpd_ssize_t i;

    /* most significant word */
    x = mpd_msword(dec);
    s = word_to_string(s, x, mpd_word_digits(x), NULL);

    /* remaining full words */
    for (i=dec->len-2; i >= 0; --i) {
        x = dec->data[i];
        s = word_to_string(s, x, MPD_RDIGITS, NULL);
    }

    return s;
}

/* Print the coefficient of dec to string s. len(dec) > 0. dot is either
   NULL or a pointer to the location of a decimal point. */
static inline char *
coeff_to_string_dot(char *s, char *dot, const mpd_t *dec)
{
    mpd_uint_t x;
    mpd_ssize_t i;

    /* most significant word */
    x = mpd_msword(dec);
    s = word_to_string(s, x, mpd_word_digits(x), dot);

    /* remaining full words */
    for (i=dec->len-2; i >= 0; --i) {
        x = dec->data[i];
        s = word_to_string(s, x, MPD_RDIGITS, dot);
    }

    return s;
}

/* Format type */
#define MPD_FMT_LOWER      0x00000000
#define MPD_FMT_UPPER      0x00000001
#define MPD_FMT_TOSCI      0x00000002
#define MPD_FMT_TOENG      0x00000004
#define MPD_FMT_EXP        0x00000008
#define MPD_FMT_FIXED      0x00000010
#define MPD_FMT_PERCENT    0x00000020
#define MPD_FMT_SIGN_SPACE 0x00000040
#define MPD_FMT_SIGN_PLUS  0x00000080

/* Default place of the decimal point for MPD_FMT_TOSCI, MPD_FMT_EXP */
#define MPD_DEFAULT_DOTPLACE 1

/*
 * Set *result to the string representation of a decimal. Return the length
 * of *result, not including the terminating '\0' character.
 *
 * Formatting is done according to 'flags'. A return value of -1 with *result
 * set to NULL indicates MPD_Malloc_error.
 *
 * 'dplace' is the default place of the decimal point. It is always set to
 * MPD_DEFAULT_DOTPLACE except for zeros in combination with MPD_FMT_EXP.
 */
static mpd_ssize_t
_mpd_to_string(char **result, const mpd_t *dec, int flags, mpd_ssize_t dplace)
{
    char *decstring = NULL, *cp = NULL;
    mpd_ssize_t ldigits;
    mpd_ssize_t mem = 0, k;

    if (mpd_isspecial(dec)) {

        mem = sizeof "-Infinity%";
        if (mpd_isnan(dec) && dec->len > 0) {
            /* diagnostic code */
            mem += dec->digits;
        }
        cp = decstring = mpd_alloc(mem, sizeof *decstring);
        if (cp == NULL) {
            *result = NULL;
            return -1;
        }

        if (mpd_isnegative(dec)) {
            *cp++ = '-';
        }
        else if (flags&MPD_FMT_SIGN_SPACE) {
            *cp++ = ' ';
        }
        else if (flags&MPD_FMT_SIGN_PLUS) {
            *cp++ = '+';
        }

        if (mpd_isnan(dec)) {
            if (mpd_isqnan(dec)) {
                strcpy(cp, "NaN");
                cp += 3;
            }
            else {
                strcpy(cp, "sNaN");
                cp += 4;
            }
            if (dec->len > 0) { /* diagnostic code */
                cp = coeff_to_string(cp, dec);
            }
        }
        else if (mpd_isinfinite(dec)) {
            strcpy(cp, "Infinity");
            cp += 8;
        }
        else { /* debug */
            abort(); /* GCOV_NOT_REACHED */
        }
    }
    else {
        assert(dec->len > 0);

        /*
         * For easier manipulation of the decimal point's location
         * and the exponent that is finally printed, the number is
         * rescaled to a virtual representation with exp = 0. Here
         * ldigits denotes the number of decimal digits to the left
         * of the decimal point and remains constant once initialized.
         *
         * dplace is the location of the decimal point relative to
         * the start of the coefficient. Note that 3) always holds
         * when dplace is shifted.
         *
         *   1) ldigits := dec->digits - dec->exp
         *   2) dplace  := ldigits            (initially)
         *   3) exp     := ldigits - dplace   (initially exp = 0)
         *
         *   0.00000_.____._____000000.
         *    ^      ^    ^           ^
         *    |      |    |           |
         *    |      |    |           `- dplace >= digits
         *    |      |    `- dplace in the middle of the coefficient
         *    |      ` dplace = 1 (after the first coefficient digit)
         *    `- dplace <= 0
         */

        ldigits = dec->digits + dec->exp;

        if (flags&MPD_FMT_EXP) {
            ;
        }
        else if (flags&MPD_FMT_FIXED || (dec->exp <= 0 && ldigits > -6)) {
            /* MPD_FMT_FIXED: always use fixed point notation.
             * MPD_FMT_TOSCI, MPD_FMT_TOENG: for a certain range,
             * override exponent notation. */
            dplace = ldigits;
        }
        else if (flags&MPD_FMT_TOENG) {
            if (mpd_iszero(dec)) {
                /* If the exponent is divisible by three,
                 * dplace = 1. Otherwise, move dplace one
                 * or two places to the left. */
                dplace = -1 + mod_mpd_ssize_t(dec->exp+2, 3);
            }
            else { /* ldigits-1 is the adjusted exponent, which
                * should be divisible by three. If not, move
                * dplace one or two places to the right. */
                dplace += mod_mpd_ssize_t(ldigits-1, 3);
            }
        }

        /*
         * Basic space requirements:
         *
         * [-][.][coeffdigits][E][-][expdigits+1][%]['\0']
         *
         * If the decimal point lies outside of the coefficient digits,
         * space is adjusted accordingly.
         */
        if (dplace <= 0) {
            mem = -dplace + dec->digits + 2;
        }
        else if (dplace >= dec->digits) {
            mem = dplace;
        }
        else {
            mem = dec->digits;
        }
        mem += (MPD_EXPDIGITS+1+6);

        cp = decstring = mpd_alloc(mem, sizeof *decstring);
        if (cp == NULL) {
            *result = NULL;
            return -1;
        }


        if (mpd_isnegative(dec)) {
            *cp++ = '-';
        }
        else if (flags&MPD_FMT_SIGN_SPACE) {
            *cp++ = ' ';
        }
        else if (flags&MPD_FMT_SIGN_PLUS) {
            *cp++ = '+';
        }

        if (dplace <= 0) {
            /* space: -dplace+dec->digits+2 */
            *cp++ = '0';
            *cp++ = '.';
            for (k = 0; k < -dplace; k++) {
                *cp++ = '0';
            }
            cp = coeff_to_string(cp, dec);
        }
        else if (dplace >= dec->digits) {
            /* space: dplace */
            cp = coeff_to_string(cp, dec);
            for (k = 0; k < dplace-dec->digits; k++) {
                *cp++ = '0';
            }
        }
        else {
            /* space: dec->digits+1 */
            cp = coeff_to_string_dot(cp, cp+dplace, dec);
        }

        /*
         * Conditions for printing an exponent:
         *
         *   MPD_FMT_TOSCI, MPD_FMT_TOENG: only if ldigits != dplace
         *   MPD_FMT_FIXED:                never (ldigits == dplace)
         *   MPD_FMT_EXP:                  always
         */
        if (ldigits != dplace || flags&MPD_FMT_EXP) {
            /* space: expdigits+2 */
            *cp++ = (flags&MPD_FMT_UPPER) ? 'E' : 'e';
            cp = exp_to_string(cp, ldigits-dplace);
        }
    }

    if (flags&MPD_FMT_PERCENT) {
        *cp++ = '%';
    }

    assert(cp < decstring+mem);
    assert(cp-decstring < MPD_SSIZE_MAX);

    *cp = '\0';
    *result = decstring;
    return (mpd_ssize_t)(cp-decstring);
}

char *
mpd_to_sci(const mpd_t *dec, int fmt)
{
    char *res;
    int flags = MPD_FMT_TOSCI;

    flags |= fmt ? MPD_FMT_UPPER : MPD_FMT_LOWER;
    (void)_mpd_to_string(&res, dec, flags, MPD_DEFAULT_DOTPLACE);
    return res;
}

char *
mpd_to_eng(const mpd_t *dec, int fmt)
{
    char *res;
    int flags = MPD_FMT_TOENG;

    flags |= fmt ? MPD_FMT_UPPER : MPD_FMT_LOWER;
    (void)_mpd_to_string(&res, dec, flags, MPD_DEFAULT_DOTPLACE);
    return res;
}

mpd_ssize_t
mpd_to_sci_size(char **res, const mpd_t *dec, int fmt)
{
    int flags = MPD_FMT_TOSCI;

    flags |= fmt ? MPD_FMT_UPPER : MPD_FMT_LOWER;
    return _mpd_to_string(res, dec, flags, MPD_DEFAULT_DOTPLACE);
}

mpd_ssize_t
mpd_to_eng_size(char **res, const mpd_t *dec, int fmt)
{
    int flags = MPD_FMT_TOENG;

    flags |= fmt ? MPD_FMT_UPPER : MPD_FMT_LOWER;
    return _mpd_to_string(res, dec, flags, MPD_DEFAULT_DOTPLACE);
}

/* Copy a single UTF-8 char to dest. See: The Unicode Standard, version 5.2,
   chapter 3.9: Well-formed UTF-8 byte sequences. */
static int
_mpd_copy_utf8(char dest[5], const char *s)
{
    const uchar *cp = (const uchar *)s;
    uchar lb, ub;
    int count, i;


    if (*cp == 0) {
        /* empty string */
        dest[0] = '\0';
        return 0;
    }
    else if (*cp <= 0x7f) {
        /* ascii */
        dest[0] = *cp;
        dest[1] = '\0';
        return 1;
    }
    else if (0xc2 <= *cp && *cp <= 0xdf) {
        lb = 0x80; ub = 0xbf;
        count = 2;
    }
    else if (*cp == 0xe0) {
        lb = 0xa0; ub = 0xbf;
        count = 3;
    }
    else if (*cp <= 0xec) {
        lb = 0x80; ub = 0xbf;
        count = 3;
    }
    else if (*cp == 0xed) {
        lb = 0x80; ub = 0x9f;
        count = 3;
    }
    else if (*cp <= 0xef) {
        lb = 0x80; ub = 0xbf;
        count = 3;
    }
    else if (*cp == 0xf0) {
        lb = 0x90; ub = 0xbf;
        count = 4;
    }
    else if (*cp <= 0xf3) {
        lb = 0x80; ub = 0xbf;
        count = 4;
    }
    else if (*cp == 0xf4) {
        lb = 0x80; ub = 0x8f;
        count = 4;
    }
    else {
        /* invalid */
        goto error;
    }

    dest[0] = *cp++;
    if (*cp < lb || ub < *cp) {
        goto error;
    }
    dest[1] = *cp++;
    for (i = 2; i < count; i++) {
        if (*cp < 0x80 || 0xbf < *cp) {
            goto error;
        }
        dest[i] = *cp++;
    }
    dest[i] = '\0';

    return count;

error:
    dest[0] = '\0';
    return -1;
}

int
mpd_validate_lconv(mpd_spec_t *spec)
{
    size_t n;
#if CHAR_MAX == SCHAR_MAX
    const char *cp = spec->grouping;
    while (*cp != '\0') {
        if (*cp++ < 0) {
            return -1;
        }
    }
#endif
    n = strlen(spec->dot);
    if (n == 0 || n > 4) {
        return -1;
    }
    if (strlen(spec->sep) > 4) {
        return -1;
    }

    return 0;
}

int
mpd_parse_fmt_str(mpd_spec_t *spec, const char *fmt, int caps)
{
    char *cp = (char *)fmt;
    int have_align = 0, n;

    /* defaults */
    spec->min_width = 0;
    spec->prec = -1;
    spec->type = caps ? 'G' : 'g';
    spec->align = '>';
    spec->sign = '-';
    spec->dot = "";
    spec->sep = "";
    spec->grouping = "";


    /* presume that the first character is a UTF-8 fill character */
    if ((n = _mpd_copy_utf8(spec->fill, cp)) < 0) {
        return 0;
    }

    /* alignment directive, prefixed by a fill character */
    if (*cp && (*(cp+n) == '<' || *(cp+n) == '>' ||
                *(cp+n) == '=' || *(cp+n) == '^')) {
        cp += n;
        spec->align = *cp++;
        have_align = 1;
    } /* alignment directive */
    else {
        /* default fill character */
        spec->fill[0] = ' ';
        spec->fill[1] = '\0';
        if (*cp == '<' || *cp == '>' ||
            *cp == '=' || *cp == '^') {
            spec->align = *cp++;
            have_align = 1;
        }
    }

    /* sign formatting */
    if (*cp == '+' || *cp == '-' || *cp == ' ') {
        spec->sign = *cp++;
    }

    /* zero padding */
    if (*cp == '0') {
        /* zero padding implies alignment, which should not be
         * specified twice. */
        if (have_align) {
            return 0;
        }
        spec->align = 'z';
        spec->fill[0] = *cp++;
        spec->fill[1] = '\0';
    }

    /* minimum width */
    if (isdigit((uchar)*cp)) {
        if (*cp == '0') {
            return 0;
        }
        errno = 0;
        spec->min_width = mpd_strtossize(cp, &cp, 10);
        if (errno == ERANGE || errno == EINVAL) {
            return 0;
        }
    }

    /* thousands separator */
    if (*cp == ',') {
        spec->dot = ".";
        spec->sep = ",";
        spec->grouping = "\003\003";
        cp++;
    }

    /* fraction digits or significant digits */
    if (*cp == '.') {
        cp++;
        if (!isdigit((uchar)*cp)) {
            return 0;
        }
        errno = 0;
        spec->prec = mpd_strtossize(cp, &cp, 10);
        if (errno == ERANGE || errno == EINVAL) {
            return 0;
        }
    }

    /* type */
    if (*cp == 'E' || *cp == 'e' || *cp == 'F' || *cp == 'f' ||
        *cp == 'G' || *cp == 'g' || *cp == '%') {
        spec->type = *cp++;
    }
    else if (*cp == 'N' || *cp == 'n') {
        /* locale specific conversion */
        struct lconv *lc;
        /* separator has already been specified */
        if (*spec->sep) {
            return 0;
        }
        spec->type = *cp++;
        spec->type = (spec->type == 'N') ? 'G' : 'g';
        lc = localeconv();
        spec->dot = lc->decimal_point;
        spec->sep = lc->thousands_sep;
        spec->grouping = lc->grouping;
        if (mpd_validate_lconv(spec) < 0) {
            return 0; /* GCOV_NOT_REACHED */
        }
    }

    /* check correctness */
    if (*cp != '\0') {
        return 0;
    }

    return 1;
}

/*
 * The following functions assume that spec->min_width <= MPD_MAX_PREC, which
 * is made sure in mpd_qformat_spec. Then, even with a spec that inserts a
 * four-byte separator after each digit, nbytes in the following struct
 * cannot overflow.
 */

/* Multibyte string */
typedef struct {
    mpd_ssize_t nbytes; /* length in bytes */
    mpd_ssize_t nchars; /* length in chars */
    mpd_ssize_t cur;    /* current write index */
    char *data;
} mpd_mbstr_t;

static inline void
_mpd_bcopy(char *dest, const char *src, mpd_ssize_t n)
{
    while (--n >= 0) {
        dest[n] = src[n];
    }
}

static inline void
_mbstr_copy_char(mpd_mbstr_t *dest, const char *src, mpd_ssize_t n)
{
    dest->nbytes += n;
    dest->nchars += (n > 0 ? 1 : 0);
    dest->cur -= n;

    if (dest->data != NULL) {
        _mpd_bcopy(dest->data+dest->cur, src, n);
    }
}

static inline void
_mbstr_copy_ascii(mpd_mbstr_t *dest, const char *src, mpd_ssize_t n)
{
    dest->nbytes += n;
    dest->nchars += n;
    dest->cur -= n;

    if (dest->data != NULL) {
        _mpd_bcopy(dest->data+dest->cur, src, n);
    }
}

static inline void
_mbstr_copy_pad(mpd_mbstr_t *dest, mpd_ssize_t n)
{
    dest->nbytes += n;
    dest->nchars += n;
    dest->cur -= n;

    if (dest->data != NULL) {
        char *cp = dest->data + dest->cur;
        while (--n >= 0) {
            cp[n] = '0';
        }
    }
}

/*
 * Copy a numeric string to dest->data, adding separators in the integer
 * part according to spec->grouping. If leading zero padding is enabled
 * and the result is smaller than spec->min_width, continue adding zeros
 * and separators until the minimum width is reached.
 *
 * The final length of dest->data is stored in dest->nbytes. The number
 * of UTF-8 characters is stored in dest->nchars.
 *
 * First run (dest->data == NULL): determine the length of the result
 * string and store it in dest->nbytes.
 *
 * Second run (write to dest->data): data is written in chunks and in
 * reverse order, starting with the rest of the numeric string.
 */
static void
_mpd_add_sep_dot(mpd_mbstr_t *dest,
                 const char *sign, /* location of optional sign */
                 const char *src, mpd_ssize_t n_src, /* integer part and length */
                 const char *dot, /* location of optional decimal point */
                 const char *rest, mpd_ssize_t n_rest, /* remaining part and length */
                 const mpd_spec_t *spec)
{
    mpd_ssize_t n_sep, n_sign, consume;
    const char *g;
    int pad = 0;

    n_sign = sign ? 1 : 0;
    n_sep = (mpd_ssize_t)strlen(spec->sep);
    /* Initial write index: set to location of '\0' in the output string.
     * Irrelevant for the first run. */
    dest->cur = dest->nbytes;
    dest->nbytes = dest->nchars = 0;

    _mbstr_copy_ascii(dest, rest, n_rest);

    if (dot) {
        _mbstr_copy_char(dest, dot, (mpd_ssize_t)strlen(dot));
    }

    g = spec->grouping;
    consume = *g;
    while (1) {
        /* If the group length is 0 or CHAR_MAX or greater than the
         * number of source bytes, consume all remaining bytes. */
        if (*g == 0 || *g == CHAR_MAX || consume > n_src) {
            consume = n_src;
        }
        n_src -= consume;
        if (pad) {
            _mbstr_copy_pad(dest, consume);
        }
        else {
            _mbstr_copy_ascii(dest, src+n_src, consume);
        }

        if (n_src == 0) {
            /* Either the real source of intpart digits or the virtual
             * source of padding zeros is exhausted. */
            if (spec->align == 'z' &&
                dest->nchars + n_sign < spec->min_width) {
                /* Zero padding is set and length < min_width:
                 * Generate n_src additional characters. */
                n_src = spec->min_width - (dest->nchars + n_sign);
                /* Next iteration:
                 *   case *g == 0 || *g == CHAR_MAX:
                 *      consume all padding characters
                 *   case consume < g*:
                 *      fill remainder of current group
                 *   case consume == g*
                 *      copying is a no-op */
                consume = *g - consume;
                /* Switch on virtual source of zeros. */
                pad = 1;
                continue;
            }
            break;
        }

        if (n_sep > 0) {
            /* If padding is switched on, separators are counted
             * as padding characters. This rule does not apply if
             * the separator would be the first character of the
             * result string. */
            if (pad && n_src > 1) n_src -= 1;
            _mbstr_copy_char(dest, spec->sep, n_sep);
        }

        /* If non-NUL, use the next value for grouping. */
        if (*g && *(g+1)) g++;
        consume = *g;
    }

    if (sign) {
        _mbstr_copy_ascii(dest, sign, 1);
    }

    if (dest->data) {
        dest->data[dest->nbytes] = '\0';
    }
}

/*
 * Convert a numeric-string to its locale-specific appearance.
 * The string must have one of these forms:
 *
 *     1) [sign] digits [exponent-part]
 *     2) [sign] digits '.' [digits] [exponent-part]
 *
 * Not allowed, since _mpd_to_string() never returns this form:
 *
 *     3) [sign] '.' digits [exponent-part]
 *
 * Input: result->data := original numeric string (ASCII)
 *        result->bytes := strlen(result->data)
 *        result->nchars := strlen(result->data)
 *
 * Output: result->data := modified or original string
 *         result->bytes := strlen(result->data)
 *         result->nchars := number of characters (possibly UTF-8)
 */
static int
_mpd_apply_lconv(mpd_mbstr_t *result, const mpd_spec_t *spec, uint32_t *status)
{
    const char *sign = NULL, *intpart = NULL, *dot = NULL;
    const char *rest, *dp;
    char *decstring;
    mpd_ssize_t n_int, n_rest;

    /* original numeric string */
    dp = result->data;

    /* sign */
    if (*dp == '+' || *dp == '-' || *dp == ' ') {
        sign = dp++;
    }
    /* integer part */
    assert(isdigit((uchar)*dp));
    intpart = dp++;
    while (isdigit((uchar)*dp)) {
        dp++;
    }
    n_int = (mpd_ssize_t)(dp-intpart);
    /* decimal point */
    if (*dp == '.') {
        dp++; dot = spec->dot;
    }
    /* rest */
    rest = dp;
    n_rest = result->nbytes - (mpd_ssize_t)(dp-result->data);

    if (dot == NULL && (*spec->sep == '\0' || *spec->grouping == '\0')) {
        /* _mpd_add_sep_dot() would not change anything */
        return 1;
    }

    /* Determine the size of the new decimal string after inserting the
     * decimal point, optional separators and optional padding. */
    decstring = result->data;
    result->data = NULL;
    _mpd_add_sep_dot(result, sign, intpart, n_int, dot,
                     rest, n_rest, spec);

    result->data = mpd_alloc(result->nbytes+1, 1);
    if (result->data == NULL) {
        *status |= MPD_Malloc_error;
        mpd_free(decstring);
        return 0;
    }

    /* Perform actual writes. */
    _mpd_add_sep_dot(result, sign, intpart, n_int, dot,
                     rest, n_rest, spec);

    mpd_free(decstring);
    return 1;
}

/* Add padding to the formatted string if necessary. */
static int
_mpd_add_pad(mpd_mbstr_t *result, const mpd_spec_t *spec, uint32_t *status)
{
    if (result->nchars < spec->min_width) {
        mpd_ssize_t add_chars, add_bytes;
        size_t lpad = 0, rpad = 0;
        size_t n_fill, len, i, j;
        char align = spec->align;
        uint8_t err = 0;
        char *cp;

        n_fill = strlen(spec->fill);
        add_chars = (spec->min_width - result->nchars);
        /* max value: MPD_MAX_PREC * 4 */
        add_bytes = add_chars * (mpd_ssize_t)n_fill;

        cp = result->data = mpd_realloc(result->data,
                                        result->nbytes+add_bytes+1,
                                        sizeof *result->data, &err);
        if (err) {
            *status |= MPD_Malloc_error;
            mpd_free(result->data);
            return 0;
        }

        if (align == 'z') {
            align = '=';
        }

        if (align == '<') {
            rpad = add_chars;
        }
        else if (align == '>' || align == '=') {
            lpad = add_chars;
        }
        else { /* align == '^' */
            lpad = add_chars/2;
            rpad = add_chars-lpad;
        }

        len = result->nbytes;
        if (align == '=' && (*cp == '-' || *cp == '+' || *cp == ' ')) {
            /* leave sign in the leading position */
            cp++; len--;
        }

        memmove(cp+n_fill*lpad, cp, len);
        for (i = 0; i < lpad; i++) {
            for (j = 0; j < n_fill; j++) {
                cp[i*n_fill+j] = spec->fill[j];
            }
        }
        cp += (n_fill*lpad + len);
        for (i = 0; i < rpad; i++) {
            for (j = 0; j < n_fill; j++) {
                cp[i*n_fill+j] = spec->fill[j];
            }
        }

        result->nbytes += add_bytes;
        result->nchars += add_chars;
        result->data[result->nbytes] = '\0';
    }

    return 1;
}

/* Round a number to prec digits. The adjusted exponent stays the same
   or increases by one if rounding up crosses a power of ten boundary.
   If result->digits would exceed MPD_MAX_PREC+1, MPD_Invalid_operation
   is set and the result is NaN. */
static inline void
_mpd_round(mpd_t *result, const mpd_t *a, mpd_ssize_t prec,
           const mpd_context_t *ctx, uint32_t *status)
{
    mpd_ssize_t exp = a->exp + a->digits - prec;

    if (prec <= 0) {
        mpd_seterror(result, MPD_Invalid_operation, status); /* GCOV_NOT_REACHED */
        return; /* GCOV_NOT_REACHED */
    }
    if (mpd_isspecial(a) || mpd_iszero(a)) {
        mpd_qcopy(result, a, status); /* GCOV_NOT_REACHED */
        return; /* GCOV_NOT_REACHED */
    }

    mpd_qrescale_fmt(result, a, exp, ctx, status);
    if (result->digits > prec) {
        mpd_qrescale_fmt(result, result, exp+1, ctx, status);
    }
}

/*
 * Return the string representation of an mpd_t, formatted according to 'spec'.
 * The format specification is assumed to be valid. Memory errors are indicated
 * as usual. This function is quiet.
 */
char *
mpd_qformat_spec(const mpd_t *dec, const mpd_spec_t *spec,
                 const mpd_context_t *ctx, uint32_t *status)
{
    mpd_uint_t dt[MPD_MINALLOC_MAX];
    mpd_t tmp = {MPD_STATIC|MPD_STATIC_DATA,0,0,0,MPD_MINALLOC_MAX,dt};
    mpd_ssize_t dplace = MPD_DEFAULT_DOTPLACE;
    mpd_mbstr_t result;
    mpd_spec_t stackspec;
    char type = spec->type;
    int flags = 0;


    if (spec->min_width > MPD_MAX_PREC) {
        *status |= MPD_Invalid_operation;
        return NULL;
    }

    if (isupper((uchar)type)) {
        type = tolower((uchar)type);
        flags |= MPD_FMT_UPPER;
    }
    if (spec->sign == ' ') {
        flags |= MPD_FMT_SIGN_SPACE;
    }
    else if (spec->sign == '+') {
        flags |= MPD_FMT_SIGN_PLUS;
    }

    if (mpd_isspecial(dec)) {
        if (spec->align == 'z') {
            stackspec = *spec;
            stackspec.fill[0] = ' ';
            stackspec.fill[1] = '\0';
            stackspec.align = '>';
            spec = &stackspec;
        }
        if (type == '%') {
            flags |= MPD_FMT_PERCENT;
        }
    }
    else {
        uint32_t workstatus = 0;
        mpd_ssize_t prec;

        switch (type) {
        case 'g': flags |= MPD_FMT_TOSCI; break;
        case 'e': flags |= MPD_FMT_EXP; break;
        case '%': flags |= MPD_FMT_PERCENT;
                  if (!mpd_qcopy(&tmp, dec, status)) {
                      return NULL;
                  }
                  tmp.exp += 2;
                  dec = &tmp;
                  type = 'f'; /* fall through */
        case 'f': flags |= MPD_FMT_FIXED; break;
        default: abort(); /* debug: GCOV_NOT_REACHED */
        }

        if (spec->prec >= 0) {
            if (spec->prec > MPD_MAX_PREC) {
                *status |= MPD_Invalid_operation;
                goto error;
            }

            switch (type) {
            case 'g':
                prec = (spec->prec == 0) ? 1 : spec->prec;
                if (dec->digits > prec) {
                    _mpd_round(&tmp, dec, prec, ctx,
                               &workstatus);
                    dec = &tmp;
                }
                break;
            case 'e':
                if (mpd_iszero(dec)) {
                    dplace = 1-spec->prec;
                }
                else {
                    _mpd_round(&tmp, dec, spec->prec+1, ctx,
                               &workstatus);
                    dec = &tmp;
                }
                break;
            case 'f':
                mpd_qrescale(&tmp, dec, -spec->prec, ctx,
                             &workstatus);
                dec = &tmp;
                break;
            }
        }

        if (type == 'f') {
            if (mpd_iszero(dec) && dec->exp > 0) {
                mpd_qrescale(&tmp, dec, 0, ctx, &workstatus);
                dec = &tmp;
            }
        }

        if (workstatus&MPD_Errors) {
            *status |= (workstatus&MPD_Errors);
            goto error;
        }
    }

    /*
     * At this point, for all scaled or non-scaled decimals:
     *   1) 1 <= digits <= MAX_PREC+1
     *   2) adjexp(scaled) = adjexp(orig) [+1]
     *   3)   case 'g': MIN_ETINY <= exp <= MAX_EMAX+1
     *        case 'e': MIN_ETINY-MAX_PREC <= exp <= MAX_EMAX+1
     *        case 'f': MIN_ETINY <= exp <= MAX_EMAX+1
     *   4) max memory alloc in _mpd_to_string:
     *        case 'g': MAX_PREC+36
     *        case 'e': MAX_PREC+36
     *        case 'f': 2*MPD_MAX_PREC+30
     */
    result.nbytes = _mpd_to_string(&result.data, dec, flags, dplace);
    result.nchars = result.nbytes;
    if (result.nbytes < 0) {
        *status |= MPD_Malloc_error;
        goto error;
    }

    if (*spec->dot != '\0' && !mpd_isspecial(dec)) {
        if (result.nchars > MPD_MAX_PREC+36) {
            /* Since a group length of one is not explicitly
             * disallowed, ensure that it is always possible to
             * insert a four byte separator after each digit. */
            *status |= MPD_Invalid_operation;
            mpd_free(result.data);
            goto error;
        }
        if (!_mpd_apply_lconv(&result, spec, status)) {
            goto error;
        }
    }

    if (spec->min_width) {
        if (!_mpd_add_pad(&result, spec, status)) {
            goto error;
        }
    }

    mpd_del(&tmp);
    return result.data;

error:
    mpd_del(&tmp);
    return NULL;
}

char *
mpd_qformat(const mpd_t *dec, const char *fmt, const mpd_context_t *ctx,
            uint32_t *status)
{
    mpd_spec_t spec;

    if (!mpd_parse_fmt_str(&spec, fmt, 1)) {
        *status |= MPD_Invalid_operation;
        return NULL;
    }

    return mpd_qformat_spec(dec, &spec, ctx, status);
}

/*
 * The specification has a *condition* called Invalid_operation and an
 * IEEE *signal* called Invalid_operation. The former corresponds to
 * MPD_Invalid_operation, the latter to MPD_IEEE_Invalid_operation.
 * MPD_IEEE_Invalid_operation comprises the following conditions:
 *
 * [MPD_Conversion_syntax, MPD_Division_impossible, MPD_Division_undefined,
 *  MPD_Fpu_error, MPD_Invalid_context, MPD_Invalid_operation,
 *  MPD_Malloc_error]
 *
 * In the following functions, 'flag' denotes the condition, 'signal'
 * denotes the IEEE signal.
 */

static const char *mpd_flag_string[MPD_NUM_FLAGS] = {
    "Clamped",
    "Conversion_syntax",
    "Division_by_zero",
    "Division_impossible",
    "Division_undefined",
    "Fpu_error",
    "Inexact",
    "Invalid_context",
    "Invalid_operation",
    "Malloc_error",
    "Not_implemented",
    "Overflow",
    "Rounded",
    "Subnormal",
    "Underflow",
};

static const char *mpd_signal_string[MPD_NUM_FLAGS] = {
    "Clamped",
    "IEEE_Invalid_operation",
    "Division_by_zero",
    "IEEE_Invalid_operation",
    "IEEE_Invalid_operation",
    "IEEE_Invalid_operation",
    "Inexact",
    "IEEE_Invalid_operation",
    "IEEE_Invalid_operation",
    "IEEE_Invalid_operation",
    "Not_implemented",
    "Overflow",
    "Rounded",
    "Subnormal",
    "Underflow",
};

/* print conditions to buffer, separated by spaces */
int
mpd_snprint_flags(char *dest, int nmemb, uint32_t flags)
{
    char *cp;
    int n, j;

    assert(nmemb >= MPD_MAX_FLAG_STRING);

    *dest = '\0'; cp = dest;
    for (j = 0; j < MPD_NUM_FLAGS; j++) {
        if (flags & (1U<<j)) {
            n = snprintf(cp, nmemb, "%s ", mpd_flag_string[j]);
            if (n < 0 || n >= nmemb) return -1;
            cp += n; nmemb -= n;
        }
    }

    if (cp != dest) {
        *(--cp) = '\0';
    }

    return (int)(cp-dest);
}

/* print conditions to buffer, in list form */
int
mpd_lsnprint_flags(char *dest, int nmemb, uint32_t flags, const char *flag_string[])
{
    char *cp;
    int n, j;

    assert(nmemb >= MPD_MAX_FLAG_LIST);
    if (flag_string == NULL) {
        flag_string = mpd_flag_string;
    }

    *dest = '[';
    *(dest+1) = '\0';
    cp = dest+1;
    --nmemb;

    for (j = 0; j < MPD_NUM_FLAGS; j++) {
        if (flags & (1U<<j)) {
            n = snprintf(cp, nmemb, "%s, ", flag_string[j]);
            if (n < 0 || n >= nmemb) return -1;
            cp += n; nmemb -= n;
        }
    }

    /* erase the last ", " */
    if (cp != dest+1) {
        cp -= 2;
    }

    *cp++ = ']';
    *cp = '\0';

    return (int)(cp-dest); /* strlen, without NUL terminator */
}

/* print signals to buffer, in list form */
int
mpd_lsnprint_signals(char *dest, int nmemb, uint32_t flags, const char *signal_string[])
{
    char *cp;
    int n, j;
    int ieee_invalid_done = 0;

    assert(nmemb >= MPD_MAX_SIGNAL_LIST);
    if (signal_string == NULL) {
        signal_string = mpd_signal_string;
    }

    *dest = '[';
    *(dest+1) = '\0';
    cp = dest+1;
    --nmemb;

    for (j = 0; j < MPD_NUM_FLAGS; j++) {
        uint32_t f = flags & (1U<<j);
        if (f) {
            if (f&MPD_IEEE_Invalid_operation) {
                if (ieee_invalid_done) {
                    continue;
                }
                ieee_invalid_done = 1;
            }
            n = snprintf(cp, nmemb, "%s, ", signal_string[j]);
            if (n < 0 || n >= nmemb) return -1;
            cp += n; nmemb -= n;
        }
    }

    /* erase the last ", " */
    if (cp != dest+1) {
        cp -= 2;
    }

    *cp++ = ']';
    *cp = '\0';

    return (int)(cp-dest); /* strlen, without NUL terminator */
}

/* The following two functions are mainly intended for debugging. */
void
mpd_fprint(FILE *file, const mpd_t *dec)
{
    char *decstring;

    decstring = mpd_to_sci(dec, 1);
    if (decstring != NULL) {
        fprintf(file, "%s\n", decstring);
        mpd_free(decstring);
    }
    else {
        fputs("mpd_fprint: output error\n", file); /* GCOV_NOT_REACHED */
    }
}

void
mpd_print(const mpd_t *dec)
{
    char *decstring;

    decstring = mpd_to_sci(dec, 1);
    if (decstring != NULL) {
        printf("%s\n", decstring);
        mpd_free(decstring);
    }
    else {
        fputs("mpd_fprint: output error\n", stderr); /* GCOV_NOT_REACHED */
    }
}


