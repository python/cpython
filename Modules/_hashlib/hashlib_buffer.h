#ifndef _HASHLIB_HASHLIB_BUFFER_H
#define _HASHLIB_HASHLIB_BUFFER_H

#include "Python.h"

/*
 * Given an buffer-like OBJ, fill in the buffer VIEW with the result
 * of PyObject_GetBuffer.
 *
 * On error, set an exception and execute the ERRACTION statements,
 * e.g. 'return NULL' or 'goto error'.
 *
 * Parameters
 *
 *      OBJ         An object supporting the buffer API.
 *      VIEW        A Py_buffer pointer to fill.
 *      ERRACTION   The statements to execute on error.
 */
#define GET_BUFFER_VIEW_OR_ERROR(OBJ, VIEW, ERRACTION)                      \
    do {                                                                    \
        if (PyUnicode_Check((OBJ))) {                                       \
            PyErr_SetString(PyExc_TypeError,                                \
                            "strings must be encoded before hashing");      \
            ERRACTION;                                                      \
        }                                                                   \
        if (!PyObject_CheckBuffer((OBJ))) {                                 \
            PyErr_SetString(PyExc_TypeError,                                \
                            "object supporting the buffer API required");   \
            ERRACTION;                                                      \
        }                                                                   \
        if (PyObject_GetBuffer((OBJ), (VIEW), PyBUF_SIMPLE) == -1) {        \
            ERRACTION;                                                      \
        }                                                                   \
        if ((VIEW)->ndim > 1) {                                             \
            PyErr_SetString(PyExc_BufferError,                              \
                            "buffer must be one-dimensional");              \
            PyBuffer_Release((VIEW));                                       \
            ERRACTION;                                                      \
        }                                                                   \
    } while(0)

/* Specialization of GET_BUFFER_VIEW_OR_ERROR() returning NULL on error. */
#define GET_BUFFER_VIEW_OR_ERROUT(OBJ, VIEW) \
    GET_BUFFER_VIEW_OR_ERROR(OBJ, VIEW, return NULL)

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
PyAPI_FUNC(int)
_Py_hashlib_data_argument(PyObject **res, PyObject *data, PyObject *string);

#endif // !_HASHLIB_HASHLIB_BUFFER_H
