
/* Function object implementation */

#include "Python.h"
#include "compile.h"
#include "eval.h"
#include "structmember.h"

PyObject *
PyFunction_New(PyObject *code, PyObject *globals)
{
	PyFunctionObject *op = PyObject_GC_New(PyFunctionObject,
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
	_PyObject_GC_TRACK(op);
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

#define RR ()

static PyMemberDef func_memberlist[] = {
        {"func_closure",  T_OBJECT,     OFF(func_closure),
	 RESTRICTED|READONLY},
        {"func_doc",      T_OBJECT,     OFF(func_doc), WRITE_RESTRICTED},
        {"__doc__",       T_OBJECT,     OFF(func_doc), WRITE_RESTRICTED},
        {"func_globals",  T_OBJECT,     OFF(func_globals),
	 RESTRICTED|READONLY},
        {"func_name",     T_OBJECT,     OFF(func_name),         READONLY},
        {"__name__",      T_OBJECT,     OFF(func_name),         READONLY},
        {NULL}  /* Sentinel */
};

static int
restricted(void)
{
	if (!PyEval_GetRestricted())
		return 0;
	PyErr_SetString(PyExc_RuntimeError,
		"function attributes not accessible in restricted mode");
	return 1;
}

static PyObject *
func_get_dict(PyFunctionObject *op)
{
	if (restricted())
		return NULL;
	if (op->func_dict == NULL) {
		op->func_dict = PyDict_New();
		if (op->func_dict == NULL)
			return NULL;
	}
	Py_INCREF(op->func_dict);
	return op->func_dict;
}

static int
func_set_dict(PyFunctionObject *op, PyObject *value)
{
	PyObject *tmp;

	if (restricted())
		return -1;
	/* It is illegal to del f.func_dict */
	if (value == NULL) {
		PyErr_SetString(PyExc_TypeError,
				"function's dictionary may not be deleted");
		return -1;
	}
	/* Can only set func_dict to a dictionary */
	if (!PyDict_Check(value)) {
		PyErr_SetString(PyExc_TypeError,
				"setting function's dictionary to a non-dict");
		return -1;
	}
	tmp = op->func_dict;
	Py_INCREF(value);
	op->func_dict = value;
	Py_XDECREF(tmp);
	return 0;
}

static PyObject *
func_get_code(PyFunctionObject *op)
{
	if (restricted())
		return NULL;
	Py_INCREF(op->func_code);
	return op->func_code;
}

static int
func_set_code(PyFunctionObject *op, PyObject *value)
{
	PyObject *tmp;

	if (restricted())
		return -1;
	/* Not legal to del f.func_code or to set it to anything
	 * other than a code object. */
	if (value == NULL || !PyCode_Check(value)) {
		PyErr_SetString(PyExc_TypeError,
				"func_code must be set to a code object");
		return -1;
	}
	tmp = op->func_code;
	Py_INCREF(value);
	op->func_code = value;
	Py_DECREF(tmp);
	return 0;
}

static PyObject *
func_get_defaults(PyFunctionObject *op)
{
	if (restricted())
		return NULL;
	if (op->func_defaults == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	Py_INCREF(op->func_defaults);
	return op->func_defaults;
}

static int
func_set_defaults(PyFunctionObject *op, PyObject *value)
{
	PyObject *tmp;

	if (restricted())
		return -1;
	/* Legal to del f.func_defaults.
	 * Can only set func_defaults to NULL or a tuple. */
	if (value == Py_None)
		value = NULL;
	if (value != NULL && !PyTuple_Check(value)) {
		PyErr_SetString(PyExc_TypeError,
				"func_defaults must be set to a tuple object");
		return -1;
	}
	tmp = op->func_defaults;
	Py_XINCREF(value);
	op->func_defaults = value;
	Py_XDECREF(tmp);
	return 0;
}

static PyGetSetDef func_getsetlist[] = {
        {"func_code", (getter)func_get_code, (setter)func_set_code},
        {"func_defaults", (getter)func_get_defaults,
	 (setter)func_set_defaults},
	{"func_dict", (getter)func_get_dict, (setter)func_set_dict},
	{"__dict__", (getter)func_get_dict, (setter)func_set_dict},
	{NULL} /* Sentinel */
};

static void
func_dealloc(PyFunctionObject *op)
{
	_PyObject_GC_UNTRACK(op);
	if (op->func_weakreflist != NULL)
		PyObject_ClearWeakRefs((PyObject *) op);
	Py_DECREF(op->func_code);
	Py_DECREF(op->func_globals);
	Py_DECREF(op->func_name);
	Py_XDECREF(op->func_defaults);
	Py_XDECREF(op->func_doc);
	Py_XDECREF(op->func_dict);
	Py_XDECREF(op->func_closure);
	PyObject_GC_Del(op);
}

static PyObject*
func_repr(PyFunctionObject *op)
{
	if (op->func_name == Py_None)
		return PyString_FromFormat("<anonymous function at %p>", op);
	return PyString_FromFormat("<function %s at %p>",
				   PyString_AsString(op->func_name),
				   op);
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

static PyObject *
function_call(PyObject *func, PyObject *arg, PyObject *kw)
{
	PyObject *result;
	PyObject *argdefs;
	PyObject **d, **k;
	int nk, nd;

	argdefs = PyFunction_GET_DEFAULTS(func);
	if (argdefs != NULL && PyTuple_Check(argdefs)) {
		d = &PyTuple_GET_ITEM((PyTupleObject *)argdefs, 0);
		nd = PyTuple_Size(argdefs);
	}
	else {
		d = NULL;
		nd = 0;
	}

	if (kw != NULL && PyDict_Check(kw)) {
		int pos, i;
		nk = PyDict_Size(kw);
		k = PyMem_NEW(PyObject *, 2*nk);
		if (k == NULL) {
			PyErr_NoMemory();
			return NULL;
		}
		pos = i = 0;
		while (PyDict_Next(kw, &pos, &k[i], &k[i+1]))
			i += 2;
		nk = i/2;
		/* XXX This is broken if the caller deletes dict items! */
	}
	else {
		k = NULL;
		nk = 0;
	}

	result = PyEval_EvalCodeEx(
		(PyCodeObject *)PyFunction_GET_CODE(func),
		PyFunction_GET_GLOBALS(func), (PyObject *)NULL,
		&PyTuple_GET_ITEM(arg, 0), PyTuple_Size(arg),
		k, nk, d, nd,
		PyFunction_GET_CLOSURE(func));

	if (k != NULL)
		PyMem_DEL(k);

	return result;
}

/* Bind a function to an object */
static PyObject *
func_descr_get(PyObject *func, PyObject *obj, PyObject *type)
{
	if (obj == Py_None)
		obj = NULL;
	return PyMethod_New(func, obj, type);
}

PyTypeObject PyFunction_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"function",
	sizeof(PyFunctionObject),
	0,
	(destructor)func_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)func_repr,			/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	function_call,				/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	PyObject_GenericSetAttr,		/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
	0,					/* tp_doc */
	(traverseproc)func_traverse,		/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	offsetof(PyFunctionObject, func_weakreflist), /* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	func_memberlist,			/* tp_members */
	func_getsetlist,			/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	func_descr_get,				/* tp_descr_get */
	0,					/* tp_descr_set */
	offsetof(PyFunctionObject, func_dict),	/* tp_dictoffset */
};


/* Class method object */

/* A class method receives the class as implicit first argument,
   just like an instance method receives the instance.
   To declare a class method, use this idiom:

     class C:
         def f(cls, arg1, arg2, ...): ...
	 f = classmethod(f)
   
   It can be called either on the class (e.g. C.f()) or on an instance
   (e.g. C().f()); the instance is ignored except for its class.
   If a class method is called for a derived class, the derived class
   object is passed as the implied first argument.

   Class methods are different than C++ or Java static methods.
   If you want those, see static methods below.
*/

typedef struct {
	PyObject_HEAD
	PyObject *cm_callable;
} classmethod;

static void
cm_dealloc(classmethod *cm)
{
	Py_XDECREF(cm->cm_callable);
	cm->ob_type->tp_free((PyObject *)cm);
}

static PyObject *
cm_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
	classmethod *cm = (classmethod *)self;

	if (cm->cm_callable == NULL) {
		PyErr_SetString(PyExc_RuntimeError,
				"uninitialized classmethod object");
		return NULL;
	}
	if (type == NULL)
		type = (PyObject *)(obj->ob_type);
 	return PyMethod_New(cm->cm_callable,
			    type, (PyObject *)(type->ob_type));
}

static int
cm_init(PyObject *self, PyObject *args, PyObject *kwds)
{
	classmethod *cm = (classmethod *)self;
	PyObject *callable;

	if (!PyArg_ParseTuple(args, "O:classmethod", &callable))
		return -1;
	Py_INCREF(callable);
	cm->cm_callable = callable;
	return 0;
}

static char classmethod_doc[] =
"classmethod(function) -> method\n\
\n\
Convert a function to be a class method.\n\
\n\
A class method receives the class as implicit first argument,\n\
just like an instance method receives the instance.\n\
To declare a class method, use this idiom:\n\
\n\
  class C:\n\
      def f(cls, arg1, arg2, ...): ...\n\
      f = classmethod(f)\n\
\n\
It can be called either on the class (e.g. C.f()) or on an instance\n\
(e.g. C().f()).  The instance is ignored except for its class.\n\
If a class method is called for a derived class, the derived class\n\
object is passed as the implied first argument.\n\
\n\
Class methods are different than C++ or Java static methods.\n\
If you want those, see the staticmethod builtin.";

PyTypeObject PyClassMethod_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"classmethod",
	sizeof(classmethod),
	0,
	(destructor)cm_dealloc,			/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	classmethod_doc,			/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	cm_descr_get,				/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	cm_init,				/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	PyType_GenericNew,			/* tp_new */
	_PyObject_Del,				/* tp_free */
};

PyObject *
PyClassMethod_New(PyObject *callable)
{
	classmethod *cm = (classmethod *)
		PyType_GenericAlloc(&PyClassMethod_Type, 0);
	if (cm != NULL) {
		Py_INCREF(callable);
		cm->cm_callable = callable;
	}
	return (PyObject *)cm;
}


/* Static method object */

/* A static method does not receive an implicit first argument.
   To declare a static method, use this idiom:

     class C:
         def f(arg1, arg2, ...): ...
	 f = staticmethod(f)

   It can be called either on the class (e.g. C.f()) or on an instance
   (e.g. C().f()); the instance is ignored except for its class.

   Static methods in Python are similar to those found in Java or C++.
   For a more advanced concept, see class methods above.
*/

typedef struct {
	PyObject_HEAD
	PyObject *sm_callable;
} staticmethod;

static void
sm_dealloc(staticmethod *sm)
{
	Py_XDECREF(sm->sm_callable);
	sm->ob_type->tp_free((PyObject *)sm);
}

static PyObject *
sm_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
	staticmethod *sm = (staticmethod *)self;

	if (sm->sm_callable == NULL) {
		PyErr_SetString(PyExc_RuntimeError,
				"uninitialized staticmethod object");
		return NULL;
	}
	Py_INCREF(sm->sm_callable);
	return sm->sm_callable;
}

static int
sm_init(PyObject *self, PyObject *args, PyObject *kwds)
{
	staticmethod *sm = (staticmethod *)self;
	PyObject *callable;

	if (!PyArg_ParseTuple(args, "O:staticmethod", &callable))
		return -1;
	Py_INCREF(callable);
	sm->sm_callable = callable;
	return 0;
}

static char staticmethod_doc[] =
"staticmethod(function) -> method\n\
\n\
Convert a function to be a static method.\n\
\n\
A static method does not receive an implicit first argument.\n\
To declare a static method, use this idiom:\n\
\n\
     class C:\n\
         def f(arg1, arg2, ...): ...\n\
	 f = staticmethod(f)\n\
\n\
It can be called either on the class (e.g. C.f()) or on an instance\n\
(e.g. C().f()).  The instance is ignored except for its class.\n\
\n\
Static methods in Python are similar to those found in Java or C++.\n\
For a more advanced concept, see the classmethod builtin.";

PyTypeObject PyStaticMethod_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"staticmethod",
	sizeof(staticmethod),
	0,
	(destructor)sm_dealloc,			/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	staticmethod_doc,			/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	sm_descr_get,				/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	sm_init,				/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	PyType_GenericNew,			/* tp_new */
	_PyObject_Del,				/* tp_free */
};

PyObject *
PyStaticMethod_New(PyObject *callable)
{
	staticmethod *sm = (staticmethod *)
		PyType_GenericAlloc(&PyStaticMethod_Type, 0);
	if (sm != NULL) {
		Py_INCREF(callable);
		sm->sm_callable = callable;
	}
	return (PyObject *)sm;
}
