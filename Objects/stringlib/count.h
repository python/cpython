/* stringlib: count implementation */

#ifndef STRINGLIB_COUNT_H
#define STRINGLIB_COUNT_H

#ifndef STRINGLIB_FASTSEARCH_H
#error must include "stringlib/fastsearch.h" before including this module
#endif

Py_LOCAL_INLINE(Py_ssize_t)
stringlib_count(const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                const STRINGLIB_CHAR* sub, Py_ssize_t sub_len,
                Py_ssize_t maxcount)
{
    Py_ssize_t count;

    if (str_len < 0)
        return 0; /* start > len(str) */
    if (sub_len == 0)
        return (str_len < maxcount) ? str_len + 1 : maxcount;

    count = fastsearch(str, str_len, sub, sub_len, maxcount, FAST_COUNT);

    if (count < 0)
        return 0; /* no match */

    return count;
}

#endif
