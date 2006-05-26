/* stringlib: partition implementation */

#ifndef STRINGLIB_PARTITION_H
#define STRINGLIB_PARTITION_H

Py_LOCAL(PyObject*)
partition(PyObject* str_obj, const STRINGLIB_CHAR* str, Py_ssize_t str_len,
	  PyObject* sep_obj, const STRINGLIB_CHAR* sep, Py_ssize_t sep_len)
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

    pos = fastsearch(str, str_len, sep, sep_len, FAST_SEARCH);

    if (pos < 0) {
	Py_INCREF(str_obj);
	PyTuple_SET_ITEM(out, 0, (PyObject*) str_obj);
	Py_INCREF(STRINGLIB_EMPTY);
	PyTuple_SET_ITEM(out, 1, (PyObject*) STRINGLIB_EMPTY);
	Py_INCREF(STRINGLIB_EMPTY);
	PyTuple_SET_ITEM(out, 2, (PyObject*) STRINGLIB_EMPTY);
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

#endif
