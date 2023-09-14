// ParkingLot is an internal API for building efficient synchronization
// primitives like mutexes and events.
//
// The API and name is inspired by WebKit's WTF::ParkingLot, which in turn
// is inspired Linux's futex API.
// See https://webkit.org/blog/6161/locking-in-webkit/.
//
// The core functionality is an atomic "compare-and-sleep" operation along with
// an atomic "wake-up" operation.

#ifndef Py_INTERNAL_PARKING_LOT_H
#define Py_INTERNAL_PARKING_LOT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_time.h"          // _PyTime_t


enum {
    // The thread was unparked by another thread.
    Py_PARK_OK = 0,

    // The value of `address` did not match `expected`.
    Py_PARK_AGAIN = -1,

    // The thread was unparked due to a timeout.
    Py_PARK_TIMEOUT = -2,

    // The thread was interrupted by a signal.
    Py_PARK_INTR = -3,
};

// Checks that `*address == *expected` and puts the thread to sleep until an
// unpark operation is called on the same `address`. Otherwise, the function
// returns `Py_PARK_AGAIN`. The comparison is performed atomically
// with respect to unpark operations.
//
// The `address_size` argument is the size of the data pointed to by the
// `address` and `expected` pointers (i.e., sizeof(*address)). It must be
// 1, 2, 4, or 8.
//
// The `timeout_ns` argument specifies the maximum amount of time to wait, with
// -1 indicating an infinite wait.
//
// `arg`, which can be NULL, is passed to the unpark operation.
//
// If `detach` is true, then the thread will detach/release the GIL while
// waiting.
//
// Example usage:
//
//  if (_Py_atomic_compare_exchange_uint8(address, &expected, new_value)) {
//    int res = _PyParkingLot_Park(address, &new_value, sizeof(*address),
//                                 timeout_ns, NULL, 1);
//    ...
//  }
PyAPI_FUNC(int)
_PyParkingLot_Park(const void *address, const void *expected,
                   size_t address_size, _PyTime_t timeout_ns,
                   void *arg, int detach);

struct _PyUnpark {
    // The `arg` value passed to _PyParkingLot_Park().
    void *arg;

    // Are there more threads waiting on the address? May be true in cases
    // where threads are waiting on a different address that maps to the same
    // internal bucket.
    int has_more_waiters;
};

// Unpark a single thread waiting on `address`.
//
// The `unpark` is a pointer to a `struct _PyUnpark`.
//
// Usage:
//  _PyParkingLot_Unpark(address, unpark, {
//      if (unpark) {
//          void *arg = unpark->arg;
//          int has_more_waiters = unpark->has_more_waiters;
//          ...
//      }
//  });
#define _PyParkingLot_Unpark(address, unpark, ...)                          \
    do {                                                                    \
        struct _PyUnpark *(unpark);                                         \
        unpark = _PyParkingLot_BeginUnpark((address));                      \
        __VA_ARGS__                                                         \
        _PyParkingLot_FinishUnpark((address), unpark);                      \
    } while (0);

// Implements half of an unpark operation.
// Prefer using the _PyParkingLot_Unpark() macro.
PyAPI_FUNC(struct _PyUnpark *)
_PyParkingLot_BeginUnpark(const void *address);

// Finishes the unpark operation and wakes up the thread selected by
// _PyParkingLot_BeginUnpark.
// Prefer using the _PyParkingLot_Unpark() macro.
PyAPI_FUNC(void)
_PyParkingLot_FinishUnpark(const void *address, struct _PyUnpark *unpark);

// Unparks all threads waiting on `address`.
PyAPI_FUNC(void) _PyParkingLot_UnparkAll(const void *address);

// Initialize/deinitialize the thread-local state used by parking lot.
void _PyParkingLot_InitThread(void);
void _PyParkingLot_DeinitThread(void);

// Resets the parking lot state after a fork. Forgets all parked threads.
PyAPI_FUNC(void) _PyParkingLot_AfterFork(void);


// The _PySemaphore API a simplified cross-platform semaphore used to implement
// parking lot. It is not intended to be used directly by other modules.
typedef struct _PySemaphore _PySemaphore;

// Puts the current thread to sleep until _PySemaphore_Wakeup() is called.
// If `detach` is true, then the thread will detach/release the GIL while
// sleeping.
PyAPI_FUNC(int)
_PySemaphore_Wait(_PySemaphore *sema, _PyTime_t timeout_ns, int detach);

// Wakes up a single thread waiting on sema. Note that _PySemaphore_Wakeup()
// can be called before _PySemaphore_Wait().
PyAPI_FUNC(void)
_PySemaphore_Wakeup(_PySemaphore *sema);

// Allocates/releases a semaphore from the thread-local pool.
PyAPI_FUNC(_PySemaphore *) _PySemaphore_Alloc(void);
PyAPI_FUNC(void) _PySemaphore_Free(_PySemaphore *sema);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PARKING_LOT_H */
