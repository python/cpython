#ifndef Py_SLICEOBJECT_H
#define Py_SLICEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/* The unique ellipsis object "..." */

PyAPI_DATA(PyObject) _Py_EllipsisObject; /* Don't use this directly */

#define Py_Ellipsis (&_Py_EllipsisObject)

/* Slice object interface */

/*

A slice object containing start, stop, and step data members (the
names are from range).  After much talk with Guido, it was decided to
let these be any arbitrary python type. 
*/

typedef struct {
    PyObject_HEAD
    PyObject *start, *stop, *step;
} PySliceObject;

PyAPI_DATA(PyTypeObject) PySlice_Type;

#define PySlice_Check(op) ((op)->ob_type == &PySlice_Type)

PyAPI_FUNC(PyObject *) PySlice_New(PyObject* start, PyObject* stop,
                                  PyObject* step);
PyAPI_FUNC(int) PySlice_GetIndices(PySliceObject *r, int length,
                                  int *start, int *stop, int *step);
PyAPI_FUNC(int) PySlice_GetIndicesEx(PySliceObject *r, int length,
				    int *start, int *stop, 
				    int *step, int *slicelength);

#ifdef __cplusplus
}
#endif
#endif /* !Py_SLICEOBJECT_H */
