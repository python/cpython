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

#include "pycore_time.h"        // _PyTime_t


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
// returns `Py_PARK_AGAIN`. The comparison behaves like memcmp, but is
// performed atomically with respect to unpark operations.
//
// The `address_size` argument is the size of the data pointed to by the
// `address` and `expected` pointers (i.e., sizeof(*address)). It must be
// 1, 2, 4, or 8.
//
// The `timeout_ns` argument specifies the maximum amount of time to wait, with
// -1 indicating an infinite wait.
//
// `park_arg`, which can be NULL, is passed to the unpark operation.
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
                   void *park_arg, int detach);

// Callback for _PyParkingLot_Unpark:
//
// `arg` is the data of the same name provided to the _PyParkingLot_Unpark()
//      call.
// `park_arg` is the data provided to _PyParkingLot_Park() call or NULL if
//      no waiting thread was found.
// `has_more_waiters` is true if there are more threads waiting on the same
//      address. May be true in cases where threads are waiting on a different
//      address that map to the same internal bucket.
typedef void _Py_unpark_fn_t(void *arg, void *park_arg, int has_more_waiters);

// Unparks a single thread waiting on `address`.
//
// Note that fn() is called regardless of whether a thread was unparked. If
// no threads are waiting on `address` then the `park_arg` argument to fn()
// will be NULL.
//
// Example usage:
//  void callback(void *arg, void *park_arg, int has_more_waiters);
//  _PyParkingLot_Unpark(address, &callback, arg);
PyAPI_FUNC(void)
_PyParkingLot_Unpark(const void *address, _Py_unpark_fn_t *fn, void *arg);

// Unparks all threads waiting on `address`.
PyAPI_FUNC(void) _PyParkingLot_UnparkAll(const void *address);

// Resets the parking lot state after a fork. Forgets all parked threads.
PyAPI_FUNC(void) _PyParkingLot_AfterFork(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PARKING_LOT_H */
