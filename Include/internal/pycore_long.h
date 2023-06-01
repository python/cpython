#ifndef Py_INTERNAL_LONG_H
#define Py_INTERNAL_LONG_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/*
 * Default int base conversion size limitation: Denial of Service prevention.
 *
 * Chosen such that this isn't wildly slow on modern hardware and so that
 * everyone's existing deployed numpy test suite passes before
 * https://github.com/numpy/numpy/issues/22098 is widely available.
 *
 * $ python -m timeit -s 's = "1"*4300' 'int(s)'
 * 2000 loops, best of 5: 125 usec per loop
 * $ python -m timeit -s 's = "1"*4300; v = int(s)' 'str(v)'
 * 1000 loops, best of 5: 311 usec per loop
 * (zen2 cloud VM)
 *
 * 4300 decimal digits fits a ~14284 bit number.
 */
#define _PY_LONG_DEFAULT_MAX_STR_DIGITS 4300
/*
 * Threshold for max digits check.  For performance reasons int() and
 * int.__str__() don't checks values that are smaller than this
 * threshold.  Acts as a guaranteed minimum size limit for bignums that
 * applications can expect from CPython.
 *
 * % python -m timeit -s 's = "1"*640; v = int(s)' 'str(int(s))'
 * 20000 loops, best of 5: 12 usec per loop
 *
 * "640 digits should be enough for anyone." - gps
 * fits a ~2126 bit decimal number.
 */
#define _PY_LONG_MAX_STR_DIGITS_THRESHOLD 640

#if ((_PY_LONG_DEFAULT_MAX_STR_DIGITS != 0) && \
   (_PY_LONG_DEFAULT_MAX_STR_DIGITS < _PY_LONG_MAX_STR_DIGITS_THRESHOLD))
# error "_PY_LONG_DEFAULT_MAX_STR_DIGITS smaller than threshold."
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_LONG_H */
