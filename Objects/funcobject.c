
/* Function object implementation */

#include "Python.h"
#include "compile.h"
#include "structmember.h"

PyObject *
PyFunction_New(PyObject *code, PyObject *globals)
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
			if (!PyString_Check(doc) && !PyUnicode_Check(doc))
				doc = Py_None;
		}
		else
			doc = Py_None;
		Py_INCREF(doc);
		op->func_doc = doc;
	}
	PyObject_GC_Init(op);
	return (PyObject *)op;
}

PyObject *
PyFunction_GetCode(PyObject *op)
{
	if (!PyFunction_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyFunctionObject *) op) -> func_code;
}

PyObject *
PyFunction_GetGlobals(PyObject *op)
{
	if (!PyFunction_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyFunctionObject *) op) -> func_globals;
}

PyObject *
PyFunction_GetDefaults(PyObject *op)
{
	if (!PyFunction_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyFunctionObject *) op) -> func_defaults;
}

int
PyFunction_SetDefaults(PyObject *op, PyObject *defaults)
{
	if (!PyFunction_Check(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	if (defaults == Py_None)
		defaults = NULL;
	else if (PyTuple_Check(defaults)) {
		Py_XINCREF(defaults);
	}
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
	{"func_code",	T_OBJECT,	OFF(func_code)},
	{"func_globals",T_OBJECT,	OFF(func_globals),	READONLY},
	{"func_name",	T_OBJECT,	OFF(func_name),		READONLY},
	{"__name__",	T_OBJECT,	OFF(func_name),		READONLY},
	{"func_defaults",T_OBJECT,	OFF(func_defaults)},
	{"func_doc",	T_OBJECT,	OFF(func_doc)},
	{"__doc__",	T_OBJECT,	OFF(func_doc)},
	{NULL}	/* Sentinel */
};

static PyObject *
func_getattr(PyFunctionObject *op, char *name)
{
	if (name[0] != '_' && PyEval_GetRestricted()) {
		PyErr_SetString(PyExc_RuntimeError,
		  "function attributes not accessible in restricted mode");
		return NULL;
	}
	return PyMember_Get((char *)op, func_memberlist, name);
}

static int
func_setattr(PyFunctionObject *op, char *name, PyObject *value)
{
	if (PyEval_GetRestricted()) {
		PyErr_SetString(PyExc_RuntimeError,
		  "function attributes not settable in restricted mode");
		return -1;
	}
	if (strcmp(name, "func_code") == 0) {
		if (value == NULL || !PyCode_Check(value)) {
			PyErr_SetString(
				PyExc_TypeError,
				"func_code must be set to a code object");
			return -1;
		}
	}
	else if (strcmp(name, "func_defaults") == 0) {
		if (value != Py_None && 
		    (value == NULL || !PyTuple_Check(value))) {
			PyErr_SetString(
				PyExc_TypeError,
				"func_defaults must be set to a tuple object");
			return -1;
		}
		if (value == Py_None)
			value = NULL;
	}
	return PyMember_Set((char *)op, func_memberlist, name, value);
}

static void
func_dealloc(PyFunctionObject *op)
{
	PyObject_GC_Fini(op);
	Py_DECREF(op->func_code);
	Py_DECREF(op->func_globals);
	Py_DECREF(op->func_name);
	Py_XDECREF(op->func_defaults);
	Py_XDECREF(op->func_doc);
	op = (PyFunctionObject *) PyObject_AS_GC(op);
	PyObject_DEL(op);
}

static PyObject*
func_repr(PyFunctionObject *op)
{
	char buf[140];
	if (op->func_name == Py_None)
		sprintf(buf, "<anonymous function at %p>", op);
	else
		sprintf(buf, "<function %.100s at %p>",
			PyString_AsString(op->func_name),
			op);
	return PyString_FromString(buf);
}

static int
func_compare(PyFunctionObject *f, PyFunctionObject *g)
{
	int c;
	if (f->func_globals != g->func_globals)
		return (f->func_globals < g->func_globals) ? -1 : 1;
	if (f->func_defaults != g->func_defaults) {
		if (f->func_defaults == NULL)
			return -1;
		if (g->func_defaults == NULL)
			return 1;
		c = PyObject_Compare(f->func_defaults, g->func_defaults);
		if (c != 0)
			return c;
	}
	return PyObject_Compare(f->func_code, g->func_code);
}

static long
func_hash(PyFunctionObject *f)
{
	long h,x;
	h = PyObject_Hash(f->func_code);
	if (h == -1) return h;
	x = _Py_HashPointer(f->func_globals);
	if (x == -1) return x;
	h ^= x;
	if (h == -1) h = -2;
	return h;
}

static int
func_traverse(PyFunctionObject *f, visitproc visit, void *arg)
{
	int err;
	if (f->func_code) {
		err = visit(f->func_code, arg);
		if (err)
			return err;
	}
	if (f->func_globals) {
		err = visit(f->func_globals, arg);
		if (err)
			return err;
	}
	if (f->func_defaults) {
		err = visit(f->func_defaults, arg);
		if (err)
			return err;
	}
	if (f->func_doc) {
		err = visit(f->func_doc, arg);
		if (err)
			return err;
	}
	if (f->func_name) {
		err = visit(f->func_name, arg);
		if (err)
			return err;
	}
	return 0;
}

PyTypeObject PyFunction_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"function",
	sizeof(PyFunctionObject) + PyGC_HEAD_SIZE,
	0,
	(destructor)func_dealloc, /*tp_dealloc*/
	0,		/*tp_print*/
	(getattrfunc)func_getattr, /*tp_getattr*/
	(setattrfunc)func_setattr, /*tp_setattr*/
	(cmpfunc)func_compare, /*tp_compare*/
	(reprfunc)func_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)func_hash, /*tp_hash*/
	0,		/*tp_call*/
	0,		/*tp_str*/
	0,		/*tp_getattro*/
	0,		/*tp_setattro*/
	0,		/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_GC, /*tp_flags*/
	0,		/* tp_doc */
	(traverseproc)func_traverse,	/* tp_traverse */
};
