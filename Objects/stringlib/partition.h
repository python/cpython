/* stringlib: partition implementation */

#ifndef STRINGLIB_FASTSEARCH_H
#error must include "stringlib/fastsearch.h" before including this module
#endif

Py_LOCAL_INLINE(PyObject*)
STRINGLIB(partition)(PyObject* str_obj,
                    const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                    PyObject* sep_obj,
                    const STRINGLIB_CHAR* sep, Py_ssize_t sep_len)
{
    PyObject* out;
    Py_ssize_t pos;

    if (sep_len == 0) {
        PyErr_SetString(PyExc_ValueError, "empty separator");
        return NULL;
    }

    out = PyTuple_New(3);
    if (!out)
        return NULL;

    pos = FASTSEARCH(str, str_len, sep, sep_len, -1, FAST_SEARCH);

    if (pos < 0) {
#if STRINGLIB_MUTABLE
        PyTuple_SET_ITEM(out, 0, STRINGLIB_NEW(str, str_len));
        PyTuple_SET_ITEM(out, 1, STRINGLIB_NEW(NULL, 0));
        PyTuple_SET_ITEM(out, 2, STRINGLIB_NEW(NULL, 0));

        if (PyErr_Occurred()) {
            Py_DECREF(out);
            return NULL;
        }
#else
        Py_INCREF(str_obj);
        PyTuple_SET_ITEM(out, 0, (PyObject*) str_obj);
        Py_INCREF(STRINGLIB_EMPTY);
        PyTuple_SET_ITEM(out, 1, (PyObject*) STRINGLIB_EMPTY);
        Py_INCREF(STRINGLIB_EMPTY);
        PyTuple_SET_ITEM(out, 2, (PyObject*) STRINGLIB_EMPTY);
#endif
        return out;
    }

    PyTuple_SET_ITEM(out, 0, STRINGLIB_NEW(str, pos));
    Py_INCREF(sep_obj);
    PyTuple_SET_ITEM(out, 1, sep_obj);
    pos += sep_len;
    PyTuple_SET_ITEM(out, 2, STRINGLIB_NEW(str + pos, str_len - pos));

    if (PyErr_Occurred()) {
        Py_DECREF(out);
        return NULL;
    }

    return out;
}

Py_LOCAL_INLINE(PyObject*)
STRINGLIB(rpartition)(PyObject* str_obj,
                     const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                     PyObject* sep_obj,
                     const STRINGLIB_CHAR* sep, Py_ssize_t sep_len)
{
    PyObject* out;
    Py_ssize_t pos;

    if (sep_len == 0) {
        PyErr_SetString(PyExc_ValueError, "empty separator");
        return NULL;
    }

    out = PyTuple_New(3);
    if (!out)
        return NULL;

    pos = FASTSEARCH(str, str_len, sep, sep_len, -1, FAST_RSEARCH);

    if (pos < 0) {
#if STRINGLIB_MUTABLE
        PyTuple_SET_ITEM(out, 0, STRINGLIB_NEW(NULL, 0));
        PyTuple_SET_ITEM(out, 1, STRINGLIB_NEW(NULL, 0));
        PyTuple_SET_ITEM(out, 2, STRINGLIB_NEW(str, str_len));

        if (PyErr_Occurred()) {
            Py_DECREF(out);
            return NULL;
        }
#else
        Py_INCREF(STRINGLIB_EMPTY);
        PyTuple_SET_ITEM(out, 0, (PyObject*) STRINGLIB_EMPTY);
        Py_INCREF(STRINGLIB_EMPTY);
        PyTuple_SET_ITEM(out, 1, (PyObject*) STRINGLIB_EMPTY);
        Py_INCREF(str_obj);
        PyTuple_SET_ITEM(out, 2, (PyObject*) str_obj);
#endif
        return out;
    }

    PyTuple_SET_ITEM(out, 0, STRINGLIB_NEW(str, pos));
    Py_INCREF(sep_obj);
    PyTuple_SET_ITEM(out, 1, sep_obj);
    pos += sep_len;
    PyTuple_SET_ITEM(out, 2, STRINGLIB_NEW(str + pos, str_len - pos));

    if (PyErr_Occurred()) {
        Py_DECREF(out);
        return NULL;
    }

    return out;
}

