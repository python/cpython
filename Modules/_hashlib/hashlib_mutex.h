#ifndef _HASHLIB_HASHLIB_MUTEX_H
#define _HASHLIB_HASHLIB_MUTEX_H

#include "Python.h"
#include "pycore_lock.h"    // PyMutex

/*
 * Maximum number of bytes for a message for which the GIL is held
 * when performing incremental hashing.
 */
#define HASHLIB_GIL_MINSIZE 2048

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

/* Prevent undefined behaviors via multiple threads entering the C API. */
#define HASHLIB_LOCK_HEAD                   \
    bool use_mutex;                         \
    PyMutex mutex;

#ifdef Py_GIL_DISABLED
#define HASHLIB_INIT_MUTEX(OBJ)             \
    do {                                    \
        (OBJ)->mutex = (PyMutex){0};        \
        (OBJ)->use_mutex = true;            \
    } while (0)
#else
#define HASHLIB_INIT_MUTEX(OBJ)             \
    do {                                    \
        (OBJ)->mutex = (PyMutex){0};        \
        (OBJ)->use_mutex = false;           \
    } while (0)
#endif

#define ENTER_HASHLIB(OBJ)                  \
    do {                                    \
        if ((OBJ)->use_mutex) {             \
            PyMutex_Lock(&(OBJ)->mutex);    \
        }                                   \
    } while (0)

#define LEAVE_HASHLIB(OBJ)                  \
    do {                                    \
        if ((OBJ)->use_mutex) {             \
            PyMutex_Unlock(&(OBJ)->mutex);  \
        }                                   \
    } while (0)

#endif // !_HASHLIB_HASHLIB_MUTEX_H
