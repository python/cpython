
/* Module object implementation */

#include "Python.h"

typedef struct {
	PyObject_HEAD
	PyObject *md_dict;
} PyModuleObject;

PyObject *
PyModule_New(char *name)
{
	PyModuleObject *m;
	PyObject *nameobj;
	m = PyObject_NEW(PyModuleObject, &PyModule_Type);
	if (m == NULL)
		return NULL;
	nameobj = PyString_FromString(name);
	m->md_dict = PyDict_New();
	if (m->md_dict == NULL || nameobj == NULL)
		goto fail;
	if (PyDict_SetItemString(m->md_dict, "__name__", nameobj) != 0)
		goto fail;
	if (PyDict_SetItemString(m->md_dict, "__doc__", Py_None) != 0)
		goto fail;
	Py_DECREF(nameobj);
	return (PyObject *)m;

 fail:
	Py_XDECREF(nameobj);
	Py_DECREF(m);
	return NULL;
}

PyObject *
PyModule_GetDict(PyObject *m)
{
	if (!PyModule_Check(m)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyModuleObject *)m) -> md_dict;
}

char *
PyModule_GetName(PyObject *m)
{
	PyObject *nameobj;
	if (!PyModule_Check(m)) {
		PyErr_BadArgument();
		return NULL;
	}
	nameobj = PyDict_GetItemString(((PyModuleObject *)m)->md_dict,
				       "__name__");
	if (nameobj == NULL || !PyString_Check(nameobj)) {
		PyErr_SetString(PyExc_SystemError, "nameless module");
		return NULL;
	}
	return PyString_AsString(nameobj);
}

char *
PyModule_GetFilename(PyObject *m)
{
	PyObject *fileobj;
	if (!PyModule_Check(m)) {
		PyErr_BadArgument();
		return NULL;
	}
	fileobj = PyDict_GetItemString(((PyModuleObject *)m)->md_dict,
				       "__file__");
	if (fileobj == NULL || !PyString_Check(fileobj)) {
		PyErr_SetString(PyExc_SystemError, "module filename missing");
		return NULL;
	}
	return PyString_AsString(fileobj);
}

void
_PyModule_Clear(PyObject *m)
{
	/* To make the execution order of destructors for global
	   objects a bit more predictable, we first zap all objects
	   whose name starts with a single underscore, before we clear
	   the entire dictionary.  We zap them by replacing them with
	   None, rather than deleting them from the dictionary, to
	   avoid rehashing the dictionary (to some extent). */

	int pos;
	PyObject *key, *value;
	PyObject *d;

	d = ((PyModuleObject *)m)->md_dict;

	/* First, clear only names starting with a single underscore */
	pos = 0;
	while (PyDict_Next(d, &pos, &key, &value)) {
		if (value != Py_None && PyString_Check(key)) {
			char *s = PyString_AsString(key);
			if (s[0] == '_' && s[1] != '_') {
				if (Py_VerboseFlag > 1)
				    PySys_WriteStderr("#   clear[1] %s\n", s);
				PyDict_SetItem(d, key, Py_None);
			}
		}
	}

	/* Next, clear all names except for __builtins__ */
	pos = 0;
	while (PyDict_Next(d, &pos, &key, &value)) {
		if (value != Py_None && PyString_Check(key)) {
			char *s = PyString_AsString(key);
			if (s[0] != '_' || strcmp(s, "__builtins__") != 0) {
				if (Py_VerboseFlag > 1)
				    PySys_WriteStderr("#   clear[2] %s\n", s);
				PyDict_SetItem(d, key, Py_None);
			}
		}
	}

	/* Note: we leave __builtins__ in place, so that destructors
	   of non-global objects defined in this module can still use
	   builtins, in particularly 'None'. */

}

/* Methods */

static void
module_dealloc(PyModuleObject *m)
{
	if (m->md_dict != NULL) {
		_PyModule_Clear((PyObject *)m);
		Py_DECREF(m->md_dict);
	}
	PyObject_DEL(m);
}

static PyObject *
module_repr(PyModuleObject *m)
{
	char buf[400];
	char *name;
	char *filename;
	name = PyModule_GetName((PyObject *)m);
	if (name == NULL) {
		PyErr_Clear();
		name = "?";
	}
	filename = PyModule_GetFilename((PyObject *)m);
	if (filename == NULL) {
		PyErr_Clear();
		sprintf(buf, "<module '%.80s' (built-in)>", name);
	} else {
		sprintf(buf, "<module '%.80s' from '%.255s'>", name, filename);
	}

	return PyString_FromString(buf);
}

static PyObject *
module_getattr(PyModuleObject *m, char *name)
{
	PyObject *res;
	if (strcmp(name, "__dict__") == 0) {
		Py_INCREF(m->md_dict);
		return m->md_dict;
	}
	res = PyDict_GetItemString(m->md_dict, name);
	if (res == NULL)
		PyErr_SetString(PyExc_AttributeError, name);
	else
		Py_INCREF(res);
	return res;
}

static int
module_setattr(PyModuleObject *m, char *name, PyObject *v)
{
	if (name[0] == '_' && strcmp(name, "__dict__") == 0) {
		PyErr_SetString(PyExc_TypeError,
				"read-only special attribute");
		return -1;
	}
	if (v == NULL) {
		int rv = PyDict_DelItemString(m->md_dict, name);
		if (rv < 0)
			PyErr_SetString(PyExc_AttributeError,
				   "delete non-existing module attribute");
		return rv;
	}
	else
		return PyDict_SetItemString(m->md_dict, name, v);
}

PyTypeObject PyModule_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"module",		/*tp_name*/
	sizeof(PyModuleObject),	/*tp_size*/
	0,			/*tp_itemsize*/
	(destructor)module_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)module_getattr, /*tp_getattr*/
	(setattrfunc)module_setattr, /*tp_setattr*/
	0,			/*tp_compare*/
	(reprfunc)module_repr, /*tp_repr*/
};
