/*
 * Secret Labs' Regular Expression Engine
 *
 * regular expression matching engine
 *
 * Copyright (c) 1997-2001 by Secret Labs AB.  All rights reserved.
 *
 * See the sre.c file for information on usage and redistribution.
 */

/* String matching engine */

/* This file is included three times, with different character settings */

LOCAL(int)
SRE(at)(SRE_STATE* state, const SRE_CHAR* ptr, SRE_CODE at)
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
        return (((SRE_CHAR *)state->end - ptr == 1 &&
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

    }

    return 0;
}

LOCAL(int)
SRE(charset)(SRE_STATE* state, const SRE_CODE* set, SRE_CODE ch)
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
            set++;
            break;

        case SRE_OP_CHARSET:
            /* <CHARSET> <bitmap> */
            if (ch < 256 &&
                (set[ch/SRE_CODE_BITS] & (1u << (ch & (SRE_CODE_BITS-1)))))
                return ok;
            set += 256/SRE_CODE_BITS;
            break;

        case SRE_OP_RANGE:
            /* <RANGE> <lower> <upper> */
            if (set[0] <= ch && ch <= set[1])
                return ok;
            set += 2;
            break;

        case SRE_OP_RANGE_UNI_IGNORE:
            /* <RANGE_UNI_IGNORE> <lower> <upper> */
        {
            SRE_CODE uch;
            /* ch is already lower cased */
            if (set[0] <= ch && ch <= set[1])
                return ok;
            uch = sre_upper_unicode(ch);
            if (set[0] <= uch && uch <= set[1])
                return ok;
            set += 2;
            break;
        }

        case SRE_OP_NEGATE:
            ok = !ok;
            break;

        case SRE_OP_BIGCHARSET:
            /* <BIGCHARSET> <blockcount> <256 blockindices> <blocks> */
        {
            Py_ssize_t count, block;
            count = *(set++);

            if (ch < 0x10000u)
                block = ((unsigned char*)set)[ch >> 8];
            else
                block = -1;
            set += 256/sizeof(SRE_CODE);
            if (block >=0 &&
                (set[(block * 256 + (ch & 255))/SRE_CODE_BITS] &
                    (1u << (ch & (SRE_CODE_BITS-1)))))
                return ok;
            set += count * (256/SRE_CODE_BITS);
            break;
        }

        default:
            /* internal error -- there's not much we can do about it
               here, so let's just pretend it didn't match... */
            return 0;
        }
    }
}

LOCAL(int)
SRE(charset_loc_ignore)(SRE_STATE* state, const SRE_CODE* set, SRE_CODE ch)
{
    SRE_CODE lo, up;
    lo = sre_lower_locale(ch);
    if (SRE(charset)(state, set, lo))
       return 1;

    up = sre_upper_locale(ch);
    return up != lo && SRE(charset)(state, set, up);
}

LOCAL(Py_ssize_t) SRE(match)(SRE_STATE* state, const SRE_CODE* pattern, int toplevel);

LOCAL(Py_ssize_t)
SRE(count)(SRE_STATE* state, const SRE_CODE* pattern, Py_ssize_t maxcount)
{
    SRE_CODE chr;
    SRE_CHAR c;
    const SRE_CHAR* ptr = (const SRE_CHAR *)state->ptr;
    const SRE_CHAR* end = (const SRE_CHAR *)state->end;
    Py_ssize_t i;

    /* adjust end */
    if (maxcount < end - ptr && maxcount != SRE_MAXREPEAT)
        end = ptr + maxcount;

    switch (pattern[0]) {

    case SRE_OP_IN:
        /* repeated set */
        TRACE(("|%p|%p|COUNT IN\n", pattern, ptr));
        while (ptr < end && SRE(charset)(state, pattern + 2, *ptr))
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
        c = (SRE_CHAR) chr;
#if SIZEOF_SRE_CHAR < 4
        if ((SRE_CODE) c != chr)
            ; /* literal can't match: doesn't fit in char width */
        else
#endif
        while (ptr < end && *ptr == c)
            ptr++;
        break;

    case SRE_OP_LITERAL_IGNORE:
        /* repeated literal */
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT LITERAL_IGNORE %d\n", pattern, ptr, chr));
        while (ptr < end && (SRE_CODE) sre_lower_ascii(*ptr) == chr)
            ptr++;
        break;

    case SRE_OP_LITERAL_UNI_IGNORE:
        /* repeated literal */
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT LITERAL_UNI_IGNORE %d\n", pattern, ptr, chr));
        while (ptr < end && (SRE_CODE) sre_lower_unicode(*ptr) == chr)
            ptr++;
        break;

    case SRE_OP_LITERAL_LOC_IGNORE:
        /* repeated literal */
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT LITERAL_LOC_IGNORE %d\n", pattern, ptr, chr));
        while (ptr < end && char_loc_ignore(chr, *ptr))
            ptr++;
        break;

    case SRE_OP_NOT_LITERAL:
        /* repeated non-literal */
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT NOT_LITERAL %d\n", pattern, ptr, chr));
        c = (SRE_CHAR) chr;
#if SIZEOF_SRE_CHAR < 4
        if ((SRE_CODE) c != chr)
            ptr = end; /* literal can't match: doesn't fit in char width */
        else
#endif
        while (ptr < end && *ptr != c)
            ptr++;
        break;

    case SRE_OP_NOT_LITERAL_IGNORE:
        /* repeated non-literal */
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT NOT_LITERAL_IGNORE %d\n", pattern, ptr, chr));
        while (ptr < end && (SRE_CODE) sre_lower_ascii(*ptr) != chr)
            ptr++;
        break;

    case SRE_OP_NOT_LITERAL_UNI_IGNORE:
        /* repeated non-literal */
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT NOT_LITERAL_UNI_IGNORE %d\n", pattern, ptr, chr));
        while (ptr < end && (SRE_CODE) sre_lower_unicode(*ptr) != chr)
            ptr++;
        break;

    case SRE_OP_NOT_LITERAL_LOC_IGNORE:
        /* repeated non-literal */
        chr = pattern[1];
        TRACE(("|%p|%p|COUNT NOT_LITERAL_LOC_IGNORE %d\n", pattern, ptr, chr));
        while (ptr < end && !char_loc_ignore(chr, *ptr))
            ptr++;
        break;

    default:
        /* repeated single character pattern */
        TRACE(("|%p|%p|COUNT SUBPATTERN\n", pattern, ptr));
        while ((SRE_CHAR*) state->ptr < end) {
            i = SRE(match)(state, pattern, 0);
            if (i < 0)
                return i;
            if (!i)
                break;
        }
        TRACE(("|%p|%p|COUNT %zd\n", pattern, ptr,
               (SRE_CHAR*) state->ptr - ptr));
        return (SRE_CHAR*) state->ptr - ptr;
    }

    TRACE(("|%p|%p|COUNT %zd\n", pattern, ptr,
           ptr - (SRE_CHAR*) state->ptr));
    return ptr - (SRE_CHAR*) state->ptr;
}

/* The macros below should be used to protect recursive SRE(match)()
 * calls that *failed* and do *not* return immediately (IOW, those
 * that will backtrack). Explaining:
 *
 * - Recursive SRE(match)() returned true: that's usually a success
 *   (besides atypical cases like ASSERT_NOT), therefore there's no
 *   reason to restore lastmark;
 *
 * - Recursive SRE(match)() returned false but the current SRE(match)()
 *   is returning to the caller: If the current SRE(match)() is the
 *   top function of the recursion, returning false will be a matching
 *   failure, and it doesn't matter where lastmark is pointing to.
 *   If it's *not* the top function, it will be a recursive SRE(match)()
 *   failure by itself, and the calling SRE(match)() will have to deal
 *   with the failure by the same rules explained here (it will restore
 *   lastmark by itself if necessary);
 *
 * - Recursive SRE(match)() returned false, and will continue the
 *   outside 'for' loop: must be protected when breaking, since the next
 *   OP could potentially depend on lastmark;
 *
 * - Recursive SRE(match)() returned false, and will be called again
 *   inside a local for/while loop: must be protected between each
 *   loop iteration, since the recursive SRE(match)() could do anything,
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

#define DATA_STACK_ALLOC(state, type, ptr) \
do { \
    alloc_pos = state->data_stack_base; \
    TRACE(("allocating %s in %zd (%zd)\n", \
           Py_STRINGIFY(type), alloc_pos, sizeof(type))); \
    if (sizeof(type) > state->data_stack_size - alloc_pos) { \
        int j = data_stack_grow(state, sizeof(type)); \
        if (j < 0) return j; \
        if (ctx_pos != -1) \
            DATA_STACK_LOOKUP_AT(state, SRE(match_context), ctx, ctx_pos); \
    } \
    ptr = (type*)(state->data_stack+alloc_pos); \
    state->data_stack_base += sizeof(type); \
} while (0)

#define DATA_STACK_LOOKUP_AT(state, type, ptr, pos) \
do { \
    TRACE(("looking up %s at %zd\n", Py_STRINGIFY(type), pos)); \
    ptr = (type*)(state->data_stack+pos); \
} while (0)

#define DATA_STACK_PUSH(state, data, size) \
do { \
    TRACE(("copy data in %p to %zd (%zd)\n", \
           data, state->data_stack_base, size)); \
    if (size > state->data_stack_size - state->data_stack_base) { \
        int j = data_stack_grow(state, size); \
        if (j < 0) return j; \
        if (ctx_pos != -1) \
            DATA_STACK_LOOKUP_AT(state, SRE(match_context), ctx, ctx_pos); \
    } \
    memcpy(state->data_stack+state->data_stack_base, data, size); \
    state->data_stack_base += size; \
} while (0)

/* We add an explicit cast to memcpy here because MSVC has a bug when
   compiling C code where it believes that `const void**` cannot be
   safely casted to `void*`, see bpo-39943 for details. */
#define DATA_STACK_POP(state, data, size, discard) \
do { \
    TRACE(("copy data to %p from %zd (%zd)\n", \
           data, state->data_stack_base-size, size)); \
    memcpy((void*) data, state->data_stack+state->data_stack_base-size, size); \
    if (discard) \
        state->data_stack_base -= size; \
} while (0)

#define DATA_STACK_POP_DISCARD(state, size) \
do { \
    TRACE(("discard data from %zd (%zd)\n", \
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
    do if (lastmark >= 0) { \
        size_t _marks_size = (lastmark+1) * sizeof(void*); \
        DATA_STACK_PUSH(state, state->mark, _marks_size); \
    } while (0)
#define MARK_POP(lastmark) \
    do if (lastmark >= 0) { \
        size_t _marks_size = (lastmark+1) * sizeof(void*); \
        DATA_STACK_POP(state, state->mark, _marks_size, 1); \
    } while (0)
#define MARK_POP_KEEP(lastmark) \
    do if (lastmark >= 0) { \
        size_t _marks_size = (lastmark+1) * sizeof(void*); \
        DATA_STACK_POP(state, state->mark, _marks_size, 0); \
    } while (0)
#define MARK_POP_DISCARD(lastmark) \
    do if (lastmark >= 0) { \
        size_t _marks_size = (lastmark+1) * sizeof(void*); \
        DATA_STACK_POP_DISCARD(state, _marks_size); \
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
#define JUMP_POSS_REPEAT_1   14
#define JUMP_POSS_REPEAT_2   15
#define JUMP_ATOMIC_GROUP    16

#define DO_JUMPX(jumpvalue, jumplabel, nextpattern, toplevel_) \
    ctx->pattern = pattern; \
    ctx->ptr = ptr; \
    DATA_ALLOC(SRE(match_context), nextctx); \
    nextctx->pattern = nextpattern; \
    nextctx->toplevel = toplevel_; \
    nextctx->jump = jumpvalue; \
    nextctx->last_ctx_pos = ctx_pos; \
    pattern = nextpattern; \
    ctx_pos = alloc_pos; \
    ctx = nextctx; \
    goto entrance; \
    jumplabel: \
    pattern = ctx->pattern; \
    ptr = ctx->ptr;

#define DO_JUMP(jumpvalue, jumplabel, nextpattern) \
    DO_JUMPX(jumpvalue, jumplabel, nextpattern, ctx->toplevel)

#define DO_JUMP0(jumpvalue, jumplabel, nextpattern) \
    DO_JUMPX(jumpvalue, jumplabel, nextpattern, 0)

typedef struct {
    Py_ssize_t count;
    union {
        SRE_CODE chr;
        SRE_REPEAT* rep;
    } u;
    int lastmark;
    int lastindex;
    const SRE_CODE* pattern;
    const SRE_CHAR* ptr;
    int toplevel;
    int jump;
    Py_ssize_t last_ctx_pos;
} SRE(match_context);

#define MAYBE_CHECK_SIGNALS                                        \
    do {                                                           \
        if ((0 == (++sigcount & 0xfff)) && PyErr_CheckSignals()) { \
            RETURN_ERROR(SRE_ERROR_INTERRUPTED);                   \
        }                                                          \
    } while (0)

#ifdef HAVE_COMPUTED_GOTOS
    #ifndef USE_COMPUTED_GOTOS
    #define USE_COMPUTED_GOTOS 1
    #endif
#elif defined(USE_COMPUTED_GOTOS) && USE_COMPUTED_GOTOS
    #error "Computed gotos are not supported on this compiler."
#else
    #undef USE_COMPUTED_GOTOS
    #define USE_COMPUTED_GOTOS 0
#endif

#if USE_COMPUTED_GOTOS
    #define TARGET(OP) TARGET_ ## OP
    #define DISPATCH                       \
        do {                               \
            MAYBE_CHECK_SIGNALS;           \
            goto *sre_targets[*pattern++]; \
        } while (0)
#else
    #define TARGET(OP) case OP
    #define DISPATCH goto dispatch
#endif

/* check if string matches the given pattern.  returns <0 for
   error, 0 for failure, and 1 for success */
LOCAL(Py_ssize_t)
SRE(match)(SRE_STATE* state, const SRE_CODE* pattern, int toplevel)
{
    const SRE_CHAR* end = (const SRE_CHAR *)state->end;
    Py_ssize_t alloc_pos, ctx_pos = -1;
    Py_ssize_t ret = 0;
    int jump;
    unsigned int sigcount = state->sigcount;

    SRE(match_context)* ctx;
    SRE(match_context)* nextctx;

    TRACE(("|%p|%p|ENTER\n", pattern, state->ptr));

    DATA_ALLOC(SRE(match_context), ctx);
    ctx->last_ctx_pos = -1;
    ctx->jump = JUMP_NONE;
    ctx->toplevel = toplevel;
    ctx_pos = alloc_pos;

#if USE_COMPUTED_GOTOS
#include "sre_targets.h"
#endif

entrance:

    ;  // Fashion statement.
    const SRE_CHAR *ptr = (SRE_CHAR *)state->ptr;

    if (pattern[0] == SRE_OP_INFO) {
        /* optimization info block */
        /* <INFO> <1=skip> <2=flags> <3=min> ... */
        if (pattern[3] && (uintptr_t)(end - ptr) < pattern[3]) {
            TRACE(("reject (got %tu chars, need %zu)\n",
                   end - ptr, (size_t) pattern[3]));
            RETURN_FAILURE;
        }
        pattern += pattern[1] + 1;
    }

#if USE_COMPUTED_GOTOS
    DISPATCH;
#else
dispatch:
    MAYBE_CHECK_SIGNALS;
    switch (*pattern++)
#endif
    {

        TARGET(SRE_OP_MARK):
            /* set mark */
            /* <MARK> <gid> */
            TRACE(("|%p|%p|MARK %d\n", pattern,
                   ptr, pattern[0]));
            {
                int i = pattern[0];
                if (i & 1)
                    state->lastindex = i/2 + 1;
                if (i > state->lastmark) {
                    /* state->lastmark is the highest valid index in the
                       state->mark array.  If it is increased by more than 1,
                       the intervening marks must be set to NULL to signal
                       that these marks have not been encountered. */
                    int j = state->lastmark + 1;
                    while (j < i)
                        state->mark[j++] = NULL;
                    state->lastmark = i;
                }
                state->mark[i] = ptr;
            }
            pattern++;
            DISPATCH;

        TARGET(SRE_OP_LITERAL):
            /* match literal string */
            /* <LITERAL> <code> */
            TRACE(("|%p|%p|LITERAL %d\n", pattern,
                   ptr, *pattern));
            if (ptr >= end || (SRE_CODE) ptr[0] != pattern[0])
                RETURN_FAILURE;
            pattern++;
            ptr++;
            DISPATCH;

        TARGET(SRE_OP_NOT_LITERAL):
            /* match anything that is not literal character */
            /* <NOT_LITERAL> <code> */
            TRACE(("|%p|%p|NOT_LITERAL %d\n", pattern,
                   ptr, *pattern));
            if (ptr >= end || (SRE_CODE) ptr[0] == pattern[0])
                RETURN_FAILURE;
            pattern++;
            ptr++;
            DISPATCH;

        TARGET(SRE_OP_SUCCESS):
            /* end of pattern */
            TRACE(("|%p|%p|SUCCESS\n", pattern, ptr));
            if (ctx->toplevel &&
                ((state->match_all && ptr != state->end) ||
                 (state->must_advance && ptr == state->start)))
            {
                RETURN_FAILURE;
            }
            state->ptr = ptr;
            RETURN_SUCCESS;

        TARGET(SRE_OP_AT):
            /* match at given position */
            /* <AT> <code> */
            TRACE(("|%p|%p|AT %d\n", pattern, ptr, *pattern));
            if (!SRE(at)(state, ptr, *pattern))
                RETURN_FAILURE;
            pattern++;
            DISPATCH;

        TARGET(SRE_OP_CATEGORY):
            /* match at given category */
            /* <CATEGORY> <code> */
            TRACE(("|%p|%p|CATEGORY %d\n", pattern,
                   ptr, *pattern));
            if (ptr >= end || !sre_category(pattern[0], ptr[0]))
                RETURN_FAILURE;
            pattern++;
            ptr++;
            DISPATCH;

        TARGET(SRE_OP_ANY):
            /* match anything (except a newline) */
            /* <ANY> */
            TRACE(("|%p|%p|ANY\n", pattern, ptr));
            if (ptr >= end || SRE_IS_LINEBREAK(ptr[0]))
                RETURN_FAILURE;
            ptr++;
            DISPATCH;

        TARGET(SRE_OP_ANY_ALL):
            /* match anything */
            /* <ANY_ALL> */
            TRACE(("|%p|%p|ANY_ALL\n", pattern, ptr));
            if (ptr >= end)
                RETURN_FAILURE;
            ptr++;
            DISPATCH;

        TARGET(SRE_OP_IN):
            /* match set member (or non_member) */
            /* <IN> <skip> <set> */
            TRACE(("|%p|%p|IN\n", pattern, ptr));
            if (ptr >= end ||
                !SRE(charset)(state, pattern + 1, *ptr))
                RETURN_FAILURE;
            pattern += pattern[0];
            ptr++;
            DISPATCH;

        TARGET(SRE_OP_LITERAL_IGNORE):
            TRACE(("|%p|%p|LITERAL_IGNORE %d\n",
                   pattern, ptr, pattern[0]));
            if (ptr >= end ||
                sre_lower_ascii(*ptr) != *pattern)
                RETURN_FAILURE;
            pattern++;
            ptr++;
            DISPATCH;

        TARGET(SRE_OP_LITERAL_UNI_IGNORE):
            TRACE(("|%p|%p|LITERAL_UNI_IGNORE %d\n",
                   pattern, ptr, pattern[0]));
            if (ptr >= end ||
                sre_lower_unicode(*ptr) != *pattern)
                RETURN_FAILURE;
            pattern++;
            ptr++;
            DISPATCH;

        TARGET(SRE_OP_LITERAL_LOC_IGNORE):
            TRACE(("|%p|%p|LITERAL_LOC_IGNORE %d\n",
                   pattern, ptr, pattern[0]));
            if (ptr >= end
                || !char_loc_ignore(*pattern, *ptr))
                RETURN_FAILURE;
            pattern++;
            ptr++;
            DISPATCH;

        TARGET(SRE_OP_NOT_LITERAL_IGNORE):
            TRACE(("|%p|%p|NOT_LITERAL_IGNORE %d\n",
                   pattern, ptr, *pattern));
            if (ptr >= end ||
                sre_lower_ascii(*ptr) == *pattern)
                RETURN_FAILURE;
            pattern++;
            ptr++;
            DISPATCH;

        TARGET(SRE_OP_NOT_LITERAL_UNI_IGNORE):
            TRACE(("|%p|%p|NOT_LITERAL_UNI_IGNORE %d\n",
                   pattern, ptr, *pattern));
            if (ptr >= end ||
                sre_lower_unicode(*ptr) == *pattern)
                RETURN_FAILURE;
            pattern++;
            ptr++;
            DISPATCH;

        TARGET(SRE_OP_NOT_LITERAL_LOC_IGNORE):
            TRACE(("|%p|%p|NOT_LITERAL_LOC_IGNORE %d\n",
                   pattern, ptr, *pattern));
            if (ptr >= end
                || char_loc_ignore(*pattern, *ptr))
                RETURN_FAILURE;
            pattern++;
            ptr++;
            DISPATCH;

        TARGET(SRE_OP_IN_IGNORE):
            TRACE(("|%p|%p|IN_IGNORE\n", pattern, ptr));
            if (ptr >= end
                || !SRE(charset)(state, pattern+1,
                                 (SRE_CODE)sre_lower_ascii(*ptr)))
                RETURN_FAILURE;
            pattern += pattern[0];
            ptr++;
            DISPATCH;

        TARGET(SRE_OP_IN_UNI_IGNORE):
            TRACE(("|%p|%p|IN_UNI_IGNORE\n", pattern, ptr));
            if (ptr >= end
                || !SRE(charset)(state, pattern+1,
                                 (SRE_CODE)sre_lower_unicode(*ptr)))
                RETURN_FAILURE;
            pattern += pattern[0];
            ptr++;
            DISPATCH;

        TARGET(SRE_OP_IN_LOC_IGNORE):
            TRACE(("|%p|%p|IN_LOC_IGNORE\n", pattern, ptr));
            if (ptr >= end
                || !SRE(charset_loc_ignore)(state, pattern+1, *ptr))
                RETURN_FAILURE;
            pattern += pattern[0];
            ptr++;
            DISPATCH;

        TARGET(SRE_OP_JUMP):
        TARGET(SRE_OP_INFO):
            /* jump forward */
            /* <JUMP> <offset> */
            TRACE(("|%p|%p|JUMP %d\n", pattern,
                   ptr, pattern[0]));
            pattern += pattern[0];
            DISPATCH;

        TARGET(SRE_OP_BRANCH):
            /* alternation */
            /* <BRANCH> <0=skip> code <JUMP> ... <NULL> */
            TRACE(("|%p|%p|BRANCH\n", pattern, ptr));
            LASTMARK_SAVE();
            if (state->repeat)
                MARK_PUSH(ctx->lastmark);
            for (; pattern[0]; pattern += pattern[0]) {
                if (pattern[1] == SRE_OP_LITERAL &&
                    (ptr >= end ||
                     (SRE_CODE) *ptr != pattern[2]))
                    continue;
                if (pattern[1] == SRE_OP_IN &&
                    (ptr >= end ||
                     !SRE(charset)(state, pattern + 3,
                                   (SRE_CODE) *ptr)))
                    continue;
                state->ptr = ptr;
                DO_JUMP(JUMP_BRANCH, jump_branch, pattern+1);
                if (ret) {
                    if (state->repeat)
                        MARK_POP_DISCARD(ctx->lastmark);
                    RETURN_ON_ERROR(ret);
                    RETURN_SUCCESS;
                }
                if (state->repeat)
                    MARK_POP_KEEP(ctx->lastmark);
                LASTMARK_RESTORE();
            }
            if (state->repeat)
                MARK_POP_DISCARD(ctx->lastmark);
            RETURN_FAILURE;

        TARGET(SRE_OP_REPEAT_ONE):
            /* match repeated sequence (maximizing regexp) */

            /* this operator only works if the repeated item is
               exactly one character wide, and we're not already
               collecting backtracking points.  for other cases,
               use the MAX_REPEAT operator */

            /* <REPEAT_ONE> <skip> <1=min> <2=max> item <SUCCESS> tail */

            TRACE(("|%p|%p|REPEAT_ONE %d %d\n", pattern, ptr,
                   pattern[1], pattern[2]));

            if ((Py_ssize_t) pattern[1] > end - ptr)
                RETURN_FAILURE; /* cannot match */

            state->ptr = ptr;

            ret = SRE(count)(state, pattern+3, pattern[2]);
            RETURN_ON_ERROR(ret);
            DATA_LOOKUP_AT(SRE(match_context), ctx, ctx_pos);
            ctx->count = ret;
            ptr += ctx->count;

            /* when we arrive here, count contains the number of
               matches, and ptr points to the tail of the target
               string.  check if the rest of the pattern matches,
               and backtrack if not. */

            if (ctx->count < (Py_ssize_t) pattern[1])
                RETURN_FAILURE;

            if (pattern[pattern[0]] == SRE_OP_SUCCESS &&
                ptr == state->end &&
                !(ctx->toplevel && state->must_advance && ptr == state->start))
            {
                /* tail is empty.  we're finished */
                state->ptr = ptr;
                RETURN_SUCCESS;
            }

            LASTMARK_SAVE();
            if (state->repeat)
                MARK_PUSH(ctx->lastmark);

            if (pattern[pattern[0]] == SRE_OP_LITERAL) {
                /* tail starts with a literal. skip positions where
                   the rest of the pattern cannot possibly match */
                ctx->u.chr = pattern[pattern[0]+1];
                for (;;) {
                    while (ctx->count >= (Py_ssize_t) pattern[1] &&
                           (ptr >= end || *ptr != ctx->u.chr)) {
                        ptr--;
                        ctx->count--;
                    }
                    if (ctx->count < (Py_ssize_t) pattern[1])
                        break;
                    state->ptr = ptr;
                    DO_JUMP(JUMP_REPEAT_ONE_1, jump_repeat_one_1,
                            pattern+pattern[0]);
                    if (ret) {
                        if (state->repeat)
                            MARK_POP_DISCARD(ctx->lastmark);
                        RETURN_ON_ERROR(ret);
                        RETURN_SUCCESS;
                    }
                    if (state->repeat)
                        MARK_POP_KEEP(ctx->lastmark);
                    LASTMARK_RESTORE();

                    ptr--;
                    ctx->count--;
                }
                if (state->repeat)
                    MARK_POP_DISCARD(ctx->lastmark);
            } else {
                /* general case */
                while (ctx->count >= (Py_ssize_t) pattern[1]) {
                    state->ptr = ptr;
                    DO_JUMP(JUMP_REPEAT_ONE_2, jump_repeat_one_2,
                            pattern+pattern[0]);
                    if (ret) {
                        if (state->repeat)
                            MARK_POP_DISCARD(ctx->lastmark);
                        RETURN_ON_ERROR(ret);
                        RETURN_SUCCESS;
                    }
                    if (state->repeat)
                        MARK_POP_KEEP(ctx->lastmark);
                    LASTMARK_RESTORE();

                    ptr--;
                    ctx->count--;
                }
                if (state->repeat)
                    MARK_POP_DISCARD(ctx->lastmark);
            }
            RETURN_FAILURE;

        TARGET(SRE_OP_MIN_REPEAT_ONE):
            /* match repeated sequence (minimizing regexp) */

            /* this operator only works if the repeated item is
               exactly one character wide, and we're not already
               collecting backtracking points.  for other cases,
               use the MIN_REPEAT operator */

            /* <MIN_REPEAT_ONE> <skip> <1=min> <2=max> item <SUCCESS> tail */

            TRACE(("|%p|%p|MIN_REPEAT_ONE %d %d\n", pattern, ptr,
                   pattern[1], pattern[2]));

            if ((Py_ssize_t) pattern[1] > end - ptr)
                RETURN_FAILURE; /* cannot match */

            state->ptr = ptr;

            if (pattern[1] == 0)
                ctx->count = 0;
            else {
                /* count using pattern min as the maximum */
                ret = SRE(count)(state, pattern+3, pattern[1]);
                RETURN_ON_ERROR(ret);
                DATA_LOOKUP_AT(SRE(match_context), ctx, ctx_pos);
                if (ret < (Py_ssize_t) pattern[1])
                    /* didn't match minimum number of times */
                    RETURN_FAILURE;
                /* advance past minimum matches of repeat */
                ctx->count = ret;
                ptr += ctx->count;
            }

            if (pattern[pattern[0]] == SRE_OP_SUCCESS &&
                !(ctx->toplevel &&
                  ((state->match_all && ptr != state->end) ||
                   (state->must_advance && ptr == state->start))))
            {
                /* tail is empty.  we're finished */
                state->ptr = ptr;
                RETURN_SUCCESS;

            } else {
                /* general case */
                LASTMARK_SAVE();
                if (state->repeat)
                    MARK_PUSH(ctx->lastmark);

                while ((Py_ssize_t)pattern[2] == SRE_MAXREPEAT
                       || ctx->count <= (Py_ssize_t)pattern[2]) {
                    state->ptr = ptr;
                    DO_JUMP(JUMP_MIN_REPEAT_ONE,jump_min_repeat_one,
                            pattern+pattern[0]);
                    if (ret) {
                        if (state->repeat)
                            MARK_POP_DISCARD(ctx->lastmark);
                        RETURN_ON_ERROR(ret);
                        RETURN_SUCCESS;
                    }
                    if (state->repeat)
                        MARK_POP_KEEP(ctx->lastmark);
                    LASTMARK_RESTORE();

                    state->ptr = ptr;
                    ret = SRE(count)(state, pattern+3, 1);
                    RETURN_ON_ERROR(ret);
                    DATA_LOOKUP_AT(SRE(match_context), ctx, ctx_pos);
                    if (ret == 0)
                        break;
                    assert(ret == 1);
                    ptr++;
                    ctx->count++;
                }
                if (state->repeat)
                    MARK_POP_DISCARD(ctx->lastmark);
            }
            RETURN_FAILURE;

        TARGET(SRE_OP_POSSESSIVE_REPEAT_ONE):
            /* match repeated sequence (maximizing regexp) without
               backtracking */

            /* this operator only works if the repeated item is
               exactly one character wide, and we're not already
               collecting backtracking points.  for other cases,
               use the MAX_REPEAT operator */

            /* <POSSESSIVE_REPEAT_ONE> <skip> <1=min> <2=max> item <SUCCESS>
               tail */

            TRACE(("|%p|%p|POSSESSIVE_REPEAT_ONE %d %d\n", pattern,
                   ptr, pattern[1], pattern[2]));

            if (ptr + pattern[1] > end) {
                RETURN_FAILURE; /* cannot match */
            }

            state->ptr = ptr;

            ret = SRE(count)(state, pattern + 3, pattern[2]);
            RETURN_ON_ERROR(ret);
            DATA_LOOKUP_AT(SRE(match_context), ctx, ctx_pos);
            ctx->count = ret;
            ptr += ctx->count;

            /* when we arrive here, count contains the number of
               matches, and ptr points to the tail of the target
               string.  check if the rest of the pattern matches,
               and fail if not. */

            /* Test for not enough repetitions in match */
            if (ctx->count < (Py_ssize_t) pattern[1]) {
                RETURN_FAILURE;
            }

            /* Update the pattern to point to the next op code */
            pattern += pattern[0];

            /* Let the tail be evaluated separately and consider this
               match successful. */
            if (*pattern == SRE_OP_SUCCESS &&
                ptr == state->end &&
                !(ctx->toplevel && state->must_advance && ptr == state->start))
            {
                /* tail is empty.  we're finished */
                state->ptr = ptr;
                RETURN_SUCCESS;
            }

            /* Attempt to match the rest of the string */
            DISPATCH;

        TARGET(SRE_OP_REPEAT):
            /* create repeat context.  all the hard work is done
               by the UNTIL operator (MAX_UNTIL, MIN_UNTIL) */
            /* <REPEAT> <skip> <1=min> <2=max>
               <3=repeat_index> item <UNTIL> tail */
            TRACE(("|%p|%p|REPEAT %d %d\n", pattern, ptr,
                   pattern[1], pattern[2]));

            /* install new repeat context */
            /* TODO(https://github.com/python/cpython/issues/67877): Fix this
             * potential memory leak. */
            ctx->u.rep = (SRE_REPEAT*) PyObject_Malloc(sizeof(*ctx->u.rep));
            if (!ctx->u.rep) {
                PyErr_NoMemory();
                RETURN_FAILURE;
            }
            ctx->u.rep->count = -1;
            ctx->u.rep->pattern = pattern;
            ctx->u.rep->prev = state->repeat;
            ctx->u.rep->last_ptr = NULL;
            state->repeat = ctx->u.rep;

            state->ptr = ptr;
            DO_JUMP(JUMP_REPEAT, jump_repeat, pattern+pattern[0]);
            state->repeat = ctx->u.rep->prev;
            PyObject_Free(ctx->u.rep);

            if (ret) {
                RETURN_ON_ERROR(ret);
                RETURN_SUCCESS;
            }
            RETURN_FAILURE;

        TARGET(SRE_OP_MAX_UNTIL):
            /* maximizing repeat */
            /* <REPEAT> <skip> <1=min> <2=max> item <MAX_UNTIL> tail */

            /* FIXME: we probably need to deal with zero-width
               matches in here... */

            ctx->u.rep = state->repeat;
            if (!ctx->u.rep)
                RETURN_ERROR(SRE_ERROR_STATE);

            state->ptr = ptr;

            ctx->count = ctx->u.rep->count+1;

            TRACE(("|%p|%p|MAX_UNTIL %zd\n", pattern,
                   ptr, ctx->count));

            if (ctx->count < (Py_ssize_t) ctx->u.rep->pattern[1]) {
                /* not enough matches */
                ctx->u.rep->count = ctx->count;
                DO_JUMP(JUMP_MAX_UNTIL_1, jump_max_until_1,
                        ctx->u.rep->pattern+3);
                if (ret) {
                    RETURN_ON_ERROR(ret);
                    RETURN_SUCCESS;
                }
                ctx->u.rep->count = ctx->count-1;
                state->ptr = ptr;
                RETURN_FAILURE;
            }

            if ((ctx->count < (Py_ssize_t) ctx->u.rep->pattern[2] ||
                ctx->u.rep->pattern[2] == SRE_MAXREPEAT) &&
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
                state->ptr = ptr;
            }

            /* cannot match more repeated items here.  make sure the
               tail matches */
            state->repeat = ctx->u.rep->prev;
            DO_JUMP(JUMP_MAX_UNTIL_3, jump_max_until_3, pattern);
            state->repeat = ctx->u.rep; // restore repeat before return

            RETURN_ON_SUCCESS(ret);
            state->ptr = ptr;
            RETURN_FAILURE;

        TARGET(SRE_OP_MIN_UNTIL):
            /* minimizing repeat */
            /* <REPEAT> <skip> <1=min> <2=max> item <MIN_UNTIL> tail */

            ctx->u.rep = state->repeat;
            if (!ctx->u.rep)
                RETURN_ERROR(SRE_ERROR_STATE);

            state->ptr = ptr;

            ctx->count = ctx->u.rep->count+1;

            TRACE(("|%p|%p|MIN_UNTIL %zd %p\n", pattern,
                   ptr, ctx->count, ctx->u.rep->pattern));

            if (ctx->count < (Py_ssize_t) ctx->u.rep->pattern[1]) {
                /* not enough matches */
                ctx->u.rep->count = ctx->count;
                DO_JUMP(JUMP_MIN_UNTIL_1, jump_min_until_1,
                        ctx->u.rep->pattern+3);
                if (ret) {
                    RETURN_ON_ERROR(ret);
                    RETURN_SUCCESS;
                }
                ctx->u.rep->count = ctx->count-1;
                state->ptr = ptr;
                RETURN_FAILURE;
            }

            /* see if the tail matches */
            state->repeat = ctx->u.rep->prev;

            LASTMARK_SAVE();
            if (state->repeat)
                MARK_PUSH(ctx->lastmark);

            DO_JUMP(JUMP_MIN_UNTIL_2, jump_min_until_2, pattern);
            SRE_REPEAT *repeat_of_tail = state->repeat;
            state->repeat = ctx->u.rep; // restore repeat before return

            if (ret) {
                if (repeat_of_tail)
                    MARK_POP_DISCARD(ctx->lastmark);
                RETURN_ON_ERROR(ret);
                RETURN_SUCCESS;
            }
            if (repeat_of_tail)
                MARK_POP(ctx->lastmark);
            LASTMARK_RESTORE();

            state->ptr = ptr;

            if ((ctx->count >= (Py_ssize_t) ctx->u.rep->pattern[2]
                && ctx->u.rep->pattern[2] != SRE_MAXREPEAT) ||
                state->ptr == ctx->u.rep->last_ptr)
                RETURN_FAILURE;

            ctx->u.rep->count = ctx->count;
            /* zero-width match protection */
            DATA_PUSH(&ctx->u.rep->last_ptr);
            ctx->u.rep->last_ptr = state->ptr;
            DO_JUMP(JUMP_MIN_UNTIL_3,jump_min_until_3,
                    ctx->u.rep->pattern+3);
            DATA_POP(&ctx->u.rep->last_ptr);
            if (ret) {
                RETURN_ON_ERROR(ret);
                RETURN_SUCCESS;
            }
            ctx->u.rep->count = ctx->count-1;
            state->ptr = ptr;
            RETURN_FAILURE;

        TARGET(SRE_OP_POSSESSIVE_REPEAT):
            /* create possessive repeat contexts. */
            /* <POSSESSIVE_REPEAT> <skip> <1=min> <2=max> pattern
               <SUCCESS> tail */
            TRACE(("|%p|%p|POSSESSIVE_REPEAT %d %d\n", pattern,
                   ptr, pattern[1], pattern[2]));

            /* Set the global Input pointer to this context's Input
               pointer */
            state->ptr = ptr;

            /* Initialize Count to 0 */
            ctx->count = 0;

            /* Check for minimum required matches. */
            while (ctx->count < (Py_ssize_t)pattern[1]) {
                /* not enough matches */
                DO_JUMP0(JUMP_POSS_REPEAT_1, jump_poss_repeat_1,
                         &pattern[3]);
                if (ret) {
                    RETURN_ON_ERROR(ret);
                    ctx->count++;
                }
                else {
                    state->ptr = ptr;
                    RETURN_FAILURE;
                }
            }

            /* Clear the context's Input stream pointer so that it
               doesn't match the global state so that the while loop can
               be entered. */
            ptr = NULL;

            /* Keep trying to parse the <pattern> sub-pattern until the
               end is reached, creating a new context each time. */
            while ((ctx->count < (Py_ssize_t)pattern[2] ||
                    (Py_ssize_t)pattern[2] == SRE_MAXREPEAT) &&
                   state->ptr != ptr) {
                /* Save the Capture Group Marker state into the current
                   Context and back up the current highest number
                   Capture Group marker. */
                LASTMARK_SAVE();
                MARK_PUSH(ctx->lastmark);

                /* zero-width match protection */
                /* Set the context's Input Stream pointer to be the
                   current Input Stream pointer from the global
                   state.  When the loop reaches the next iteration,
                   the context will then store the last known good
                   position with the global state holding the Input
                   Input Stream position that has been updated with
                   the most recent match.  Thus, if state's Input
                   stream remains the same as the one stored in the
                   current Context, we know we have successfully
                   matched an empty string and that all subsequent
                   matches will also be the empty string until the
                   maximum number of matches are counted, and because
                   of this, we could immediately stop at that point and
                   consider this match successful. */
                ptr = state->ptr;

                /* We have not reached the maximin matches, so try to
                   match once more. */
                DO_JUMP0(JUMP_POSS_REPEAT_2, jump_poss_repeat_2,
                         &pattern[3]);

                /* Check to see if the last attempted match
                   succeeded. */
                if (ret) {
                    /* Drop the saved highest number Capture Group
                       marker saved above and use the newly updated
                       value. */
                    MARK_POP_DISCARD(ctx->lastmark);
                    RETURN_ON_ERROR(ret);

                    /* Success, increment the count. */
                    ctx->count++;
                }
                /* Last attempted match failed. */
                else {
                    /* Restore the previously saved highest number
                       Capture Group marker since the last iteration
                       did not match, then restore that to the global
                       state. */
                    MARK_POP(ctx->lastmark);
                    LASTMARK_RESTORE();

                    /* Restore the global Input Stream pointer
                       since it can change after jumps. */
                    state->ptr = ptr;

                    /* We have sufficient matches, so exit loop. */
                    break;
                }
            }

            /* Evaluate Tail */
            /* Jump to end of pattern indicated by skip, and then skip
               the SUCCESS op code that follows it. */
            pattern += pattern[0] + 1;
            ptr = state->ptr;
            DISPATCH;

        TARGET(SRE_OP_ATOMIC_GROUP):
            /* Atomic Group Sub Pattern */
            /* <ATOMIC_GROUP> <skip> pattern <SUCCESS> tail */
            TRACE(("|%p|%p|ATOMIC_GROUP\n", pattern, ptr));

            /* Set the global Input pointer to this context's Input
               pointer */
            state->ptr = ptr;

            /* Evaluate the Atomic Group in a new context, terminating
               when the end of the group, represented by a SUCCESS op
               code, is reached. */
            /* Group Pattern begins at an offset of 1 code. */
            DO_JUMP0(JUMP_ATOMIC_GROUP, jump_atomic_group,
                     &pattern[1]);

            /* Test Exit Condition */
            RETURN_ON_ERROR(ret);

            if (ret == 0) {
                /* Atomic Group failed to Match. */
                state->ptr = ptr;
                RETURN_FAILURE;
            }

            /* Evaluate Tail */
            /* Jump to end of pattern indicated by skip, and then skip
               the SUCCESS op code that follows it. */
            pattern += pattern[0];
            ptr = state->ptr;
            DISPATCH;

        TARGET(SRE_OP_GROUPREF):
            /* match backreference */
            TRACE(("|%p|%p|GROUPREF %d\n", pattern,
                   ptr, pattern[0]));
            {
                int groupref = pattern[0] * 2;
                if (groupref >= state->lastmark) {
                    RETURN_FAILURE;
                } else {
                    SRE_CHAR* p = (SRE_CHAR*) state->mark[groupref];
                    SRE_CHAR* e = (SRE_CHAR*) state->mark[groupref+1];
                    if (!p || !e || e < p)
                        RETURN_FAILURE;
                    while (p < e) {
                        if (ptr >= end || *ptr != *p)
                            RETURN_FAILURE;
                        p++;
                        ptr++;
                    }
                }
            }
            pattern++;
            DISPATCH;

        TARGET(SRE_OP_GROUPREF_IGNORE):
            /* match backreference */
            TRACE(("|%p|%p|GROUPREF_IGNORE %d\n", pattern,
                   ptr, pattern[0]));
            {
                int groupref = pattern[0] * 2;
                if (groupref >= state->lastmark) {
                    RETURN_FAILURE;
                } else {
                    SRE_CHAR* p = (SRE_CHAR*) state->mark[groupref];
                    SRE_CHAR* e = (SRE_CHAR*) state->mark[groupref+1];
                    if (!p || !e || e < p)
                        RETURN_FAILURE;
                    while (p < e) {
                        if (ptr >= end ||
                            sre_lower_ascii(*ptr) != sre_lower_ascii(*p))
                            RETURN_FAILURE;
                        p++;
                        ptr++;
                    }
                }
            }
            pattern++;
            DISPATCH;

        TARGET(SRE_OP_GROUPREF_UNI_IGNORE):
            /* match backreference */
            TRACE(("|%p|%p|GROUPREF_UNI_IGNORE %d\n", pattern,
                   ptr, pattern[0]));
            {
                int groupref = pattern[0] * 2;
                if (groupref >= state->lastmark) {
                    RETURN_FAILURE;
                } else {
                    SRE_CHAR* p = (SRE_CHAR*) state->mark[groupref];
                    SRE_CHAR* e = (SRE_CHAR*) state->mark[groupref+1];
                    if (!p || !e || e < p)
                        RETURN_FAILURE;
                    while (p < e) {
                        if (ptr >= end ||
                            sre_lower_unicode(*ptr) != sre_lower_unicode(*p))
                            RETURN_FAILURE;
                        p++;
                        ptr++;
                    }
                }
            }
            pattern++;
            DISPATCH;

        TARGET(SRE_OP_GROUPREF_LOC_IGNORE):
            /* match backreference */
            TRACE(("|%p|%p|GROUPREF_LOC_IGNORE %d\n", pattern,
                   ptr, pattern[0]));
            {
                int groupref = pattern[0] * 2;
                if (groupref >= state->lastmark) {
                    RETURN_FAILURE;
                } else {
                    SRE_CHAR* p = (SRE_CHAR*) state->mark[groupref];
                    SRE_CHAR* e = (SRE_CHAR*) state->mark[groupref+1];
                    if (!p || !e || e < p)
                        RETURN_FAILURE;
                    while (p < e) {
                        if (ptr >= end ||
                            sre_lower_locale(*ptr) != sre_lower_locale(*p))
                            RETURN_FAILURE;
                        p++;
                        ptr++;
                    }
                }
            }
            pattern++;
            DISPATCH;

        TARGET(SRE_OP_GROUPREF_EXISTS):
            TRACE(("|%p|%p|GROUPREF_EXISTS %d\n", pattern,
                   ptr, pattern[0]));
            /* <GROUPREF_EXISTS> <group> <skip> codeyes <JUMP> codeno ... */
            {
                int groupref = pattern[0] * 2;
                if (groupref >= state->lastmark) {
                    pattern += pattern[1];
                    DISPATCH;
                } else {
                    SRE_CHAR* p = (SRE_CHAR*) state->mark[groupref];
                    SRE_CHAR* e = (SRE_CHAR*) state->mark[groupref+1];
                    if (!p || !e || e < p) {
                        pattern += pattern[1];
                        DISPATCH;
                    }
                }
            }
            pattern += 2;
            DISPATCH;

        TARGET(SRE_OP_ASSERT):
            /* assert subpattern */
            /* <ASSERT> <skip> <back> <pattern> */
            TRACE(("|%p|%p|ASSERT %d\n", pattern,
                   ptr, pattern[1]));
            if ((uintptr_t)(ptr - (SRE_CHAR *)state->beginning) < pattern[1])
                RETURN_FAILURE;
            state->ptr = ptr - pattern[1];
            DO_JUMP0(JUMP_ASSERT, jump_assert, pattern+2);
            RETURN_ON_FAILURE(ret);
            pattern += pattern[0];
            DISPATCH;

        TARGET(SRE_OP_ASSERT_NOT):
            /* assert not subpattern */
            /* <ASSERT_NOT> <skip> <back> <pattern> */
            TRACE(("|%p|%p|ASSERT_NOT %d\n", pattern,
                   ptr, pattern[1]));
            if ((uintptr_t)(ptr - (SRE_CHAR *)state->beginning) >= pattern[1]) {
                state->ptr = ptr - pattern[1];
                LASTMARK_SAVE();
                if (state->repeat)
                    MARK_PUSH(ctx->lastmark);

                DO_JUMP0(JUMP_ASSERT_NOT, jump_assert_not, pattern+2);
                if (ret) {
                    if (state->repeat)
                        MARK_POP_DISCARD(ctx->lastmark);
                    RETURN_ON_ERROR(ret);
                    RETURN_FAILURE;
                }
                if (state->repeat)
                    MARK_POP(ctx->lastmark);
                LASTMARK_RESTORE();
            }
            pattern += pattern[0];
            DISPATCH;

        TARGET(SRE_OP_FAILURE):
            /* immediate failure */
            TRACE(("|%p|%p|FAILURE\n", pattern, ptr));
            RETURN_FAILURE;

#if !USE_COMPUTED_GOTOS
        default:
#endif
        // Also any unused opcodes:
        TARGET(SRE_OP_RANGE_UNI_IGNORE):
        TARGET(SRE_OP_SUBPATTERN):
        TARGET(SRE_OP_RANGE):
        TARGET(SRE_OP_NEGATE):
        TARGET(SRE_OP_BIGCHARSET):
        TARGET(SRE_OP_CHARSET):
            TRACE(("|%p|%p|UNKNOWN %d\n", pattern, ptr,
                   pattern[-1]));
            RETURN_ERROR(SRE_ERROR_ILLEGAL);

    }

exit:
    ctx_pos = ctx->last_ctx_pos;
    jump = ctx->jump;
    DATA_POP_DISCARD(ctx);
    if (ctx_pos == -1) {
        state->sigcount = sigcount;
        return ret;
    }
    DATA_LOOKUP_AT(SRE(match_context), ctx, ctx_pos);

    switch (jump) {
        case JUMP_MAX_UNTIL_2:
            TRACE(("|%p|%p|JUMP_MAX_UNTIL_2\n", pattern, ptr));
            goto jump_max_until_2;
        case JUMP_MAX_UNTIL_3:
            TRACE(("|%p|%p|JUMP_MAX_UNTIL_3\n", pattern, ptr));
            goto jump_max_until_3;
        case JUMP_MIN_UNTIL_2:
            TRACE(("|%p|%p|JUMP_MIN_UNTIL_2\n", pattern, ptr));
            goto jump_min_until_2;
        case JUMP_MIN_UNTIL_3:
            TRACE(("|%p|%p|JUMP_MIN_UNTIL_3\n", pattern, ptr));
            goto jump_min_until_3;
        case JUMP_BRANCH:
            TRACE(("|%p|%p|JUMP_BRANCH\n", pattern, ptr));
            goto jump_branch;
        case JUMP_MAX_UNTIL_1:
            TRACE(("|%p|%p|JUMP_MAX_UNTIL_1\n", pattern, ptr));
            goto jump_max_until_1;
        case JUMP_MIN_UNTIL_1:
            TRACE(("|%p|%p|JUMP_MIN_UNTIL_1\n", pattern, ptr));
            goto jump_min_until_1;
        case JUMP_POSS_REPEAT_1:
            TRACE(("|%p|%p|JUMP_POSS_REPEAT_1\n", pattern, ptr));
            goto jump_poss_repeat_1;
        case JUMP_POSS_REPEAT_2:
            TRACE(("|%p|%p|JUMP_POSS_REPEAT_2\n", pattern, ptr));
            goto jump_poss_repeat_2;
        case JUMP_REPEAT:
            TRACE(("|%p|%p|JUMP_REPEAT\n", pattern, ptr));
            goto jump_repeat;
        case JUMP_REPEAT_ONE_1:
            TRACE(("|%p|%p|JUMP_REPEAT_ONE_1\n", pattern, ptr));
            goto jump_repeat_one_1;
        case JUMP_REPEAT_ONE_2:
            TRACE(("|%p|%p|JUMP_REPEAT_ONE_2\n", pattern, ptr));
            goto jump_repeat_one_2;
        case JUMP_MIN_REPEAT_ONE:
            TRACE(("|%p|%p|JUMP_MIN_REPEAT_ONE\n", pattern, ptr));
            goto jump_min_repeat_one;
        case JUMP_ATOMIC_GROUP:
            TRACE(("|%p|%p|JUMP_ATOMIC_GROUP\n", pattern, ptr));
            goto jump_atomic_group;
        case JUMP_ASSERT:
            TRACE(("|%p|%p|JUMP_ASSERT\n", pattern, ptr));
            goto jump_assert;
        case JUMP_ASSERT_NOT:
            TRACE(("|%p|%p|JUMP_ASSERT_NOT\n", pattern, ptr));
            goto jump_assert_not;
        case JUMP_NONE:
            TRACE(("|%p|%p|RETURN %zd\n", pattern,
                   ptr, ret));
            break;
    }

    return ret; /* should never get here */
}

/* need to reset capturing groups between two SRE(match) callings in loops */
#define RESET_CAPTURE_GROUP() \
    do { state->lastmark = state->lastindex = -1; } while (0)

LOCAL(Py_ssize_t)
SRE(search)(SRE_STATE* state, SRE_CODE* pattern)
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

    if (ptr > end)
        return 0;

    if (pattern[0] == SRE_OP_INFO) {
        /* optimization info block */
        /* <INFO> <1=skip> <2=flags> <3=min> <4=max> <5=prefix info>  */

        flags = pattern[2];

        if (pattern[3] && (uintptr_t)(end - ptr) < pattern[3]) {
            TRACE(("reject (got %tu chars, need %zu)\n",
                   end - ptr, (size_t) pattern[3]));
            return 0;
        }
        if (pattern[3] > 1) {
            /* adjust end point (but make sure we leave at least one
               character in there, so literal search will work) */
            end -= pattern[3] - 1;
            if (end <= ptr)
                end = ptr;
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

    TRACE(("prefix = %p %zd %zd\n",
           prefix, prefix_len, prefix_skip));
    TRACE(("charset = %p\n", charset));

    if (prefix_len == 1) {
        /* pattern starts with a literal character */
        SRE_CHAR c = (SRE_CHAR) prefix[0];
#if SIZEOF_SRE_CHAR < 4
        if ((SRE_CODE) c != prefix[0])
            return 0; /* literal can't match: doesn't fit in char width */
#endif
        end = (SRE_CHAR *)state->end;
        state->must_advance = 0;
        while (ptr < end) {
            while (*ptr != c) {
                if (++ptr >= end)
                    return 0;
            }
            TRACE(("|%p|%p|SEARCH LITERAL\n", pattern, ptr));
            state->start = ptr;
            state->ptr = ptr + prefix_skip;
            if (flags & SRE_INFO_LITERAL)
                return 1; /* we got all of it */
            status = SRE(match)(state, pattern + 2*prefix_skip, 0);
            if (status != 0)
                return status;
            ++ptr;
            RESET_CAPTURE_GROUP();
        }
        return 0;
    }

    if (prefix_len > 1) {
        /* pattern starts with a known prefix.  use the overlap
           table to skip forward as fast as we possibly can */
        Py_ssize_t i = 0;

        end = (SRE_CHAR *)state->end;
        if (prefix_len > end - ptr)
            return 0;
#if SIZEOF_SRE_CHAR < 4
        for (i = 0; i < prefix_len; i++)
            if ((SRE_CODE)(SRE_CHAR) prefix[i] != prefix[i])
                return 0; /* literal can't match: doesn't fit in char width */
#endif
        while (ptr < end) {
            SRE_CHAR c = (SRE_CHAR) prefix[0];
            while (*ptr++ != c) {
                if (ptr >= end)
                    return 0;
            }
            if (ptr >= end)
                return 0;

            i = 1;
            state->must_advance = 0;
            do {
                if (*ptr == (SRE_CHAR) prefix[i]) {
                    if (++i != prefix_len) {
                        if (++ptr >= end)
                            return 0;
                        continue;
                    }
                    /* found a potential match */
                    TRACE(("|%p|%p|SEARCH SCAN\n", pattern, ptr));
                    state->start = ptr - (prefix_len - 1);
                    state->ptr = ptr - (prefix_len - prefix_skip - 1);
                    if (flags & SRE_INFO_LITERAL)
                        return 1; /* we got all of it */
                    status = SRE(match)(state, pattern + 2*prefix_skip, 0);
                    if (status != 0)
                        return status;
                    /* close but no cigar -- try again */
                    if (++ptr >= end)
                        return 0;
                    RESET_CAPTURE_GROUP();
                }
                i = overlap[i];
            } while (i != 0);
        }
        return 0;
    }

    if (charset) {
        /* pattern starts with a character from a known set */
        end = (SRE_CHAR *)state->end;
        state->must_advance = 0;
        for (;;) {
            while (ptr < end && !SRE(charset)(state, charset, *ptr))
                ptr++;
            if (ptr >= end)
                return 0;
            TRACE(("|%p|%p|SEARCH CHARSET\n", pattern, ptr));
            state->start = ptr;
            state->ptr = ptr;
            status = SRE(match)(state, pattern, 0);
            if (status != 0)
                break;
            ptr++;
            RESET_CAPTURE_GROUP();
        }
    } else {
        /* general case */
        assert(ptr <= end);
        TRACE(("|%p|%p|SEARCH\n", pattern, ptr));
        state->start = state->ptr = ptr;
        status = SRE(match)(state, pattern, 1);
        state->must_advance = 0;
        if (status == 0 && pattern[0] == SRE_OP_AT &&
            (pattern[1] == SRE_AT_BEGINNING ||
             pattern[1] == SRE_AT_BEGINNING_STRING))
        {
            state->start = state->ptr = ptr = end;
            return 0;
        }
        while (status == 0 && ptr < end) {
            ptr++;
            RESET_CAPTURE_GROUP();
            TRACE(("|%p|%p|SEARCH\n", pattern, ptr));
            state->start = state->ptr = ptr;
            status = SRE(match)(state, pattern, 0);
        }
    }

    return status;
}

#undef SRE_CHAR
#undef SIZEOF_SRE_CHAR
#undef SRE

/* vim:ts=4:sw=4:et
*/
