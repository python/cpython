
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
		op->func_weakreflist = NULL;
		Py_INCREF(code);
		op->func_code = code;
		Py_INCREF(globals);
		op->func_globals = globals;
		op->func_name = ((PyCodeObject *)code)->co_name;
		Py_INCREF(op->func_name);
		op->func_defaults = NULL; /* No default arguments */
		op->func_closure = NULL;
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
		op->func_dict = NULL;
	}
	else
		return NULL;
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

PyObject *
PyFunction_GetClosure(PyObject *op)
{
	if (!PyFunction_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyFunctionObject *) op) -> func_closure;
}

int
PyFunction_SetClosure(PyObject *op, PyObject *closure)
{
	if (!PyFunction_Check(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	if (closure == Py_None)
		closure = NULL;
	else if (PyTuple_Check(closure)) {
		Py_XINCREF(closure);
	}
	else {
		PyErr_SetString(PyExc_SystemError, "non-tuple closure");
		return -1;
	}
	Py_XDECREF(((PyFunctionObject *) op) -> func_closure);
	((PyFunctionObject *) op) -> func_closure = closure;
	return 0;
}

/* Methods */

#define OFF(x) offsetof(PyFunctionObject, x)

static struct memberlist func_memberlist[] = {
        {"func_code",     T_OBJECT,     OFF(func_code)},
        {"func_globals",  T_OBJECT,     OFF(func_globals),      READONLY},
        {"func_name",     T_OBJECT,     OFF(func_name),         READONLY},
        {"__name__",      T_OBJECT,     OFF(func_name),         READONLY},
        {"func_closure",  T_OBJECT,     OFF(func_closure),      READONLY},
        {"func_defaults", T_OBJECT,     OFF(func_defaults)},
        {"func_doc",      T_OBJECT,     OFF(func_doc)},
        {"__doc__",       T_OBJECT,     OFF(func_doc)},
        {"func_dict",     T_OBJECT,     OFF(func_dict)},
        {"__dict__",      T_OBJECT,     OFF(func_dict)},
        {NULL}  /* Sentinel */
};

static PyObject *
func_getattro(PyFunctionObject *op, PyObject *name)
{
	PyObject *rtn;
	char *sname = PyString_AsString(name);
	
	if (sname[0] != '_' && PyEval_GetRestricted()) {
		PyErr_SetString(PyExc_RuntimeError,
		  "function attributes not accessible in restricted mode");
		return NULL;
	}

	/* no API for PyMember_HasAttr() */
	rtn = PyMember_Get((char *)op, func_memberlist, sname);

	if (rtn == NULL && PyErr_ExceptionMatches(PyExc_AttributeError)) {
		PyErr_Clear();
		if (op->func_dict != NULL) {
			rtn = PyDict_GetItem(op->func_dict, name);
			Py_XINCREF(rtn);
		}
		if (rtn == NULL)
			PyErr_SetObject(PyExc_AttributeError, name);
	}
	return rtn;
}

static int
func_setattro(PyFunctionObject *op, PyObject *name, PyObject *value)
{
	int rtn;
	char *sname = PyString_AsString(name);

	if (PyEval_GetRestricted()) {
		PyErr_SetString(PyExc_RuntimeError,
		  "function attributes not settable in restricted mode");
		return -1;
	}
	if (strcmp(sname, "func_code") == 0) {
		/* not legal to del f.func_code or to set it to anything
		 * other than a code object.
		 */
		if (value == NULL || !PyCode_Check(value)) {
			PyErr_SetString(
				PyExc_TypeError,
				"func_code must be set to a code object");
			return -1;
		}
	}
	else if (strcmp(sname, "func_defaults") == 0) {
		/* legal to del f.func_defaults.  Can only set
		 * func_defaults to NULL or a tuple.
		 */
		if (value == Py_None)
			value = NULL;
		if (value != NULL && !PyTuple_Check(value)) {
			PyErr_SetString(
				PyExc_TypeError,
				"func_defaults must be set to a tuple object");
			return -1;
		}
	}
	else if (!strcmp(sname, "func_dict") || !strcmp(sname, "__dict__")) {
		/* legal to del f.func_dict.  Can only set func_dict to
		 * NULL or a dictionary.
		 */
		if (value == Py_None)
			value = NULL;
		if (value != NULL && !PyDict_Check(value)) {
			PyErr_SetString(
				PyExc_TypeError,
				"func_dict must be set to a dict object");
			return -1;
		}
	}

	rtn = PyMember_Set((char *)op, func_memberlist, sname, value);
	if (rtn < 0 && PyErr_ExceptionMatches(PyExc_AttributeError)) {
		PyErr_Clear();
		if (op->func_dict == NULL) {
			/* don't create the dict if we're deleting an
			 * attribute.  In that case, we know we'll get an
			 * AttributeError.
			 */
			if (value == NULL) {
				PyErr_SetString(PyExc_AttributeError, sname);
				return -1;
			}
			op->func_dict = PyDict_New();
			if (op->func_dict == NULL)
				return -1;
		}
                if (value == NULL)
			rtn = PyDict_DelItem(op->func_dict, name);
                else
			rtn = PyDict_SetItem(op->func_dict, name, value);
		/* transform KeyError into AttributeError */
		if (rtn < 0 && PyErr_ExceptionMatches(PyExc_KeyError))
			PyErr_SetString(PyExc_AttributeError, sname);
	}
	return rtn;
}

static void
func_dealloc(PyFunctionObject *op)
{
	PyObject_ClearWeakRefs((PyObject *) op);
	PyObject_GC_Fini(op);
	Py_DECREF(op->func_code);
	Py_DECREF(op->func_globals);
	Py_DECREF(op->func_name);
	Py_XDECREF(op->func_defaults);
	Py_XDECREF(op->func_doc);
	Py_XDECREF(op->func_dict);
	Py_XDECREF(op->func_closure);
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
	if (f->func_dict) {
		err = visit(f->func_dict, arg);
		if (err)
			return err;
	}
	if (f->func_closure) {
		err = visit(f->func_closure, arg);
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
	0, /*tp_getattr*/
	0, /*tp_setattr*/
	0, /*tp_compare*/
	(reprfunc)func_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	0,		/*tp_hash*/
	0,		/*tp_call*/
	0,		/*tp_str*/
	(getattrofunc)func_getattro,	     /*tp_getattro*/
	(setattrofunc)func_setattro,	     /*tp_setattro*/
	0,		/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_GC | Py_TPFLAGS_HAVE_WEAKREFS,
	0,		/* tp_doc */
	(traverseproc)func_traverse,	/* tp_traverse */
	0,		/* tp_clear */
	0,		/* tp_richcompare */
	offsetof(PyFunctionObject, func_weakreflist), /* tp_weaklistoffset */
};
