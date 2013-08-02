/* Common code for use by all hashlib related modules. */

/*
 * Given a PyObject* obj, fill in the Py_buffer* viewp with the result
 * of PyObject_GetBuffer.  Sets an exception and issues a return NULL
 * on any errors.
 */
#define GET_BUFFER_VIEW_OR_ERROUT(obj, viewp) do { \
        if (PyUnicode_Check((obj))) { \
            PyErr_SetString(PyExc_TypeError, \
                            "Unicode-objects must be encoded before hashing");\
            return NULL; \
        } \
        if (!PyObject_CheckBuffer((obj))) { \
            PyErr_SetString(PyExc_TypeError, \
                            "object supporting the buffer API required"); \
            return NULL; \
        } \
        if (PyObject_GetBuffer((obj), (viewp), PyBUF_SIMPLE) == -1) { \
            return NULL; \
        } \
        if ((viewp)->ndim > 1) { \
            PyErr_SetString(PyExc_BufferError, \
                            "Buffer must be single dimension"); \
            PyBuffer_Release((viewp)); \
            return NULL; \
        } \
    } while(0);

/*
 * Helper code to synchronize access to the hash object when the GIL is
 * released around a CPU consuming hashlib operation. All code paths that
 * access a mutable part of obj must be enclosed in a ENTER_HASHLIB /
 * LEAVE_HASHLIB block or explicitly acquire and release the lock inside
 * a PY_BEGIN / END_ALLOW_THREADS block if they wish to release the GIL for
 * an operation.
 */

#ifdef WITH_THREAD
#include "pythread.h"
    #define ENTER_HASHLIB(obj) \
        if ((obj)->lock) { \
            if (!PyThread_acquire_lock((obj)->lock, 0)) { \
                Py_BEGIN_ALLOW_THREADS \
                PyThread_acquire_lock((obj)->lock, 1); \
                Py_END_ALLOW_THREADS \
            } \
        }
    #define LEAVE_HASHLIB(obj) \
        if ((obj)->lock) { \
            PyThread_release_lock((obj)->lock); \
        }
#else
    #define ENTER_HASHLIB(obj)
    #define LEAVE_HASHLIB(obj)
#endif

/* TODO(gps): We should probably make this a module or EVPobject attribute
 * to allow the user to optimize based on the platform they're using. */
#define HASHLIB_GIL_MINSIZE 2048

