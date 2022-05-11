/* Cell object interface */

#ifndef Py_LIMITED_API
#ifndef Py_CELLOBJECT_H
#define Py_CELLOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PyObject_HEAD
    /* Content of the cell or NULL when empty */
    PyObject *ob_ref;
} PyCellObject;

PyAPI_DATA(PyTypeObject) PyCell_Type;

#define PyCell_Check(op) Py_IS_TYPE(op, &PyCell_Type)

PyAPI_FUNC(PyObject *) PyCell_New(PyObject *);
PyAPI_FUNC(PyObject *) PyCell_Get(PyObject *);
PyAPI_FUNC(int) PyCell_Set(PyObject *, PyObject *);

static inline PyObject* PyCell_GET(PyObject *op) {
    assert(PyCell_Check(op));
    PyCellObject *cell = _Py_CAST(PyCellObject*, op);
    return cell->ob_ref;
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyCell_GET(op) PyCell_GET(_PyObject_CAST(op))
#endif

static inline void PyCell_SET(PyObject *op, PyObject *value) {
    assert(PyCell_Check(op));
    PyCellObject *cell = _Py_CAST(PyCellObject*, op);
    cell->ob_ref = value;
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030c0000
#  define PyCell_SET(op, value) PyCell_SET(_PyObject_CAST(op), (value))
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_TUPLEOBJECT_H */
#endif /* Py_LIMITED_API */
