/* Common code for use by all hashlib related modules. */

#include "pycore_lock.h"        // PyMutex

/*
 * Internal error messages used for reporting an unsupported hash algorithm.
 * The algorithm can be given by its name, a callable or a PEP-247 module.
 * The same message is raised by Lib/hashlib.py::__get_builtin_constructor()
 * and _hmacmodule.c::find_hash_info().
 */
#define HASHLIB_UNSUPPORTED_ALGORITHM       "unsupported hash algorithm %S"
#define HASHLIB_UNSUPPORTED_STR_ALGORITHM   "unsupported hash algorithm %s"

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

#define GET_BUFFER_VIEW_OR_ERROUT(OBJ, VIEW)                \
    GET_BUFFER_VIEW_OR_ERROR(OBJ, VIEW, return NULL)

/*
 * Helper code to synchronize access to the hash object when the GIL is
 * released around a CPU consuming hashlib operation.
 *
 * Code accessing a mutable part of the hash object must be enclosed in
 * an HASHLIB_{ACQUIRE,RELEASE}_LOCK block or explicitly acquire and release
 * the mutex inside a Py_BEGIN_ALLOW_THREADS -- Py_END_ALLOW_THREADS block if
 * they wish to release the GIL for an operation.
 */

#define HASHLIB_OBJECT_HEAD                                             \
    PyObject_HEAD                                                       \
    /* Guard against race conditions during incremental update(). */    \
    PyMutex mutex;

#define HASHLIB_INIT_MUTEX(OBJ)         \
    do {                                \
        (OBJ)->mutex = (PyMutex){0};    \
    } while (0)

#define HASHLIB_ACQUIRE_LOCK(OBJ)   PyMutex_Lock(&(OBJ)->mutex)
#define HASHLIB_RELEASE_LOCK(OBJ)   PyMutex_Unlock(&(OBJ)->mutex)

/*
 * Message length above which the GIL is to be released
 * when performing hashing operations.
 */
#define HASHLIB_GIL_MINSIZE         2048

// Macros for executing code while conditionally holding the GIL.
//
// These only drop the GIL if the lock acquisition itself is likely to
// block. Thus the non-blocking acquire gating the GIL release for a
// blocking lock acquisition. The intent of these macros is to surround
// the assumed always "fast" operations that you aren't releasing the
// GIL around.

/*
 * Execute a suite of C statements 'STATEMENTS'.
 *
 * The GIL is held if 'SIZE' is below the HASHLIB_GIL_MINSIZE threshold.
 */
#define HASHLIB_EXTERNAL_INSTRUCTIONS_UNLOCKED(SIZE, STATEMENTS)    \
    do {                                                            \
        if ((SIZE) > HASHLIB_GIL_MINSIZE) {                         \
            Py_BEGIN_ALLOW_THREADS                                  \
            STATEMENTS;                                             \
            Py_END_ALLOW_THREADS                                    \
        }                                                           \
        else {                                                      \
            STATEMENTS;                                             \
        }                                                           \
    } while (0)

/*
 * Lock 'OBJ' and execute a suite of C statements 'STATEMENTS'.
 *
 * The GIL is held if 'SIZE' is below the HASHLIB_GIL_MINSIZE threshold.
 */
#define HASHLIB_EXTERNAL_INSTRUCTIONS_LOCKED(OBJ, SIZE, STATEMENTS) \
    do {                                                            \
        if ((SIZE) > HASHLIB_GIL_MINSIZE) {                         \
            Py_BEGIN_ALLOW_THREADS                                  \
            HASHLIB_ACQUIRE_LOCK(OBJ);                              \
            STATEMENTS;                                             \
            HASHLIB_RELEASE_LOCK(OBJ);                              \
            Py_END_ALLOW_THREADS                                    \
        }                                                           \
        else {                                                      \
            HASHLIB_ACQUIRE_LOCK(OBJ);                              \
            STATEMENTS;                                             \
            HASHLIB_RELEASE_LOCK(OBJ);                              \
        }                                                           \
    } while (0)

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
