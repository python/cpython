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

/* Function object implementation */

#include "Python.h"
#include "compile.h"
#include "structmember.h"

PyObject *
PyFunction_New(code, globals)
	PyObject *code;
	PyObject *globals;
{
	PyFunctionObject *op = PyObject_NEW(PyFunctionObject,
					    &PyFunction_Type);
	if (op != NULL) {
		PyObject *doc;
		PyObject *consts;
		Py_INCREF(code);
		op->func_code = code;
		Py_INCREF(globals);
		op->func_globals = globals;
		op->func_name = ((PyCodeObject *)code)->co_name;
		Py_INCREF(op->func_name);
		op->func_defaults = NULL; /* No default arguments */
		consts = ((PyCodeObject *)code)->co_consts;
		if (PyTuple_Size(consts) >= 1) {
			doc = PyTuple_GetItem(consts, 0);
			if (!PyString_Check(doc))
				doc = Py_None;
		}
		else
			doc = Py_None;
		Py_INCREF(doc);
		op->func_doc = doc;
	}
	return (PyObject *)op;
}

PyObject *
PyFunction_GetCode(op)
	PyObject *op;
{
	if (!PyFunction_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyFunctionObject *) op) -> func_code;
}

PyObject *
PyFunction_GetGlobals(op)
	PyObject *op;
{
	if (!PyFunction_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyFunctionObject *) op) -> func_globals;
}

PyObject *
PyFunction_GetDefaults(op)
	PyObject *op;
{
	if (!PyFunction_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyFunctionObject *) op) -> func_defaults;
}

int
PyFunction_SetDefaults(op, defaults)
	PyObject *op;
	PyObject *defaults;
{
	if (!PyFunction_Check(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	if (defaults == Py_None)
		defaults = NULL;
	else if (PyTuple_Check(defaults))
		Py_XINCREF(defaults);
	else {
		PyErr_SetString(PyExc_SystemError, "non-tuple default args");
		return -1;
	}
	Py_XDECREF(((PyFunctionObject *) op) -> func_defaults);
	((PyFunctionObject *) op) -> func_defaults = defaults;
	return 0;
}

/* Methods */

#define OFF(x) offsetof(PyFunctionObject, x)

static struct memberlist func_memberlist[] = {
	{"func_code",	T_OBJECT,	OFF(func_code),		READONLY},
	{"func_globals",T_OBJECT,	OFF(func_globals),	READONLY},
	{"func_name",	T_OBJECT,	OFF(func_name),		READONLY},
	{"__name__",	T_OBJECT,	OFF(func_name),		READONLY},
	{"func_defaults",T_OBJECT,	OFF(func_defaults),	READONLY},
	{"func_doc",	T_OBJECT,	OFF(func_doc)},
	{"__doc__",	T_OBJECT,	OFF(func_doc)},
	{NULL}	/* Sentinel */
};

static PyObject *
func_getattr(op, name)
	PyFunctionObject *op;
	char *name;
{
	if (name[0] != '_' && PyEval_GetRestricted()) {
		PyErr_SetString(PyExc_RuntimeError,
		  "function attributes not accessible in restricted mode");
		return NULL;
	}
	return PyMember_Get((char *)op, func_memberlist, name);
}

static void
func_dealloc(op)
	PyFunctionObject *op;
{
	Py_DECREF(op->func_code);
	Py_DECREF(op->func_globals);
	Py_DECREF(op->func_name);
	Py_XDECREF(op->func_defaults);
	Py_XDECREF(op->func_doc);
	PyMem_DEL(op);
}

static PyObject*
func_repr(op)
	PyFunctionObject *op;
{
	char buf[140];
	if (op->func_name == Py_None)
		sprintf(buf, "<anonymous function at %lx>", (long)op);
	else
		sprintf(buf, "<function %.100s at %lx>",
			PyString_AsString(op->func_name),
			(long)op);
	return PyString_FromString(buf);
}

static int
func_compare(f, g)
	PyFunctionObject *f, *g;
{
	int c;
	if (f->func_globals != g->func_globals)
		return (f->func_globals < g->func_globals) ? -1 : 1;
	c = PyObject_Compare(f->func_defaults, g->func_defaults);
	if (c != 0)
		return c;
	return PyObject_Compare(f->func_code, g->func_code);
}

static long
func_hash(f)
	PyFunctionObject *f;
{
	long h;
	h = PyObject_Hash(f->func_code);
	if (h == -1) return h;
	h = h ^ (long)f->func_globals;
	if (h == -1) h = -2;
	return h;
}

PyTypeObject PyFunction_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"function",
	sizeof(PyFunctionObject),
	0,
	(destructor)func_dealloc, /*tp_dealloc*/
	0,		/*tp_print*/
	(getattrfunc)func_getattr, /*tp_getattr*/
	0,		/*tp_setattr*/
	(cmpfunc)func_compare, /*tp_compare*/
	(reprfunc)func_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)func_hash, /*tp_hash*/
};
