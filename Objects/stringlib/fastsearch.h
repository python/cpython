/* stringlib: fastsearch implementation */

#ifndef STRINGLIB_FASTSEARCH_H
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

Py_LOCAL_INLINE(Py_ssize_t)
fastsearch(const STRINGLIB_CHAR* s, Py_ssize_t n,
           const STRINGLIB_CHAR* p, Py_ssize_t m,
           int mode)
{
    long mask;
    Py_ssize_t skip, count = 0;
    Py_ssize_t i, j, mlast, w;

    w = n - m;

    if (w < 0)
        return -1;

    /* look for special cases */
    if (m <= 1) {
        if (m <= 0)
            return -1;
        /* use special case for 1-character strings */
        if (mode == FAST_COUNT) {
            for (i = 0; i < n; i++)
                if (s[i] == p[0])
                    count++;
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

    if (mode != FAST_RSEARCH) {

        /* create compressed boyer-moore delta 1 table */

        /* process pattern[:-1] */
        for (mask = i = 0; i < mlast; i++) {
            mask |= (1 << (p[i] & 0x1F));
            if (p[i] == p[mlast])
                skip = mlast - i - 1;
        }
        /* process pattern[-1] outside the loop */
        mask |= (1 << (p[mlast] & 0x1F));

        for (i = 0; i <= w; i++) {
            /* note: using mlast in the skip path slows things down on x86 */
            if (s[i+m-1] == p[m-1]) {
                /* candidate match */
                for (j = 0; j < mlast; j++)
                    if (s[i+j] != p[j])
                        break;
                if (j == mlast) {
                    /* got a match! */
                    if (mode != FAST_COUNT)
                        return i;
                    count++;
                    i = i + mlast;
                    continue;
                }
                /* miss: check if next character is part of pattern */
                if (!(mask & (1 << (s[i+m] & 0x1F))))
                    i = i + m;
                else
                    i = i + skip;
            } else {
                /* skip: check if next character is part of pattern */
                if (!(mask & (1 << (s[i+m] & 0x1F))))
                    i = i + m;
            }
        }
    } else {    /* FAST_RSEARCH */

        /* create compressed boyer-moore delta 1 table */

        /* process pattern[0] outside the loop */
        mask = (1 << (p[0] & 0x1F));
        /* process pattern[:0:-1] */
        for (i = mlast; i > 0; i--) {
            mask |= (1 << (p[i] & 0x1F));
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
                if (!(mask & (1 << (s[i-1] & 0x1F))))
                    i = i - m;
                else
                    i = i - skip;
            } else {
                /* skip: check if previous character is part of pattern */
                if (!(mask & (1 << (s[i-1] & 0x1F))))
                    i = i - m;
            }
        }
    }

    if (mode != FAST_COUNT)
        return -1;
    return count;
}

#endif

/*
Local variables:
c-basic-offset: 4
indent-tabs-mode: nil
End:
*/
