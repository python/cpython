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

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03060100
PyAPI_FUNC(int) PySlice_Unpack(PyObject *slice,
                               Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t *step);
PyAPI_FUNC(Py_ssize_t) PySlice_AdjustIndices(Py_ssize_t length,
                                             Py_ssize_t *start, Py_ssize_t *stop,
                                             Py_ssize_t step);

static inline int
_PySlice_GetIndicesEx(PyObject *slice, Py_ssize_t length,
                      Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t *step,
                      Py_ssize_t *slicelen)
{
    if (PySlice_Unpack(slice, start, stop, step) < 0) {
        *slicelen = 0;
        return -1;
    }

    *slicelen = PySlice_AdjustIndices(length, start, stop, *step);
    return 0;
}
#define PySlice_GetIndicesEx _PySlice_GetIndicesEx
#endif

#ifndef Py_LIMITED_API
#  define _Py_CPYTHON_SLICEOBJECT_H
#  include "cpython/sliceobject.h"
#  undef _Py_CPYTHON_SLICEOBJECT_H
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_SLICEOBJECT_H */
