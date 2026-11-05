#ifndef Py_INTERNAL_PYBUFFER_H
#define Py_INTERNAL_PYBUFFER_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


// Exported for the _interpchannels module.
PyAPI_FUNC(int) _PyBuffer_ReleaseInInterpreter(
        PyInterpreterState *interp, Py_buffer *view);
PyAPI_FUNC(int) _PyBuffer_ReleaseInInterpreterAndRawFree(
        PyInterpreterState *interp, Py_buffer *view);

/* Search the items of the one-dimensional buffer of obj for value, comparing
   their bytes when that is equivalent to comparing them as objects.

   _PyBuffer_FindItem() returns the index of the first item in [start, stop)
   equal to value, -1 if there is none, or -2 with an exception set on error.
   Negative start and stop count from the end, and are clipped like slice
   bounds.

   _PyBuffer_CountItem() returns the number of items equal to value as an
   int, or NULL with an exception set on error.

   Implemented in memoryobject.c. */
PyAPI_FUNC(Py_ssize_t) _PyBuffer_FindItem(
        PyObject *obj, PyObject *value,
        Py_ssize_t start, Py_ssize_t stop);
PyAPI_FUNC(PyObject *) _PyBuffer_CountItem(PyObject *obj, PyObject *value);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYBUFFER_H */
