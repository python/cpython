#ifndef Py_INTERNAL_ITEROBJECT_H
#define Py_INTERNAL_ITEROBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

extern PyTypeObject _PyACallIter_Type;
extern PyTypeObject _PyACallIterAwaitable_Type;

// Like PyCallIter_New(), but the iteration also stops when *callable* raises
// an exception matching *stop_exc* (an exception class or a tuple of exception
// classes).  Both *sentinel* and *stop_exc* can be NULL.
extern PyObject *_PyCallIter_NewEx(PyObject *callable, PyObject *sentinel,
                                   PyObject *stop_exc);

// The asynchronous counterpart of _PyCallIter_NewEx(): the result of
// *callable* is awaited, and StopAsyncIteration stops the iteration.
extern PyObject *_PyACallIter_New(PyObject *callable, PyObject *sentinel,
                                  PyObject *stop_exc);

// Return NULL if *stop_exc* has no effect: *implied_exc* stops the iteration
// in any case, and an empty tuple never matches a raised exception.
static inline PyObject *
_PyIter_NormalizeStopException(PyObject *stop_exc, PyObject *implied_exc)
{
    if (stop_exc == implied_exc ||
        (PyTuple_Check(stop_exc) && PyTuple_GET_SIZE(stop_exc) == 0))
    {
        return NULL;
    }
    return stop_exc;
}

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_ITEROBJECT_H */
