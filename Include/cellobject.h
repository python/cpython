/* Cell object interface */

#ifndef Py_CELLOBJECT_H
#define Py_CELLOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	PyObject_HEAD
	PyObject *ob_ref;
} PyCellObject;

extern DL_IMPORT(PyTypeObject) PyCell_Type;

#define PyCell_Check(op) ((op)->ob_type == &PyCell_Type)

extern DL_IMPORT(PyObject *) PyCell_New(PyObject *);
extern DL_IMPORT(PyObject *) PyCell_Get(PyObject *);
extern DL_IMPORT(int) PyCell_Set(PyObject *, PyObject *);

#define PyCell_GET(op) (((PyCellObject *)(op))->ob_ref)
#define PyCell_SET(op, v) (((PyCellObject *)(op))->ob_ref = v)

#ifdef __cplusplus
}
#endif
#endif /* !Py_TUPLEOBJECT_H */
