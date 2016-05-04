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

/*
This function is a helper for the "find" family (find, rfind, index,
rindex) and for count, startswith and endswith, because they all have
the same behaviour for the arguments.

It does not touch the variables received until it knows everything
is ok.
*/

#define FORMAT_BUFFER_SIZE 50

Py_LOCAL_INLINE(int)
STRINGLIB(parse_args_finds)(const char * function_name, PyObject *args,
                           PyObject **subobj,
                           Py_ssize_t *start, Py_ssize_t *end)
{
    PyObject *tmp_subobj;
    Py_ssize_t tmp_start = 0;
    Py_ssize_t tmp_end = PY_SSIZE_T_MAX;
    PyObject *obj_start=Py_None, *obj_end=Py_None;
    char format[FORMAT_BUFFER_SIZE] = "O|OO:";
    size_t len = strlen(format);

    strncpy(format + len, function_name, FORMAT_BUFFER_SIZE - len - 1);
    format[FORMAT_BUFFER_SIZE - 1] = '\0';

    if (!PyArg_ParseTuple(args, format, &tmp_subobj, &obj_start, &obj_end))
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

    *start = tmp_start;
    *end = tmp_end;
    *subobj = tmp_subobj;
    return 1;
}

#undef FORMAT_BUFFER_SIZE
