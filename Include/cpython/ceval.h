#ifndef Py_CPYTHON_CEVAL_H
#  error "this header file must not be included directly"
#endif

#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(int) _Py_CheckRecursionLimit;

#ifdef USE_STACKCHECK
/* With USE_STACKCHECK macro defined, trigger stack checks in
   _Py_CheckRecursiveCall() on every 64th call to Py_EnterRecursiveCall. */
static inline int _Py_MakeRecCheck(PyThreadState *tstate)  {
    return (++tstate->recursion_depth > _Py_CheckRecursionLimit
            || ++tstate->stackcheck_counter > 64);
}
#else
static inline int _Py_MakeRecCheck(PyThreadState *tstate) {
    return (++tstate->recursion_depth > _Py_CheckRecursionLimit);
}
#endif

PyAPI_FUNC(int) _Py_CheckRecursiveCall(
    PyThreadState *tstate,
    const char *where);

static inline int _Py_EnterRecursiveCall(PyThreadState *tstate,
                                         const char *where) {
    return (_Py_MakeRecCheck(tstate) && _Py_CheckRecursiveCall(tstate, where));
}

static inline int _Py_EnterRecursiveCall_inline(const char *where) {
    PyThreadState *tstate = PyThreadState_GET();
    return _Py_EnterRecursiveCall(tstate, where);
}

#define Py_EnterRecursiveCall(where) _Py_EnterRecursiveCall_inline(where)


/* Compute the "lower-water mark" for a recursion limit. When
 * Py_LeaveRecursiveCall() is called with a recursion depth below this mark,
 * the overflowed flag is reset to 0. */
#define _Py_RecursionLimitLowerWaterMark(limit) \
    (((limit) > 200) \
        ? ((limit) - 50) \
        : (3 * ((limit) >> 2)))

#define _Py_MakeEndRecCheck(x) \
    (--(x) < _Py_RecursionLimitLowerWaterMark(_Py_CheckRecursionLimit))

static inline void _Py_LeaveRecursiveCall(PyThreadState *tstate)  {
    if (_Py_MakeEndRecCheck(tstate->recursion_depth)) {
        tstate->overflowed = 0;
    }
}

static inline void _Py_LeaveRecursiveCall_inline(void)  {
    PyThreadState *tstate = PyThreadState_GET();
    _Py_LeaveRecursiveCall(tstate);
}

#define Py_LeaveRecursiveCall() _Py_LeaveRecursiveCall_inline()

#ifdef __cplusplus
}
#endif
