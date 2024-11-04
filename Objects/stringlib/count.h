/* stringlib: count implementation */

#ifndef STRINGLIB_FASTSEARCH_H
#error must include "stringlib/fastsearch.h" before including this module
#endif

// gh-97982: Implementing asciilib_count() is not worth it, FASTSEARCH() does
// not specialize the code for ASCII strings. Use ucs1lib_count() for ASCII and
// UCS1 strings: it's the same than asciilib_count().
#if !STRINGLIB_IS_UNICODE || STRINGLIB_MAX_CHAR > 0x7Fu

Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(count)(const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                const STRINGLIB_CHAR* sub, Py_ssize_t sub_len,
                Py_ssize_t maxcount)
{
    Py_ssize_t count;

    if (str_len < 0)
        return 0; /* start > len(str) */
    if (sub_len == 0)
        return (str_len < maxcount) ? str_len + 1 : maxcount;

    count = FASTSEARCH(str, str_len, sub, sub_len, maxcount, FAST_COUNT);

    if (count < 0)
        return 0; /* no match */

    return count;
}

#endif
