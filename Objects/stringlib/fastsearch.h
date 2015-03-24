/* stringlib: fastsearch implementation */

#define STRINGLIB_FASTSEARCH_H

/* fast search/count implementation, based on a mix between boyer-
   moore and horspool, with a few more bells and whistles on the top.
   for some more background, see: http://effbot.org/zone/stringlib.htm */

/* note: fastsearch may access s[n], which isn't a problem when using
   Python's ordinary string types, but may cause problems if you're
   using this code in other contexts.  also, the count mode returns -1
   if there cannot possible be a match in the target string, and 0 if
   it has actually checked for matches, but didn't find any.  callers
   beware! */

#define FAST_COUNT 0
#define FAST_SEARCH 1
#define FAST_RSEARCH 2

#if LONG_BIT >= 128
#define STRINGLIB_BLOOM_WIDTH 128
#elif LONG_BIT >= 64
#define STRINGLIB_BLOOM_WIDTH 64
#elif LONG_BIT >= 32
#define STRINGLIB_BLOOM_WIDTH 32
#else
#error "LONG_BIT is smaller than 32"
#endif

#define STRINGLIB_BLOOM_ADD(mask, ch) \
    ((mask |= (1UL << ((ch) & (STRINGLIB_BLOOM_WIDTH -1)))))
#define STRINGLIB_BLOOM(mask, ch)     \
    ((mask &  (1UL << ((ch) & (STRINGLIB_BLOOM_WIDTH -1)))))


Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(fastsearch_memchr_1char)(const STRINGLIB_CHAR* s, Py_ssize_t n,
                                   STRINGLIB_CHAR ch, unsigned char needle,
                                   int mode)
{
    if (mode == FAST_SEARCH) {
        const STRINGLIB_CHAR *ptr = s;
        const STRINGLIB_CHAR *e = s + n;
        while (ptr < e) {
            void *candidate = memchr((const void *) ptr, needle, (e - ptr) * sizeof(STRINGLIB_CHAR));
            if (candidate == NULL)
                return -1;
            ptr = (const STRINGLIB_CHAR *) _Py_ALIGN_DOWN(candidate, sizeof(STRINGLIB_CHAR));
            if (sizeof(STRINGLIB_CHAR) == 1 || *ptr == ch)
                return (ptr - s);
            /* False positive */
            ptr++;
        }
        return -1;
    }
#ifdef HAVE_MEMRCHR
    /* memrchr() is a GNU extension, available since glibc 2.1.91.
       it doesn't seem as optimized as memchr(), but is still quite
       faster than our hand-written loop in FASTSEARCH below */
    else if (mode == FAST_RSEARCH) {
        while (n > 0) {
            const STRINGLIB_CHAR *found;
            void *candidate = memrchr((const void *) s, needle, n * sizeof(STRINGLIB_CHAR));
            if (candidate == NULL)
                return -1;
            found = (const STRINGLIB_CHAR *) _Py_ALIGN_DOWN(candidate, sizeof(STRINGLIB_CHAR));
            n = found - s;
            if (sizeof(STRINGLIB_CHAR) == 1 || *found == ch)
                return n;
            /* False positive */
        }
        return -1;
    }
#endif
    else {
        assert(0); /* Should never get here */
        return 0;
    }

#undef DO_MEMCHR
}

Py_LOCAL_INLINE(Py_ssize_t)
FASTSEARCH(const STRINGLIB_CHAR* s, Py_ssize_t n,
           const STRINGLIB_CHAR* p, Py_ssize_t m,
           Py_ssize_t maxcount, int mode)
{
    unsigned long mask;
    Py_ssize_t skip, count = 0;
    Py_ssize_t i, j, mlast, w;

    w = n - m;

    if (w < 0 || (mode == FAST_COUNT && maxcount == 0))
        return -1;

    /* look for special cases */
    if (m <= 1) {
        if (m <= 0)
            return -1;
        /* use special case for 1-character strings */
        if (n > 10 && (mode == FAST_SEARCH
#ifdef HAVE_MEMRCHR
                    || mode == FAST_RSEARCH
#endif
                    )) {
            /* use memchr if we can choose a needle without two many likely
               false positives */
            unsigned char needle;
            needle = p[0] & 0xff;
#if STRINGLIB_SIZEOF_CHAR > 1
            /* If looking for a multiple of 256, we'd have too
               many false positives looking for the '\0' byte in UCS2
               and UCS4 representations. */
            if (needle != 0)
#endif
                return STRINGLIB(fastsearch_memchr_1char)
                       (s, n, p[0], needle, mode);
        }
        if (mode == FAST_COUNT) {
            for (i = 0; i < n; i++)
                if (s[i] == p[0]) {
                    count++;
                    if (count == maxcount)
                        return maxcount;
                }
            return count;
        } else if (mode == FAST_SEARCH) {
            for (i = 0; i < n; i++)
                if (s[i] == p[0])
                    return i;
        } else {    /* FAST_RSEARCH */
            for (i = n - 1; i > -1; i--)
                if (s[i] == p[0])
                    return i;
        }
        return -1;
    }

    mlast = m - 1;
    skip = mlast - 1;
    mask = 0;

    if (mode != FAST_RSEARCH) {
        const STRINGLIB_CHAR *ss = s + m - 1;
        const STRINGLIB_CHAR *pp = p + m - 1;

        /* create compressed boyer-moore delta 1 table */

        /* process pattern[:-1] */
        for (i = 0; i < mlast; i++) {
            STRINGLIB_BLOOM_ADD(mask, p[i]);
            if (p[i] == p[mlast])
                skip = mlast - i - 1;
        }
        /* process pattern[-1] outside the loop */
        STRINGLIB_BLOOM_ADD(mask, p[mlast]);

        for (i = 0; i <= w; i++) {
            /* note: using mlast in the skip path slows things down on x86 */
            if (ss[i] == pp[0]) {
                /* candidate match */
                for (j = 0; j < mlast; j++)
                    if (s[i+j] != p[j])
                        break;
                if (j == mlast) {
                    /* got a match! */
                    if (mode != FAST_COUNT)
                        return i;
                    count++;
                    if (count == maxcount)
                        return maxcount;
                    i = i + mlast;
                    continue;
                }
                /* miss: check if next character is part of pattern */
                if (!STRINGLIB_BLOOM(mask, ss[i+1]))
                    i = i + m;
                else
                    i = i + skip;
            } else {
                /* skip: check if next character is part of pattern */
                if (!STRINGLIB_BLOOM(mask, ss[i+1]))
                    i = i + m;
            }
        }
    } else {    /* FAST_RSEARCH */

        /* create compressed boyer-moore delta 1 table */

        /* process pattern[0] outside the loop */
        STRINGLIB_BLOOM_ADD(mask, p[0]);
        /* process pattern[:0:-1] */
        for (i = mlast; i > 0; i--) {
            STRINGLIB_BLOOM_ADD(mask, p[i]);
            if (p[i] == p[0])
                skip = i - 1;
        }

        for (i = w; i >= 0; i--) {
            if (s[i] == p[0]) {
                /* candidate match */
                for (j = mlast; j > 0; j--)
                    if (s[i+j] != p[j])
                        break;
                if (j == 0)
                    /* got a match! */
                    return i;
                /* miss: check if previous character is part of pattern */
                if (i > 0 && !STRINGLIB_BLOOM(mask, s[i-1]))
                    i = i - m;
                else
                    i = i - skip;
            } else {
                /* skip: check if previous character is part of pattern */
                if (i > 0 && !STRINGLIB_BLOOM(mask, s[i-1]))
                    i = i - m;
            }
        }
    }

    if (mode != FAST_COUNT)
        return -1;
    return count;
}

