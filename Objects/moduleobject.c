/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Module object implementation */

#include "Python.h"

typedef struct {
	PyObject_HEAD
	PyObject *md_dict;
} PyModuleObject;

PyObject *
PyModule_New(name)
	char *name;
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
PyModule_GetDict(m)
	PyObject *m;
{
	if (!PyModule_Check(m)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyModuleObject *)m) -> md_dict;
}

char *
PyModule_GetName(m)
	PyObject *m;
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

void
_PyModule_Clear(m)
	PyObject *m;
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
				    fprintf(stderr, "#   clear[1] %s\n", s);
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
				    fprintf(stderr, "#   clear[2] %s\n", s);
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
module_dealloc(m)
	PyModuleObject *m;
{
	if (m->md_dict != NULL) {
		_PyModule_Clear((PyObject *)m);
		Py_DECREF(m->md_dict);
	}
	free((char *)m);
}

static PyObject *
module_repr(m)
	PyModuleObject *m;
{
	char buf[100];
	char *name = PyModule_GetName((PyObject *)m);
	if (name == NULL) {
		PyErr_Clear();
		name = "?";
	}
	sprintf(buf, "<module '%.80s'>", name);
	return PyString_FromString(buf);
}

static PyObject *
module_getattr(m, name)
	PyModuleObject *m;
	char *name;
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
module_setattr(m, name, v)
	PyModuleObject *m;
	char *name;
	PyObject *v;
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
