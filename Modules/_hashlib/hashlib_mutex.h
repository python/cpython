#ifndef _HASHLIB_HASHLIB_MUTEX_H
#define _HASHLIB_HASHLIB_MUTEX_H

#include "Python.h"
#include "pycore_lock.h"    // PyMutex

/*
 * Message length above which the GIL is to be released
 * when performing hashing operations.
 */
#define HASHLIB_GIL_MINSIZE 2048

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

#endif // !_HASHLIB_HASHLIB_MUTEX_H
