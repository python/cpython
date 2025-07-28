#ifndef _HASHLIB_HASHLIB_BUFFER_H
#define _HASHLIB_HASHLIB_BUFFER_H

#include "Python.h"

/*
 * Allow to use the 'data' or 'string' keyword in hashlib.new()
 * and other hash functions named constructors.
 *
 * - If 'data' and 'string' are both non-NULL, set an exception and return -1.
 * - If 'data' and 'string' are both NULL, set '*res' to NULL and return 0.
 * - Otherwise, set '*res' to 'data' or 'string' and return 1. A deprecation
 *   warning is set when 'string' is specified.
 */
static inline int
_Py_hashlib_data_argument(PyObject **res, PyObject *data, PyObject *string)
{
    if (data != NULL && string == NULL) {
        // called as H(data) or H(data=...)
        *res = data;
        return 1;
    }
    else if (data == NULL && string != NULL) {
        // called as H(string=...)
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                         "the 'string' keyword parameter is deprecated since "
                         "Python 3.15 and slated for removal in Python 3.19; "
                         "use the 'data' keyword parameter or pass the data "
                         "to hash as a positional argument instead", 1) < 0)
        {
            *res = NULL;
            return -1;
        }
        *res = string;
        return 1;
    }
    else if (data == NULL && string == NULL) {
        // fast path when no data is given
        assert(!PyErr_Occurred());
        *res = NULL;
        return 0;
    }
    else {
        // called as H(data=..., string)
        *res = NULL;
        PyErr_SetString(PyExc_TypeError,
                        "'data' and 'string' are mutually exclusive "
                        "and support for 'string' keyword parameter "
                        "is slated for removal in a future version.");
        return -1;
    }
}

/*
 * Obtain a buffer view from a buffer-like object 'obj'.
 *
 * On success, store the result in 'view' and return 0.
 * On error, set an exception and return -1.
 */
static inline int
_Py_hashlib_get_buffer_view(PyObject *obj, Py_buffer *view)
{
    if (PyUnicode_Check(obj)) {
        PyErr_SetString(PyExc_TypeError,
                        "Strings must be encoded before hashing");
        return -1;
    }
    if (!PyObject_CheckBuffer(obj)) {
        PyErr_SetString(PyExc_TypeError,
                        "object supporting the buffer API required");
        return -1;
    }
    if (PyObject_GetBuffer(obj, view, PyBUF_SIMPLE) == -1) {
        return -1;
    }
    if (view->ndim > 1) {
        PyErr_SetString(PyExc_BufferError,
                        "Buffer must be single dimension");
        PyBuffer_Release(view);
        return -1;
    }
    return 0;
}

/*
 * Call _Py_hashlib_get_buffer_view() and check if it succeeded.
 *
 * On error, set an exception and execute the ERRACTION statements.
 */
#define GET_BUFFER_VIEW_OR_ERROR(OBJ, VIEW, ERRACTION)      \
    do {                                                    \
        if (_Py_hashlib_get_buffer_view(OBJ, VIEW) < 0) {   \
            assert(PyErr_Occurred());                       \
            ERRACTION;                                      \
        }                                                   \
    } while (0)

/* Specialization of GET_BUFFER_VIEW_OR_ERROR() returning NULL on error. */
#define GET_BUFFER_VIEW_OR_ERROUT(OBJ, VIEW)                \
    GET_BUFFER_VIEW_OR_ERROR(OBJ, VIEW, return NULL)

#endif // !_HASHLIB_HASHLIB_BUFFER_H
