// Lightweight locks and other synchronization mechanisms.
//
// These implementations are based on WebKit's WTF::Lock. See
// https://webkit.org/blog/6161/locking-in-webkit/ for a description of the
// design.
#ifndef Py_INTERNAL_LOCK_H
#define Py_INTERNAL_LOCK_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_time.h"          // _PyTime_t


// A mutex that occupies one byte. The lock can be zero initialized.
//
// Only the two least significant bits are used. The remaining bits should be
// zero:
// 0b00: unlocked
// 0b01: locked
// 0b10: unlocked and has parked threads
// 0b11: locked and has parked threads
//
// Typical initialization:
//   PyMutex m = (PyMutex){0};
//
// Typical usage:
//   PyMutex_Lock(&m);
//   ...
//   PyMutex_Unlock(&m);

// NOTE: In Py_GIL_DISABLED builds, `struct _PyMutex` is defined in Include/object.h.
// The Py_GIL_DISABLED builds need the definition in Include/object.h for the
// `ob_mutex` field in PyObject. For the default (non-free-threaded) build,
// we define the struct here to avoid exposing it in the public API.
#ifndef Py_GIL_DISABLED
struct _PyMutex { uint8_t v; };
#endif

typedef struct _PyMutex PyMutex;

#define _Py_UNLOCKED    0
#define _Py_LOCKED      1
#define _Py_HAS_PARKED  2
#define _Py_ONCE_INITIALIZED 4

// (private) slow path for locking the mutex
PyAPI_FUNC(void) _PyMutex_LockSlow(PyMutex *m);

// (private) slow path for unlocking the mutex
PyAPI_FUNC(void) _PyMutex_UnlockSlow(PyMutex *m);

static inline int
PyMutex_LockFast(uint8_t *lock_bits)
{
    uint8_t expected = _Py_UNLOCKED;
    return _Py_atomic_compare_exchange_uint8(lock_bits, &expected, _Py_LOCKED);
}

// Locks the mutex.
//
// If the mutex is currently locked, the calling thread will be parked until
// the mutex is unlocked. If the current thread holds the GIL, then the GIL
// will be released while the thread is parked.
static inline void
PyMutex_Lock(PyMutex *m)
{
    uint8_t expected = _Py_UNLOCKED;
    if (!_Py_atomic_compare_exchange_uint8(&m->v, &expected, _Py_LOCKED)) {
        _PyMutex_LockSlow(m);
    }
}

// Unlocks the mutex.
static inline void
PyMutex_Unlock(PyMutex *m)
{
    uint8_t expected = _Py_LOCKED;
    if (!_Py_atomic_compare_exchange_uint8(&m->v, &expected, _Py_UNLOCKED)) {
        _PyMutex_UnlockSlow(m);
    }
}

// Checks if the mutex is currently locked.
static inline int
PyMutex_IsLocked(PyMutex *m)
{
    return (_Py_atomic_load_uint8(&m->v) & _Py_LOCKED) != 0;
}

// Re-initializes the mutex after a fork to the unlocked state.
static inline void
_PyMutex_at_fork_reinit(PyMutex *m)
{
    memset(m, 0, sizeof(*m));
}

typedef enum _PyLockFlags {
    // Do not detach/release the GIL when waiting on the lock.
    _Py_LOCK_DONT_DETACH = 0,

    // Detach/release the GIL while waiting on the lock.
    _PY_LOCK_DETACH = 1,

    // Handle signals if interrupted while waiting on the lock.
    _PY_LOCK_HANDLE_SIGNALS = 2,
} _PyLockFlags;

// Lock a mutex with an optional timeout and additional options. See
// _PyLockFlags for details.
extern PyLockStatus
_PyMutex_LockTimed(PyMutex *m, _PyTime_t timeout_ns, _PyLockFlags flags);

// Lock a mutex with aditional options. See _PyLockFlags for details.
static inline void
PyMutex_LockFlags(PyMutex *m, _PyLockFlags flags)
{
    uint8_t expected = _Py_UNLOCKED;
    if (!_Py_atomic_compare_exchange_uint8(&m->v, &expected, _Py_LOCKED)) {
        _PyMutex_LockTimed(m, -1, flags);
    }
}

// Unlock a mutex, returns 0 if the mutex is not locked (used for improved
// error messages).
extern int _PyMutex_TryUnlock(PyMutex *m);


// PyEvent is a one-time event notification
typedef struct {
    uint8_t v;
} PyEvent;

// Set the event and notify any waiting threads.
// Export for '_testinternalcapi' shared extension
PyAPI_FUNC(void) _PyEvent_Notify(PyEvent *evt);

// Wait for the event to be set. If the event is already set, then this returns
// immediately.
PyAPI_FUNC(void) PyEvent_Wait(PyEvent *evt);

// Wait for the event to be set, or until the timeout expires. If the event is
// already set, then this returns immediately. Returns 1 if the event was set,
// and 0 if the timeout expired or thread was interrupted.
PyAPI_FUNC(int) PyEvent_WaitTimed(PyEvent *evt, _PyTime_t timeout_ns);


// _PyRawMutex implements a word-sized mutex that that does not depend on the
// parking lot API, and therefore can be used in the parking lot
// implementation.
//
// The mutex uses a packed representation: the least significant bit is used to
// indicate whether the mutex is locked or not. The remaining bits are either
// zero or a pointer to a `struct raw_mutex_entry` (see lock.c).
typedef struct {
    uintptr_t v;
} _PyRawMutex;

// Slow paths for lock/unlock
extern void _PyRawMutex_LockSlow(_PyRawMutex *m);
extern void _PyRawMutex_UnlockSlow(_PyRawMutex *m);

static inline void
_PyRawMutex_Lock(_PyRawMutex *m)
{
    uintptr_t unlocked = _Py_UNLOCKED;
    if (_Py_atomic_compare_exchange_uintptr(&m->v, &unlocked, _Py_LOCKED)) {
        return;
    }
    _PyRawMutex_LockSlow(m);
}

static inline void
_PyRawMutex_Unlock(_PyRawMutex *m)
{
    uintptr_t locked = _Py_LOCKED;
    if (_Py_atomic_compare_exchange_uintptr(&m->v, &locked, _Py_UNLOCKED)) {
        return;
    }
    _PyRawMutex_UnlockSlow(m);
}

// A data structure that can be used to run initialization code once in a
// thread-safe manner. The C++11 equivalent is std::call_once.
typedef struct {
    uint8_t v;
} _PyOnceFlag;

// Type signature for one-time initialization functions. The function should
// return 0 on success and -1 on failure.
typedef int _Py_once_fn_t(void *arg);

// (private) slow path for one time initialization
PyAPI_FUNC(int)
_PyOnceFlag_CallOnceSlow(_PyOnceFlag *flag, _Py_once_fn_t *fn, void *arg);

// Calls `fn` once using `flag`. The `arg` is passed to the call to `fn`.
//
// Returns 0 on success and -1 on failure.
//
// If `fn` returns 0 (success), then subsequent calls immediately return 0.
// If `fn` returns -1 (failure), then subsequent calls will retry the call.
static inline int
_PyOnceFlag_CallOnce(_PyOnceFlag *flag, _Py_once_fn_t *fn, void *arg)
{
    if (_Py_atomic_load_uint8(&flag->v) == _Py_ONCE_INITIALIZED) {
        return 0;
    }
    return _PyOnceFlag_CallOnceSlow(flag, fn, arg);
}

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_LOCK_H */
