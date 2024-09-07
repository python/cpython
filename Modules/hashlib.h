/* Common code for use by all hashlib related modules. */

#include "pycore_lock.h"        // PyMutex

/*
 * Given a PyObject* obj, fill in the Py_buffer* viewp with the result
 * of PyObject_GetBuffer.  Sets an exception and issues the erraction
 * on any errors, e.g. 'return NULL' or 'goto error'.
 */
#define GET_BUFFER_VIEW_OR_ERROR(obj, viewp, erraction) do { \
        if (PyUnicode_Check((obj))) { \
            PyErr_SetString(PyExc_TypeError, \
                            "Strings must be encoded before hashing");\
            erraction; \
        } \
        if (!PyObject_CheckBuffer((obj))) { \
            PyErr_SetString(PyExc_TypeError, \
                            "object supporting the buffer API required"); \
            erraction; \
        } \
        if (PyObject_GetBuffer((obj), (viewp), PyBUF_SIMPLE) == -1) { \
            erraction; \
        } \
        if ((viewp)->ndim > 1) { \
            PyErr_SetString(PyExc_BufferError, \
                            "Buffer must be single dimension"); \
            PyBuffer_Release((viewp)); \
            erraction; \
        } \
    } while(0)

#define GET_BUFFER_VIEW_OR_ERROUT(obj, viewp) \
    GET_BUFFER_VIEW_OR_ERROR(obj, viewp, return NULL)

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

