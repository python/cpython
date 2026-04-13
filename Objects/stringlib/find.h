/* stringlib: find/index implementation */

#ifndef STRINGLIB_FASTSEARCH_H
#error must include "stringlib/fastsearch.h" before including this module
#endif

Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(find)(const STRINGLIB_CHAR* str, Py_ssize_t str_len,
               const STRINGLIB_CHAR* sub, Py_ssize_t sub_len,
               Py_ssize_t offset)
{
    Py_ssize_t pos;

    assert(str_len >= 0);
    if (sub_len == 0)
        return offset;

    pos = FASTSEARCH(str, str_len, sub, sub_len, -1, FAST_SEARCH);

    if (pos >= 0)
        pos += offset;

    return pos;
}

Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(rfind)(const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                const STRINGLIB_CHAR* sub, Py_ssize_t sub_len,
                Py_ssize_t offset)
{
    Py_ssize_t pos;

    assert(str_len >= 0);
    if (sub_len == 0)
        return str_len + offset;

    pos = FASTSEARCH(str, str_len, sub, sub_len, -1, FAST_RSEARCH);

    if (pos >= 0)
        pos += offset;

    return pos;
}

Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(find_slice)(const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                     const STRINGLIB_CHAR* sub, Py_ssize_t sub_len,
                     Py_ssize_t start, Py_ssize_t end)
{
    return STRINGLIB(find)(str + start, end - start, sub, sub_len, start);
}

Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(rfind_slice)(const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                      const STRINGLIB_CHAR* sub, Py_ssize_t sub_len,
                      Py_ssize_t start, Py_ssize_t end)
{
    return STRINGLIB(rfind)(str + start, end - start, sub, sub_len, start);
}

#ifdef STRINGLIB_WANT_CONTAINS_OBJ

Py_LOCAL_INLINE(int)
STRINGLIB(contains_obj)(PyObject* str, PyObject* sub)
{
    return STRINGLIB(find)(
        STRINGLIB_STR(str), STRINGLIB_LEN(str),
        STRINGLIB_STR(sub), STRINGLIB_LEN(sub), 0
        ) != -1;
}

#endif /* STRINGLIB_WANT_CONTAINS_OBJ */
