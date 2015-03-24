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

#if STRINGLIB_IS_UNICODE

/*
Wraps stringlib_parse_args_finds() and additionally ensures that the
first argument is a unicode object.

Note that we receive a pointer to the pointer of the substring object,
so when we create that object in this function we don't DECREF it,
because it continues living in the caller functions (those functions,
after finishing using the substring, must DECREF it).
*/

Py_LOCAL_INLINE(int)
STRINGLIB(parse_args_finds_unicode)(const char * function_name, PyObject *args,
                                   PyObject **substring,
                                   Py_ssize_t *start, Py_ssize_t *end)
{
    PyObject *tmp_substring;

    if(STRINGLIB(parse_args_finds)(function_name, args, &tmp_substring,
                                  start, end)) {
        tmp_substring = PyUnicode_FromObject(tmp_substring);
        if (!tmp_substring)
            return 0;
        *substring = tmp_substring;
        return 1;
    }
    return 0;
}

#else /* !STRINGLIB_IS_UNICODE */

/*
Wraps stringlib_parse_args_finds() and additionally checks whether the
first argument is an integer in range(0, 256).

If this is the case, writes the integer value to the byte parameter
and sets subobj to NULL. Otherwise, sets the first argument to subobj
and doesn't touch byte. The other parameters are similar to those of
stringlib_parse_args_finds().
*/

Py_LOCAL_INLINE(int)
STRINGLIB(parse_args_finds_byte)(const char *function_name, PyObject *args,
                                 PyObject **subobj, char *byte,
                                 Py_ssize_t *start, Py_ssize_t *end)
{
    PyObject *tmp_subobj;
    Py_ssize_t ival;
    PyObject *err;

    if(!STRINGLIB(parse_args_finds)(function_name, args, &tmp_subobj,
                                    start, end))
        return 0;

    if (!PyNumber_Check(tmp_subobj)) {
        *subobj = tmp_subobj;
        return 1;
    }

    ival = PyNumber_AsSsize_t(tmp_subobj, PyExc_OverflowError);
    if (ival == -1) {
        err = PyErr_Occurred();
        if (err && !PyErr_GivenExceptionMatches(err, PyExc_OverflowError)) {
            PyErr_Clear();
            *subobj = tmp_subobj;
            return 1;
        }
    }

    if (ival < 0 || ival > 255) {
        PyErr_SetString(PyExc_ValueError, "byte must be in range(0, 256)");
        return 0;
    }

    *subobj = NULL;
    *byte = (char)ival;
    return 1;
}

#endif /* STRINGLIB_IS_UNICODE */
