
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
	PyObject *weakreflist; /* List of weak references */

	/* Invariants:
	 *     data is a dictionary whose values are all True.
	 *     data points to the same dict for the whole life of the set.
	 * For frozensets only:
	 *     data is immutable.
	 *     hash is the hash of the frozenset or -1 if not computed yet.
	 */
} PySetObject;

PyAPI_DATA(PyTypeObject) PySet_Type;
PyAPI_DATA(PyTypeObject) PyFrozenSet_Type;

#define PyFrozenSet_CheckExact(ob) ((ob)->ob_type == &PyFrozenSet_Type)
#define PyAnySet_Check(ob) \
	((ob)->ob_type == &PySet_Type || (ob)->ob_type == &PyFrozenSet_Type || \
	  PyType_IsSubtype((ob)->ob_type, &PySet_Type) || \
	  PyType_IsSubtype((ob)->ob_type, &PyFrozenSet_Type))

#ifdef __cplusplus
}
#endif
#endif /* !Py_SETOBJECT_H */
