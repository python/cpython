/*
 * Secret Labs' Regular Expression Engine
 *
 * regular expression matching engine
 *
 * partial history:
 * 1999-10-24 fl  created (based on existing template matcher code)
 * 2000-03-06 fl  first alpha, sort of (0.5)
 * 2000-06-30 fl  added fast search optimization (0.9.3)
 * 2000-06-30 fl  added assert (lookahead) primitives, etc (0.9.4)
 * 2000-07-02 fl  added charset optimizations, etc (0.9.5)
 * 2000-07-03 fl  store code in pattern object, lookbehind, etc
 * 2000-07-08 fl  added regs attribute
 * 2000-07-21 fl  reset lastindex in scanner methods (0.9.6)
 * 2000-08-01 fl  fixes for 1.6b1 (0.9.8)
 * 2000-08-03 fl  added recursion limit
 * 2000-08-07 fl  use PyOS_CheckStack() if available
 * 2000-08-08 fl  changed findall to return empty strings instead of None
 * 2000-08-27 fl  properly propagate memory errors
 * 2000-09-02 fl  return -1 instead of None for start/end/span
 * 2000-09-20 fl  added expand method
 * 2000-09-21 fl  don't use the buffer interface for unicode strings
 * 2000-10-03 fl  fixed assert_not primitive; support keyword arguments
 *
 * Copyright (c) 1997-2000 by Secret Labs AB.  All rights reserved.
 *
 * This version of the SRE library can be redistributed under CNRI's
 * Python 1.6 license.  For any other use, please contact Secret Labs
 * AB (info@pythonware.com).
 *
 * Portions of this engine have been developed in cooperation with
 * CNRI.  Hewlett-Packard provided funding for 1.6 integration and
 * other compatibility work.
 */

#ifndef SRE_RECURSIVE

char copyright[] = " SRE 0.9.8 Copyright (c) 1997-2000 by Secret Labs AB ";

#include "Python.h"

#include "sre.h"

#include <ctype.h>

/* name of this module, minus the leading underscore */
#define MODULE "sre"

/* defining this one enables tracing */
#undef VERBOSE

#if PY_VERSION_HEX >= 0x01060000
/* defining this enables unicode support (default under 1.6a1 and later) */
#define HAVE_UNICODE
#endif

/* -------------------------------------------------------------------- */
/* optional features */

/* prevent run-away recursion (bad patterns on long strings) */

#if !defined(USE_STACKCHECK)
#if defined(MS_WIN64) || defined(__LP64__) || defined(_LP64)
/* require smaller recursion limit for a number of 64-bit platforms:
   Win64 (MS_WIN64), Linux64 (__LP64__), Monterey (64-bit AIX) (_LP64) */
/* FIXME: maybe the limit should be 40000 / sizeof(void*) ? */
#define USE_RECURSION_LIMIT 7500
#else
#define USE_RECURSION_LIMIT 10000
#endif
#endif

/* enables fast searching */
#define USE_FAST_SEARCH

/* enables aggressive inlining (always on for Visual C) */
#undef USE_INLINE

/* -------------------------------------------------------------------- */

#if defined(_MSC_VER)
#pragma optimize("agtw", on) /* doesn't seem to make much difference... */
#pragma warning(disable: 4710) /* who cares if functions are not inlined ;-) */
/* fastest possible local call under MSVC */
#define LOCAL(type) static __inline type __fastcall
#elif defined(USE_INLINE)
#define LOCAL(type) static inline type
#else
#define LOCAL(type) static type
#endif

/* error codes */
#define SRE_ERROR_ILLEGAL -1 /* illegal opcode */
#define SRE_ERROR_STATE -2 /* illegal state */
#define SRE_ERROR_RECURSION_LIMIT -3 /* runaway recursion */
#define SRE_ERROR_MEMORY -9 /* out of memory */

#if defined(VERBOSE)
#define TRACE(v) printf v
#else
#define TRACE(v)
#endif

/* -------------------------------------------------------------------- */
/* search engine state */

/* default character predicates (run sre_chars.py to regenerate tables) */

#define SRE_DIGIT_MASK 1
#define SRE_SPACE_MASK 2
#define SRE_LINEBREAK_MASK 4
#define SRE_ALNUM_MASK 8
#define SRE_WORD_MASK 16

static char sre_char_info[128] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 6, 2,
2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 25, 25, 25, 25, 25, 25, 25, 25,
25, 25, 0, 0, 0, 0, 0, 0, 0, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 0, 0,
0, 0, 16, 0, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 0, 0, 0, 0, 0 };

static char sre_char_lower[128] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
61, 62, 63, 64, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
122, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105,
106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
120, 121, 122, 123, 124, 125, 126, 127 };

static unsigned int sre_lower(unsigned int ch)
{
    return ((ch) < 128 ? sre_char_lower[ch] : ch);
}

#define SRE_IS_DIGIT(ch)\
    ((ch) < 128 ? (sre_char_info[(ch)] & SRE_DIGIT_MASK) : 0)
#define SRE_IS_SPACE(ch)\
    ((ch) < 128 ? (sre_char_info[(ch)] & SRE_SPACE_MASK) : 0)
#define SRE_IS_LINEBREAK(ch)\
    ((ch) < 128 ? (sre_char_info[(ch)] & SRE_LINEBREAK_MASK) : 0)
#define SRE_IS_ALNUM(ch)\
    ((ch) < 128 ? (sre_char_info[(ch)] & SRE_ALNUM_MASK) : 0)
#define SRE_IS_WORD(ch)\
    ((ch) < 128 ? (sre_char_info[(ch)] & SRE_WORD_MASK) : 0)

/* locale-specific character predicates */

static unsigned int sre_lower_locale(unsigned int ch)
{
    return ((ch) < 256 ? tolower((ch)) : ch);
}
#define SRE_LOC_IS_DIGIT(ch) ((ch) < 256 ? isdigit((ch)) : 0)
#define SRE_LOC_IS_SPACE(ch) ((ch) < 256 ? isspace((ch)) : 0)
#define SRE_LOC_IS_LINEBREAK(ch) ((ch) == '\n')
#define SRE_LOC_IS_ALNUM(ch) ((ch) < 256 ? isalnum((ch)) : 0)
#define SRE_LOC_IS_WORD(ch) (SRE_LOC_IS_ALNUM((ch)) || (ch) == '_')

/* unicode-specific character predicates */

#if defined(HAVE_UNICODE)
static unsigned int sre_lower_unicode(unsigned int ch)
{
    return (unsigned int) Py_UNICODE_TOLOWER((Py_UNICODE)(ch));
}
#define SRE_UNI_IS_DIGIT(ch) Py_UNICODE_ISDIGIT((Py_UNICODE)(ch))
#define SRE_UNI_IS_SPACE(ch) Py_UNICODE_ISSPACE((Py_UNICODE)(ch))
#define SRE_UNI_IS_LINEBREAK(ch) Py_UNICODE_ISLINEBREAK((Py_UNICODE)(ch))
#define SRE_UNI_IS_ALNUM(ch) Py_UNICODE_ISALNUM((Py_UNICODE)(ch))
#define SRE_UNI_IS_WORD(ch) (SRE_UNI_IS_ALNUM((ch)) || (ch) == '_')
#endif

LOCAL(int)
sre_category(SRE_CODE category, unsigned int ch)
{
    switch (category) {

    case SRE_CATEGORY_DIGIT:
        return SRE_IS_DIGIT(ch);
    case SRE_CATEGORY_NOT_DIGIT:
        return !SRE_IS_DIGIT(ch);
    case SRE_CATEGORY_SPACE:
        return SRE_IS_SPACE(ch);
    case SRE_CATEGORY_NOT_SPACE:
        return !SRE_IS_SPACE(ch);
    case SRE_CATEGORY_WORD:
        return SRE_IS_WORD(ch);
    case SRE_CATEGORY_NOT_WORD:
        return !SRE_IS_WORD(ch);
    case SRE_CATEGORY_LINEBREAK:
        return SRE_IS_LINEBREAK(ch);
    case SRE_CATEGORY_NOT_LINEBREAK:
        return !SRE_IS_LINEBREAK(ch);

    case SRE_CATEGORY_LOC_WORD:
        return SRE_LOC_IS_WORD(ch);
    case SRE_CATEGORY_LOC_NOT_WORD:
        return !SRE_LOC_IS_WORD(ch);

#if defined(HAVE_UNICODE)
    case SRE_CATEGORY_UNI_DIGIT:
        return SRE_UNI_IS_DIGIT(ch);
    case SRE_CATEGORY_UNI_NOT_DIGIT:
        return !SRE_UNI_IS_DIGIT(ch);
    case SRE_CATEGORY_UNI_SPACE:
        return SRE_UNI_IS_SPACE(ch);
    case SRE_CATEGORY_UNI_NOT_SPACE:
        return !SRE_UNI_IS_SPACE(ch);
    case SRE_CATEGORY_UNI_WORD:
        return SRE_UNI_IS_WORD(ch);
    case SRE_CATEGORY_UNI_NOT_WORD:
        return !SRE_UNI_IS_WORD(ch);
    case SRE_CATEGORY_UNI_LINEBREAK:
        return SRE_UNI_IS_LINEBREAK(ch);
    case SRE_CATEGORY_UNI_NOT_LINEBREAK:
        return !SRE_UNI_IS_LINEBREAK(ch);
#endif
    }
    return 0;
}

/* helpers */

static void
mark_fini(SRE_STATE* state)
{
    if (state->mark_stack) {
        free(state->mark_stack);
        state->mark_stack = NULL;
    }
    state->mark_stack_size = state->mark_stack_base = 0;
}

static int
mark_save(SRE_STATE* state, int lo, int hi)
{
    void* stack;
    int size;
    int minsize, newsize;

    if (hi <= lo)
        return 0;

    size = (hi - lo) + 1;

    newsize = state->mark_stack_size;
    minsize = state->mark_stack_base + size;

    if (newsize < minsize) {
        /* create new stack */
        if (!newsize) {
            newsize = 512;
            if (newsize < minsize)
                newsize = minsize;
            TRACE(("allocate stack %d\n", newsize));
            stack = malloc(sizeof(void*) * newsize);
        } else {
            /* grow the stack */
            while (newsize < minsize)
                newsize += newsize;
            TRACE(("grow stack to %d\n", newsize));
            stack = realloc(state->mark_stack, sizeof(void*) * newsize);
        }
        if (!stack) {
            mark_fini(state);
            return SRE_ERROR_MEMORY;
        }
        state->mark_stack = stack;
        state->mark_stack_size = newsize;
    }

    TRACE(("copy %d:%d to %d (%d)\n", lo, hi, state->mark_stack_base, size));

    memcpy(state->mark_stack + state->mark_stack_base, state->mark + lo,
           size * sizeof(void*));

    state->mark_stack_base += size;

    return 0;
}

static int
mark_restore(SRE_STATE* state, int lo, int hi)
{
    int size;

    if (hi <= lo)
        return 0;

    size = (hi - lo) + 1;

    state->mark_stack_base -= size;

    TRACE(("copy %d:%d from %d\n", lo, hi, state->mark_stack_base));

    memcpy(state->mark + lo, state->mark_stack + state->mark_stack_base,
           size * sizeof(void*));

    return 0;
}

/* generate 8-bit version */

#define SRE_CHAR unsigned char
#define SRE_AT sre_at
#define SRE_COUNT sre_count
#define SRE_CHARSET sre_charset
#define SRE_INFO sre_info
#define SRE_MATCH sre_match
#define SRE_SEARCH sre_search

#if defined(HAVE_UNICODE)

#define SRE_RECURSIVE
#include "_sre.c"
#undef SRE_RECURSIVE

#undef SRE_SEARCH
#undef SRE_MATCH
#undef SRE_INFO
#undef SRE_CHARSET
#undef SRE_COUNT
#undef SRE_AT
#undef SRE_CHAR

/* generate 16-bit unicode version */

#define SRE_CHAR Py_UNICODE
#define SRE_AT sre_uat
#define SRE_COUNT sre_ucount
#define SRE_CHARSET sre_ucharset
#define SRE_INFO sre_uinfo
#define SRE_MATCH sre_umatch
#define SRE_SEARCH sre_usearch
#endif

#endif /* SRE_RECURSIVE */

/* -------------------------------------------------------------------- */
/* String matching engine */

/* the following section is compiled twice, with different character
   settings */

LOCAL(int)
SRE_AT(SRE_STATE* state, SRE_CHAR* ptr, SRE_CODE at)
{
    /* check if pointer is at given position */

    int this, that;

    switch (at) {

    case SRE_AT_BEGINNING:
        return ((void*) ptr == state->beginning);

    case SRE_AT_BEGINNING_LINE:
        return ((void*) ptr == state->beginning ||
                SRE_IS_LINEBREAK((int) ptr[-1]));

    case SRE_AT_END:
        return (((void*) (ptr+1) == state->end &&
                 SRE_IS_LINEBREAK((int) ptr[0])) ||
                ((void*) ptr == state->end));

    case SRE_AT_END_LINE:
        return ((void*) ptr == state->end ||
                SRE_IS_LINEBREAK((int) ptr[0]));

    case SRE_AT_BOUNDARY:
        if (state->beginning == state->end)
            return 0;
        that = ((void*) ptr > state->beginning) ?
            SRE_IS_WORD((int) ptr[-1]) : 0;
        this = ((void*) ptr < state->end) ?
            SRE_IS_WORD((int) ptr[0]) : 0;
        return this != that;

    case SRE_AT_NON_BOUNDARY:
        if (state->beginning == state->end)
            return 0;
        that = ((void*) ptr > state->beginning) ?
            SRE_IS_WORD((int) ptr[-1]) : 0;
        this = ((void*) ptr < state->end) ?
            SRE_IS_WORD((int) ptr[0]) : 0;
        return this == that;
    }

    return 0;
}

LOCAL(int)
SRE_CHARSET(SRE_CODE* set, SRE_CODE ch)
{
    /* check if character is a member of the given set */

    int ok = 1;

    for (;;) {
        switch (*set++) {

        case SRE_OP_LITERAL:
            /* <LITERAL> <code> */
            if (ch == set[0])
                return ok;
            set++;
            break;

        case SRE_OP_RANGE:
            /* <RANGE> <lower> <upper> */
            if (set[0] <= ch && ch <= set[1])
                return ok;
            set += 2;
            break;

        case SRE_OP_CHARSET:
            /* <CHARSET> <bitmap> (16 bits per code word) */
            if (ch < 256 && (set[ch >> 4] & (1 << (ch & 15))))
                return ok;
            set += 16;
            break;

        case SRE_OP_CATEGORY:
            /* <CATEGORY> <code> */
            if (sre_category(set[0], (int) ch))
                return ok;
            set += 1;
            break;

        case SRE_OP_NEGATE:
            ok = !ok;
            break;

        case SRE_OP_FAILURE:
            return !ok;

        default:
            /* internal error -- there's not much we can do about it
               here, so let's just pretend it didn't match... */
            return 0;
        }
    }
}

LOCAL(int) SRE_MATCH(SRE_STATE* state, SRE_CODE* pattern, int level);

LOCAL(int)
SRE_COUNT(SRE_STATE* state, SRE_CODE* pattern, int maxcount, int level)
{
    SRE_CODE chr;
    SRE_CHAR* ptr = state->ptr;
    SRE_CHAR* end = state->end;
    int i;

    /* adjust end */
    if (maxcount < end - ptr && maxcount != 65535)
        end = ptr + maxcount;

    switch (pattern[0]) {

    case SRE_OP_ANY:
        /* repeated dot wildcard. */
        TRACE(("|%p|%p|COUNT ANY\n", pattern, ptr));
        while (ptr < end && !SRE_IS_LINEBREAK(*ptr))
            ptr++;
        break;

    case SRE_OP_ANY_ALL:
        /* repeated dot wildcare.  skip to the end of the target
           string, and backtrack from there */
        TRACE(("|%p|%p|COUNT ANY_ALL\n", pattern, ptr));
        ptr = end;
        break;

    case SRE_OP_LITERAL:
        /* repeated literal */
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT LITERAL %d\n", pattern, ptr, chr));
        while (ptr < end && (SRE_CODE) *ptr == chr)
            ptr++;
        break;

    case SRE_OP_LITERAL_IGNORE:
        /* repeated literal */
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT LITERAL_IGNORE %d\n", pattern, ptr, chr));
        while (ptr < end && (SRE_CODE) state->lower(*ptr) == chr)
            ptr++;
        break;

    case SRE_OP_NOT_LITERAL:
        /* repeated non-literal */
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT NOT_LITERAL %d\n", pattern, ptr, chr));
        while (ptr < end && (SRE_CODE) *ptr != chr)
            ptr++;
        break;
                
    case SRE_OP_NOT_LITERAL_IGNORE:
        /* repeated non-literal */
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT NOT_LITERAL_IGNORE %d\n", pattern, ptr, chr));
        while (ptr < end && (SRE_CODE) state->lower(*ptr) != chr)
            ptr++;
        break;

    case SRE_OP_IN:
        /* repeated set */
        TRACE(("|%p|%p|COUNT IN\n", pattern, ptr));
        while (ptr < end && SRE_CHARSET(pattern + 2, *ptr))
            ptr++;
        break;

    default:
        /* repeated single character pattern */
        TRACE(("|%p|%p|COUNT SUBPATTERN\n", pattern, ptr));
        while ((SRE_CHAR*) state->ptr < end) {
            i = SRE_MATCH(state, pattern, level);
            if (i < 0)
                return i;
            if (!i)
                break;
        }
        TRACE(("|%p|%p|COUNT %d\n", pattern, ptr,
               (SRE_CHAR*) state->ptr - ptr));
        return (SRE_CHAR*) state->ptr - ptr;
    }

    TRACE(("|%p|%p|COUNT %d\n", pattern, ptr, ptr - (SRE_CHAR*) state->ptr));
    return ptr - (SRE_CHAR*) state->ptr;
}

#if 0 /* not used in this release */
LOCAL(int)
SRE_INFO(SRE_STATE* state, SRE_CODE* pattern)
{
    /* check if an SRE_OP_INFO block matches at the current position.
       returns the number of SRE_CODE objects to skip if successful, 0
       if no match */

    SRE_CHAR* end = state->end;
    SRE_CHAR* ptr = state->ptr;
    int i;

    /* check minimal length */
    if (pattern[3] && (end - ptr) < pattern[3])
        return 0;

    /* check known prefix */
    if (pattern[2] & SRE_INFO_PREFIX && pattern[5] > 1) {
        /* <length> <skip> <prefix data> <overlap data> */
        for (i = 0; i < pattern[5]; i++)
            if ((SRE_CODE) ptr[i] != pattern[7 + i])
                return 0;
        return pattern[0] + 2 * pattern[6];
    }
    return pattern[0];
}
#endif

LOCAL(int)
SRE_MATCH(SRE_STATE* state, SRE_CODE* pattern, int level)
{
    /* check if string matches the given pattern.  returns <0 for
       error, 0 for failure, and 1 for success */

    SRE_CHAR* end = state->end;
    SRE_CHAR* ptr = state->ptr;
    int i, count;
    SRE_REPEAT* rp;
    int lastmark;
    SRE_CODE chr;

    SRE_REPEAT rep; /* FIXME: <fl> allocate in STATE instead */

    TRACE(("|%p|%p|ENTER %d\n", pattern, ptr, level));

#if defined(USE_STACKCHECK)
    if (level % 10 == 0 && PyOS_CheckStack())
        return SRE_ERROR_RECURSION_LIMIT;
#endif

#if defined(USE_RECURSION_LIMIT)
    if (level > USE_RECURSION_LIMIT)
        return SRE_ERROR_RECURSION_LIMIT;
#endif

    if (pattern[0] == SRE_OP_INFO) {
        /* optimization info block */
        /* <INFO> <1=skip> <2=flags> <3=min> ... */
        if (pattern[3] && (end - ptr) < pattern[3]) {
            TRACE(("reject (got %d chars, need %d)\n",
                   (end - ptr), pattern[3]));
            return 0;
        }
        pattern += pattern[1] + 1;
    }

    for (;;) {

        switch (*pattern++) {

        case SRE_OP_FAILURE:
            /* immediate failure */
            TRACE(("|%p|%p|FAILURE\n", pattern, ptr));
            return 0;

        case SRE_OP_SUCCESS:
            /* end of pattern */
            TRACE(("|%p|%p|SUCCESS\n", pattern, ptr));
            state->ptr = ptr;
            return 1;

        case SRE_OP_AT:
            /* match at given position */
            /* <AT> <code> */
            TRACE(("|%p|%p|AT %d\n", pattern, ptr, *pattern));
            if (!SRE_AT(state, ptr, *pattern))
                return 0;
            pattern++;
            break;

        case SRE_OP_CATEGORY:
            /* match at given category */
            /* <CATEGORY> <code> */
            TRACE(("|%p|%p|CATEGORY %d\n", pattern, ptr, *pattern));
            if (ptr >= end || !sre_category(pattern[0], ptr[0]))
                return 0;
            pattern++;
            ptr++;
            break;

        case SRE_OP_LITERAL:
            /* match literal string */
            /* <LITERAL> <code> */
            TRACE(("|%p|%p|LITERAL %d\n", pattern, ptr, *pattern));
            if (ptr >= end || (SRE_CODE) ptr[0] != pattern[0])
                return 0;
            pattern++;
            ptr++;
            break;

        case SRE_OP_NOT_LITERAL:
            /* match anything that is not literal character */
            /* <NOT_LITERAL> <code> */
            TRACE(("|%p|%p|NOT_LITERAL %d\n", pattern, ptr, *pattern));
            if (ptr >= end || (SRE_CODE) ptr[0] == pattern[0])
                return 0;
            pattern++;
            ptr++;
            break;

        case SRE_OP_ANY:
            /* match anything (except a newline) */
            /* <ANY> */
            TRACE(("|%p|%p|ANY\n", pattern, ptr));
            if (ptr >= end || SRE_IS_LINEBREAK(ptr[0]))
                return 0;
            ptr++;
            break;

        case SRE_OP_ANY_ALL:
            /* match anything */
            /* <ANY_ALL> */
            TRACE(("|%p|%p|ANY_ALL\n", pattern, ptr));
            if (ptr >= end)
                return 0;
            ptr++;
            break;

        case SRE_OP_IN:
            /* match set member (or non_member) */
            /* <IN> <skip> <set> */
            TRACE(("|%p|%p|IN\n", pattern, ptr));
            if (ptr >= end || !SRE_CHARSET(pattern + 1, *ptr))
                return 0;
            pattern += pattern[0];
            ptr++;
            break;

        case SRE_OP_GROUPREF:
            /* match backreference */
            TRACE(("|%p|%p|GROUPREF %d\n", pattern, ptr, pattern[0]));
            i = pattern[0];
            {
                SRE_CHAR* p = (SRE_CHAR*) state->mark[i+i];
                SRE_CHAR* e = (SRE_CHAR*) state->mark[i+i+1];
                if (!p || !e || e < p)
                    return 0;
                while (p < e) {
                    if (ptr >= end || *ptr != *p)
                        return 0;
                    p++; ptr++;
                }
            }
            pattern++;
            break;

        case SRE_OP_GROUPREF_IGNORE:
            /* match backreference */
            TRACE(("|%p|%p|GROUPREF_IGNORE %d\n", pattern, ptr, pattern[0]));
            i = pattern[0];
            {
                SRE_CHAR* p = (SRE_CHAR*) state->mark[i+i];
                SRE_CHAR* e = (SRE_CHAR*) state->mark[i+i+1];
                if (!p || !e || e < p)
                    return 0;
                while (p < e) {
                    if (ptr >= end ||
                        state->lower(*ptr) != state->lower(*p))
                        return 0;
                    p++; ptr++;
                }
            }
            pattern++;
            break;

        case SRE_OP_LITERAL_IGNORE:
            TRACE(("|%p|%p|LITERAL_IGNORE %d\n", pattern, ptr, pattern[0]));
            if (ptr >= end ||
                state->lower(*ptr) != state->lower(*pattern))
                return 0;
            pattern++;
            ptr++;
            break;

        case SRE_OP_NOT_LITERAL_IGNORE:
            TRACE(("|%p|%p|NOT_LITERAL_IGNORE %d\n", pattern, ptr, *pattern));
            if (ptr >= end ||
                state->lower(*ptr) == state->lower(*pattern))
                return 0;
            pattern++;
            ptr++;
            break;

        case SRE_OP_IN_IGNORE:
            TRACE(("|%p|%p|IN_IGNORE\n", pattern, ptr));
            if (ptr >= end
                || !SRE_CHARSET(pattern + 1, (SRE_CODE) state->lower(*ptr)))
                return 0;
            pattern += pattern[0];
            ptr++;
            break;

        case SRE_OP_MARK:
            /* set mark */
            /* <MARK> <gid> */
            TRACE(("|%p|%p|MARK %d\n", pattern, ptr, pattern[0]));
            i = pattern[0];
            if (i & 1)
                state->lastindex = i/2 + 1;
            if (i > state->lastmark)
                state->lastmark = i;
            state->mark[i] = ptr;
            pattern++;
            break;

        case SRE_OP_JUMP:
        case SRE_OP_INFO:
            /* jump forward */
            /* <JUMP> <offset> */
            TRACE(("|%p|%p|JUMP %d\n", pattern, ptr, pattern[0]));
            pattern += pattern[0];
            break;

        case SRE_OP_ASSERT:
            /* assert subpattern */
            /* <ASSERT> <skip> <back> <pattern> */
            TRACE(("|%p|%p|ASSERT %d\n", pattern, ptr, pattern[1]));
            state->ptr = ptr - pattern[1];
            if (state->ptr < state->beginning)
                return 0;
            i = SRE_MATCH(state, pattern + 2, level + 1);
            if (i <= 0)
                return i;
            pattern += pattern[0];
            break;

        case SRE_OP_ASSERT_NOT:
            /* assert not subpattern */
            /* <ASSERT_NOT> <skip> <back> <pattern> */
            TRACE(("|%p|%p|ASSERT_NOT %d\n", pattern, ptr, pattern[1]));
            state->ptr = ptr - pattern[1];
            if (state->ptr < state->beginning)
                return 0;
            i = SRE_MATCH(state, pattern + 2, level + 1);
            if (i < 0)
                return i;
            if (i)
                return 0;
            pattern += pattern[0];
            break;

        case SRE_OP_BRANCH:
            /* alternation */
            /* <BRANCH> <0=skip> code <JUMP> ... <NULL> */
            TRACE(("|%p|%p|BRANCH\n", pattern, ptr));
            lastmark = state->lastmark;
            for (; pattern[0]; pattern += pattern[0]) {
                if (pattern[1] == SRE_OP_LITERAL &&
                    (ptr >= end || (SRE_CODE) *ptr != pattern[2]))
                    continue;
                if (pattern[1] == SRE_OP_IN &&
                    (ptr >= end || !SRE_CHARSET(pattern + 3, (SRE_CODE) *ptr)))
                    continue;
                state->ptr = ptr;
                i = SRE_MATCH(state, pattern + 1, level + 1);
                if (i)
                    return i;
                if (state->lastmark > lastmark) {
                    memset(
                        state->mark + lastmark + 1, 0,
                        (state->lastmark - lastmark) * sizeof(void*)
                        );
                    state->lastmark = lastmark;
                }
            }
            return 0;

        case SRE_OP_REPEAT_ONE:
            /* match repeated sequence (maximizing regexp) */

            /* this operator only works if the repeated item is
               exactly one character wide, and we're not already
               collecting backtracking points.  for other cases,
               use the MAX_REPEAT operator instead */

            /* <REPEAT_ONE> <skip> <1=min> <2=max> item <SUCCESS> tail */

            TRACE(("|%p|%p|REPEAT_ONE %d %d\n", pattern, ptr,
                   pattern[1], pattern[2]));

            if (ptr + pattern[1] > end)
                return 0; /* cannot match */

            state->ptr = ptr;

            count = SRE_COUNT(state, pattern + 3, pattern[2], level + 1);
            if (count < 0)
                return count;

            ptr += count;

            /* when we arrive here, count contains the number of
               matches, and ptr points to the tail of the target
               string.  check if the rest of the pattern matches,
               and backtrack if not. */

            if (count < (int) pattern[1])
                return 0;

            if (pattern[pattern[0]] == SRE_OP_SUCCESS) {
                /* tail is empty.  we're finished */
                state->ptr = ptr;
                return 1;

            } else if (pattern[pattern[0]] == SRE_OP_LITERAL) {
                /* tail starts with a literal. skip positions where
                   the rest of the pattern cannot possibly match */
                chr = pattern[pattern[0]+1];
                for (;;) {
                    while (count >= (int) pattern[1] &&
                           (ptr >= end || *ptr != chr)) {
                        ptr--;
                        count--;
                    }
                    if (count < (int) pattern[1])
                        break;
                    state->ptr = ptr;
                    i = SRE_MATCH(state, pattern + pattern[0], level + 1);
                    if (i)
                        return i;
                    ptr--;
                    count--;
                }

            } else {
                /* general case */
                lastmark = state->lastmark;
                while (count >= (int) pattern[1]) {
                    state->ptr = ptr;
                    i = SRE_MATCH(state, pattern + pattern[0], level + 1);
                    if (i)
                        return i;
                    ptr--;
                    count--;
                    if (state->lastmark > lastmark) {
                        memset(
                            state->mark + lastmark + 1, 0,
                            (state->lastmark - lastmark) * sizeof(void*)
                            );
                        state->lastmark = lastmark;
                    }
                }
            }
            return 0;

        case SRE_OP_REPEAT:
            /* create repeat context.  all the hard work is done
               by the UNTIL operator */
            /* <REPEAT> <skip> <1=min> <2=max> item <UNTIL> tail */
            TRACE(("|%p|%p|REPEAT %d %d\n", pattern, ptr,
                   pattern[1], pattern[2]));

            rep.count = -1;
            rep.pattern = pattern;

            /* install new repeat context */
            rep.prev = state->repeat;
            state->repeat = &rep;

            state->ptr = ptr;
            i = SRE_MATCH(state, pattern + pattern[0], level + 1);

            state->repeat = rep.prev;

            return i;

        case SRE_OP_MAX_UNTIL:
            /* maximizing repeat */
            /* <REPEAT> <skip> <1=min> <2=max> item <MAX_UNTIL> tail */

            /* FIXME: we probably need to deal with zero-width
               matches in here... */

            rp = state->repeat;
            if (!rp)
                return SRE_ERROR_STATE;

            state->ptr = ptr;

            count = rp->count + 1;

            TRACE(("|%p|%p|MAX_UNTIL %d\n", pattern, ptr, count));

            if (count < rp->pattern[1]) {
                /* not enough matches */
                rp->count = count;
                /* RECURSIVE */
                i = SRE_MATCH(state, rp->pattern + 3, level + 1);
                if (i)
                    return i;
                rp->count = count - 1;
                state->ptr = ptr;
                return 0;
            }

            if (count < rp->pattern[2] || rp->pattern[2] == 65535) {
                /* we may have enough matches, but if we can
                   match another item, do so */
                rp->count = count;
                lastmark = state->lastmark;
                i = mark_save(state, 0, lastmark);
                if (i < 0)
                    return i;
                /* RECURSIVE */
                i = SRE_MATCH(state, rp->pattern + 3, level + 1);
                if (i)
                    return i;
                i = mark_restore(state, 0, lastmark);
                if (i < 0)
                    return i;
                rp->count = count - 1;
                state->ptr = ptr;
            }

            /* cannot match more repeated items here.  make sure the
               tail matches */
            state->repeat = rp->prev;
            i = SRE_MATCH(state, pattern, level + 1);
            if (i)
                return i;
            state->repeat = rp;
            return 0;

        case SRE_OP_MIN_UNTIL:
            /* minimizing repeat */
            /* <REPEAT> <skip> <1=min> <2=max> item <MIN_UNTIL> tail */

            rp = state->repeat;
            if (!rp)
                return SRE_ERROR_STATE;

            count = rp->count + 1;

            TRACE(("|%p|%p|MIN_UNTIL %d\n", pattern, ptr, count));

            state->ptr = ptr;

            if (count < rp->pattern[1]) {
                /* not enough matches */
                rp->count = count;
                /* RECURSIVE */
                i = SRE_MATCH(state, rp->pattern + 3, level + 1);
                if (i)
                    return i;
                rp->count = count-1;
                state->ptr = ptr;
                return 0;
            }

            /* see if the tail matches */
            state->repeat = rp->prev;
            i = SRE_MATCH(state, pattern, level + 1);
            if (i) {
                /* free(rp); */
                return i;
            }
            state->repeat = rp;

            if (count >= rp->pattern[2] && rp->pattern[2] != 65535)
                return 0;

            rp->count = count;
            /* RECURSIVE */
            i = SRE_MATCH(state, rp->pattern + 3, level + 1);
            if (i)
                return i;
            rp->count = count - 1;
            return 0;

        default:
            TRACE(("|%p|%p|UNKNOWN %d\n", pattern, ptr, pattern[-1]));
            return SRE_ERROR_ILLEGAL;
        }
    }

    /* shouldn't end up here */
    return SRE_ERROR_ILLEGAL;
}

LOCAL(int)
SRE_SEARCH(SRE_STATE* state, SRE_CODE* pattern)
{
    SRE_CHAR* ptr = state->start;
    SRE_CHAR* end = state->end;
    int status = 0;
    int prefix_len = 0;
    int prefix_skip = 0;
    SRE_CODE* prefix = NULL;
    SRE_CODE* charset = NULL;
    SRE_CODE* overlap = NULL;
    int flags = 0;

    if (pattern[0] == SRE_OP_INFO) {
        /* optimization info block */
        /* <INFO> <1=skip> <2=flags> <3=min> <4=max> <5=prefix info>  */

        flags = pattern[2];

        if (pattern[3] > 0) {
            /* adjust end point (but make sure we leave at least one
               character in there, so literal search will work) */
            end -= pattern[3]-1;
            if (end <= ptr)
                end = ptr+1;
        }

        if (flags & SRE_INFO_PREFIX) {
            /* pattern starts with a known prefix */
            /* <length> <skip> <prefix data> <overlap data> */
            prefix_len = pattern[5];
            prefix_skip = pattern[6];
            prefix = pattern + 7;
            overlap = prefix + prefix_len - 1;
        } else if (flags & SRE_INFO_CHARSET)
            /* pattern starts with a character from a known set */
            /* <charset> */
            charset = pattern + 5;

        pattern += 1 + pattern[1];
    }

    TRACE(("prefix = %p %d %d\n", prefix, prefix_len, prefix_skip));
    TRACE(("charset = %p\n", charset));

#if defined(USE_FAST_SEARCH)
    if (prefix_len > 1) {
        /* pattern starts with a known prefix.  use the overlap
           table to skip forward as fast as we possibly can */
        int i = 0;
        end = state->end;
        while (ptr < end) {
            for (;;) {
                if ((SRE_CODE) ptr[0] != prefix[i]) {
                    if (!i)
                        break;
                    else
                        i = overlap[i];
                } else {
                    if (++i == prefix_len) {
                        /* found a potential match */
                        TRACE(("|%p|%p|SEARCH SCAN\n", pattern, ptr));
                        state->start = ptr + 1 - prefix_len;
                        state->ptr = ptr + 1 - prefix_len + prefix_skip;
                        if (flags & SRE_INFO_LITERAL)
                            return 1; /* we got all of it */
                        status = SRE_MATCH(state, pattern + 2*prefix_skip, 1);
                        if (status != 0)
                            return status;
                        /* close but no cigar -- try again */
                        i = overlap[i];
                    }
                    break;
                }
                
            }
            ptr++;
        }
        return 0;
    }
#endif

    if (pattern[0] == SRE_OP_LITERAL) {
        /* pattern starts with a literal character.  this is used
           for short prefixes, and if fast search is disabled */
        SRE_CODE chr = pattern[1];
        end = state->end;
        for (;;) {
            while (ptr < end && (SRE_CODE) ptr[0] != chr)
                ptr++;
            if (ptr == end)
                return 0;
            TRACE(("|%p|%p|SEARCH LITERAL\n", pattern, ptr));
            state->start = ptr;
            state->ptr = ++ptr;
            status = SRE_MATCH(state, pattern + 2, 1);
            if (status != 0)
                break;
        }
    } else if (charset) {
        /* pattern starts with a character from a known set */
        end = state->end;
        for (;;) {
            while (ptr < end && !SRE_CHARSET(charset, ptr[0]))
                ptr++;
            if (ptr == end)
                return 0;
            TRACE(("|%p|%p|SEARCH CHARSET\n", pattern, ptr));
            state->start = ptr;
            state->ptr = ptr;
            status = SRE_MATCH(state, pattern, 1);
            if (status != 0)
                break;
            ptr++;
        }
    } else
        /* general case */
        while (ptr <= end) {
            TRACE(("|%p|%p|SEARCH\n", pattern, ptr));
            state->start = state->ptr = ptr++;
            status = SRE_MATCH(state, pattern, 1);
            if (status != 0)
                break;
        }

    return status;
}
    

#if !defined(SRE_RECURSIVE)

/* -------------------------------------------------------------------- */
/* factories and destructors */

/* see sre.h for object declarations */

staticforward PyTypeObject Pattern_Type;
staticforward PyTypeObject Match_Type;
staticforward PyTypeObject Scanner_Type;

static PyObject *
_compile(PyObject* self_, PyObject* args)
{
    /* "compile" pattern descriptor to pattern object */

    PatternObject* self;
    int i, n;

    PyObject* pattern;
    int flags = 0;
    PyObject* code;
    int groups = 0;
    PyObject* groupindex = NULL;
    PyObject* indexgroup = NULL;
    if (!PyArg_ParseTuple(args, "OiO|iOO", &pattern, &flags, &code,
                          &groups, &groupindex, &indexgroup))
        return NULL;

    code = PySequence_Fast(code, "code argument must be a sequence");
    if (!code)
        return NULL;

#if PY_VERSION_HEX >= 0x01060000
    n = PySequence_Size(code);
#else
    n = PySequence_Length(code);
#endif

    self = PyObject_NEW_VAR(PatternObject, &Pattern_Type, 100*n);
    if (!self) {
        Py_DECREF(code);
        return NULL;
    }

    for (i = 0; i < n; i++) {
        PyObject *o = PySequence_Fast_GET_ITEM(code, i);
        self->code[i] = (SRE_CODE) PyInt_AsLong(o);
    }

    Py_DECREF(code);

    if (PyErr_Occurred())
        return NULL;

    Py_INCREF(pattern);
    self->pattern = pattern;

    self->flags = flags;

    self->groups = groups;

    Py_XINCREF(groupindex);
    self->groupindex = groupindex;

    Py_XINCREF(indexgroup);
    self->indexgroup = indexgroup;

    return (PyObject*) self;
}

static PyObject *
sre_codesize(PyObject* self, PyObject* args)
{
    return Py_BuildValue("i", sizeof(SRE_CODE));
}

static PyObject *
sre_getlower(PyObject* self, PyObject* args)
{
    int character, flags;
    if (!PyArg_ParseTuple(args, "ii", &character, &flags))
        return NULL;
    if (flags & SRE_FLAG_LOCALE)
        return Py_BuildValue("i", sre_lower_locale(character));
#if defined(HAVE_UNICODE)
    if (flags & SRE_FLAG_UNICODE)
        return Py_BuildValue("i", sre_lower_unicode(character));
#endif
    return Py_BuildValue("i", sre_lower(character));
}

LOCAL(void)
state_reset(SRE_STATE* state)
{
    int i;

    state->lastmark = 0;

    /* FIXME: dynamic! */
    for (i = 0; i < SRE_MARK_SIZE; i++)
        state->mark[i] = NULL;

    state->lastindex = -1;

    state->repeat = NULL;

    mark_fini(state);
}

LOCAL(PyObject*)
state_init(SRE_STATE* state, PatternObject* pattern, PyObject* string,
           int start, int end)
{
    /* prepare state object */

    PyBufferProcs *buffer;
    int size, bytes;
    void* ptr;

    memset(state, 0, sizeof(SRE_STATE));

    state->lastindex = -1;

#if defined(HAVE_UNICODE)
    if (PyUnicode_Check(string)) {
        /* unicode strings doesn't always support the buffer interface */
        ptr = (void*) PyUnicode_AS_DATA(string);
        bytes = PyUnicode_GET_DATA_SIZE(string);
        size = PyUnicode_GET_SIZE(string);
        state->charsize = sizeof(Py_UNICODE);

    } else {
#endif

    /* get pointer to string buffer */
    buffer = string->ob_type->tp_as_buffer;
    if (!buffer || !buffer->bf_getreadbuffer || !buffer->bf_getsegcount ||
        buffer->bf_getsegcount(string, NULL) != 1) {
        PyErr_SetString(PyExc_TypeError, "expected string or buffer");
        return NULL;
    }

    /* determine buffer size */
    bytes = buffer->bf_getreadbuffer(string, 0, &ptr);
    if (bytes < 0) {
        PyErr_SetString(PyExc_TypeError, "buffer has negative size");
        return NULL;
    }

    /* determine character size */
#if PY_VERSION_HEX >= 0x01060000
    size = PyObject_Size(string);
#else
    size = PyObject_Length(string);
#endif

    if (PyString_Check(string) || bytes == size)
        state->charsize = 1;
#if defined(HAVE_UNICODE)
    else if (bytes == (int) (size * sizeof(Py_UNICODE)))
        state->charsize = sizeof(Py_UNICODE);
#endif
    else {
        PyErr_SetString(PyExc_TypeError, "buffer size mismatch");
        return NULL;
    }

#if defined(HAVE_UNICODE)
    }
#endif

    /* adjust boundaries */
    if (start < 0)
        start = 0;
    else if (start > size)
        start = size;

    if (end < 0)
        end = 0;
    else if (end > size)
        end = size;

    state->beginning = ptr;

    state->start = (void*) ((char*) ptr + start * state->charsize);
    state->end = (void*) ((char*) ptr + end * state->charsize);

    Py_INCREF(string);
    state->string = string;
    state->pos = start;
    state->endpos = end;

    if (pattern->flags & SRE_FLAG_LOCALE)
        state->lower = sre_lower_locale;
#if defined(HAVE_UNICODE)
    else if (pattern->flags & SRE_FLAG_UNICODE)
        state->lower = sre_lower_unicode;
#endif
    else
        state->lower = sre_lower;

    return string;
}

LOCAL(void)
state_fini(SRE_STATE* state)
{
    Py_XDECREF(state->string);
    mark_fini(state);
}

LOCAL(PyObject*)
state_getslice(SRE_STATE* state, int index, PyObject* string)
{
    int i, j;

    index = (index - 1) * 2;

    if (string == Py_None || !state->mark[index] || !state->mark[index+1]) {
        i = j = 0;
    } else {
        i = ((char*)state->mark[index] - (char*)state->beginning) /
            state->charsize;
        j = ((char*)state->mark[index+1] - (char*)state->beginning) /
            state->charsize;
    }

    return PySequence_GetSlice(string, i, j);
}

static void
pattern_error(int status)
{
    switch (status) {
    case SRE_ERROR_RECURSION_LIMIT:
        PyErr_SetString(
            PyExc_RuntimeError,
            "maximum recursion limit exceeded"
            );
        break;
    case SRE_ERROR_MEMORY:
        PyErr_NoMemory();
        break;
    default:
        /* other error codes indicate compiler/engine bugs */
        PyErr_SetString(
            PyExc_RuntimeError,
            "internal error in regular expression engine"
            );
    }
}

static PyObject*
pattern_new_match(PatternObject* pattern, SRE_STATE* state, int status)
{
    /* create match object (from state object) */

    MatchObject* match;
    int i, j;
    char* base;
    int n;

    if (status > 0) {

        /* create match object (with room for extra group marks) */
        match = PyObject_NEW_VAR(MatchObject, &Match_Type,
                                 2*(pattern->groups+1));
        if (!match)
            return NULL;

        Py_INCREF(pattern);
        match->pattern = pattern;

        Py_INCREF(state->string);
        match->string = state->string;

        match->regs = NULL;
        match->groups = pattern->groups+1;

        /* fill in group slices */

        base = (char*) state->beginning;
        n = state->charsize;

        match->mark[0] = ((char*) state->start - base) / n;
        match->mark[1] = ((char*) state->ptr - base) / n;

        for (i = j = 0; i < pattern->groups; i++, j+=2)
            if (j+1 <= state->lastmark && state->mark[j] && state->mark[j+1]) {
                match->mark[j+2] = ((char*) state->mark[j] - base) / n;
                match->mark[j+3] = ((char*) state->mark[j+1] - base) / n;
            } else
                match->mark[j+2] = match->mark[j+3] = -1; /* undefined */

        match->pos = state->pos;
        match->endpos = state->endpos;

        match->lastindex = state->lastindex;

        return (PyObject*) match;

    } else if (status == 0) {

        /* no match */
        Py_INCREF(Py_None);
        return Py_None;

    }

    /* internal error */
    pattern_error(status);
    return NULL;
}

static PyObject*
pattern_scanner(PatternObject* pattern, PyObject* args)
{
    /* create search state object */

    ScannerObject* self;

    PyObject* string;
    int start = 0;
    int end = INT_MAX;
    if (!PyArg_ParseTuple(args, "O|ii:scanner", &string, &start, &end))
        return NULL;

    /* create scanner object */
    self = PyObject_NEW(ScannerObject, &Scanner_Type);
    if (!self)
        return NULL;

    string = state_init(&self->state, pattern, string, start, end);
    if (!string) {
        PyObject_Del(self);
        return NULL;
    }

    Py_INCREF(pattern);
    self->pattern = (PyObject*) pattern;

    return (PyObject*) self;
}

static void
pattern_dealloc(PatternObject* self)
{
    Py_XDECREF(self->pattern);
    Py_XDECREF(self->groupindex);
    PyObject_DEL(self);
}

static PyObject*
pattern_match(PatternObject* self, PyObject* args, PyObject* kw)
{
    SRE_STATE state;
    int status;

    PyObject* string;
    int start = 0;
    int end = INT_MAX;
    static char* kwlist[] = { "pattern", "pos", "endpos", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|ii:match", kwlist,
                                     &string, &start, &end))
        return NULL;

    string = state_init(&state, self, string, start, end);
    if (!string)
        return NULL;

    state.ptr = state.start;

    TRACE(("|%p|%p|MATCH\n", PatternObject_GetCode(self), state.ptr));

    if (state.charsize == 1) {
        status = sre_match(&state, PatternObject_GetCode(self), 1);
    } else {
#if defined(HAVE_UNICODE)
        status = sre_umatch(&state, PatternObject_GetCode(self), 1);
#endif
    }

    TRACE(("|%p|%p|END\n", PatternObject_GetCode(self), state.ptr));

    state_fini(&state);

    return pattern_new_match(self, &state, status);
}

static PyObject*
pattern_search(PatternObject* self, PyObject* args, PyObject* kw)
{
    SRE_STATE state;
    int status;

    PyObject* string;
    int start = 0;
    int end = INT_MAX;
    static char* kwlist[] = { "pattern", "pos", "endpos", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|ii:search", kwlist,
                                     &string, &start, &end))
        return NULL;

    string = state_init(&state, self, string, start, end);
    if (!string)
        return NULL;

    TRACE(("|%p|%p|SEARCH\n", PatternObject_GetCode(self), state.ptr));

    if (state.charsize == 1) {
        status = sre_search(&state, PatternObject_GetCode(self));
    } else {
#if defined(HAVE_UNICODE)
        status = sre_usearch(&state, PatternObject_GetCode(self));
#endif
    }

    TRACE(("|%p|%p|END\n", PatternObject_GetCode(self), state.ptr));

    state_fini(&state);

    return pattern_new_match(self, &state, status);
}

static PyObject*
call(char* function, PyObject* args)
{
    PyObject* name;
    PyObject* module;
    PyObject* func;
    PyObject* result;

    name = PyString_FromString(MODULE);
    if (!name)
        return NULL;
    module = PyImport_Import(name);
    Py_DECREF(name);
    if (!module)
        return NULL;
    func = PyObject_GetAttrString(module, function);
    Py_DECREF(module);
    if (!func)
        return NULL;
    result = PyObject_CallObject(func, args);
    Py_DECREF(func);
    Py_DECREF(args);
    return result;
}

static PyObject*
pattern_sub(PatternObject* self, PyObject* args, PyObject* kw)
{
    PyObject* template;
    PyObject* string;
    PyObject* count = Py_False; /* zero */
    static char* kwlist[] = { "repl", "string", "count", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO|O:sub", kwlist,
                                     &template, &string, &count))
        return NULL;

    /* delegate to Python code */
    return call("_sub", Py_BuildValue("OOOO", self, template, string, count));
}

static PyObject*
pattern_subn(PatternObject* self, PyObject* args, PyObject* kw)
{
    PyObject* template;
    PyObject* string;
    PyObject* count = Py_False; /* zero */
    static char* kwlist[] = { "repl", "string", "count", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO|O:subn", kwlist,
                                     &template, &string, &count))
        return NULL;

    /* delegate to Python code */
    return call("_subn", Py_BuildValue("OOOO", self, template, string, count));
}

static PyObject*
pattern_split(PatternObject* self, PyObject* args, PyObject* kw)
{
    PyObject* string;
    PyObject* maxsplit = Py_False; /* zero */
    static char* kwlist[] = { "source", "maxsplit", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O:split", kwlist,
                                     &string, &maxsplit))
        return NULL;

    /* delegate to Python code */
    return call("_split", Py_BuildValue("OOO", self, string, maxsplit));
}

static PyObject*
pattern_findall(PatternObject* self, PyObject* args, PyObject* kw)
{
    SRE_STATE state;
    PyObject* list;
    int status;
    int i;

    PyObject* string;
    int start = 0;
    int end = INT_MAX;
    static char* kwlist[] = { "source", "pos", "endpos", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|ii:findall", kwlist,
                                     &string, &start, &end))
        return NULL;

    string = state_init(&state, self, string, start, end);
    if (!string)
        return NULL;

    list = PyList_New(0);

    while (state.start <= state.end) {

        PyObject* item;
        
        state.ptr = state.start;

        if (state.charsize == 1) {
            status = sre_search(&state, PatternObject_GetCode(self));
        } else {
#if defined(HAVE_UNICODE)
            status = sre_usearch(&state, PatternObject_GetCode(self));
#endif
        }

        if (status > 0) {

            /* don't bother to build a match object */
            switch (self->groups) {
            case 0:
                item = PySequence_GetSlice(
                    string,
                    ((char*) state.start - (char*) state.beginning) /
                    state.charsize,
                    ((char*) state.ptr - (char*) state.beginning) /
                    state.charsize);
                if (!item)
                    goto error;
                break;
            case 1:
                item = state_getslice(&state, 1, string);
                if (!item)
                    goto error;
                break;
            default:
                item = PyTuple_New(self->groups);
                if (!item)
                    goto error;
                for (i = 0; i < self->groups; i++) {
                    PyObject* o = state_getslice(&state, i+1, string);
                    if (!o) {
                        Py_DECREF(item);
                        goto error;
                    }
                    PyTuple_SET_ITEM(item, i, o);
                }
                break;
            }

            status = PyList_Append(list, item);
            Py_DECREF(item);

            if (status < 0)
                goto error;

            if (state.ptr == state.start)
                state.start = (void*) ((char*) state.ptr + state.charsize);
            else
                state.start = state.ptr;

        } else {

            if (status == 0)
                break;

            pattern_error(status);
            goto error;

        }
    }

    state_fini(&state);
    return list;

error:
    Py_DECREF(list);
    state_fini(&state);
    return NULL;
    
}

static PyMethodDef pattern_methods[] = {
    {"match", (PyCFunction) pattern_match, METH_VARARGS|METH_KEYWORDS},
    {"search", (PyCFunction) pattern_search, METH_VARARGS|METH_KEYWORDS},
    {"sub", (PyCFunction) pattern_sub, METH_VARARGS|METH_KEYWORDS},
    {"subn", (PyCFunction) pattern_subn, METH_VARARGS|METH_KEYWORDS},
    {"split", (PyCFunction) pattern_split, METH_VARARGS|METH_KEYWORDS},
    {"findall", (PyCFunction) pattern_findall, METH_VARARGS|METH_KEYWORDS},
    /* experimental */
    {"scanner", (PyCFunction) pattern_scanner, METH_VARARGS},
    {NULL, NULL}
};

static PyObject*  
pattern_getattr(PatternObject* self, char* name)
{
    PyObject* res;

    res = Py_FindMethod(pattern_methods, (PyObject*) self, name);

    if (res)
        return res;

    PyErr_Clear();

    /* attributes */
    if (!strcmp(name, "pattern")) {
        Py_INCREF(self->pattern);
        return self->pattern;
    }

    if (!strcmp(name, "flags"))
        return Py_BuildValue("i", self->flags);

    if (!strcmp(name, "groups"))
        return Py_BuildValue("i", self->groups);

    if (!strcmp(name, "groupindex") && self->groupindex) {
        Py_INCREF(self->groupindex);
        return self->groupindex;
    }

    PyErr_SetString(PyExc_AttributeError, name);
    return NULL;
}

statichere PyTypeObject Pattern_Type = {
    PyObject_HEAD_INIT(NULL)
    0, "SRE_Pattern",
    sizeof(PatternObject), sizeof(SRE_CODE),
    (destructor)pattern_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)pattern_getattr /*tp_getattr*/
};

/* -------------------------------------------------------------------- */
/* match methods */

static void
match_dealloc(MatchObject* self)
{
    Py_XDECREF(self->regs);
    Py_XDECREF(self->string);
    Py_DECREF(self->pattern);
    PyObject_DEL(self);
}

static PyObject*
match_getslice_by_index(MatchObject* self, int index, PyObject* def)
{
    if (index < 0 || index >= self->groups) {
        /* raise IndexError if we were given a bad group number */
        PyErr_SetString(
            PyExc_IndexError,
            "no such group"
            );
        return NULL;
    }

    index *= 2;

    if (self->string == Py_None || self->mark[index] < 0) {
        /* return default value if the string or group is undefined */
        Py_INCREF(def);
        return def;
    }

    return PySequence_GetSlice(
        self->string, self->mark[index], self->mark[index+1]
        );
}

static int
match_getindex(MatchObject* self, PyObject* index)
{
    int i;

    if (PyInt_Check(index))
        return (int) PyInt_AS_LONG(index);

    i = -1;

    if (self->pattern->groupindex) {
        index = PyObject_GetItem(self->pattern->groupindex, index);
        if (index) {
            if (PyInt_Check(index))
                i = (int) PyInt_AS_LONG(index);
            Py_DECREF(index);
        } else
            PyErr_Clear();
    }

    return i;
}

static PyObject*
match_getslice(MatchObject* self, PyObject* index, PyObject* def)
{
    return match_getslice_by_index(self, match_getindex(self, index), def);
}

static PyObject*
match_expand(MatchObject* self, PyObject* args)
{
    PyObject* template;
    if (!PyArg_ParseTuple(args, "O:expand", &template))
        return NULL;

    /* delegate to Python code */
    return call(
        "_expand",
        Py_BuildValue("OOO", self->pattern, self, template)
        );
}

static PyObject*
match_group(MatchObject* self, PyObject* args)
{
    PyObject* result;
    int i, size;

    size = PyTuple_GET_SIZE(args);

    switch (size) {
    case 0:
        result = match_getslice(self, Py_False, Py_None);
        break;
    case 1:
        result = match_getslice(self, PyTuple_GET_ITEM(args, 0), Py_None);
        break;
    default:
        /* fetch multiple items */
        result = PyTuple_New(size);
        if (!result)
            return NULL;
        for (i = 0; i < size; i++) {
            PyObject* item = match_getslice(
                self, PyTuple_GET_ITEM(args, i), Py_None
                );
            if (!item) {
                Py_DECREF(result);
                return NULL;
            }
            PyTuple_SET_ITEM(result, i, item);
        }
        break;
    }
    return result;
}

static PyObject*
match_groups(MatchObject* self, PyObject* args, PyObject* kw)
{
    PyObject* result;
    int index;

    PyObject* def = Py_None;
    static char* kwlist[] = { "default", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O:groups", kwlist, &def))
        return NULL;

    result = PyTuple_New(self->groups-1);
    if (!result)
        return NULL;

    for (index = 1; index < self->groups; index++) {
        PyObject* item;
        item = match_getslice_by_index(self, index, def);
        if (!item) {
            Py_DECREF(result);
            return NULL;
        }
        PyTuple_SET_ITEM(result, index-1, item);
    }

    return result;
}

static PyObject*
match_groupdict(MatchObject* self, PyObject* args, PyObject* kw)
{
    PyObject* result;
    PyObject* keys;
    int index;

    PyObject* def = Py_None;
    static char* kwlist[] = { "default", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O:groups", kwlist, &def))
        return NULL;

    result = PyDict_New();
    if (!result || !self->pattern->groupindex)
        return result;

    keys = PyMapping_Keys(self->pattern->groupindex);
    if (!keys) {
        Py_DECREF(result);
        return NULL;
    }

    for (index = 0; index < PyList_GET_SIZE(keys); index++) {
        PyObject* key;
        PyObject* item;
        key = PyList_GET_ITEM(keys, index);
        if (!key) {
            Py_DECREF(keys);
            Py_DECREF(result);
            return NULL;
        }
        item = match_getslice(self, key, def);
        if (!item) {
            Py_DECREF(key);
            Py_DECREF(keys);
            Py_DECREF(result);
            return NULL;
        }
        /* FIXME: <fl> this can fail, right? */
        PyDict_SetItem(result, key, item);
    }

    Py_DECREF(keys);

    return result;
}

static PyObject*
match_start(MatchObject* self, PyObject* args)
{
    int index;

    PyObject* index_ = Py_False; /* zero */
    if (!PyArg_ParseTuple(args, "|O:start", &index_))
        return NULL;

    index = match_getindex(self, index_);

    if (index < 0 || index >= self->groups) {
        PyErr_SetString(
            PyExc_IndexError,
            "no such group"
            );
        return NULL;
    }

    /* mark is -1 if group is undefined */
    return Py_BuildValue("i", self->mark[index*2]);
}

static PyObject*
match_end(MatchObject* self, PyObject* args)
{
    int index;

    PyObject* index_ = Py_False; /* zero */
    if (!PyArg_ParseTuple(args, "|O:end", &index_))
        return NULL;

    index = match_getindex(self, index_);

    if (index < 0 || index >= self->groups) {
        PyErr_SetString(
            PyExc_IndexError,
            "no such group"
            );
        return NULL;
    }

    /* mark is -1 if group is undefined */
    return Py_BuildValue("i", self->mark[index*2+1]);
}

LOCAL(PyObject*)
_pair(int i1, int i2)
{
    PyObject* pair;
    PyObject* item;

    pair = PyTuple_New(2);
    if (!pair)
        return NULL;

    item = PyInt_FromLong(i1);
    if (!item)
        goto error;
    PyTuple_SET_ITEM(pair, 0, item);

    item = PyInt_FromLong(i2);
    if (!item)
        goto error;
    PyTuple_SET_ITEM(pair, 1, item);

    return pair;

  error:
    Py_DECREF(pair);
    return NULL;
}

static PyObject*
match_span(MatchObject* self, PyObject* args)
{
    int index;

    PyObject* index_ = Py_False; /* zero */
    if (!PyArg_ParseTuple(args, "|O:span", &index_))
        return NULL;

    index = match_getindex(self, index_);

    if (index < 0 || index >= self->groups) {
        PyErr_SetString(
            PyExc_IndexError,
            "no such group"
            );
        return NULL;
    }

    /* marks are -1 if group is undefined */
    return _pair(self->mark[index*2], self->mark[index*2+1]);
}

static PyObject*
match_regs(MatchObject* self)
{
    PyObject* regs;
    PyObject* item;
    int index;

    regs = PyTuple_New(self->groups);
    if (!regs)
        return NULL;

    for (index = 0; index < self->groups; index++) {
        item = _pair(self->mark[index*2], self->mark[index*2+1]);
        if (!item) {
            Py_DECREF(regs);
            return NULL;
        }
        PyTuple_SET_ITEM(regs, index, item);
    }

    Py_INCREF(regs);
    self->regs = regs;

    return regs;
}

static PyMethodDef match_methods[] = {
    {"group", (PyCFunction) match_group, METH_VARARGS},
    {"start", (PyCFunction) match_start, METH_VARARGS},
    {"end", (PyCFunction) match_end, METH_VARARGS},
    {"span", (PyCFunction) match_span, METH_VARARGS},
    {"groups", (PyCFunction) match_groups, METH_VARARGS|METH_KEYWORDS},
    {"groupdict", (PyCFunction) match_groupdict, METH_VARARGS|METH_KEYWORDS},
    {"expand", (PyCFunction) match_expand, METH_VARARGS},
    {NULL, NULL}
};

static PyObject*  
match_getattr(MatchObject* self, char* name)
{
    PyObject* res;

    res = Py_FindMethod(match_methods, (PyObject*) self, name);
    if (res)
        return res;

    PyErr_Clear();

    if (!strcmp(name, "lastindex")) {
        if (self->lastindex >= 0)
            return Py_BuildValue("i", self->lastindex);
        Py_INCREF(Py_None);
        return Py_None;
    }

    if (!strcmp(name, "lastgroup")) {
        if (self->pattern->indexgroup && self->lastindex >= 0) {
            PyObject* result = PySequence_GetItem(
                self->pattern->indexgroup, self->lastindex
                );
            if (result)
                return result;
            PyErr_Clear();
        }
        Py_INCREF(Py_None);
        return Py_None;
    }

    if (!strcmp(name, "string")) {
        if (self->string) {
            Py_INCREF(self->string);
            return self->string;
        } else {
            Py_INCREF(Py_None);
            return Py_None;
        }
    }

    if (!strcmp(name, "regs")) {
        if (self->regs) {
            Py_INCREF(self->regs);
            return self->regs;
        } else
            return match_regs(self);
    }

    if (!strcmp(name, "re")) {
        Py_INCREF(self->pattern);
        return (PyObject*) self->pattern;
    }

    if (!strcmp(name, "pos"))
        return Py_BuildValue("i", self->pos);

    if (!strcmp(name, "endpos"))
        return Py_BuildValue("i", self->endpos);

    PyErr_SetString(PyExc_AttributeError, name);
    return NULL;
}

/* FIXME: implement setattr("string", None) as a special case (to
   detach the associated string, if any */

statichere PyTypeObject Match_Type = {
    PyObject_HEAD_INIT(NULL)
    0, "SRE_Match",
    sizeof(MatchObject), sizeof(int),
    (destructor)match_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)match_getattr /*tp_getattr*/
};

/* -------------------------------------------------------------------- */
/* scanner methods (experimental) */

static void
scanner_dealloc(ScannerObject* self)
{
    state_fini(&self->state);
    Py_DECREF(self->pattern);
    PyObject_DEL(self);
}

static PyObject*
scanner_match(ScannerObject* self, PyObject* args)
{
    SRE_STATE* state = &self->state;
    PyObject* match;
    int status;

    state_reset(state);

    state->ptr = state->start;

    if (state->charsize == 1) {
        status = sre_match(state, PatternObject_GetCode(self->pattern), 1);
    } else {
#if defined(HAVE_UNICODE)
        status = sre_umatch(state, PatternObject_GetCode(self->pattern), 1);
#endif
    }

    match = pattern_new_match((PatternObject*) self->pattern,
                               state, status);

    if (status == 0 || state->ptr == state->start)
        state->start = (void*) ((char*) state->ptr + state->charsize);
    else
        state->start = state->ptr;

    return match;
}


static PyObject*
scanner_search(ScannerObject* self, PyObject* args)
{
    SRE_STATE* state = &self->state;
    PyObject* match;
    int status;

    state_reset(state);

    state->ptr = state->start;

    if (state->charsize == 1) {
        status = sre_search(state, PatternObject_GetCode(self->pattern));
    } else {
#if defined(HAVE_UNICODE)
        status = sre_usearch(state, PatternObject_GetCode(self->pattern));
#endif
    }

    match = pattern_new_match((PatternObject*) self->pattern,
                               state, status);

    if (status == 0 || state->ptr == state->start)
        state->start = (void*) ((char*) state->ptr + state->charsize);
    else
        state->start = state->ptr;

    return match;
}

static PyMethodDef scanner_methods[] = {
    {"match", (PyCFunction) scanner_match, 0},
    {"search", (PyCFunction) scanner_search, 0},
    {NULL, NULL}
};

static PyObject*  
scanner_getattr(ScannerObject* self, char* name)
{
    PyObject* res;

    res = Py_FindMethod(scanner_methods, (PyObject*) self, name);
    if (res)
        return res;

    PyErr_Clear();

    /* attributes */
    if (!strcmp(name, "pattern")) {
        Py_INCREF(self->pattern);
        return self->pattern;
    }

    PyErr_SetString(PyExc_AttributeError, name);
    return NULL;
}

statichere PyTypeObject Scanner_Type = {
    PyObject_HEAD_INIT(NULL)
    0, "SRE_Scanner",
    sizeof(ScannerObject), 0,
    (destructor)scanner_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)scanner_getattr, /*tp_getattr*/
};

static PyMethodDef _functions[] = {
    {"compile", _compile, 1},
    {"getcodesize", sre_codesize, 1},
    {"getlower", sre_getlower, 1},
    {NULL, NULL}
};

void
#if defined(WIN32)
__declspec(dllexport)
#endif
init_sre(void)
{
    /* Patch object types */
    Pattern_Type.ob_type = Match_Type.ob_type =
        Scanner_Type.ob_type = &PyType_Type;

    Py_InitModule("_" MODULE, _functions);
}

#endif /* !defined(SRE_RECURSIVE) */
