// Internal PyTime_t C API: see Doc/c-api/time.rst for the documentation.
//
// The PyTime_t type is an integer to support directly common arithmetic
// operations such as t1 + t2.
//
// Time formats:
//
// * Seconds.
// * Seconds as a floating-point number (C double).
// * Milliseconds (10^-3 seconds).
// * Microseconds (10^-6 seconds).
// * 100 nanoseconds (10^-7 seconds), used on Windows.
// * Nanoseconds (10^-9 seconds).
// * timeval structure, 1 microsecond (10^-6 seconds).
// * timespec structure, 1 nanosecond (10^-9 seconds).
//
// Note that PyTime_t is now specified as int64_t, in nanoseconds.
// (If we need to change this, we'll need new public API with new names.)
// Previously, PyTime_t was configurable (in theory); some comments and code
// might still allude to that.
//
// Integer overflows are detected and raise OverflowError. Conversion to a
// resolution larger than 1 nanosecond is rounded correctly with the requested
// rounding mode. Available rounding modes:
//
// * Round towards minus infinity (-inf). For example, used to read a clock.
// * Round towards infinity (+inf). For example, used for timeout to wait "at
//   least" N seconds.
// * Round to nearest with ties going to nearest even integer. For example, used
//   to round from a Python float.
// * Round away from zero. For example, used for timeout.
//
// Some functions clamp the result in the range [PyTime_MIN; PyTime_MAX]. The
// caller doesn't have to handle errors and so doesn't need to hold the GIL to
// handle exceptions. For example, _PyTime_Add(t1, t2) computes t1+t2 and
// clamps the result on overflow.
//
// Clocks:
//
// * System clock
// * Monotonic clock
// * Performance counter
//
// Internally, operations like (t * k / q) with integers are implemented in a
// way to reduce the risk of integer overflow. Such operation is used to convert a
// clock value expressed in ticks with a frequency to PyTime_t, like
// QueryPerformanceCounter() with QueryPerformanceFrequency() on Windows.


#ifndef Py_INTERNAL_TIME_H
#define Py_INTERNAL_TIME_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_runtime_structs.h" // _PyTimeFraction

#ifdef __clang__
struct timeval;
#endif

#define _SIZEOF_PYTIME_T 8

typedef enum {
    // Round towards minus infinity (-inf).
    // For example, used to read a clock.
    _PyTime_ROUND_FLOOR=0,

    // Round towards infinity (+inf).
    // For example, used for timeout to wait "at least" N seconds.
    _PyTime_ROUND_CEILING=1,

    // Round to nearest with ties going to nearest even integer.
    // For example, used to round from a Python float.
    _PyTime_ROUND_HALF_EVEN=2,

    // Round away from zero
    // For example, used for timeout. _PyTime_ROUND_CEILING rounds
    // -1e-9 to 0 milliseconds which causes bpo-31786 issue.
    // _PyTime_ROUND_UP rounds -1e-9 to -1 millisecond which keeps
    // the timeout sign as expected. select.poll(timeout) must block
    // for negative values.
    _PyTime_ROUND_UP=3,

    // _PyTime_ROUND_TIMEOUT (an alias for _PyTime_ROUND_UP) should be
    // used for timeouts.
    _PyTime_ROUND_TIMEOUT = _PyTime_ROUND_UP
} _PyTime_round_t;


// Convert a time_t to a PyLong.
// Export for '_testinternalcapi' shared extension
PyAPI_FUNC(PyObject*) _PyLong_FromTime_t(time_t sec);

// Convert a PyLong to a time_t.
// Export for '_datetime' shared extension
PyAPI_FUNC(time_t) _PyLong_AsTime_t(PyObject *obj);

// Convert a number of seconds, int or float, to time_t.
// Export for '_datetime' shared extension.
PyAPI_FUNC(int) _PyTime_ObjectToTime_t(
    PyObject *obj,
    time_t *sec,
    _PyTime_round_t);

// Convert a number of seconds, int or float, to a timeval structure.
// usec is in the range [0; 999999] and rounded towards zero.
// For example, -1.2 is converted to (-2, 800000).
// Export for '_datetime' shared extension.
PyAPI_FUNC(int) _PyTime_ObjectToTimeval(
    PyObject *obj,
    time_t *sec,
    long *usec,
    _PyTime_round_t);

// Convert a number of seconds, int or float, to a timespec structure.
// nsec is in the range [0; 999999999] and rounded towards zero.
// For example, -1.2 is converted to (-2, 800000000).
// Export for '_testinternalcapi' shared extension.
PyAPI_FUNC(int) _PyTime_ObjectToTimespec(
    PyObject *obj,
    time_t *sec,
    long *nsec,
    _PyTime_round_t);


// Create a timestamp from a number of seconds.
// Export for '_socket' shared extension.
PyAPI_FUNC(PyTime_t) _PyTime_FromSeconds(int seconds);

// Create a timestamp from a number of seconds in double.
extern int _PyTime_FromSecondsDouble(
    double seconds,
    _PyTime_round_t round,
    PyTime_t *result);

// Macro to create a timestamp from a number of seconds, no integer overflow.
// Only use the macro for small values, prefer _PyTime_FromSeconds().
#define _PYTIME_FROMSECONDS(seconds) \
            ((PyTime_t)(seconds) * (1000 * 1000 * 1000))

// Create a timestamp from a number of microseconds.
// Clamp to [PyTime_MIN; PyTime_MAX] on overflow.
extern PyTime_t _PyTime_FromMicrosecondsClamp(PyTime_t us);

// Convert a number of seconds (Python float or int) to a timestamp.
// Raise an exception and return -1 on error, return 0 on success.
// Export for '_socket' shared extension.
PyAPI_FUNC(int) _PyTime_FromSecondsObject(PyTime_t *t,
    PyObject *obj,
    _PyTime_round_t round);

// Convert a number of milliseconds (Python float or int, 10^-3) to a timestamp.
// Raise an exception and return -1 on error, return 0 on success.
// Export for 'select' shared extension.
PyAPI_FUNC(int) _PyTime_FromMillisecondsObject(PyTime_t *t,
    PyObject *obj,
    _PyTime_round_t round);

// Convert timestamp to a number of milliseconds (10^-3 seconds).
// Export for '_ssl' shared extension.
PyAPI_FUNC(PyTime_t) _PyTime_AsMilliseconds(PyTime_t t,
    _PyTime_round_t round);

// Convert timestamp to a number of microseconds (10^-6 seconds).
// Export for '_queue' shared extension.
PyAPI_FUNC(PyTime_t) _PyTime_AsMicroseconds(PyTime_t t,
    _PyTime_round_t round);

#ifdef MS_WINDOWS
// Convert timestamp to a number of 100 nanoseconds (10^-7 seconds).
extern PyTime_t _PyTime_As100Nanoseconds(PyTime_t t,
    _PyTime_round_t round);
#endif

#ifndef MS_WINDOWS
// Create a timestamp from a timeval structure.
// Raise an exception and return -1 on overflow, return 0 on success.
extern int _PyTime_FromTimeval(PyTime_t *tp, struct timeval *tv);
#endif

// Convert a timestamp to a timeval structure (microsecond resolution).
// tv_usec is always positive.
// Raise an exception and return -1 if the conversion overflowed,
// return 0 on success.
// Export for 'select' shared extension.
PyAPI_FUNC(int) _PyTime_AsTimeval(PyTime_t t,
    struct timeval *tv,
    _PyTime_round_t round);

// Similar to _PyTime_AsTimeval() but don't raise an exception on overflow.
// On overflow, clamp tv_sec to PyTime_t min/max.
// Export for 'select' shared extension.
PyAPI_FUNC(void) _PyTime_AsTimeval_clamp(PyTime_t t,
    struct timeval *tv,
    _PyTime_round_t round);

// Convert a timestamp to a number of seconds (secs) and microseconds (us).
// us is always positive. This function is similar to _PyTime_AsTimeval()
// except that secs is always a time_t type, whereas the timeval structure
// uses a C long for tv_sec on Windows.
// Raise an exception and return -1 if the conversion overflowed,
// return 0 on success.
// Export for '_datetime' shared extension.
PyAPI_FUNC(int) _PyTime_AsTimevalTime_t(
    PyTime_t t,
    time_t *secs,
    int *us,
    _PyTime_round_t round);

#if defined(HAVE_CLOCK_GETTIME) || defined(HAVE_KQUEUE)
// Create a timestamp from a timespec structure.
// Raise an exception and return -1 on overflow, return 0 on success.
extern int _PyTime_FromTimespec(PyTime_t *tp, const struct timespec *ts);

// Convert a timestamp to a timespec structure (nanosecond resolution).
// tv_nsec is always positive.
// Raise an exception and return -1 on error, return 0 on success.
// Export for '_testinternalcapi' shared extension.
PyAPI_FUNC(int) _PyTime_AsTimespec(PyTime_t t, struct timespec *ts);

// Similar to _PyTime_AsTimespec() but don't raise an exception on overflow.
// On overflow, clamp tv_sec to PyTime_t min/max.
// Export for '_testinternalcapi' shared extension.
PyAPI_FUNC(void) _PyTime_AsTimespec_clamp(PyTime_t t, struct timespec *ts);
#endif


// Compute t1 + t2. Clamp to [PyTime_MIN; PyTime_MAX] on overflow.
extern PyTime_t _PyTime_Add(PyTime_t t1, PyTime_t t2);

// Structure used by time.get_clock_info()
typedef struct {
    const char *implementation;
    int monotonic;
    int adjustable;
    double resolution;
} _Py_clock_info_t;

// Get the current time from the system clock.
// On success, set *t and *info (if not NULL), and return 0.
// On error, raise an exception and return -1.
extern int _PyTime_TimeWithInfo(
    PyTime_t *t,
    _Py_clock_info_t *info);

// Get the time of a monotonic clock, i.e. a clock that cannot go backwards.
// The clock is not affected by system clock updates. The reference point of
// the returned value is undefined, so that only the difference between the
// results of consecutive calls is valid.
//
// Fill info (if set) with information of the function used to get the time.
//
// Return 0 on success, raise an exception and return -1 on error.
// Export for '_testsinglephase' shared extension.
PyAPI_FUNC(int) _PyTime_MonotonicWithInfo(
    PyTime_t *t,
    _Py_clock_info_t *info);


// Converts a timestamp to the Gregorian time, using the local time zone.
// Return 0 on success, raise an exception and return -1 on error.
// Export for '_datetime' shared extension.
PyAPI_FUNC(int) _PyTime_localtime(time_t t, struct tm *tm);

// Converts a timestamp to the Gregorian time, assuming UTC.
// Return 0 on success, raise an exception and return -1 on error.
// Export for '_datetime' shared extension.
PyAPI_FUNC(int) _PyTime_gmtime(time_t t, struct tm *tm);


// Get the performance counter: clock with the highest available resolution to
// measure a short duration.
//
// Fill info (if set) with information of the function used to get the time.
//
// Return 0 on success, raise an exception and return -1 on error.
extern int _PyTime_PerfCounterWithInfo(
    PyTime_t *t,
    _Py_clock_info_t *info);


// --- _PyDeadline -----------------------------------------------------------

// Create a deadline.
// Pseudo code: return PyTime_MonotonicRaw() + timeout
// Export for '_ssl' shared extension.
PyAPI_FUNC(PyTime_t) _PyDeadline_Init(PyTime_t timeout);

// Get remaining time from a deadline.
// Pseudo code: return deadline - PyTime_MonotonicRaw()
// Export for '_ssl' shared extension.
PyAPI_FUNC(PyTime_t) _PyDeadline_Get(PyTime_t deadline);


// --- _PyTimeFraction -------------------------------------------------------

// Set a fraction.
// Return 0 on success.
// Return -1 if the fraction is invalid.
extern int _PyTimeFraction_Set(
    _PyTimeFraction *frac,
    PyTime_t numer,
    PyTime_t denom);

// Compute ticks * frac.numer / frac.denom.
// Clamp to [PyTime_MIN; PyTime_MAX] on overflow.
extern PyTime_t _PyTimeFraction_Mul(
    PyTime_t ticks,
    const _PyTimeFraction *frac);

// Compute a clock resolution: frac.numer / frac.denom / 1e9.
extern double _PyTimeFraction_Resolution(
    const _PyTimeFraction *frac);

extern PyStatus _PyTime_Init(struct _Py_time_runtime_state *state);

#ifdef __cplusplus
}
#endif
#endif   // !Py_INTERNAL_TIME_H
