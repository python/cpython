/* Cell object interface */
#ifndef Py_LIMITED_API
#ifndef Py_CELLOBJECT_H
#define Py_CELLOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PyObject_HEAD
    PyObject *ob_ref;       /* Content of the cell or NULL when empty */
} PyCellObject;

PyAPI_DATA(PyTypeObject) PyCell_Type;

#define PyCell_Check(op) Py_IS_TYPE(op, &PyCell_Type)

PyAPI_FUNC(PyObject *) PyCell_New(PyObject *);
PyAPI_FUNC(PyObject *) PyCell_Get(PyObject *);
PyAPI_FUNC(int) PyCell_Set(PyObject *, PyObject *);

// Cast the argument to PyCellObject* type.
#define _PyCell_CAST(op) (assert(PyCell_Check(op)), (PyCellObject *)(op))

#define PyCell_GET(op) (_PyCell_CAST(op)->ob_ref)

static inline void _PyCell_SET(PyCellObject *cell, PyObject *value)
{
    cell->ob_ref = value;
}
#define PyCell_SET(op, value) _PyCell_SET(_PyCell_CAST(op), value)

#ifdef __cplusplus
}
#endif
#endif /* !Py_TUPLEOBJECT_H */
#endif /* Py_LIMITED_API */
