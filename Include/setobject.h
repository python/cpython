
/* Set object interface */

#ifndef Py_SETOBJECT_H
#define Py_SETOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/*
This data structure is shared by set and frozenset objects.
*/

typedef struct {
	PyObject_HEAD
	PyObject *data;
	long hash;	/* only used by frozenset objects */
} PySetObject;

PyAPI_DATA(PyTypeObject) PySet_Type;
PyAPI_DATA(PyTypeObject) PyFrozenSet_Type;

#ifdef __cplusplus
}
#endif
#endif /* !Py_SETOBJECT_H */
