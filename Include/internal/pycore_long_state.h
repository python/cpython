#ifndef Py_INTERNAL_LONG_STATE_H
#define Py_INTERNAL_LONG_STATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#define _PY_NSMALLPOSINTS           257
#define _PY_NSMALLNEGINTS           5

// _PyLong_GetZero() and _PyLong_GetOne() must always be available
#if _PY_NSMALLPOSINTS < 2
#  error "_PY_NSMALLPOSINTS must be greater than 1"
#endif

struct _Py_long_state {
    /* Small integers are preallocated in this array so that they
     * can be shared.
     * The integers that are preallocated are those in the range
     *-_PY_NSMALLNEGINTS (inclusive) to _PY_NSMALLPOSINTS (not inclusive).
     */
    PyLongObject small_ints[_PY_NSMALLNEGINTS + _PY_NSMALLPOSINTS];
};

#define _PyLong_SMALL_INTS _PyRuntime.int_state.small_ints

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_LONG_STATE_H */
