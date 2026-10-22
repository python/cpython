// PyTime_t C API: see Doc/c-api/time.rst for the documentation.

#ifndef Py_LIMITED_API
#ifndef Py_PYTIME_H
#define Py_PYTIME_H
#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t PyTime_t;
#define PyTime_MIN INT64_MIN
#define PyTime_MAX INT64_MAX

PyAPI_FUNC(double) PyTime_AsSecondsDouble(PyTime_t t);
PyAPI_FUNC(int) PyTime_Monotonic(PyTime_t *result);
PyAPI_FUNC(int) PyTime_PerfCounter(PyTime_t *result);
PyAPI_FUNC(int) PyTime_Time(PyTime_t *result);

PyAPI_FUNC(int) PyTime_MonotonicRaw(PyTime_t *result);
PyAPI_FUNC(int) PyTime_PerfCounterRaw(PyTime_t *result);
PyAPI_FUNC(int) PyTime_TimeRaw(PyTime_t *result);

#ifdef __cplusplus
}
#endif
#endif /* Py_PYTIME_H */
#endif /* Py_LIMITED_API */
