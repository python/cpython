#ifndef Py_SLICEOBJECT_H
#define Py_SLICEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/* The unique ellipsis object "..." */

PyAPI_DATA(PyObject) _Py_EllipsisObject; /* Don't use this directly */

#if defined(Py_LIMITED_API) && Py_LIMITED_API+0 >= 0x030D0000
#  define Py_Ellipsis Py_GetConstantBorrowed(Py_CONSTANT_ELLIPSIS)
#else
#  define Py_Ellipsis (&_Py_EllipsisObject)
#endif

/* Slice object interface */

PyAPI_DATA(PyTypeObject) PySlice_Type;
PyAPI_DATA(PyTypeObject) PyEllipsis_Type;

#define PySlice_Check(op) Py_IS_TYPE((op), &PySlice_Type)

PyAPI_FUNC(PyObject *) PySlice_New(PyObject* start, PyObject* stop,
                                  PyObject* step);
PyAPI_FUNC(int) PySlice_GetIndices(PyObject *r, Py_ssize_t length,
                                  Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t *step);
Py_DEPRECATED(3.7)
PyAPI_FUNC(int) PySlice_GetIndicesEx(PyObject *r, Py_ssize_t length,
                                     Py_ssize_t *start, Py_ssize_t *stop,
                                     Py_ssize_t *step,
                                     Py_ssize_t *slicelength);

#if !defined(Py_LIMITED_API) || (Py_LIMITED_API+0 >= 0x03050400 && Py_LIMITED_API+0 < 0x03060000) || Py_LIMITED_API+0 >= 0x03060100
#define PySlice_GetIndicesEx(slice, length, start, stop, step, slicelen) (  \
    PySlice_Unpack((slice), (start), (stop), (step)) < 0 ?                  \
    ((*(slicelen) = 0), -1) :                                               \
    ((*(slicelen) = PySlice_AdjustIndices((length), (start), (stop), *(step))), \
     0))
PyAPI_FUNC(int) PySlice_Unpack(PyObject *slice,
                               Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t *step);
PyAPI_FUNC(Py_ssize_t) PySlice_AdjustIndices(Py_ssize_t length,
                                             Py_ssize_t *start, Py_ssize_t *stop,
                                             Py_ssize_t step);
#endif

#ifndef Py_LIMITED_API
#  define Py_CPYTHON_SLICEOBJECT_H
#  include "cpython/sliceobject.h"
#  undef Py_CPYTHON_SLICEOBJECT_H
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_SLICEOBJECT_H */
