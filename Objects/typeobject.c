
/* Type object implementation */

#include "Python.h"

/* Type object implementation */

static PyObject *
type_getattr(PyTypeObject *t, char *name)
{
	if (strcmp(name, "__name__") == 0)
		return PyString_FromString(t->tp_name);
	if (strcmp(name, "__doc__") == 0) {
		char *doc = t->tp_doc;
		if (doc != NULL)
			return PyString_FromString(doc);
		Py_INCREF(Py_None);
		return Py_None;
	}
	if (strcmp(name, "__members__") == 0)
		return Py_BuildValue("[ss]", "__doc__", "__name__");
	PyErr_SetString(PyExc_AttributeError, name);
	return NULL;
}

static PyObject *
type_repr(PyTypeObject *v)
{
	char buf[100];
	sprintf(buf, "<type '%.80s'>", v->tp_name);
	return PyString_FromString(buf);
}

PyTypeObject PyType_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/* Number of items for varobject */
	"type",			/* Name of this type */
	sizeof(PyTypeObject),	/* Basic object size */
	0,			/* Item size for varobject */
	0,			/*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)type_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	(reprfunc)type_repr,	/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,			/*tp_call*/
	0,			/*tp_str*/
	0,			/*tp_xxx1*/
	0,			/*tp_xxx2*/
	0,			/*tp_xxx3*/
	0,			/*tp_xxx4*/
	"Define the behavior of a particular type of object.",
};
