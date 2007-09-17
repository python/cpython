/*
 * Secret Labs' Regular Expression Engine
 *
 * regular expression matching engine
 *
 * partial history:
 * 1999-10-24 fl  created (based on existing template matcher code)
 * 2000-03-06 fl  first alpha, sort of
 * 2000-08-01 fl  fixes for 1.6b1
 * 2000-08-07 fl  use PyOS_CheckStack() if available
 * 2000-09-20 fl  added expand method
 * 2001-03-20 fl  lots of fixes for 2.1b2
 * 2001-04-15 fl  export copyright as Python attribute, not global
 * 2001-04-28 fl  added __copy__ methods (work in progress)
 * 2001-05-14 fl  fixes for 1.5.2 compatibility
 * 2001-07-01 fl  added BIGCHARSET support (from Martin von Loewis)
 * 2001-10-18 fl  fixed group reset issue (from Matthew Mueller)
 * 2001-10-20 fl  added split primitive; reenable unicode for 1.6/2.0/2.1
 * 2001-10-21 fl  added sub/subn primitive
 * 2001-10-24 fl  added finditer primitive (for 2.2 only)
 * 2001-12-07 fl  fixed memory leak in sub/subn (Guido van Rossum)
 * 2002-11-09 fl  fixed empty sub/subn return type
 * 2003-04-18 mvl fully support 4-byte codes
 * 2003-10-17 gn  implemented non recursive scheme
 *
 * Copyright (c) 1997-2001 by Secret Labs AB.  All rights reserved.
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

static char copyright[] =
    " SRE 2.2.2 Copyright (c) 1997-2002 by Secret Labs AB ";

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "structmember.h" /* offsetof */

#include "sre.h"

#include <ctype.h>

/* name of this module, minus the leading underscore */
#if !defined(SRE_MODULE)
#define SRE_MODULE "sre"
#endif

#define SRE_PY_MODULE "re"

/* defining this one enables tracing */
#undef VERBOSE

#if PY_VERSION_HEX >= 0x01060000
#if PY_VERSION_HEX  < 0x02020000 || defined(Py_USING_UNICODE)
/* defining this enables unicode support (default under 1.6a1 and later) */
#define HAVE_UNICODE
#endif
#endif

/* -------------------------------------------------------------------- */
/* optional features */

/* enables fast searching */
#define USE_FAST_SEARCH

/* enables aggressive inlining (always on for Visual C) */
#undef USE_INLINE

/* enables copy/deepcopy handling (work in progress) */
#undef USE_BUILTIN_COPY

#if PY_VERSION_HEX < 0x01060000
#define PyObject_DEL(op) PyMem_DEL((op))
#endif

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

/* FIXME: this assumes ASCII.  create tables in init_sre() instead */

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

static unsigned int sre_lower(unsigned int ch)
{
    return ((ch) < 128 ? (unsigned int)sre_char_lower[ch] : ch);
}

/* locale-specific character predicates */
/* !(c & ~N) == (c < N+1) for any unsigned c, this avoids
 * warnings when c's type supports only numbers < N+1 */
#define SRE_LOC_IS_DIGIT(ch) (!((ch) & ~255) ? isdigit((ch)) : 0)
#define SRE_LOC_IS_SPACE(ch) (!((ch) & ~255) ? isspace((ch)) : 0)
#define SRE_LOC_IS_LINEBREAK(ch) ((ch) == '\n')
#define SRE_LOC_IS_ALNUM(ch) (!((ch) & ~255) ? isalnum((ch)) : 0)
#define SRE_LOC_IS_WORD(ch) (SRE_LOC_IS_ALNUM((ch)) || (ch) == '_')

static unsigned int sre_lower_locale(unsigned int ch)
{
    return ((ch) < 256 ? (unsigned int)tolower((ch)) : ch);
}

/* unicode-specific character predicates */

#if defined(HAVE_UNICODE)

#define SRE_UNI_IS_DIGIT(ch) Py_UNICODE_ISDIGIT((Py_UNICODE)(ch))
#define SRE_UNI_IS_SPACE(ch) Py_UNICODE_ISSPACE((Py_UNICODE)(ch))
#define SRE_UNI_IS_LINEBREAK(ch) Py_UNICODE_ISLINEBREAK((Py_UNICODE)(ch))
#define SRE_UNI_IS_ALNUM(ch) Py_UNICODE_ISALNUM((Py_UNICODE)(ch))
#define SRE_UNI_IS_WORD(ch) (SRE_UNI_IS_ALNUM((ch)) || (ch) == '_')

static unsigned int sre_lower_unicode(unsigned int ch)
{
    return (unsigned int) Py_UNICODE_TOLOWER((Py_UNICODE)(ch));
}

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
#else
    case SRE_CATEGORY_UNI_DIGIT:
        return SRE_IS_DIGIT(ch);
    case SRE_CATEGORY_UNI_NOT_DIGIT:
        return !SRE_IS_DIGIT(ch);
    case SRE_CATEGORY_UNI_SPACE:
        return SRE_IS_SPACE(ch);
    case SRE_CATEGORY_UNI_NOT_SPACE:
        return !SRE_IS_SPACE(ch);
    case SRE_CATEGORY_UNI_WORD:
        return SRE_LOC_IS_WORD(ch);
    case SRE_CATEGORY_UNI_NOT_WORD:
        return !SRE_LOC_IS_WORD(ch);
    case SRE_CATEGORY_UNI_LINEBREAK:
        return SRE_IS_LINEBREAK(ch);
    case SRE_CATEGORY_UNI_NOT_LINEBREAK:
        return !SRE_IS_LINEBREAK(ch);
#endif
    }
    return 0;
}

/* helpers */

static void
data_stack_dealloc(SRE_STATE* state)
{
    if (state->data_stack) {
        PyMem_FREE(state->data_stack);
        state->data_stack = NULL;
    }
    state->data_stack_size = state->data_stack_base = 0;
}

static int
data_stack_grow(SRE_STATE* state, Py_ssize_t size)
{
    Py_ssize_t minsize, cursize;
    minsize = state->data_stack_base+size;
    cursize = state->data_stack_size;
    if (cursize < minsize) {
        void* stack;
        cursize = minsize+minsize/4+1024;
        TRACE(("allocate/grow stack %d\n", cursize));
        stack = PyMem_REALLOC(state->data_stack, cursize);
        if (!stack) {
            data_stack_dealloc(state);
            return SRE_ERROR_MEMORY;
        }
        state->data_stack = (char *)stack;
        state->data_stack_size = cursize;
    }
    return 0;
}

/* generate 8-bit version */

#define SRE_CHAR unsigned char
#define SRE_AT sre_at
#define SRE_COUNT sre_count
#define SRE_CHARSET sre_charset
#define SRE_INFO sre_info
#define SRE_MATCH sre_match
#define SRE_MATCH_CONTEXT sre_match_context
#define SRE_SEARCH sre_search
#define SRE_LITERAL_TEMPLATE sre_literal_template

#if defined(HAVE_UNICODE)

#define SRE_RECURSIVE
#include "_sre.c"
#undef SRE_RECURSIVE

#undef SRE_LITERAL_TEMPLATE
#undef SRE_SEARCH
#undef SRE_MATCH
#undef SRE_MATCH_CONTEXT
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
#define SRE_MATCH_CONTEXT sre_umatch_context
#define SRE_SEARCH sre_usearch
#define SRE_LITERAL_TEMPLATE sre_uliteral_template
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

    Py_ssize_t thisp, thatp;

    switch (at) {

    case SRE_AT_BEGINNING:
    case SRE_AT_BEGINNING_STRING:
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

    case SRE_AT_END_STRING:
        return ((void*) ptr == state->end);

    case SRE_AT_BOUNDARY:
        if (state->beginning == state->end)
            return 0;
        thatp = ((void*) ptr > state->beginning) ?
            SRE_IS_WORD((int) ptr[-1]) : 0;
        thisp = ((void*) ptr < state->end) ?
            SRE_IS_WORD((int) ptr[0]) : 0;
        return thisp != thatp;

    case SRE_AT_NON_BOUNDARY:
        if (state->beginning == state->end)
            return 0;
        thatp = ((void*) ptr > state->beginning) ?
            SRE_IS_WORD((int) ptr[-1]) : 0;
        thisp = ((void*) ptr < state->end) ?
            SRE_IS_WORD((int) ptr[0]) : 0;
        return thisp == thatp;

    case SRE_AT_LOC_BOUNDARY:
        if (state->beginning == state->end)
            return 0;
        thatp = ((void*) ptr > state->beginning) ?
            SRE_LOC_IS_WORD((int) ptr[-1]) : 0;
        thisp = ((void*) ptr < state->end) ?
            SRE_LOC_IS_WORD((int) ptr[0]) : 0;
        return thisp != thatp;

    case SRE_AT_LOC_NON_BOUNDARY:
        if (state->beginning == state->end)
            return 0;
        thatp = ((void*) ptr > state->beginning) ?
            SRE_LOC_IS_WORD((int) ptr[-1]) : 0;
        thisp = ((void*) ptr < state->end) ?
            SRE_LOC_IS_WORD((int) ptr[0]) : 0;
        return thisp == thatp;

#if defined(HAVE_UNICODE)
    case SRE_AT_UNI_BOUNDARY:
        if (state->beginning == state->end)
            return 0;
        thatp = ((void*) ptr > state->beginning) ?
            SRE_UNI_IS_WORD((int) ptr[-1]) : 0;
        thisp = ((void*) ptr < state->end) ?
            SRE_UNI_IS_WORD((int) ptr[0]) : 0;
        return thisp != thatp;

    case SRE_AT_UNI_NON_BOUNDARY:
        if (state->beginning == state->end)
            return 0;
        thatp = ((void*) ptr > state->beginning) ?
            SRE_UNI_IS_WORD((int) ptr[-1]) : 0;
        thisp = ((void*) ptr < state->end) ?
            SRE_UNI_IS_WORD((int) ptr[0]) : 0;
        return thisp == thatp;
#endif

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

        case SRE_OP_FAILURE:
            return !ok;

        case SRE_OP_LITERAL:
            /* <LITERAL> <code> */
            if (ch == set[0])
                return ok;
            set++;
            break;

        case SRE_OP_CATEGORY:
            /* <CATEGORY> <code> */
            if (sre_category(set[0], (int) ch))
                return ok;
            set += 1;
            break;

        case SRE_OP_CHARSET:
            if (sizeof(SRE_CODE) == 2) {
                /* <CHARSET> <bitmap> (16 bits per code word) */
                if (ch < 256 && (set[ch >> 4] & (1 << (ch & 15))))
                    return ok;
                set += 16;
            }
            else {
                /* <CHARSET> <bitmap> (32 bits per code word) */
                if (ch < 256 && (set[ch >> 5] & (1 << (ch & 31))))
                    return ok;
                set += 8;
            }
            break;

        case SRE_OP_RANGE:
            /* <RANGE> <lower> <upper> */
            if (set[0] <= ch && ch <= set[1])
                return ok;
            set += 2;
            break;

        case SRE_OP_NEGATE:
            ok = !ok;
            break;

        case SRE_OP_BIGCHARSET:
            /* <BIGCHARSET> <blockcount> <256 blockindices> <blocks> */
        {
            Py_ssize_t count, block;
            count = *(set++);

            if (sizeof(SRE_CODE) == 2) {
                block = ((unsigned char*)set)[ch >> 8];
                set += 128;
                if (set[block*16 + ((ch & 255)>>4)] & (1 << (ch & 15)))
                    return ok;
                set += count*16;
            }
            else {
                /* !(c & ~N) == (c < N+1) for any unsigned c, this avoids
                 * warnings when c's type supports only numbers < N+1 */
                if (!(ch & ~65535))
                    block = ((unsigned char*)set)[ch >> 8];
                else
                    block = -1;
                set += 64;
                if (block >=0 &&
                    (set[block*8 + ((ch & 255)>>5)] & (1 << (ch & 31))))
                    return ok;
                set += count*8;
            }
            break;
        }

        default:
            /* internal error -- there's not much we can do about it
               here, so let's just pretend it didn't match... */
            return 0;
        }
    }
}

LOCAL(Py_ssize_t) SRE_MATCH(SRE_STATE* state, SRE_CODE* pattern);

LOCAL(Py_ssize_t)
SRE_COUNT(SRE_STATE* state, SRE_CODE* pattern, Py_ssize_t maxcount)
{
    SRE_CODE chr;
    SRE_CHAR* ptr = (SRE_CHAR *)state->ptr;
    SRE_CHAR* end = (SRE_CHAR *)state->end;
    Py_ssize_t i;

    /* adjust end */
    if (maxcount < end - ptr && maxcount != 65535)
        end = ptr + maxcount;

    switch (pattern[0]) {

    case SRE_OP_IN:
        /* repeated set */
        TRACE(("|%p|%p|COUNT IN\n", pattern, ptr));
        while (ptr < end && SRE_CHARSET(pattern + 2, *ptr))
            ptr++;
        break;

    case SRE_OP_ANY:
        /* repeated dot wildcard. */
        TRACE(("|%p|%p|COUNT ANY\n", pattern, ptr));
        while (ptr < end && !SRE_IS_LINEBREAK(*ptr))
            ptr++;
        break;

    case SRE_OP_ANY_ALL:
        /* repeated dot wildcard.  skip to the end of the target
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

    default:
        /* repeated single character pattern */
        TRACE(("|%p|%p|COUNT SUBPATTERN\n", pattern, ptr));
        while ((SRE_CHAR*) state->ptr < end) {
            i = SRE_MATCH(state, pattern);
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
    Py_ssize_t i;

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

/* The macros below should be used to protect recursive SRE_MATCH()
 * calls that *failed* and do *not* return immediately (IOW, those
 * that will backtrack). Explaining:
 *
 * - Recursive SRE_MATCH() returned true: that's usually a success
 *   (besides atypical cases like ASSERT_NOT), therefore there's no
 *   reason to restore lastmark;
 *
 * - Recursive SRE_MATCH() returned false but the current SRE_MATCH()
 *   is returning to the caller: If the current SRE_MATCH() is the
 *   top function of the recursion, returning false will be a matching
 *   failure, and it doesn't matter where lastmark is pointing to.
 *   If it's *not* the top function, it will be a recursive SRE_MATCH()
 *   failure by itself, and the calling SRE_MATCH() will have to deal
 *   with the failure by the same rules explained here (it will restore
 *   lastmark by itself if necessary);
 *
 * - Recursive SRE_MATCH() returned false, and will continue the
 *   outside 'for' loop: must be protected when breaking, since the next
 *   OP could potentially depend on lastmark;
 *
 * - Recursive SRE_MATCH() returned false, and will be called again
 *   inside a local for/while loop: must be protected between each
 *   loop iteration, since the recursive SRE_MATCH() could do anything,
 *   and could potentially depend on lastmark.
 *
 * For more information, check the discussion at SF patch #712900.
 */
#define LASTMARK_SAVE()     \
    do { \
        ctx->lastmark = state->lastmark; \
        ctx->lastindex = state->lastindex; \
    } while (0)
#define LASTMARK_RESTORE()  \
    do { \
        state->lastmark = ctx->lastmark; \
        state->lastindex = ctx->lastindex; \
    } while (0)

#define RETURN_ERROR(i) do { return i; } while(0)
#define RETURN_FAILURE do { ret = 0; goto exit; } while(0)
#define RETURN_SUCCESS do { ret = 1; goto exit; } while(0)

#define RETURN_ON_ERROR(i) \
    do { if (i < 0) RETURN_ERROR(i); } while (0)
#define RETURN_ON_SUCCESS(i) \
    do { RETURN_ON_ERROR(i); if (i > 0) RETURN_SUCCESS; } while (0)
#define RETURN_ON_FAILURE(i) \
    do { RETURN_ON_ERROR(i); if (i == 0) RETURN_FAILURE; } while (0)

#define SFY(x) #x

#define DATA_STACK_ALLOC(state, type, ptr) \
do { \
    alloc_pos = state->data_stack_base; \
    TRACE(("allocating %s in %d (%d)\n", \
           SFY(type), alloc_pos, sizeof(type))); \
    if (state->data_stack_size < alloc_pos+sizeof(type)) { \
        int j = data_stack_grow(state, sizeof(type)); \
        if (j < 0) return j; \
        if (ctx_pos != -1) \
            DATA_STACK_LOOKUP_AT(state, SRE_MATCH_CONTEXT, ctx, ctx_pos); \
    } \
    ptr = (type*)(state->data_stack+alloc_pos); \
    state->data_stack_base += sizeof(type); \
} while (0)

#define DATA_STACK_LOOKUP_AT(state, type, ptr, pos) \
do { \
    TRACE(("looking up %s at %d\n", SFY(type), pos)); \
    ptr = (type*)(state->data_stack+pos); \
} while (0)

#define DATA_STACK_PUSH(state, data, size) \
do { \
    TRACE(("copy data in %p to %d (%d)\n", \
           data, state->data_stack_base, size)); \
    if (state->data_stack_size < state->data_stack_base+size) { \
        int j = data_stack_grow(state, size); \
        if (j < 0) return j; \
        if (ctx_pos != -1) \
            DATA_STACK_LOOKUP_AT(state, SRE_MATCH_CONTEXT, ctx, ctx_pos); \
    } \
    memcpy(state->data_stack+state->data_stack_base, data, size); \
    state->data_stack_base += size; \
} while (0)

#define DATA_STACK_POP(state, data, size, discard) \
do { \
    TRACE(("copy data to %p from %d (%d)\n", \
           data, state->data_stack_base-size, size)); \
    memcpy(data, state->data_stack+state->data_stack_base-size, size); \
    if (discard) \
        state->data_stack_base -= size; \
} while (0)

#define DATA_STACK_POP_DISCARD(state, size) \
do { \
    TRACE(("discard data from %d (%d)\n", \
           state->data_stack_base-size, size)); \
    state->data_stack_base -= size; \
} while(0)

#define DATA_PUSH(x) \
    DATA_STACK_PUSH(state, (x), sizeof(*(x)))
#define DATA_POP(x) \
    DATA_STACK_POP(state, (x), sizeof(*(x)), 1)
#define DATA_POP_DISCARD(x) \
    DATA_STACK_POP_DISCARD(state, sizeof(*(x)))
#define DATA_ALLOC(t,p) \
    DATA_STACK_ALLOC(state, t, p)
#define DATA_LOOKUP_AT(t,p,pos) \
    DATA_STACK_LOOKUP_AT(state,t,p,pos)

#define MARK_PUSH(lastmark) \
    do if (lastmark > 0) { \
        i = lastmark; /* ctx->lastmark may change if reallocated */ \
        DATA_STACK_PUSH(state, state->mark, (i+1)*sizeof(void*)); \
    } while (0)
#define MARK_POP(lastmark) \
    do if (lastmark > 0) { \
        DATA_STACK_POP(state, state->mark, (lastmark+1)*sizeof(void*), 1); \
    } while (0)
#define MARK_POP_KEEP(lastmark) \
    do if (lastmark > 0) { \
        DATA_STACK_POP(state, state->mark, (lastmark+1)*sizeof(void*), 0); \
    } while (0)
#define MARK_POP_DISCARD(lastmark) \
    do if (lastmark > 0) { \
        DATA_STACK_POP_DISCARD(state, (lastmark+1)*sizeof(void*)); \
    } while (0)

#define JUMP_NONE            0
#define JUMP_MAX_UNTIL_1     1
#define JUMP_MAX_UNTIL_2     2
#define JUMP_MAX_UNTIL_3     3
#define JUMP_MIN_UNTIL_1     4
#define JUMP_MIN_UNTIL_2     5
#define JUMP_MIN_UNTIL_3     6
#define JUMP_REPEAT          7
#define JUMP_REPEAT_ONE_1    8
#define JUMP_REPEAT_ONE_2    9
#define JUMP_MIN_REPEAT_ONE  10
#define JUMP_BRANCH          11
#define JUMP_ASSERT          12
#define JUMP_ASSERT_NOT      13

#define DO_JUMP(jumpvalue, jumplabel, nextpattern) \
    DATA_ALLOC(SRE_MATCH_CONTEXT, nextctx); \
    nextctx->last_ctx_pos = ctx_pos; \
    nextctx->jump = jumpvalue; \
    nextctx->pattern = nextpattern; \
    ctx_pos = alloc_pos; \
    ctx = nextctx; \
    goto entrance; \
    jumplabel: \
    while (0) /* gcc doesn't like labels at end of scopes */ \

typedef struct {
    Py_ssize_t last_ctx_pos;
    Py_ssize_t jump;
    SRE_CHAR* ptr;
    SRE_CODE* pattern;
    Py_ssize_t count;
    Py_ssize_t lastmark;
    Py_ssize_t lastindex;
    union {
        SRE_CODE chr;
        SRE_REPEAT* rep;
    } u;
} SRE_MATCH_CONTEXT;

/* check if string matches the given pattern.  returns <0 for
   error, 0 for failure, and 1 for success */
LOCAL(Py_ssize_t)
SRE_MATCH(SRE_STATE* state, SRE_CODE* pattern)
{
    SRE_CHAR* end = (SRE_CHAR *)state->end;
    Py_ssize_t alloc_pos, ctx_pos = -1;
    Py_ssize_t i, ret = 0;
    Py_ssize_t jump;

    SRE_MATCH_CONTEXT* ctx;
    SRE_MATCH_CONTEXT* nextctx;

    TRACE(("|%p|%p|ENTER\n", pattern, state->ptr));

    DATA_ALLOC(SRE_MATCH_CONTEXT, ctx);
    ctx->last_ctx_pos = -1;
    ctx->jump = JUMP_NONE;
    ctx->pattern = pattern;
    ctx_pos = alloc_pos;

entrance:

    ctx->ptr = (SRE_CHAR *)state->ptr;

    if (ctx->pattern[0] == SRE_OP_INFO) {
        /* optimization info block */
        /* <INFO> <1=skip> <2=flags> <3=min> ... */
        if (ctx->pattern[3] && (end - ctx->ptr) < ctx->pattern[3]) {
            TRACE(("reject (got %d chars, need %d)\n",
                   (end - ctx->ptr), ctx->pattern[3]));
            RETURN_FAILURE;
        }
        ctx->pattern += ctx->pattern[1] + 1;
    }

    for (;;) {

        switch (*ctx->pattern++) {

        case SRE_OP_MARK:
            /* set mark */
            /* <MARK> <gid> */
            TRACE(("|%p|%p|MARK %d\n", ctx->pattern,
                   ctx->ptr, ctx->pattern[0]));
            i = ctx->pattern[0];
            if (i & 1)
                state->lastindex = i/2 + 1;
            if (i > state->lastmark) {
                /* state->lastmark is the highest valid index in the
                   state->mark array.  If it is increased by more than 1,
                   the intervening marks must be set to NULL to signal
                   that these marks have not been encountered. */
                Py_ssize_t j = state->lastmark + 1;
                while (j < i)
                    state->mark[j++] = NULL;
                state->lastmark = i;
            }
            state->mark[i] = ctx->ptr;
            ctx->pattern++;
            break;

        case SRE_OP_LITERAL:
            /* match literal string */
            /* <LITERAL> <code> */
            TRACE(("|%p|%p|LITERAL %d\n", ctx->pattern,
                   ctx->ptr, *ctx->pattern));
            if (ctx->ptr >= end || (SRE_CODE) ctx->ptr[0] != ctx->pattern[0])
                RETURN_FAILURE;
            ctx->pattern++;
            ctx->ptr++;
            break;

        case SRE_OP_NOT_LITERAL:
            /* match anything that is not literal character */
            /* <NOT_LITERAL> <code> */
            TRACE(("|%p|%p|NOT_LITERAL %d\n", ctx->pattern,
                   ctx->ptr, *ctx->pattern));
            if (ctx->ptr >= end || (SRE_CODE) ctx->ptr[0] == ctx->pattern[0])
                RETURN_FAILURE;
            ctx->pattern++;
            ctx->ptr++;
            break;

        case SRE_OP_SUCCESS:
            /* end of pattern */
            TRACE(("|%p|%p|SUCCESS\n", ctx->pattern, ctx->ptr));
            state->ptr = ctx->ptr;
            RETURN_SUCCESS;

        case SRE_OP_AT:
            /* match at given position */
            /* <AT> <code> */
            TRACE(("|%p|%p|AT %d\n", ctx->pattern, ctx->ptr, *ctx->pattern));
            if (!SRE_AT(state, ctx->ptr, *ctx->pattern))
                RETURN_FAILURE;
            ctx->pattern++;
            break;

        case SRE_OP_CATEGORY:
            /* match at given category */
            /* <CATEGORY> <code> */
            TRACE(("|%p|%p|CATEGORY %d\n", ctx->pattern,
                   ctx->ptr, *ctx->pattern));
            if (ctx->ptr >= end || !sre_category(ctx->pattern[0], ctx->ptr[0]))
                RETURN_FAILURE;
            ctx->pattern++;
            ctx->ptr++;
            break;

        case SRE_OP_ANY:
            /* match anything (except a newline) */
            /* <ANY> */
            TRACE(("|%p|%p|ANY\n", ctx->pattern, ctx->ptr));
            if (ctx->ptr >= end || SRE_IS_LINEBREAK(ctx->ptr[0]))
                RETURN_FAILURE;
            ctx->ptr++;
            break;

        case SRE_OP_ANY_ALL:
            /* match anything */
            /* <ANY_ALL> */
            TRACE(("|%p|%p|ANY_ALL\n", ctx->pattern, ctx->ptr));
            if (ctx->ptr >= end)
                RETURN_FAILURE;
            ctx->ptr++;
            break;

        case SRE_OP_IN:
            /* match set member (or non_member) */
            /* <IN> <skip> <set> */
            TRACE(("|%p|%p|IN\n", ctx->pattern, ctx->ptr));
            if (ctx->ptr >= end || !SRE_CHARSET(ctx->pattern + 1, *ctx->ptr))
                RETURN_FAILURE;
            ctx->pattern += ctx->pattern[0];
            ctx->ptr++;
            break;

        case SRE_OP_LITERAL_IGNORE:
            TRACE(("|%p|%p|LITERAL_IGNORE %d\n",
                   ctx->pattern, ctx->ptr, ctx->pattern[0]));
            if (ctx->ptr >= end ||
                state->lower(*ctx->ptr) != state->lower(*ctx->pattern))
                RETURN_FAILURE;
            ctx->pattern++;
            ctx->ptr++;
            break;

        case SRE_OP_NOT_LITERAL_IGNORE:
            TRACE(("|%p|%p|NOT_LITERAL_IGNORE %d\n",
                   ctx->pattern, ctx->ptr, *ctx->pattern));
            if (ctx->ptr >= end ||
                state->lower(*ctx->ptr) == state->lower(*ctx->pattern))
                RETURN_FAILURE;
            ctx->pattern++;
            ctx->ptr++;
            break;

        case SRE_OP_IN_IGNORE:
            TRACE(("|%p|%p|IN_IGNORE\n", ctx->pattern, ctx->ptr));
            if (ctx->ptr >= end
                || !SRE_CHARSET(ctx->pattern+1,
                                (SRE_CODE)state->lower(*ctx->ptr)))
                RETURN_FAILURE;
            ctx->pattern += ctx->pattern[0];
            ctx->ptr++;
            break;

        case SRE_OP_JUMP:
        case SRE_OP_INFO:
            /* jump forward */
            /* <JUMP> <offset> */
            TRACE(("|%p|%p|JUMP %d\n", ctx->pattern,
                   ctx->ptr, ctx->pattern[0]));
            ctx->pattern += ctx->pattern[0];
            break;

        case SRE_OP_BRANCH:
            /* alternation */
            /* <BRANCH> <0=skip> code <JUMP> ... <NULL> */
            TRACE(("|%p|%p|BRANCH\n", ctx->pattern, ctx->ptr));
            LASTMARK_SAVE();
            ctx->u.rep = state->repeat;
            if (ctx->u.rep)
                MARK_PUSH(ctx->lastmark);
            for (; ctx->pattern[0]; ctx->pattern += ctx->pattern[0]) {
                if (ctx->pattern[1] == SRE_OP_LITERAL &&
                    (ctx->ptr >= end ||
                     (SRE_CODE) *ctx->ptr != ctx->pattern[2]))
                    continue;
                if (ctx->pattern[1] == SRE_OP_IN &&
                    (ctx->ptr >= end ||
                     !SRE_CHARSET(ctx->pattern + 3, (SRE_CODE) *ctx->ptr)))
                    continue;
                state->ptr = ctx->ptr;
                DO_JUMP(JUMP_BRANCH, jump_branch, ctx->pattern+1);
                if (ret) {
                    if (ctx->u.rep)
                        MARK_POP_DISCARD(ctx->lastmark);
                    RETURN_ON_ERROR(ret);
                    RETURN_SUCCESS;
                }
                if (ctx->u.rep)
                    MARK_POP_KEEP(ctx->lastmark);
                LASTMARK_RESTORE();
            }
            if (ctx->u.rep)
                MARK_POP_DISCARD(ctx->lastmark);
            RETURN_FAILURE;

        case SRE_OP_REPEAT_ONE:
            /* match repeated sequence (maximizing regexp) */

            /* this operator only works if the repeated item is
               exactly one character wide, and we're not already
               collecting backtracking points.  for other cases,
               use the MAX_REPEAT operator */

            /* <REPEAT_ONE> <skip> <1=min> <2=max> item <SUCCESS> tail */

            TRACE(("|%p|%p|REPEAT_ONE %d %d\n", ctx->pattern, ctx->ptr,
                   ctx->pattern[1], ctx->pattern[2]));

            if (ctx->ptr + ctx->pattern[1] > end)
                RETURN_FAILURE; /* cannot match */

            state->ptr = ctx->ptr;

            ret = SRE_COUNT(state, ctx->pattern+3, ctx->pattern[2]);
            RETURN_ON_ERROR(ret);
            DATA_LOOKUP_AT(SRE_MATCH_CONTEXT, ctx, ctx_pos);
            ctx->count = ret;
            ctx->ptr += ctx->count;

            /* when we arrive here, count contains the number of
               matches, and ctx->ptr points to the tail of the target
               string.  check if the rest of the pattern matches,
               and backtrack if not. */

            if (ctx->count < (Py_ssize_t) ctx->pattern[1])
                RETURN_FAILURE;

            if (ctx->pattern[ctx->pattern[0]] == SRE_OP_SUCCESS) {
                /* tail is empty.  we're finished */
                state->ptr = ctx->ptr;
                RETURN_SUCCESS;
            }

            LASTMARK_SAVE();

            if (ctx->pattern[ctx->pattern[0]] == SRE_OP_LITERAL) {
                /* tail starts with a literal. skip positions where
                   the rest of the pattern cannot possibly match */
                ctx->u.chr = ctx->pattern[ctx->pattern[0]+1];
                for (;;) {
                    while (ctx->count >= (Py_ssize_t) ctx->pattern[1] &&
                           (ctx->ptr >= end || *ctx->ptr != ctx->u.chr)) {
                        ctx->ptr--;
                        ctx->count--;
                    }
                    if (ctx->count < (Py_ssize_t) ctx->pattern[1])
                        break;
                    state->ptr = ctx->ptr;
                    DO_JUMP(JUMP_REPEAT_ONE_1, jump_repeat_one_1,
                            ctx->pattern+ctx->pattern[0]);
                    if (ret) {
                        RETURN_ON_ERROR(ret);
                        RETURN_SUCCESS;
                    }

                    LASTMARK_RESTORE();

                    ctx->ptr--;
                    ctx->count--;
                }

            } else {
                /* general case */
                while (ctx->count >= (Py_ssize_t) ctx->pattern[1]) {
                    state->ptr = ctx->ptr;
                    DO_JUMP(JUMP_REPEAT_ONE_2, jump_repeat_one_2,
                            ctx->pattern+ctx->pattern[0]);
                    if (ret) {
                        RETURN_ON_ERROR(ret);
                        RETURN_SUCCESS;
                    }
                    ctx->ptr--;
                    ctx->count--;
                    LASTMARK_RESTORE();
                }
            }
            RETURN_FAILURE;

        case SRE_OP_MIN_REPEAT_ONE:
            /* match repeated sequence (minimizing regexp) */

            /* this operator only works if the repeated item is
               exactly one character wide, and we're not already
               collecting backtracking points.  for other cases,
               use the MIN_REPEAT operator */

            /* <MIN_REPEAT_ONE> <skip> <1=min> <2=max> item <SUCCESS> tail */

            TRACE(("|%p|%p|MIN_REPEAT_ONE %d %d\n", ctx->pattern, ctx->ptr,
                   ctx->pattern[1], ctx->pattern[2]));

            if (ctx->ptr + ctx->pattern[1] > end)
                RETURN_FAILURE; /* cannot match */

            state->ptr = ctx->ptr;

            if (ctx->pattern[1] == 0)
                ctx->count = 0;
            else {
                /* count using pattern min as the maximum */
                ret = SRE_COUNT(state, ctx->pattern+3, ctx->pattern[1]);
                RETURN_ON_ERROR(ret);
                DATA_LOOKUP_AT(SRE_MATCH_CONTEXT, ctx, ctx_pos);
                if (ret < (Py_ssize_t) ctx->pattern[1])
                    /* didn't match minimum number of times */
                    RETURN_FAILURE;
                /* advance past minimum matches of repeat */
                ctx->count = ret;
                ctx->ptr += ctx->count;
            }

            if (ctx->pattern[ctx->pattern[0]] == SRE_OP_SUCCESS) {
                /* tail is empty.  we're finished */
                state->ptr = ctx->ptr;
                RETURN_SUCCESS;

            } else {
                /* general case */
                LASTMARK_SAVE();
                while ((Py_ssize_t)ctx->pattern[2] == 65535
                       || ctx->count <= (Py_ssize_t)ctx->pattern[2]) {
                    state->ptr = ctx->ptr;
                    DO_JUMP(JUMP_MIN_REPEAT_ONE,jump_min_repeat_one,
                            ctx->pattern+ctx->pattern[0]);
                    if (ret) {
                        RETURN_ON_ERROR(ret);
                        RETURN_SUCCESS;
                    }
                    state->ptr = ctx->ptr;
                    ret = SRE_COUNT(state, ctx->pattern+3, 1);
                    RETURN_ON_ERROR(ret);
                    DATA_LOOKUP_AT(SRE_MATCH_CONTEXT, ctx, ctx_pos);
                    if (ret == 0)
                        break;
                    assert(ret == 1);
                    ctx->ptr++;
                    ctx->count++;
                    LASTMARK_RESTORE();
                }
            }
            RETURN_FAILURE;

        case SRE_OP_REPEAT:
            /* create repeat context.  all the hard work is done
               by the UNTIL operator (MAX_UNTIL, MIN_UNTIL) */
            /* <REPEAT> <skip> <1=min> <2=max> item <UNTIL> tail */
            TRACE(("|%p|%p|REPEAT %d %d\n", ctx->pattern, ctx->ptr,
                   ctx->pattern[1], ctx->pattern[2]));

            /* install new repeat context */
            ctx->u.rep = (SRE_REPEAT*) PyObject_MALLOC(sizeof(*ctx->u.rep));
            if (!ctx->u.rep) {
                PyErr_NoMemory();
                RETURN_FAILURE;
            }
            ctx->u.rep->count = -1;
            ctx->u.rep->pattern = ctx->pattern;
            ctx->u.rep->prev = state->repeat;
            ctx->u.rep->last_ptr = NULL;
            state->repeat = ctx->u.rep;

            state->ptr = ctx->ptr;
            DO_JUMP(JUMP_REPEAT, jump_repeat, ctx->pattern+ctx->pattern[0]);
            state->repeat = ctx->u.rep->prev;
            PyObject_FREE(ctx->u.rep);

            if (ret) {
                RETURN_ON_ERROR(ret);
                RETURN_SUCCESS;
            }
            RETURN_FAILURE;

        case SRE_OP_MAX_UNTIL:
            /* maximizing repeat */
            /* <REPEAT> <skip> <1=min> <2=max> item <MAX_UNTIL> tail */

            /* FIXME: we probably need to deal with zero-width
               matches in here... */

            ctx->u.rep = state->repeat;
            if (!ctx->u.rep)
                RETURN_ERROR(SRE_ERROR_STATE);

            state->ptr = ctx->ptr;

            ctx->count = ctx->u.rep->count+1;

            TRACE(("|%p|%p|MAX_UNTIL %d\n", ctx->pattern,
                   ctx->ptr, ctx->count));

            if (ctx->count < ctx->u.rep->pattern[1]) {
                /* not enough matches */
                ctx->u.rep->count = ctx->count;
                DO_JUMP(JUMP_MAX_UNTIL_1, jump_max_until_1,
                        ctx->u.rep->pattern+3);
                if (ret) {
                    RETURN_ON_ERROR(ret);
                    RETURN_SUCCESS;
                }
                ctx->u.rep->count = ctx->count-1;
                state->ptr = ctx->ptr;
                RETURN_FAILURE;
            }

            if ((ctx->count < ctx->u.rep->pattern[2] ||
                ctx->u.rep->pattern[2] == 65535) &&
                state->ptr != ctx->u.rep->last_ptr) {
                /* we may have enough matches, but if we can
                   match another item, do so */
                ctx->u.rep->count = ctx->count;
                LASTMARK_SAVE();
                MARK_PUSH(ctx->lastmark);
                /* zero-width match protection */
                DATA_PUSH(&ctx->u.rep->last_ptr);
                ctx->u.rep->last_ptr = state->ptr;
                DO_JUMP(JUMP_MAX_UNTIL_2, jump_max_until_2,
                        ctx->u.rep->pattern+3);
                DATA_POP(&ctx->u.rep->last_ptr);
                if (ret) {
                    MARK_POP_DISCARD(ctx->lastmark);
                    RETURN_ON_ERROR(ret);
                    RETURN_SUCCESS;
                }
                MARK_POP(ctx->lastmark);
                LASTMARK_RESTORE();
                ctx->u.rep->count = ctx->count-1;
                state->ptr = ctx->ptr;
            }

            /* cannot match more repeated items here.  make sure the
               tail matches */
            state->repeat = ctx->u.rep->prev;
            DO_JUMP(JUMP_MAX_UNTIL_3, jump_max_until_3, ctx->pattern);
            RETURN_ON_SUCCESS(ret);
            state->repeat = ctx->u.rep;
            state->ptr = ctx->ptr;
            RETURN_FAILURE;

        case SRE_OP_MIN_UNTIL:
            /* minimizing repeat */
            /* <REPEAT> <skip> <1=min> <2=max> item <MIN_UNTIL> tail */

            ctx->u.rep = state->repeat;
            if (!ctx->u.rep)
                RETURN_ERROR(SRE_ERROR_STATE);

            state->ptr = ctx->ptr;

            ctx->count = ctx->u.rep->count+1;

            TRACE(("|%p|%p|MIN_UNTIL %d %p\n", ctx->pattern,
                   ctx->ptr, ctx->count, ctx->u.rep->pattern));

            if (ctx->count < ctx->u.rep->pattern[1]) {
                /* not enough matches */
                ctx->u.rep->count = ctx->count;
                DO_JUMP(JUMP_MIN_UNTIL_1, jump_min_until_1,
                        ctx->u.rep->pattern+3);
                if (ret) {
                    RETURN_ON_ERROR(ret);
                    RETURN_SUCCESS;
                }
                ctx->u.rep->count = ctx->count-1;
                state->ptr = ctx->ptr;
                RETURN_FAILURE;
            }

            LASTMARK_SAVE();

            /* see if the tail matches */
            state->repeat = ctx->u.rep->prev;
            DO_JUMP(JUMP_MIN_UNTIL_2, jump_min_until_2, ctx->pattern);
            if (ret) {
                RETURN_ON_ERROR(ret);
                RETURN_SUCCESS;
            }

            state->repeat = ctx->u.rep;
            state->ptr = ctx->ptr;

            LASTMARK_RESTORE();

            if (ctx->count >= ctx->u.rep->pattern[2]
                && ctx->u.rep->pattern[2] != 65535)
                RETURN_FAILURE;

            ctx->u.rep->count = ctx->count;
            DO_JUMP(JUMP_MIN_UNTIL_3,jump_min_until_3,
                    ctx->u.rep->pattern+3);
            if (ret) {
                RETURN_ON_ERROR(ret);
                RETURN_SUCCESS;
            }
            ctx->u.rep->count = ctx->count-1;
            state->ptr = ctx->ptr;
            RETURN_FAILURE;

        case SRE_OP_GROUPREF:
            /* match backreference */
            TRACE(("|%p|%p|GROUPREF %d\n", ctx->pattern,
                   ctx->ptr, ctx->pattern[0]));
            i = ctx->pattern[0];
            {
                Py_ssize_t groupref = i+i;
                if (groupref >= state->lastmark) {
                    RETURN_FAILURE;
                } else {
                    SRE_CHAR* p = (SRE_CHAR*) state->mark[groupref];
                    SRE_CHAR* e = (SRE_CHAR*) state->mark[groupref+1];
                    if (!p || !e || e < p)
                        RETURN_FAILURE;
                    while (p < e) {
                        if (ctx->ptr >= end || *ctx->ptr != *p)
                            RETURN_FAILURE;
                        p++; ctx->ptr++;
                    }
                }
            }
            ctx->pattern++;
            break;

        case SRE_OP_GROUPREF_IGNORE:
            /* match backreference */
            TRACE(("|%p|%p|GROUPREF_IGNORE %d\n", ctx->pattern,
                   ctx->ptr, ctx->pattern[0]));
            i = ctx->pattern[0];
            {
                Py_ssize_t groupref = i+i;
                if (groupref >= state->lastmark) {
                    RETURN_FAILURE;
                } else {
                    SRE_CHAR* p = (SRE_CHAR*) state->mark[groupref];
                    SRE_CHAR* e = (SRE_CHAR*) state->mark[groupref+1];
                    if (!p || !e || e < p)
                        RETURN_FAILURE;
                    while (p < e) {
                        if (ctx->ptr >= end ||
                            state->lower(*ctx->ptr) != state->lower(*p))
                            RETURN_FAILURE;
                        p++; ctx->ptr++;
                    }
                }
            }
            ctx->pattern++;
            break;

        case SRE_OP_GROUPREF_EXISTS:
            TRACE(("|%p|%p|GROUPREF_EXISTS %d\n", ctx->pattern,
                   ctx->ptr, ctx->pattern[0]));
            /* <GROUPREF_EXISTS> <group> <skip> codeyes <JUMP> codeno ... */
            i = ctx->pattern[0];
            {
                Py_ssize_t groupref = i+i;
                if (groupref >= state->lastmark) {
                    ctx->pattern += ctx->pattern[1];
                    break;
                } else {
                    SRE_CHAR* p = (SRE_CHAR*) state->mark[groupref];
                    SRE_CHAR* e = (SRE_CHAR*) state->mark[groupref+1];
                    if (!p || !e || e < p) {
                        ctx->pattern += ctx->pattern[1];
                        break;
                    }
                }
            }
            ctx->pattern += 2;
            break;

        case SRE_OP_ASSERT:
            /* assert subpattern */
            /* <ASSERT> <skip> <back> <pattern> */
            TRACE(("|%p|%p|ASSERT %d\n", ctx->pattern,
                   ctx->ptr, ctx->pattern[1]));
            state->ptr = ctx->ptr - ctx->pattern[1];
            if (state->ptr < state->beginning)
                RETURN_FAILURE;
            DO_JUMP(JUMP_ASSERT, jump_assert, ctx->pattern+2);
            RETURN_ON_FAILURE(ret);
            ctx->pattern += ctx->pattern[0];
            break;

        case SRE_OP_ASSERT_NOT:
            /* assert not subpattern */
            /* <ASSERT_NOT> <skip> <back> <pattern> */
            TRACE(("|%p|%p|ASSERT_NOT %d\n", ctx->pattern,
                   ctx->ptr, ctx->pattern[1]));
            state->ptr = ctx->ptr - ctx->pattern[1];
            if (state->ptr >= state->beginning) {
                DO_JUMP(JUMP_ASSERT_NOT, jump_assert_not, ctx->pattern+2);
                if (ret) {
                    RETURN_ON_ERROR(ret);
                    RETURN_FAILURE;
                }
            }
            ctx->pattern += ctx->pattern[0];
            break;

        case SRE_OP_FAILURE:
            /* immediate failure */
            TRACE(("|%p|%p|FAILURE\n", ctx->pattern, ctx->ptr));
            RETURN_FAILURE;

        default:
            TRACE(("|%p|%p|UNKNOWN %d\n", ctx->pattern, ctx->ptr,
                   ctx->pattern[-1]));
            RETURN_ERROR(SRE_ERROR_ILLEGAL);
        }
    }

exit:
    ctx_pos = ctx->last_ctx_pos;
    jump = ctx->jump;
    DATA_POP_DISCARD(ctx);
    if (ctx_pos == -1)
        return ret;
    DATA_LOOKUP_AT(SRE_MATCH_CONTEXT, ctx, ctx_pos);

    switch (jump) {
        case JUMP_MAX_UNTIL_2:
            TRACE(("|%p|%p|JUMP_MAX_UNTIL_2\n", ctx->pattern, ctx->ptr));
            goto jump_max_until_2;
        case JUMP_MAX_UNTIL_3:
            TRACE(("|%p|%p|JUMP_MAX_UNTIL_3\n", ctx->pattern, ctx->ptr));
            goto jump_max_until_3;
        case JUMP_MIN_UNTIL_2:
            TRACE(("|%p|%p|JUMP_MIN_UNTIL_2\n", ctx->pattern, ctx->ptr));
            goto jump_min_until_2;
        case JUMP_MIN_UNTIL_3:
            TRACE(("|%p|%p|JUMP_MIN_UNTIL_3\n", ctx->pattern, ctx->ptr));
            goto jump_min_until_3;
        case JUMP_BRANCH:
            TRACE(("|%p|%p|JUMP_BRANCH\n", ctx->pattern, ctx->ptr));
            goto jump_branch;
        case JUMP_MAX_UNTIL_1:
            TRACE(("|%p|%p|JUMP_MAX_UNTIL_1\n", ctx->pattern, ctx->ptr));
            goto jump_max_until_1;
        case JUMP_MIN_UNTIL_1:
            TRACE(("|%p|%p|JUMP_MIN_UNTIL_1\n", ctx->pattern, ctx->ptr));
            goto jump_min_until_1;
        case JUMP_REPEAT:
            TRACE(("|%p|%p|JUMP_REPEAT\n", ctx->pattern, ctx->ptr));
            goto jump_repeat;
        case JUMP_REPEAT_ONE_1:
            TRACE(("|%p|%p|JUMP_REPEAT_ONE_1\n", ctx->pattern, ctx->ptr));
            goto jump_repeat_one_1;
        case JUMP_REPEAT_ONE_2:
            TRACE(("|%p|%p|JUMP_REPEAT_ONE_2\n", ctx->pattern, ctx->ptr));
            goto jump_repeat_one_2;
        case JUMP_MIN_REPEAT_ONE:
            TRACE(("|%p|%p|JUMP_MIN_REPEAT_ONE\n", ctx->pattern, ctx->ptr));
            goto jump_min_repeat_one;
        case JUMP_ASSERT:
            TRACE(("|%p|%p|JUMP_ASSERT\n", ctx->pattern, ctx->ptr));
            goto jump_assert;
        case JUMP_ASSERT_NOT:
            TRACE(("|%p|%p|JUMP_ASSERT_NOT\n", ctx->pattern, ctx->ptr));
            goto jump_assert_not;
        case JUMP_NONE:
            TRACE(("|%p|%p|RETURN %d\n", ctx->pattern, ctx->ptr, ret));
            break;
    }

    return ret; /* should never get here */
}

LOCAL(Py_ssize_t)
SRE_SEARCH(SRE_STATE* state, SRE_CODE* pattern)
{
    SRE_CHAR* ptr = (SRE_CHAR *)state->start;
    SRE_CHAR* end = (SRE_CHAR *)state->end;
    Py_ssize_t status = 0;
    Py_ssize_t prefix_len = 0;
    Py_ssize_t prefix_skip = 0;
    SRE_CODE* prefix = NULL;
    SRE_CODE* charset = NULL;
    SRE_CODE* overlap = NULL;
    int flags = 0;

    if (pattern[0] == SRE_OP_INFO) {
        /* optimization info block */
        /* <INFO> <1=skip> <2=flags> <3=min> <4=max> <5=prefix info>  */

        flags = pattern[2];

        if (pattern[3] > 1) {
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
        Py_ssize_t i = 0;
        end = (SRE_CHAR *)state->end;
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
                        status = SRE_MATCH(state, pattern + 2*prefix_skip);
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
        end = (SRE_CHAR *)state->end;
        for (;;) {
            while (ptr < end && (SRE_CODE) ptr[0] != chr)
                ptr++;
            if (ptr >= end)
                return 0;
            TRACE(("|%p|%p|SEARCH LITERAL\n", pattern, ptr));
            state->start = ptr;
            state->ptr = ++ptr;
            if (flags & SRE_INFO_LITERAL)
                return 1; /* we got all of it */
            status = SRE_MATCH(state, pattern + 2);
            if (status != 0)
                break;
        }
    } else if (charset) {
        /* pattern starts with a character from a known set */
        end = (SRE_CHAR *)state->end;
        for (;;) {
            while (ptr < end && !SRE_CHARSET(charset, ptr[0]))
                ptr++;
            if (ptr >= end)
                return 0;
            TRACE(("|%p|%p|SEARCH CHARSET\n", pattern, ptr));
            state->start = ptr;
            state->ptr = ptr;
            status = SRE_MATCH(state, pattern);
            if (status != 0)
                break;
            ptr++;
        }
    } else
        /* general case */
        while (ptr <= end) {
            TRACE(("|%p|%p|SEARCH\n", pattern, ptr));
            state->start = state->ptr = ptr++;
            status = SRE_MATCH(state, pattern);
            if (status != 0)
                break;
        }

    return status;
}

LOCAL(int)
SRE_LITERAL_TEMPLATE(SRE_CHAR* ptr, Py_ssize_t len)
{
    /* check if given string is a literal template (i.e. no escapes) */
    while (len-- > 0)
        if (*ptr++ == '\\')
            return 0;
    return 1;
}

#if !defined(SRE_RECURSIVE)

/* -------------------------------------------------------------------- */
/* factories and destructors */

/* see sre.h for object declarations */
static PyObject*pattern_new_match(PatternObject*, SRE_STATE*, int);
static PyObject*pattern_scanner(PatternObject*, PyObject*);

static PyObject *
sre_codesize(PyObject* self, PyObject *unused)
{
    return Py_BuildValue("l", sizeof(SRE_CODE));
}

static PyObject *
sre_getlower(PyObject* self, PyObject* args)
{
    int character, flags;
    if (!PyArg_ParseTuple(args, "ii", &character, &flags))
        return NULL;
    if (flags & SRE_FLAG_LOCALE)
        return Py_BuildValue("i", sre_lower_locale(character));
    if (flags & SRE_FLAG_UNICODE)
#if defined(HAVE_UNICODE)
        return Py_BuildValue("i", sre_lower_unicode(character));
#else
        return Py_BuildValue("i", sre_lower_locale(character));
#endif
    return Py_BuildValue("i", sre_lower(character));
}

LOCAL(void)
state_reset(SRE_STATE* state)
{
    /* FIXME: dynamic! */
    /*memset(state->mark, 0, sizeof(*state->mark) * SRE_MARK_SIZE);*/

    state->lastmark = -1;
    state->lastindex = -1;

    state->repeat = NULL;

    data_stack_dealloc(state);
}

static void*
getstring(PyObject* string, Py_ssize_t* p_length, int* p_charsize)
{
    /* given a python object, return a data pointer, a length (in
       characters), and a character size.  return NULL if the object
       is not a string (or not compatible) */

    PyBufferProcs *buffer;
    Py_ssize_t size, bytes;
    int charsize;
    void* ptr;

#if defined(HAVE_UNICODE)
    if (PyUnicode_Check(string)) {
        /* unicode strings doesn't always support the buffer interface */
        ptr = (void*) PyUnicode_AS_DATA(string);
        bytes = PyUnicode_GET_DATA_SIZE(string);
        size = PyUnicode_GET_SIZE(string);
        charsize = sizeof(Py_UNICODE);

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
        charsize = 1;
#if defined(HAVE_UNICODE)
    else if (bytes == (Py_ssize_t) (size * sizeof(Py_UNICODE)))
        charsize = sizeof(Py_UNICODE);
#endif
    else {
        PyErr_SetString(PyExc_TypeError, "buffer size mismatch");
        return NULL;
    }

#if defined(HAVE_UNICODE)
    }
#endif

    *p_length = size;
    *p_charsize = charsize;

    return ptr;
}

LOCAL(PyObject*)
state_init(SRE_STATE* state, PatternObject* pattern, PyObject* string,
           Py_ssize_t start, Py_ssize_t end)
{
    /* prepare state object */

    Py_ssize_t length;
    int charsize;
    void* ptr;

    memset(state, 0, sizeof(SRE_STATE));

    state->lastmark = -1;
    state->lastindex = -1;

    ptr = getstring(string, &length, &charsize);
    if (!ptr)
        return NULL;

    /* adjust boundaries */
    if (start < 0)
        start = 0;
    else if (start > length)
        start = length;

    if (end < 0)
        end = 0;
    else if (end > length)
        end = length;

    state->charsize = charsize;

    state->beginning = ptr;

    state->start = (void*) ((char*) ptr + start * state->charsize);
    state->end = (void*) ((char*) ptr + end * state->charsize);

    Py_INCREF(string);
    state->string = string;
    state->pos = start;
    state->endpos = end;

    if (pattern->flags & SRE_FLAG_LOCALE)
        state->lower = sre_lower_locale;
    else if (pattern->flags & SRE_FLAG_UNICODE)
#if defined(HAVE_UNICODE)
        state->lower = sre_lower_unicode;
#else
        state->lower = sre_lower_locale;
#endif
    else
        state->lower = sre_lower;

    return string;
}

LOCAL(void)
state_fini(SRE_STATE* state)
{
    Py_XDECREF(state->string);
    data_stack_dealloc(state);
}

/* calculate offset from start of string */
#define STATE_OFFSET(state, member)\
    (((char*)(member) - (char*)(state)->beginning) / (state)->charsize)

LOCAL(PyObject*)
state_getslice(SRE_STATE* state, Py_ssize_t index, PyObject* string, int empty)
{
    Py_ssize_t i, j;

    index = (index - 1) * 2;

    if (string == Py_None || index >= state->lastmark || !state->mark[index] || !state->mark[index+1]) {
        if (empty)
            /* want empty string */
            i = j = 0;
        else {
            Py_INCREF(Py_None);
            return Py_None;
        }
    } else {
        i = STATE_OFFSET(state, state->mark[index]);
        j = STATE_OFFSET(state, state->mark[index+1]);
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

static void
pattern_dealloc(PatternObject* self)
{
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);
    Py_XDECREF(self->pattern);
    Py_XDECREF(self->groupindex);
    Py_XDECREF(self->indexgroup);
    PyObject_DEL(self);
}

static PyObject*
pattern_match(PatternObject* self, PyObject* args, PyObject* kw)
{
    SRE_STATE state;
    int status;

    PyObject* string;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    static char* kwlist[] = { "pattern", "pos", "endpos", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|nn:match", kwlist,
                                     &string, &start, &end))
        return NULL;

    string = state_init(&state, self, string, start, end);
    if (!string)
        return NULL;

    state.ptr = state.start;

    TRACE(("|%p|%p|MATCH\n", PatternObject_GetCode(self), state.ptr));

    if (state.charsize == 1) {
        status = sre_match(&state, PatternObject_GetCode(self));
    } else {
#if defined(HAVE_UNICODE)
        status = sre_umatch(&state, PatternObject_GetCode(self));
#endif
    }

    TRACE(("|%p|%p|END\n", PatternObject_GetCode(self), state.ptr));
    if (PyErr_Occurred())
        return NULL;

    state_fini(&state);

    return pattern_new_match(self, &state, status);
}

static PyObject*
pattern_search(PatternObject* self, PyObject* args, PyObject* kw)
{
    SRE_STATE state;
    int status;

    PyObject* string;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    static char* kwlist[] = { "pattern", "pos", "endpos", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|nn:search", kwlist,
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

    if (PyErr_Occurred())
        return NULL;

    return pattern_new_match(self, &state, status);
}

static PyObject*
call(char* module, char* function, PyObject* args)
{
    PyObject* name;
    PyObject* mod;
    PyObject* func;
    PyObject* result;

    if (!args)
        return NULL;
    name = PyString_FromString(module);
    if (!name)
        return NULL;
    mod = PyImport_Import(name);
    Py_DECREF(name);
    if (!mod)
        return NULL;
    func = PyObject_GetAttrString(mod, function);
    Py_DECREF(mod);
    if (!func)
        return NULL;
    result = PyObject_CallObject(func, args);
    Py_DECREF(func);
    Py_DECREF(args);
    return result;
}

#ifdef USE_BUILTIN_COPY
static int
deepcopy(PyObject** object, PyObject* memo)
{
    PyObject* copy;

    copy = call(
        "copy", "deepcopy",
        PyTuple_Pack(2, *object, memo)
        );
    if (!copy)
        return 0;

    Py_DECREF(*object);
    *object = copy;

    return 1; /* success */
}
#endif

static PyObject*
join_list(PyObject* list, PyObject* string)
{
    /* join list elements */

    PyObject* joiner;
#if PY_VERSION_HEX >= 0x01060000
    PyObject* function;
    PyObject* args;
#endif
    PyObject* result;

    joiner = PySequence_GetSlice(string, 0, 0);
    if (!joiner)
        return NULL;

    if (PyList_GET_SIZE(list) == 0) {
        Py_DECREF(list);
        return joiner;
    }

#if PY_VERSION_HEX >= 0x01060000
    function = PyObject_GetAttrString(joiner, "join");
    if (!function) {
        Py_DECREF(joiner);
        return NULL;
    }
    args = PyTuple_New(1);
    if (!args) {
        Py_DECREF(function);
        Py_DECREF(joiner);
        return NULL;
    }
    PyTuple_SET_ITEM(args, 0, list);
    result = PyObject_CallObject(function, args);
    Py_DECREF(args); /* also removes list */
    Py_DECREF(function);
#else
    result = call(
        "string", "join",
        PyTuple_Pack(2, list, joiner)
        );
#endif
    Py_DECREF(joiner);

    return result;
}

static PyObject*
pattern_findall(PatternObject* self, PyObject* args, PyObject* kw)
{
    SRE_STATE state;
    PyObject* list;
    int status;
    Py_ssize_t i, b, e;

    PyObject* string;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    static char* kwlist[] = { "source", "pos", "endpos", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|nn:findall", kwlist,
                                     &string, &start, &end))
        return NULL;

    string = state_init(&state, self, string, start, end);
    if (!string)
        return NULL;

    list = PyList_New(0);
    if (!list) {
        state_fini(&state);
        return NULL;
    }

    while (state.start <= state.end) {

        PyObject* item;

        state_reset(&state);

        state.ptr = state.start;

        if (state.charsize == 1) {
            status = sre_search(&state, PatternObject_GetCode(self));
        } else {
#if defined(HAVE_UNICODE)
            status = sre_usearch(&state, PatternObject_GetCode(self));
#endif
        }

	if (PyErr_Occurred())
	    goto error;

        if (status <= 0) {
            if (status == 0)
                break;
            pattern_error(status);
            goto error;
        }

        /* don't bother to build a match object */
        switch (self->groups) {
        case 0:
            b = STATE_OFFSET(&state, state.start);
            e = STATE_OFFSET(&state, state.ptr);
            item = PySequence_GetSlice(string, b, e);
            if (!item)
                goto error;
            break;
        case 1:
            item = state_getslice(&state, 1, string, 1);
            if (!item)
                goto error;
            break;
        default:
            item = PyTuple_New(self->groups);
            if (!item)
                goto error;
            for (i = 0; i < self->groups; i++) {
                PyObject* o = state_getslice(&state, i+1, string, 1);
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

    }

    state_fini(&state);
    return list;

error:
    Py_DECREF(list);
    state_fini(&state);
    return NULL;

}

#if PY_VERSION_HEX >= 0x02020000
static PyObject*
pattern_finditer(PatternObject* pattern, PyObject* args)
{
    PyObject* scanner;
    PyObject* search;
    PyObject* iterator;

    scanner = pattern_scanner(pattern, args);
    if (!scanner)
        return NULL;

    search = PyObject_GetAttrString(scanner, "search");
    Py_DECREF(scanner);
    if (!search)
        return NULL;

    iterator = PyCallIter_New(search, Py_None);
    Py_DECREF(search);

    return iterator;
}
#endif

static PyObject*
pattern_split(PatternObject* self, PyObject* args, PyObject* kw)
{
    SRE_STATE state;
    PyObject* list;
    PyObject* item;
    int status;
    Py_ssize_t n;
    Py_ssize_t i;
    void* last;

    PyObject* string;
    Py_ssize_t maxsplit = 0;
    static char* kwlist[] = { "source", "maxsplit", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|n:split", kwlist,
                                     &string, &maxsplit))
        return NULL;

    string = state_init(&state, self, string, 0, PY_SSIZE_T_MAX);
    if (!string)
        return NULL;

    list = PyList_New(0);
    if (!list) {
        state_fini(&state);
        return NULL;
    }

    n = 0;
    last = state.start;

    while (!maxsplit || n < maxsplit) {

        state_reset(&state);

        state.ptr = state.start;

        if (state.charsize == 1) {
            status = sre_search(&state, PatternObject_GetCode(self));
        } else {
#if defined(HAVE_UNICODE)
            status = sre_usearch(&state, PatternObject_GetCode(self));
#endif
        }

	if (PyErr_Occurred())
	    goto error;

        if (status <= 0) {
            if (status == 0)
                break;
            pattern_error(status);
            goto error;
        }

        if (state.start == state.ptr) {
            if (last == state.end)
                break;
            /* skip one character */
            state.start = (void*) ((char*) state.ptr + state.charsize);
            continue;
        }

        /* get segment before this match */
        item = PySequence_GetSlice(
            string, STATE_OFFSET(&state, last),
            STATE_OFFSET(&state, state.start)
            );
        if (!item)
            goto error;
        status = PyList_Append(list, item);
        Py_DECREF(item);
        if (status < 0)
            goto error;

        /* add groups (if any) */
        for (i = 0; i < self->groups; i++) {
            item = state_getslice(&state, i+1, string, 0);
            if (!item)
                goto error;
            status = PyList_Append(list, item);
            Py_DECREF(item);
            if (status < 0)
                goto error;
        }

        n = n + 1;

        last = state.start = state.ptr;

    }

    /* get segment following last match (even if empty) */
    item = PySequence_GetSlice(
        string, STATE_OFFSET(&state, last), state.endpos
        );
    if (!item)
        goto error;
    status = PyList_Append(list, item);
    Py_DECREF(item);
    if (status < 0)
        goto error;

    state_fini(&state);
    return list;

error:
    Py_DECREF(list);
    state_fini(&state);
    return NULL;

}

static PyObject*
pattern_subx(PatternObject* self, PyObject* ptemplate, PyObject* string,
             Py_ssize_t count, Py_ssize_t subn)
{
    SRE_STATE state;
    PyObject* list;
    PyObject* item;
    PyObject* filter;
    PyObject* args;
    PyObject* match;
    void* ptr;
    int status;
    Py_ssize_t n;
    Py_ssize_t i, b, e;
    int bint;
    int filter_is_callable;

    if (PyCallable_Check(ptemplate)) {
        /* sub/subn takes either a function or a template */
        filter = ptemplate;
        Py_INCREF(filter);
        filter_is_callable = 1;
    } else {
        /* if not callable, check if it's a literal string */
        int literal;
        ptr = getstring(ptemplate, &n, &bint);
        b = bint;
        if (ptr) {
            if (b == 1) {
		    literal = sre_literal_template((unsigned char *)ptr, n);
            } else {
#if defined(HAVE_UNICODE)
		    literal = sre_uliteral_template((Py_UNICODE *)ptr, n);
#endif
            }
        } else {
            PyErr_Clear();
            literal = 0;
        }
        if (literal) {
            filter = ptemplate;
            Py_INCREF(filter);
            filter_is_callable = 0;
        } else {
            /* not a literal; hand it over to the template compiler */
            filter = call(
                SRE_PY_MODULE, "_subx",
                PyTuple_Pack(2, self, ptemplate)
                );
            if (!filter)
                return NULL;
            filter_is_callable = PyCallable_Check(filter);
        }
    }

    string = state_init(&state, self, string, 0, PY_SSIZE_T_MAX);
    if (!string) {
        Py_DECREF(filter);
        return NULL;
    }

    list = PyList_New(0);
    if (!list) {
        Py_DECREF(filter);
        state_fini(&state);
        return NULL;
    }

    n = i = 0;

    while (!count || n < count) {

        state_reset(&state);

        state.ptr = state.start;

        if (state.charsize == 1) {
            status = sre_search(&state, PatternObject_GetCode(self));
        } else {
#if defined(HAVE_UNICODE)
            status = sre_usearch(&state, PatternObject_GetCode(self));
#endif
        }

	if (PyErr_Occurred())
	    goto error;

        if (status <= 0) {
            if (status == 0)
                break;
            pattern_error(status);
            goto error;
        }

        b = STATE_OFFSET(&state, state.start);
        e = STATE_OFFSET(&state, state.ptr);

        if (i < b) {
            /* get segment before this match */
            item = PySequence_GetSlice(string, i, b);
            if (!item)
                goto error;
            status = PyList_Append(list, item);
            Py_DECREF(item);
            if (status < 0)
                goto error;

        } else if (i == b && i == e && n > 0)
            /* ignore empty match on latest position */
            goto next;

        if (filter_is_callable) {
            /* pass match object through filter */
            match = pattern_new_match(self, &state, 1);
            if (!match)
                goto error;
            args = PyTuple_Pack(1, match);
            if (!args) {
                Py_DECREF(match);
                goto error;
            }
            item = PyObject_CallObject(filter, args);
            Py_DECREF(args);
            Py_DECREF(match);
            if (!item)
                goto error;
        } else {
            /* filter is literal string */
            item = filter;
            Py_INCREF(item);
        }

        /* add to list */
        if (item != Py_None) {
            status = PyList_Append(list, item);
            Py_DECREF(item);
            if (status < 0)
                goto error;
        }

        i = e;
        n = n + 1;

next:
        /* move on */
        if (state.ptr == state.start)
            state.start = (void*) ((char*) state.ptr + state.charsize);
        else
            state.start = state.ptr;

    }

    /* get segment following last match */
    if (i < state.endpos) {
        item = PySequence_GetSlice(string, i, state.endpos);
        if (!item)
            goto error;
        status = PyList_Append(list, item);
        Py_DECREF(item);
        if (status < 0)
            goto error;
    }

    state_fini(&state);

    Py_DECREF(filter);

    /* convert list to single string (also removes list) */
    item = join_list(list, string);

    if (!item)
        return NULL;

    if (subn)
        return Py_BuildValue("Ni", item, n);

    return item;

error:
    Py_DECREF(list);
    state_fini(&state);
    Py_DECREF(filter);
    return NULL;

}

static PyObject*
pattern_sub(PatternObject* self, PyObject* args, PyObject* kw)
{
    PyObject* ptemplate;
    PyObject* string;
    Py_ssize_t count = 0;
    static char* kwlist[] = { "repl", "string", "count", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO|n:sub", kwlist,
                                     &ptemplate, &string, &count))
        return NULL;

    return pattern_subx(self, ptemplate, string, count, 0);
}

static PyObject*
pattern_subn(PatternObject* self, PyObject* args, PyObject* kw)
{
    PyObject* ptemplate;
    PyObject* string;
    Py_ssize_t count = 0;
    static char* kwlist[] = { "repl", "string", "count", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO|n:subn", kwlist,
                                     &ptemplate, &string, &count))
        return NULL;

    return pattern_subx(self, ptemplate, string, count, 1);
}

static PyObject*
pattern_copy(PatternObject* self, PyObject *unused)
{
#ifdef USE_BUILTIN_COPY
    PatternObject* copy;
    int offset;

    copy = PyObject_NEW_VAR(PatternObject, &Pattern_Type, self->codesize);
    if (!copy)
        return NULL;

    offset = offsetof(PatternObject, groups);

    Py_XINCREF(self->groupindex);
    Py_XINCREF(self->indexgroup);
    Py_XINCREF(self->pattern);

    memcpy((char*) copy + offset, (char*) self + offset,
           sizeof(PatternObject) + self->codesize * sizeof(SRE_CODE) - offset);
    copy->weakreflist = NULL;

    return (PyObject*) copy;
#else
    PyErr_SetString(PyExc_TypeError, "cannot copy this pattern object");
    return NULL;
#endif
}

static PyObject*
pattern_deepcopy(PatternObject* self, PyObject* memo)
{
#ifdef USE_BUILTIN_COPY
    PatternObject* copy;

    copy = (PatternObject*) pattern_copy(self);
    if (!copy)
        return NULL;

    if (!deepcopy(&copy->groupindex, memo) ||
        !deepcopy(&copy->indexgroup, memo) ||
        !deepcopy(&copy->pattern, memo)) {
        Py_DECREF(copy);
        return NULL;
    }

#else
    PyErr_SetString(PyExc_TypeError, "cannot deepcopy this pattern object");
    return NULL;
#endif
}

PyDoc_STRVAR(pattern_match_doc,
"match(string[, pos[, endpos]]) --> match object or None.\n\
    Matches zero or more characters at the beginning of the string");

PyDoc_STRVAR(pattern_search_doc,
"search(string[, pos[, endpos]]) --> match object or None.\n\
    Scan through string looking for a match, and return a corresponding\n\
    MatchObject instance. Return None if no position in the string matches.");

PyDoc_STRVAR(pattern_split_doc,
"split(string[, maxsplit = 0])  --> list.\n\
    Split string by the occurrences of pattern.");

PyDoc_STRVAR(pattern_findall_doc,
"findall(string[, pos[, endpos]]) --> list.\n\
   Return a list of all non-overlapping matches of pattern in string.");

PyDoc_STRVAR(pattern_finditer_doc,
"finditer(string[, pos[, endpos]]) --> iterator.\n\
    Return an iterator over all non-overlapping matches for the \n\
    RE pattern in string. For each match, the iterator returns a\n\
    match object.");

PyDoc_STRVAR(pattern_sub_doc,
"sub(repl, string[, count = 0]) --> newstring\n\
    Return the string obtained by replacing the leftmost non-overlapping\n\
    occurrences of pattern in string by the replacement repl.");

PyDoc_STRVAR(pattern_subn_doc,
"subn(repl, string[, count = 0]) --> (newstring, number of subs)\n\
    Return the tuple (new_string, number_of_subs_made) found by replacing\n\
    the leftmost non-overlapping occurrences of pattern with the\n\
    replacement repl.");

PyDoc_STRVAR(pattern_doc, "Compiled regular expression objects");

static PyMethodDef pattern_methods[] = {
    {"match", (PyCFunction) pattern_match, METH_VARARGS|METH_KEYWORDS,
	pattern_match_doc},
    {"search", (PyCFunction) pattern_search, METH_VARARGS|METH_KEYWORDS,
	pattern_search_doc},
    {"sub", (PyCFunction) pattern_sub, METH_VARARGS|METH_KEYWORDS,
	pattern_sub_doc},
    {"subn", (PyCFunction) pattern_subn, METH_VARARGS|METH_KEYWORDS,
	pattern_subn_doc},
    {"split", (PyCFunction) pattern_split, METH_VARARGS|METH_KEYWORDS,
	pattern_split_doc},
    {"findall", (PyCFunction) pattern_findall, METH_VARARGS|METH_KEYWORDS,
	pattern_findall_doc},
#if PY_VERSION_HEX >= 0x02020000
    {"finditer", (PyCFunction) pattern_finditer, METH_VARARGS,
	pattern_finditer_doc},
#endif
    {"scanner", (PyCFunction) pattern_scanner, METH_VARARGS},
    {"__copy__", (PyCFunction) pattern_copy, METH_NOARGS},
    {"__deepcopy__", (PyCFunction) pattern_deepcopy, METH_O},
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
    0, "_" SRE_MODULE ".SRE_Pattern",
    sizeof(PatternObject), sizeof(SRE_CODE),
    (destructor)pattern_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)pattern_getattr, /*tp_getattr*/
    0,					/* tp_setattr */
    0,					/* tp_compare */
    0,					/* tp_repr */
    0,					/* tp_as_number */
    0,					/* tp_as_sequence */
    0,					/* tp_as_mapping */
    0,					/* tp_hash */
    0,					/* tp_call */
    0,					/* tp_str */
    0,					/* tp_getattro */
    0,					/* tp_setattro */
    0,					/* tp_as_buffer */
    Py_TPFLAGS_HAVE_WEAKREFS,		/* tp_flags */
    pattern_doc,			/* tp_doc */
    0,					/* tp_traverse */
    0,					/* tp_clear */
    0,					/* tp_richcompare */
    offsetof(PatternObject, weakreflist),	/* tp_weaklistoffset */
};

static PyObject *
_compile(PyObject* self_, PyObject* args)
{
    /* "compile" pattern descriptor to pattern object */

    PatternObject* self;
    Py_ssize_t i, n;

    PyObject* pattern;
    int flags = 0;
    PyObject* code;
    Py_ssize_t groups = 0;
    PyObject* groupindex = NULL;
    PyObject* indexgroup = NULL;
    if (!PyArg_ParseTuple(args, "OiO!|nOO", &pattern, &flags,
                          &PyList_Type, &code, &groups,
                          &groupindex, &indexgroup))
        return NULL;

    n = PyList_GET_SIZE(code);

    self = PyObject_NEW_VAR(PatternObject, &Pattern_Type, n);
    if (!self)
        return NULL;

    self->codesize = n;

    for (i = 0; i < n; i++) {
        PyObject *o = PyList_GET_ITEM(code, i);
        unsigned long value = PyInt_Check(o) ? (unsigned long)PyInt_AsLong(o)
                                              : PyLong_AsUnsignedLong(o);
        self->code[i] = (SRE_CODE) value;
        if ((unsigned long) self->code[i] != value) {
            PyErr_SetString(PyExc_OverflowError,
                            "regular expression code size limit exceeded");
            break;
        }
    }

    if (PyErr_Occurred()) {
        PyObject_DEL(self);
        return NULL;
    }

    Py_INCREF(pattern);
    self->pattern = pattern;

    self->flags = flags;

    self->groups = groups;

    Py_XINCREF(groupindex);
    self->groupindex = groupindex;

    Py_XINCREF(indexgroup);
    self->indexgroup = indexgroup;

    self->weakreflist = NULL;

    return (PyObject*) self;
}

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
match_getslice_by_index(MatchObject* self, Py_ssize_t index, PyObject* def)
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

static Py_ssize_t
match_getindex(MatchObject* self, PyObject* index)
{
    Py_ssize_t i;

    if (PyInt_Check(index))
        return PyInt_AsSsize_t(index);

    i = -1;

    if (self->pattern->groupindex) {
        index = PyObject_GetItem(self->pattern->groupindex, index);
        if (index) {
            if (PyInt_Check(index) || PyLong_Check(index))
                i = PyInt_AsSsize_t(index);
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
match_expand(MatchObject* self, PyObject* ptemplate)
{
    /* delegate to Python code */
    return call(
        SRE_PY_MODULE, "_expand",
        PyTuple_Pack(3, self->pattern, self, ptemplate)
        );
}

static PyObject*
match_group(MatchObject* self, PyObject* args)
{
    PyObject* result;
    Py_ssize_t i, size;

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
    Py_ssize_t index;

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
    Py_ssize_t index;

    PyObject* def = Py_None;
    static char* kwlist[] = { "default", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O:groupdict", kwlist, &def))
        return NULL;

    result = PyDict_New();
    if (!result || !self->pattern->groupindex)
        return result;

    keys = PyMapping_Keys(self->pattern->groupindex);
    if (!keys)
        goto failed;

    for (index = 0; index < PyList_GET_SIZE(keys); index++) {
        int status;
        PyObject* key;
        PyObject* value;
        key = PyList_GET_ITEM(keys, index);
        if (!key)
            goto failed;
        value = match_getslice(self, key, def);
        if (!value) {
            Py_DECREF(key);
            goto failed;
        }
        status = PyDict_SetItem(result, key, value);
        Py_DECREF(value);
        if (status < 0)
            goto failed;
    }

    Py_DECREF(keys);

    return result;

failed:
    Py_XDECREF(keys);
    Py_DECREF(result);
    return NULL;
}

static PyObject*
match_start(MatchObject* self, PyObject* args)
{
    Py_ssize_t index;

    PyObject* index_ = Py_False; /* zero */
    if (!PyArg_UnpackTuple(args, "start", 0, 1, &index_))
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
    Py_ssize_t index;

    PyObject* index_ = Py_False; /* zero */
    if (!PyArg_UnpackTuple(args, "end", 0, 1, &index_))
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
_pair(Py_ssize_t i1, Py_ssize_t i2)
{
    PyObject* pair;
    PyObject* item;

    pair = PyTuple_New(2);
    if (!pair)
        return NULL;

    item = PyInt_FromSsize_t(i1);
    if (!item)
        goto error;
    PyTuple_SET_ITEM(pair, 0, item);

    item = PyInt_FromSsize_t(i2);
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
    Py_ssize_t index;

    PyObject* index_ = Py_False; /* zero */
    if (!PyArg_UnpackTuple(args, "span", 0, 1, &index_))
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
    Py_ssize_t index;

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

static PyObject*
match_copy(MatchObject* self, PyObject *unused)
{
#ifdef USE_BUILTIN_COPY
    MatchObject* copy;
    Py_ssize_t slots, offset;

    slots = 2 * (self->pattern->groups+1);

    copy = PyObject_NEW_VAR(MatchObject, &Match_Type, slots);
    if (!copy)
        return NULL;

    /* this value a constant, but any compiler should be able to
       figure that out all by itself */
    offset = offsetof(MatchObject, string);

    Py_XINCREF(self->pattern);
    Py_XINCREF(self->string);
    Py_XINCREF(self->regs);

    memcpy((char*) copy + offset, (char*) self + offset,
           sizeof(MatchObject) + slots * sizeof(Py_ssize_t) - offset);

    return (PyObject*) copy;
#else
    PyErr_SetString(PyExc_TypeError, "cannot copy this match object");
    return NULL;
#endif
}

static PyObject*
match_deepcopy(MatchObject* self, PyObject* memo)
{
#ifdef USE_BUILTIN_COPY
    MatchObject* copy;

    copy = (MatchObject*) match_copy(self);
    if (!copy)
        return NULL;

    if (!deepcopy((PyObject**) &copy->pattern, memo) ||
        !deepcopy(&copy->string, memo) ||
        !deepcopy(&copy->regs, memo)) {
        Py_DECREF(copy);
        return NULL;
    }

#else
    PyErr_SetString(PyExc_TypeError, "cannot deepcopy this match object");
    return NULL;
#endif
}

static PyMethodDef match_methods[] = {
    {"group", (PyCFunction) match_group, METH_VARARGS},
    {"start", (PyCFunction) match_start, METH_VARARGS},
    {"end", (PyCFunction) match_end, METH_VARARGS},
    {"span", (PyCFunction) match_span, METH_VARARGS},
    {"groups", (PyCFunction) match_groups, METH_VARARGS|METH_KEYWORDS},
    {"groupdict", (PyCFunction) match_groupdict, METH_VARARGS|METH_KEYWORDS},
    {"expand", (PyCFunction) match_expand, METH_O},
    {"__copy__", (PyCFunction) match_copy, METH_NOARGS},
    {"__deepcopy__", (PyCFunction) match_deepcopy, METH_O},
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
    0, "_" SRE_MODULE ".SRE_Match",
    sizeof(MatchObject), sizeof(Py_ssize_t),
    (destructor)match_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)match_getattr /*tp_getattr*/
};

static PyObject*
pattern_new_match(PatternObject* pattern, SRE_STATE* state, int status)
{
    /* create match object (from state object) */

    MatchObject* match;
    Py_ssize_t i, j;
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
scanner_match(ScannerObject* self, PyObject *unused)
{
    SRE_STATE* state = &self->state;
    PyObject* match;
    int status;

    state_reset(state);

    state->ptr = state->start;

    if (state->charsize == 1) {
        status = sre_match(state, PatternObject_GetCode(self->pattern));
    } else {
#if defined(HAVE_UNICODE)
        status = sre_umatch(state, PatternObject_GetCode(self->pattern));
#endif
    }
    if (PyErr_Occurred())
        return NULL;

    match = pattern_new_match((PatternObject*) self->pattern,
                               state, status);

    if (status == 0 || state->ptr == state->start)
        state->start = (void*) ((char*) state->ptr + state->charsize);
    else
        state->start = state->ptr;

    return match;
}


static PyObject*
scanner_search(ScannerObject* self, PyObject *unused)
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
    if (PyErr_Occurred())
        return NULL;

    match = pattern_new_match((PatternObject*) self->pattern,
                               state, status);

    if (status == 0 || state->ptr == state->start)
        state->start = (void*) ((char*) state->ptr + state->charsize);
    else
        state->start = state->ptr;

    return match;
}

static PyMethodDef scanner_methods[] = {
    {"match", (PyCFunction) scanner_match, METH_NOARGS},
    {"search", (PyCFunction) scanner_search, METH_NOARGS},
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
    0, "_" SRE_MODULE ".SRE_Scanner",
    sizeof(ScannerObject), 0,
    (destructor)scanner_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)scanner_getattr, /*tp_getattr*/
};

static PyObject*
pattern_scanner(PatternObject* pattern, PyObject* args)
{
    /* create search state object */

    ScannerObject* self;

    PyObject* string;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    if (!PyArg_ParseTuple(args, "O|nn:scanner", &string, &start, &end))
        return NULL;

    /* create scanner object */
    self = PyObject_NEW(ScannerObject, &Scanner_Type);
    if (!self)
        return NULL;

    string = state_init(&self->state, pattern, string, start, end);
    if (!string) {
        PyObject_DEL(self);
        return NULL;
    }

    Py_INCREF(pattern);
    self->pattern = (PyObject*) pattern;

    return (PyObject*) self;
}

static PyMethodDef _functions[] = {
    {"compile", _compile, METH_VARARGS},
    {"getcodesize", sre_codesize, METH_NOARGS},
    {"getlower", sre_getlower, METH_VARARGS},
    {NULL, NULL}
};

#if PY_VERSION_HEX < 0x02030000
DL_EXPORT(void) init_sre(void)
#else
PyMODINIT_FUNC init_sre(void)
#endif
{
    PyObject* m;
    PyObject* d;
    PyObject* x;

    /* Patch object types */
    Pattern_Type.ob_type = Match_Type.ob_type =
        Scanner_Type.ob_type = &PyType_Type;

    m = Py_InitModule("_" SRE_MODULE, _functions);
    if (m == NULL)
    	return;
    d = PyModule_GetDict(m);

    x = PyInt_FromLong(SRE_MAGIC);
    if (x) {
        PyDict_SetItemString(d, "MAGIC", x);
        Py_DECREF(x);
    }

    x = PyInt_FromLong(sizeof(SRE_CODE));
    if (x) {
        PyDict_SetItemString(d, "CODESIZE", x);
        Py_DECREF(x);
    }

    x = PyString_FromString(copyright);
    if (x) {
        PyDict_SetItemString(d, "copyright", x);
        Py_DECREF(x);
    }
}

#endif /* !defined(SRE_RECURSIVE) */

/* vim:ts=4:sw=4:et
*/
