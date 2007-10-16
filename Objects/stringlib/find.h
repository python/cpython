/* stringlib: find/index implementation */

#ifndef STRINGLIB_FIND_H
#define STRINGLIB_FIND_H

#ifndef STRINGLIB_FASTSEARCH_H
#error must include "stringlib/fastsearch.h" before including this module
#endif

Py_LOCAL_INLINE(Py_ssize_t)
stringlib_find(const STRINGLIB_CHAR* str, Py_ssize_t str_len,
               const STRINGLIB_CHAR* sub, Py_ssize_t sub_len,
               Py_ssize_t offset)
{
    Py_ssize_t pos;

    if (sub_len == 0) {
        if (str_len < 0)
            return -1;
        return offset;
    }

    pos = fastsearch(str, str_len, sub, sub_len, FAST_SEARCH);

    if (pos >= 0)
        pos += offset;

    return pos;
}

Py_LOCAL_INLINE(Py_ssize_t)
stringlib_rfind(const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                const STRINGLIB_CHAR* sub, Py_ssize_t sub_len,
                Py_ssize_t offset)
{
    /* XXX - create reversefastsearch helper! */
    if (sub_len == 0) {
        if (str_len < 0)
            return -1;
	return str_len + offset;
    } else {
	Py_ssize_t j, pos = -1;
	for (j = str_len - sub_len; j >= 0; --j)
            if (STRINGLIB_CMP(str+j, sub, sub_len) == 0) {
                pos = j + offset;
                break;
            }
        return pos;
    }
}

Py_LOCAL_INLINE(Py_ssize_t)
stringlib_find_slice(const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                     const STRINGLIB_CHAR* sub, Py_ssize_t sub_len,
                     Py_ssize_t start, Py_ssize_t end)
{
    if (start < 0)
        start += str_len;
    if (start < 0)
        start = 0;
    if (end > str_len)
        end = str_len;
    if (end < 0)
        end += str_len;
    if (end < 0)
        end = 0;

    return stringlib_find(
        str + start, end - start,
        sub, sub_len, start
        );
}

Py_LOCAL_INLINE(Py_ssize_t)
stringlib_rfind_slice(const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                      const STRINGLIB_CHAR* sub, Py_ssize_t sub_len,
                      Py_ssize_t start, Py_ssize_t end)
{
    if (start < 0)
        start += str_len;
    if (start < 0)
        start = 0;
    if (end > str_len)
        end = str_len;
    if (end < 0)
        end += str_len;
    if (end < 0)
        end = 0;

    return stringlib_rfind(str + start, end - start, sub, sub_len, start);
}

#ifdef STRINGLIB_WANT_CONTAINS_OBJ

Py_LOCAL_INLINE(int)
stringlib_contains_obj(PyObject* str, PyObject* sub)
{
    return stringlib_find(
        STRINGLIB_STR(str), STRINGLIB_LEN(str),
        STRINGLIB_STR(sub), STRINGLIB_LEN(sub), 0
        ) != -1;
}

#endif /* STRINGLIB_STR */

#endif /* STRINGLIB_FIND_H */

/*
Local variables:
c-basic-offset: 4
indent-tabs-mode: nil
End:
*/
