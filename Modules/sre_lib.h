/*
 * Secret Labs' Regular Expression Engine
 *
 * regular expression matching engine
 *
 * Copyright (c) 1997-2001 by Secret Labs AB.  All rights reserved.
 *
 * See the _sre.c file for information on usage and redistribution.
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
        TRACE(("|%p|%p|COUNT %" PY_FORMAT_SIZE_T "d\n", pattern, ptr,
               (SRE_CHAR*) state->ptr - ptr));
        return (SRE_CHAR*) state->ptr - ptr;
    }

    TRACE(("|%p|%p|COUNT %" PY_FORMAT_SIZE_T "d\n", pattern, ptr,
           ptr - (SRE_CHAR*) state->ptr));
    return ptr - (SRE_CHAR*) state->ptr;
}

#if 0 /* not used in this release */
LOCAL(int)
SRE(info)(SRE_STATE* state, const SRE_CODE* pattern)
{
    /* check if an SRE_OP_INFO block matches at the current position.
       returns the number of SRE_CODE objects to skip if successful, 0
       if no match */

    const SRE_CHAR* end = (const SRE_CHAR*) state->end;
    const SRE_CHAR* ptr = (const SRE_CHAR*) state->ptr;
    Py_ssize_t i;

    /* check minimal length */
    if (pattern[3] && end - ptr < pattern[3])
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
    TRACE(("allocating %s in %" PY_FORMAT_SIZE_T "d " \
           "(%" PY_FORMAT_SIZE_T "d)\n", \
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
    TRACE(("looking up %s at %" PY_FORMAT_SIZE_T "d\n", Py_STRINGIFY(type), pos)); \
    ptr = (type*)(state->data_stack+pos); \
} while (0)

#define DATA_STACK_PUSH(state, data, size) \
do { \
    TRACE(("copy data in %p to %" PY_FORMAT_SIZE_T "d " \
           "(%" PY_FORMAT_SIZE_T "d)\n", \
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
    TRACE(("copy data to %p from %" PY_FORMAT_SIZE_T "d " \
           "(%" PY_FORMAT_SIZE_T "d)\n", \
           data, state->data_stack_base-size, size)); \
    memcpy((void*) data, state->data_stack+state->data_stack_base-size, size); \
    if (discard) \
        state->data_stack_base -= size; \
} while (0)

#define DATA_STACK_POP_DISCARD(state, size) \
do { \
    TRACE(("discard data from %" PY_FORMAT_SIZE_T "d " \
           "(%" PY_FORMAT_SIZE_T "d)\n", \
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

#define DO_JUMPX(jumpvalue, jumplabel, nextpattern, toplevel_) \
    DATA_ALLOC(SRE(match_context), nextctx); \
    nextctx->last_ctx_pos = ctx_pos; \
    nextctx->jump = jumpvalue; \
    nextctx->pattern = nextpattern; \
    nextctx->toplevel = toplevel_; \
    ctx_pos = alloc_pos; \
    ctx = nextctx; \
    goto entrance; \
    jumplabel: \
    while (0) /* gcc doesn't like labels at end of scopes */ \

#define DO_JUMP(jumpvalue, jumplabel, nextpattern) \
    DO_JUMPX(jumpvalue, jumplabel, nextpattern, ctx->toplevel)

#define DO_JUMP0(jumpvalue, jumplabel, nextpattern) \
    DO_JUMPX(jumpvalue, jumplabel, nextpattern, 0)

typedef struct {
    Py_ssize_t last_ctx_pos;
    Py_ssize_t jump;
    const SRE_CHAR* ptr;
    const SRE_CODE* pattern;
    Py_ssize_t count;
    Py_ssize_t lastmark;
    Py_ssize_t lastindex;
    union {
        SRE_CODE chr;
        SRE_REPEAT* rep;
    } u;
    int toplevel;
} SRE(match_context);

/* check if string matches the given pattern.  returns <0 for
   error, 0 for failure, and 1 for success */
LOCAL(Py_ssize_t)
SRE(match)(SRE_STATE* state, const SRE_CODE* pattern, int toplevel)
{
    const SRE_CHAR* end = (const SRE_CHAR *)state->end;
    Py_ssize_t alloc_pos, ctx_pos = -1;
    Py_ssize_t i, ret = 0;
    Py_ssize_t jump;
    unsigned int sigcount=0;

    SRE(match_context)* ctx;
    SRE(match_context)* nextctx;

    TRACE(("|%p|%p|ENTER\n", pattern, state->ptr));

    DATA_ALLOC(SRE(match_context), ctx);
    ctx->last_ctx_pos = -1;
    ctx->jump = JUMP_NONE;
    ctx->pattern = pattern;
    ctx->toplevel = toplevel;
    ctx_pos = alloc_pos;

entrance:

    ctx->ptr = (SRE_CHAR *)state->ptr;

    if (ctx->pattern[0] == SRE_OP_INFO) {
        /* optimization info block */
        /* <INFO> <1=skip> <2=flags> <3=min> ... */
        if (ctx->pattern[3] && (uintptr_t)(end - ctx->ptr) < ctx->pattern[3]) {
            TRACE(("reject (got %" PY_FORMAT_SIZE_T "d chars, "
                   "need %" PY_FORMAT_SIZE_T "d)\n",
                   end - ctx->ptr, (Py_ssize_t) ctx->pattern[3]));
            RETURN_FAILURE;
        }
        ctx->pattern += ctx->pattern[1] + 1;
    }

    for (;;) {
        ++sigcount;
        if ((0 == (sigcount & 0xfff)) && PyErr_CheckSignals())
            RETURN_ERROR(SRE_ERROR_INTERRUPTED);

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
            if (ctx->toplevel &&
                ((state->match_all && ctx->ptr != state->end) ||
                 (state->must_advance && ctx->ptr == state->start)))
            {
                RETURN_FAILURE;
            }
            state->ptr = ctx->ptr;
            RETURN_SUCCESS;

        case SRE_OP_AT:
            /* match at given position */
            /* <AT> <code> */
            TRACE(("|%p|%p|AT %d\n", ctx->pattern, ctx->ptr, *ctx->pattern));
            if (!SRE(at)(state, ctx->ptr, *ctx->pattern))
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
            if (ctx->ptr >= end ||
                !SRE(charset)(state, ctx->pattern + 1, *ctx->ptr))
                RETURN_FAILURE;
            ctx->pattern += ctx->pattern[0];
            ctx->ptr++;
            break;

        case SRE_OP_LITERAL_IGNORE:
            TRACE(("|%p|%p|LITERAL_IGNORE %d\n",
                   ctx->pattern, ctx->ptr, ctx->pattern[0]));
            if (ctx->ptr >= end ||
                sre_lower_ascii(*ctx->ptr) != *ctx->pattern)
                RETURN_FAILURE;
            ctx->pattern++;
            ctx->ptr++;
            break;

        case SRE_OP_LITERAL_UNI_IGNORE:
            TRACE(("|%p|%p|LITERAL_UNI_IGNORE %d\n",
                   ctx->pattern, ctx->ptr, ctx->pattern[0]));
            if (ctx->ptr >= end ||
                sre_lower_unicode(*ctx->ptr) != *ctx->pattern)
                RETURN_FAILURE;
            ctx->pattern++;
            ctx->ptr++;
            break;

        case SRE_OP_LITERAL_LOC_IGNORE:
            TRACE(("|%p|%p|LITERAL_LOC_IGNORE %d\n",
                   ctx->pattern, ctx->ptr, ctx->pattern[0]));
            if (ctx->ptr >= end
                || !char_loc_ignore(*ctx->pattern, *ctx->ptr))
                RETURN_FAILURE;
            ctx->pattern++;
            ctx->ptr++;
            break;

        case SRE_OP_NOT_LITERAL_IGNORE:
            TRACE(("|%p|%p|NOT_LITERAL_IGNORE %d\n",
                   ctx->pattern, ctx->ptr, *ctx->pattern));
            if (ctx->ptr >= end ||
                sre_lower_ascii(*ctx->ptr) == *ctx->pattern)
                RETURN_FAILURE;
            ctx->pattern++;
            ctx->ptr++;
            break;

        case SRE_OP_NOT_LITERAL_UNI_IGNORE:
            TRACE(("|%p|%p|NOT_LITERAL_UNI_IGNORE %d\n",
                   ctx->pattern, ctx->ptr, *ctx->pattern));
            if (ctx->ptr >= end ||
                sre_lower_unicode(*ctx->ptr) == *ctx->pattern)
                RETURN_FAILURE;
            ctx->pattern++;
            ctx->ptr++;
            break;

        case SRE_OP_NOT_LITERAL_LOC_IGNORE:
            TRACE(("|%p|%p|NOT_LITERAL_LOC_IGNORE %d\n",
                   ctx->pattern, ctx->ptr, *ctx->pattern));
            if (ctx->ptr >= end
                || char_loc_ignore(*ctx->pattern, *ctx->ptr))
                RETURN_FAILURE;
            ctx->pattern++;
            ctx->ptr++;
            break;

        case SRE_OP_IN_IGNORE:
            TRACE(("|%p|%p|IN_IGNORE\n", ctx->pattern, ctx->ptr));
            if (ctx->ptr >= end
                || !SRE(charset)(state, ctx->pattern+1,
                                 (SRE_CODE)sre_lower_ascii(*ctx->ptr)))
                RETURN_FAILURE;
            ctx->pattern += ctx->pattern[0];
            ctx->ptr++;
            break;

        case SRE_OP_IN_UNI_IGNORE:
            TRACE(("|%p|%p|IN_UNI_IGNORE\n", ctx->pattern, ctx->ptr));
            if (ctx->ptr >= end
                || !SRE(charset)(state, ctx->pattern+1,
                                 (SRE_CODE)sre_lower_unicode(*ctx->ptr)))
                RETURN_FAILURE;
            ctx->pattern += ctx->pattern[0];
            ctx->ptr++;
            break;

        case SRE_OP_IN_LOC_IGNORE:
            TRACE(("|%p|%p|IN_LOC_IGNORE\n", ctx->pattern, ctx->ptr));
            if (ctx->ptr >= end
                || !SRE(charset_loc_ignore)(state, ctx->pattern+1, *ctx->ptr))
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
                     !SRE(charset)(state, ctx->pattern + 3,
                                   (SRE_CODE) *ctx->ptr)))
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

            if ((Py_ssize_t) ctx->pattern[1] > end - ctx->ptr)
                RETURN_FAILURE; /* cannot match */

            state->ptr = ctx->ptr;

            ret = SRE(count)(state, ctx->pattern+3, ctx->pattern[2]);
            RETURN_ON_ERROR(ret);
            DATA_LOOKUP_AT(SRE(match_context), ctx, ctx_pos);
            ctx->count = ret;
            ctx->ptr += ctx->count;

            /* when we arrive here, count contains the number of
               matches, and ctx->ptr points to the tail of the target
               string.  check if the rest of the pattern matches,
               and backtrack if not. */

            if (ctx->count < (Py_ssize_t) ctx->pattern[1])
                RETURN_FAILURE;

            if (ctx->pattern[ctx->pattern[0]] == SRE_OP_SUCCESS &&
                ctx->ptr == state->end &&
                !(ctx->toplevel && state->must_advance && ctx->ptr == state->start))
            {
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

            if ((Py_ssize_t) ctx->pattern[1] > end - ctx->ptr)
                RETURN_FAILURE; /* cannot match */

            state->ptr = ctx->ptr;

            if (ctx->pattern[1] == 0)
                ctx->count = 0;
            else {
                /* count using pattern min as the maximum */
                ret = SRE(count)(state, ctx->pattern+3, ctx->pattern[1]);
                RETURN_ON_ERROR(ret);
                DATA_LOOKUP_AT(SRE(match_context), ctx, ctx_pos);
                if (ret < (Py_ssize_t) ctx->pattern[1])
                    /* didn't match minimum number of times */
                    RETURN_FAILURE;
                /* advance past minimum matches of repeat */
                ctx->count = ret;
                ctx->ptr += ctx->count;
            }

            if (ctx->pattern[ctx->pattern[0]] == SRE_OP_SUCCESS &&
                !(ctx->toplevel &&
                  ((state->match_all && ctx->ptr != state->end) ||
                   (state->must_advance && ctx->ptr == state->start))))
            {
                /* tail is empty.  we're finished */
                state->ptr = ctx->ptr;
                RETURN_SUCCESS;

            } else {
                /* general case */
                LASTMARK_SAVE();
                while ((Py_ssize_t)ctx->pattern[2] == SRE_MAXREPEAT
                       || ctx->count <= (Py_ssize_t)ctx->pattern[2]) {
                    state->ptr = ctx->ptr;
                    DO_JUMP(JUMP_MIN_REPEAT_ONE,jump_min_repeat_one,
                            ctx->pattern+ctx->pattern[0]);
                    if (ret) {
                        RETURN_ON_ERROR(ret);
                        RETURN_SUCCESS;
                    }
                    state->ptr = ctx->ptr;
                    ret = SRE(count)(state, ctx->pattern+3, 1);
                    RETURN_ON_ERROR(ret);
                    DATA_LOOKUP_AT(SRE(match_context), ctx, ctx_pos);
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

            TRACE(("|%p|%p|MAX_UNTIL %" PY_FORMAT_SIZE_T "d\n", ctx->pattern,
                   ctx->ptr, ctx->count));

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
                state->ptr = ctx->ptr;
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

            TRACE(("|%p|%p|MIN_UNTIL %" PY_FORMAT_SIZE_T "d %p\n", ctx->pattern,
                   ctx->ptr, ctx->count, ctx->u.rep->pattern));

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
                        p++;
                        ctx->ptr++;
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
                            sre_lower_ascii(*ctx->ptr) != sre_lower_ascii(*p))
                            RETURN_FAILURE;
                        p++;
                        ctx->ptr++;
                    }
                }
            }
            ctx->pattern++;
            break;

        case SRE_OP_GROUPREF_UNI_IGNORE:
            /* match backreference */
            TRACE(("|%p|%p|GROUPREF_UNI_IGNORE %d\n", ctx->pattern,
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
                            sre_lower_unicode(*ctx->ptr) != sre_lower_unicode(*p))
                            RETURN_FAILURE;
                        p++;
                        ctx->ptr++;
                    }
                }
            }
            ctx->pattern++;
            break;

        case SRE_OP_GROUPREF_LOC_IGNORE:
            /* match backreference */
            TRACE(("|%p|%p|GROUPREF_LOC_IGNORE %d\n", ctx->pattern,
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
                            sre_lower_locale(*ctx->ptr) != sre_lower_locale(*p))
                            RETURN_FAILURE;
                        p++;
                        ctx->ptr++;
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
            if (ctx->ptr - (SRE_CHAR *)state->beginning < (Py_ssize_t)ctx->pattern[1])
                RETURN_FAILURE;
            state->ptr = ctx->ptr - ctx->pattern[1];
            DO_JUMP0(JUMP_ASSERT, jump_assert, ctx->pattern+2);
            RETURN_ON_FAILURE(ret);
            ctx->pattern += ctx->pattern[0];
            break;

        case SRE_OP_ASSERT_NOT:
            /* assert not subpattern */
            /* <ASSERT_NOT> <skip> <back> <pattern> */
            TRACE(("|%p|%p|ASSERT_NOT %d\n", ctx->pattern,
                   ctx->ptr, ctx->pattern[1]));
            if (ctx->ptr - (SRE_CHAR *)state->beginning >= (Py_ssize_t)ctx->pattern[1]) {
                state->ptr = ctx->ptr - ctx->pattern[1];
                DO_JUMP0(JUMP_ASSERT_NOT, jump_assert_not, ctx->pattern+2);
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
    DATA_LOOKUP_AT(SRE(match_context), ctx, ctx_pos);

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
            TRACE(("|%p|%p|RETURN %" PY_FORMAT_SIZE_T "d\n", ctx->pattern,
                   ctx->ptr, ret));
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

        if (pattern[3] && end - ptr < (Py_ssize_t)pattern[3]) {
            TRACE(("reject (got %u chars, need %u)\n",
                   (unsigned int)(end - ptr), pattern[3]));
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

    TRACE(("prefix = %p %" PY_FORMAT_SIZE_T "d %" PY_FORMAT_SIZE_T "d\n",
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
