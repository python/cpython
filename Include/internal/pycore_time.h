// The _PyTime_t API is written to use timestamp and timeout values stored in
// various formats and to read clocks.
//
// The _PyTime_t type is an integer to support directly common arithmetic
// operations like t1 + t2.
//
// The _PyTime_t API supports a resolution of 1 nanosecond. The _PyTime_t type
// is signed to support negative timestamps. The supported range is around
// [-292.3 years; +292.3 years]. Using the Unix epoch (January 1st, 1970), the
// supported date range is around [1677-09-21; 2262-04-11].
//
// Formats:
//
// * seconds
// * seconds as a floating pointer number (C double)
// * milliseconds (10^-3 seconds)
// * microseconds (10^-6 seconds)
// * 100 nanoseconds (10^-7 seconds)
// * nanoseconds (10^-9 seconds)
// * timeval structure, 1 microsecond resolution (10^-6 seconds)
// * timespec structure, 1 nanosecond resolution (10^-9 seconds)
//
// Integer overflows are detected and raise OverflowError. Conversion to a
// resolution worse than 1 nanosecond is rounded correctly with the requested
// rounding mode. There are 4 rounding modes: floor (towards -inf), ceiling
// (towards +inf), half even and up (away from zero).
//
// Some functions clamp the result in the range [_PyTime_MIN; _PyTime_MAX], so
// the caller doesn't have to handle errors and doesn't need to hold the GIL.
// For example, _PyTime_Add(t1, t2) computes t1+t2 and clamp the result on
// overflow.
//
// Clocks:
//
// * System clock
// * Monotonic clock
// * Performance counter
//
// Operations like (t * k / q) with integers are implemented in a way to reduce
// the risk of integer overflow. Such operation is used to convert a clock
// value expressed in ticks with a frequency to _PyTime_t, like
// QueryPerformanceCounter() with QueryPerformanceFrequency().

#ifndef Py_INTERNAL_TIME_H
#define Py_INTERNAL_TIME_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


#ifdef __clang__
struct timeval;
#endif


// Create a timestamp from a number of seconds in double.
// Export for '_socket' shared extension.
PyAPI_FUNC(_PyTime_t) _PyTime_FromSecondsDouble(double seconds, _PyTime_round_t round);

// --- _PyTimeFraction -------------------------------------------------------

typedef struct {
    _PyTime_t numer;
    _PyTime_t denom;
} _PyTimeFraction;

// Set a fraction.
// Return 0 on success.
// Return -1 if the fraction is invalid.
extern int _PyTimeFraction_Set(
    _PyTimeFraction *frac,
    _PyTime_t numer,
    _PyTime_t denom);

// Compute ticks * frac.numer / frac.denom.
// Clamp to [_PyTime_MIN; _PyTime_MAX] on overflow.
extern _PyTime_t _PyTimeFraction_Mul(
    _PyTime_t ticks,
    const _PyTimeFraction *frac);

// Compute a clock resolution: frac.numer / frac.denom / 1e9.
extern double _PyTimeFraction_Resolution(
    const _PyTimeFraction *frac);


#ifdef __cplusplus
}
#endif
#endif   // !Py_INTERNAL_TIME_H
