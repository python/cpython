#ifndef Py_CPYTHON_LOCK_H
#  error "this header file must not be included directly"
#endif

#define _Py_UNLOCKED    0
#define _Py_LOCKED      1

// A mutex that occupies one byte. The lock can be zero initialized to
// represent the unlocked state.
//
// Typical initialization:
//   PyMutex m = (PyMutex){0};
//
// Or initialize as global variables:
//   static PyMutex m;
//
// Typical usage:
//   PyMutex_Lock(&m);
//   ...
//   PyMutex_Unlock(&m);
//
// The contents of the PyMutex are not part of the public API, but are
// described to aid in understanding the implementation and debugging. Only
// the two least significant bits are used. The remaining bits are always zero:
// 0b00: unlocked
// 0b01: locked
// 0b10: unlocked and has parked threads
// 0b11: locked and has parked threads
typedef struct PyMutex {
    uint8_t _bits;  // (private)
} PyMutex;

// (private) slow path for locking the mutex
PyAPI_FUNC(void) _PyMutex_LockSlow(PyMutex *m);

// (private) slow path for unlocking the mutex
PyAPI_FUNC(void) _PyMutex_UnlockSlow(PyMutex *m);

// Locks the mutex.
//
// If the mutex is currently locked, the calling thread will be parked until
// the mutex is unlocked. If the current thread holds the GIL, then the GIL
// will be released while the thread is parked.
static inline void
PyMutex_Lock(PyMutex *m)
{
    uint8_t expected = _Py_UNLOCKED;
    if (!_Py_atomic_compare_exchange_uint8(&m->_bits, &expected, _Py_LOCKED)) {
        _PyMutex_LockSlow(m);
    }
}

// Unlocks the mutex.
static inline void
PyMutex_Unlock(PyMutex *m)
{
    uint8_t expected = _Py_LOCKED;
    if (!_Py_atomic_compare_exchange_uint8(&m->_bits, &expected, _Py_UNLOCKED)) {
        _PyMutex_UnlockSlow(m);
    }
}
