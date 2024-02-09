// PyTime_t C API: see Doc/c-api/time.rst for the documentation.

#ifndef Py_LIMITED_API
#ifndef Py_PYTIME_H
#define Py_PYTIME_H
#ifdef __cplusplus
extern "C" {
#endif

typedef PY_INT64_T PyTime_t;
#define PyTime_MIN INT64_MIN
#define PyTime_MAX INT64_MAX

PyAPI_FUNC(double) PyTime_AsSecondsDouble(PyTime_t t);
PyAPI_FUNC(int) PyTime_AsNanoseconds(PyTime_t t, PY_INT64_T *result);
PyAPI_FUNC(int) PyTime_Monotonic(PyTime_t *result);
PyAPI_FUNC(int) PyTime_PerfCounter(PyTime_t *result);
PyAPI_FUNC(int) PyTime_Time(PyTime_t *result);

static inline int
_PyTime_AsNanoseconds_impl(PyTime_t t, PY_INT64_T *result)
{
    // Implementation detail: PyTime_t is already a int64_t in nanoseconds.
    // This may change. We might e.g. extend PyTime_t to 128 bits, then
    // this function can start raising OverflowError.
    *result = t;
    return 0;
}

#define PyTime_AsNanoseconds _PyTime_AsNanoseconds_impl

#ifdef __cplusplus
}
#endif
#endif /* Py_PYTIME_H */
#endif /* Py_LIMITED_API */
