
#include "Python.h"
#include "structmember.h"

/* Functional module written and maintained 
   by Hye-Shik Chang <perky@FreeBSD.org>
   with adaptations by Raymond Hettinger <python@rcn.com>
   Copyright (c) 2004, 2005 Python Software Foundation.
   All rights reserved.
*/

/* partial object **********************************************************/

typedef struct {
	PyObject_HEAD
	PyObject *fn;
	PyObject *args;
	PyObject *kw;
} partialobject;

static PyTypeObject partial_type;

static PyObject *
partial_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
	PyObject *func;
	partialobject *pto;

	if (PyTuple_GET_SIZE(args) < 1) {
		PyErr_SetString(PyExc_TypeError,
				"type 'partial' takes at least one argument");
		return NULL;
	}

	func = PyTuple_GET_ITEM(args, 0);
	if (!PyCallable_Check(func)) {
		PyErr_SetString(PyExc_TypeError,
				"the first argument must be callable");
		return NULL;
	}

	/* create partialobject structure */
	pto = (partialobject *)type->tp_alloc(type, 0);
	if (pto == NULL)
		return NULL;

	pto->fn = func;
	Py_INCREF(func);
	pto->args = PyTuple_GetSlice(args, 1, INT_MAX);
	if (pto->args == NULL) {
		pto->kw = NULL;
		Py_DECREF(pto);
		return NULL;
	}
	if (kw != NULL) {
		pto->kw = PyDict_Copy(kw);
		if (pto->kw == NULL) {
			Py_DECREF(pto);
			return NULL;
		}
	} else {
		pto->kw = Py_None;
		Py_INCREF(Py_None);
	}

	return (PyObject *)pto;
}

static void
partial_dealloc(partialobject *pto)
{
	PyObject_GC_UnTrack(pto);
	Py_XDECREF(pto->fn);
	Py_XDECREF(pto->args);
	Py_XDECREF(pto->kw);
	pto->ob_type->tp_free(pto);
}

static PyObject *
partial_call(partialobject *pto, PyObject *args, PyObject *kw)
{
	PyObject *ret;
	PyObject *argappl = NULL, *kwappl = NULL;

	assert (PyCallable_Check(pto->fn));
	assert (PyTuple_Check(pto->args));
	assert (pto->kw == Py_None  ||  PyDict_Check(pto->kw));

	if (PyTuple_GET_SIZE(pto->args) == 0) {
		argappl = args;
		Py_INCREF(args);
	} else if (PyTuple_GET_SIZE(args) == 0) {
		argappl = pto->args;
		Py_INCREF(pto->args);
	} else {
		argappl = PySequence_Concat(pto->args, args);
		if (argappl == NULL)
			return NULL;
	}

	if (pto->kw == Py_None) {
		kwappl = kw;
		Py_XINCREF(kw);
	} else {
		kwappl = PyDict_Copy(pto->kw);
		if (kwappl == NULL) {
			Py_DECREF(argappl);
			return NULL;
		}
		if (kw != NULL) {
			if (PyDict_Merge(kwappl, kw, 1) != 0) {
				Py_DECREF(argappl);
				Py_DECREF(kwappl);
				return NULL;
			}
		}
	}

	ret = PyObject_Call(pto->fn, argappl, kwappl);
	Py_DECREF(argappl);
	Py_XDECREF(kwappl);
	return ret;
}

static int
partial_traverse(partialobject *pto, visitproc visit, void *arg)
{
	Py_VISIT(pto->fn);
	Py_VISIT(pto->args);
	Py_VISIT(pto->kw);
	return 0;
}

PyDoc_STRVAR(partial_doc,
"partial(func, *args, **keywords) - new function with partial application\n\
	of the given arguments and keywords.\n");

#define OFF(x) offsetof(partialobject, x)
static PyMemberDef partial_memberlist[] = {
	{"func",	T_OBJECT,	OFF(fn),	READONLY,
	 "function object to use in future partial calls"},
	{"args",	T_OBJECT,	OFF(args),	READONLY,
	 "tuple of arguments to future partial calls"},
	{"keywords",	T_OBJECT,	OFF(kw),	READONLY,
	 "dictionary of keyword arguments to future partial calls"},
	{NULL}  /* Sentinel */
};

static PyTypeObject partial_type = {
	PyObject_HEAD_INIT(NULL)
	0,				/* ob_size */
	"functional.partial",		/* tp_name */
	sizeof(partialobject),		/* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)partial_dealloc,	/* tp_dealloc */
	0,				/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	(ternaryfunc)partial_call,	/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,	/* tp_flags */
	partial_doc,			/* tp_doc */
	(traverseproc)partial_traverse,	/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	0,				/* tp_iter */
	0,				/* tp_iternext */
	0,				/* tp_methods */
	partial_memberlist,		/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	0,				/* tp_alloc */
	partial_new,			/* tp_new */
	PyObject_GC_Del,		/* tp_free */
};


/* module level code ********************************************************/

PyDoc_STRVAR(module_doc,
"Tools for functional programming.");

static PyMethodDef module_methods[] = {
 	{NULL,		NULL}		/* sentinel */
};

PyMODINIT_FUNC
initfunctional(void)
{
	int i;
	PyObject *m;
	char *name;
	PyTypeObject *typelist[] = {
		&partial_type,
		NULL
	};

	m = Py_InitModule3("functional", module_methods, module_doc);

	for (i=0 ; typelist[i] != NULL ; i++) {
		if (PyType_Ready(typelist[i]) < 0)
			return;
		name = strchr(typelist[i]->tp_name, '.');
		assert (name != NULL);
		Py_INCREF(typelist[i]);
		PyModule_AddObject(m, name+1, (PyObject *)typelist[i]);
	}
}
