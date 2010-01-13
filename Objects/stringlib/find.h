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

    if (str_len < 0)
        return -1;
    if (sub_len == 0)
        return offset;

    pos = fastsearch(str, str_len, sub, sub_len, -1, FAST_SEARCH);

    if (pos >= 0)
        pos += offset;

    return pos;
}

Py_LOCAL_INLINE(Py_ssize_t)
stringlib_rfind(const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                const STRINGLIB_CHAR* sub, Py_ssize_t sub_len,
                Py_ssize_t offset)
{
    Py_ssize_t pos;

    if (str_len < 0)
        return -1;
    if (sub_len == 0)
        return str_len + offset;

    pos = fastsearch(str, str_len, sub, sub_len, -1, FAST_RSEARCH);

    if (pos >= 0)
        pos += offset;

    return pos;
}

/* helper macro to fixup start/end slice values */
#define ADJUST_INDICES(start, end, len)         \
    if (end > len)                              \
        end = len;                              \
    else if (end < 0) {                         \
        end += len;                             \
        if (end < 0)                            \
            end = 0;                            \
    }                                           \
    if (start < 0) {                            \
        start += len;                           \
        if (start < 0)                          \
            start = 0;                          \
    }

Py_LOCAL_INLINE(Py_ssize_t)
stringlib_find_slice(const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                     const STRINGLIB_CHAR* sub, Py_ssize_t sub_len,
                     Py_ssize_t start, Py_ssize_t end)
{
    ADJUST_INDICES(start, end, str_len);
    return stringlib_find(str + start, end - start, sub, sub_len, start);
}

Py_LOCAL_INLINE(Py_ssize_t)
stringlib_rfind_slice(const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                      const STRINGLIB_CHAR* sub, Py_ssize_t sub_len,
                      Py_ssize_t start, Py_ssize_t end)
{
    ADJUST_INDICES(start, end, str_len);
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

#endif /* STRINGLIB_WANT_CONTAINS_OBJ */

#if STRINGLIB_IS_UNICODE

/*
This function is a helper for the "find" family (find, rfind, index,
rindex) of unicodeobject.c file, because they all have the same 
behaviour for the arguments.

It does not touch the variables received until it knows everything 
is ok.

Note that we receive a pointer to the pointer of the substring object,
so when we create that object in this function we don't DECREF it,
because it continues living in the caller functions (those functions, 
after finishing using the substring, must DECREF it).
*/

Py_LOCAL_INLINE(int)
_ParseTupleFinds (PyObject *args, PyObject **substring, 
                  Py_ssize_t *start, Py_ssize_t *end) {
    PyObject *tmp_substring;
    Py_ssize_t tmp_start = 0;
    Py_ssize_t tmp_end = PY_SSIZE_T_MAX;
    PyObject *obj_start=Py_None, *obj_end=Py_None;

    if (!PyArg_ParseTuple(args, "O|OO:find", &tmp_substring,
         &obj_start, &obj_end))
        return 0;

    /* To support None in "start" and "end" arguments, meaning
       the same as if they were not passed.
    */
    if (obj_start != Py_None)
        if (!_PyEval_SliceIndex(obj_start, &tmp_start))
            return 0;
    if (obj_end != Py_None)
        if (!_PyEval_SliceIndex(obj_end, &tmp_end))
            return 0;

    tmp_substring = PyUnicode_FromObject(tmp_substring);
    if (!tmp_substring)
        return 0;

    *start = tmp_start;
    *end = tmp_end;
    *substring = tmp_substring;
    return 1;
}

#endif /* STRINGLIB_IS_UNICODE */

#endif /* STRINGLIB_FIND_H */
