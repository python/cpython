
/* Generator object interface */

#ifndef Py_GENOBJECT_H
#define Py_GENOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	PyObject_HEAD
	/* The gi_ prefix is intended to remind of generator-iterator. */

	PyFrameObject *gi_frame;

	/* True if generator is being executed. */
	int gi_running;

	/* List of weak reference. */
	PyObject *gi_weakreflist;
} PyGenObject;

PyAPI_DATA(PyTypeObject) PyGen_Type;

#define PyGen_Check(op) PyObject_TypeCheck(op, &PyGen_Type)
#define PyGen_CheckExact(op) ((op)->ob_type == &PyGen_Type)

PyAPI_FUNC(PyObject *) PyGen_New(PyFrameObject *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_GENOBJECT_H */
