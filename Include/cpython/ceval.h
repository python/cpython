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
#  define _Py_MakeRecCheck(x)  \
    (++(x) > _Py_CheckRecursionLimit || \
     ++(PyThreadState_GET()->stackcheck_counter) > 64)
#else
#  define _Py_MakeRecCheck(x)  (++(x) > _Py_CheckRecursionLimit)
#endif

PyAPI_FUNC(int) _Py_CheckRecursiveCall(const char *where);

#define _Py_EnterRecursiveCall_macro(where)  \
            (_Py_MakeRecCheck(PyThreadState_GET()->recursion_depth) &&  \
             _Py_CheckRecursiveCall(where))

#define Py_EnterRecursiveCall(where) _Py_EnterRecursiveCall_macro(where)


/* Compute the "lower-water mark" for a recursion limit. When
 * Py_LeaveRecursiveCall() is called with a recursion depth below this mark,
 * the overflowed flag is reset to 0. */
#define _Py_RecursionLimitLowerWaterMark(limit) \
    (((limit) > 200) \
        ? ((limit) - 50) \
        : (3 * ((limit) >> 2)))

#define _Py_MakeEndRecCheck(x) \
    (--(x) < _Py_RecursionLimitLowerWaterMark(_Py_CheckRecursionLimit))

#define _Py_LeaveRecursiveCall_macro()                         \
    do{ if(_Py_MakeEndRecCheck(PyThreadState_GET()->recursion_depth))  \
      PyThreadState_GET()->overflowed = 0;  \
    } while(0)

#define Py_LeaveRecursiveCall() _Py_LeaveRecursiveCall_macro()

#ifdef __cplusplus
}
#endif
