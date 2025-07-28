/* Common code for use by all hashlib related modules. */

#include "pycore_lock.h"        // PyMutex

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
 * released around a CPU consuming hashlib operation. All code paths that
 * access a mutable part of obj must be enclosed in an ENTER_HASHLIB /
 * LEAVE_HASHLIB block or explicitly acquire and release the lock inside
 * a PY_BEGIN / END_ALLOW_THREADS block if they wish to release the GIL for
 * an operation.
 *
 * These only drop the GIL if the lock acquisition itself is likely to
 * block. Thus the non-blocking acquire gating the GIL release for a
 * blocking lock acquisition. The intent of these macros is to surround
 * the assumed always "fast" operations that you aren't releasing the
 * GIL around.  Otherwise use code similar to what you see in hash
 * function update() methods.
 */

#include "pythread.h"
#define ENTER_HASHLIB(obj) \
    if ((obj)->use_mutex) { \
        PyMutex_Lock(&(obj)->mutex); \
    }
#define LEAVE_HASHLIB(obj) \
    if ((obj)->use_mutex) { \
        PyMutex_Unlock(&(obj)->mutex); \
    }

#ifdef Py_GIL_DISABLED
#define HASHLIB_INIT_MUTEX(obj) \
    do { \
        (obj)->mutex = (PyMutex){0}; \
        (obj)->use_mutex = true; \
    } while (0)
#else
#define HASHLIB_INIT_MUTEX(obj) \
    do { \
        (obj)->mutex = (PyMutex){0}; \
        (obj)->use_mutex = false; \
    } while (0)
#endif

/* TODO(gpshead): We should make this a module or class attribute
 * to allow the user to optimize based on the platform they're using. */
#define HASHLIB_GIL_MINSIZE 2048

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
