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
 *
 * The symbol is exported for '_hashlib' and HACL*-based extension modules.
 */
extern int
_Py_hashlib_data_argument(PyObject **res, PyObject *data, PyObject *string);

/*
 * Obtain a buffer view from a buffer-like object 'obj'.
 *
 * On success, store the result in 'view' and return 0.
 * On error, set an exception and return -1.
 *
 * The symbol is exported for '_hashlib' and HACL*-based extension modules.
 */
extern int
_Py_hashlib_get_buffer_view(PyObject *obj, Py_buffer *view);

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
