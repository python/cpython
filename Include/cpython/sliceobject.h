#ifndef Py_CPYTHON_SLICEOBJECT_H
#  error "this header file must not be included directly"
#endif

/* Slice object interface */

/*
A slice object containing start, stop, and step data members (the
names are from range).  After much talk with Guido, it was decided to
let these be any arbitrary python type.  Py_None stands for omitted values.
*/
typedef struct {
    PyObject_HEAD
    PyObject *start, *stop, *step;      /* not NULL */
} PySliceObject;

PyAPI_FUNC(PyObject *) _PySlice_FromIndices(Py_ssize_t start, Py_ssize_t stop);
PyAPI_FUNC(int) _PySlice_GetLongIndices(PySliceObject *self, PyObject *length,
                                 PyObject **start_ptr, PyObject **stop_ptr,
                                 PyObject **step_ptr);
