
/* Method object implementation */

#include "Python.h"
#include "structmember.h"

static PyCFunctionObject *free_list = NULL;

PyObject *
PyCFunction_NewEx(PyMethodDef *ml, PyObject *self, PyObject *module)
{
	PyCFunctionObject *op;
	op = free_list;
	if (op != NULL) {
		free_list = (PyCFunctionObject *)(op->m_self);
		PyObject_INIT(op, &PyCFunction_Type);
	}
	else {
		op = PyObject_GC_New(PyCFunctionObject, &PyCFunction_Type);
		if (op == NULL)
			return NULL;
	}
	op->m_ml = ml;
	Py_XINCREF(self);
	op->m_self = self;
	Py_XINCREF(module);
	op->m_module = module;
	_PyObject_GC_TRACK(op);
	return (PyObject *)op;
}

PyCFunction
PyCFunction_GetFunction(PyObject *op)
{
	if (!PyCFunction_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyCFunctionObject *)op) -> m_ml -> ml_meth;
}

PyObject *
PyCFunction_GetSelf(PyObject *op)
{
	if (!PyCFunction_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyCFunctionObject *)op) -> m_self;
}

int
PyCFunction_GetFlags(PyObject *op)
{
	if (!PyCFunction_Check(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return ((PyCFunctionObject *)op) -> m_ml -> ml_flags;
}

PyObject *
PyCFunction_Call(PyObject *func, PyObject *arg, PyObject *kw)
{
	PyCFunctionObject* f = (PyCFunctionObject*)func;
	PyCFunction meth = PyCFunction_GET_FUNCTION(func);
	PyObject *self = PyCFunction_GET_SELF(func);
	int size;

	switch (PyCFunction_GET_FLAGS(func) & ~(METH_CLASS | METH_STATIC | METH_COEXIST)) {
	case METH_VARARGS:
		if (kw == NULL || PyDict_Size(kw) == 0)
			return (*meth)(self, arg);
		break;
	case METH_VARARGS | METH_KEYWORDS:
	case METH_OLDARGS | METH_KEYWORDS:
		return (*(PyCFunctionWithKeywords)meth)(self, arg, kw);
	case METH_NOARGS:
		if (kw == NULL || PyDict_Size(kw) == 0) {
			size = PyTuple_GET_SIZE(arg);
			if (size == 0)
				return (*meth)(self, NULL);
			PyErr_Format(PyExc_TypeError,
			    "%.200s() takes no arguments (%d given)",
			    f->m_ml->ml_name, size);
			return NULL;
		}
		break;
	case METH_O:
		if (kw == NULL || PyDict_Size(kw) == 0) {
			size = PyTuple_GET_SIZE(arg);
			if (size == 1)
				return (*meth)(self, PyTuple_GET_ITEM(arg, 0));
			PyErr_Format(PyExc_TypeError,
			    "%.200s() takes exactly one argument (%d given)",
			    f->m_ml->ml_name, size);
			return NULL;
		}
		break;
	case METH_OLDARGS:
		/* the really old style */
		if (kw == NULL || PyDict_Size(kw) == 0) {
			size = PyTuple_GET_SIZE(arg);
			if (size == 1)
				arg = PyTuple_GET_ITEM(arg, 0);
			else if (size == 0)
				arg = NULL;
			return (*meth)(self, arg);
		}
		break;
	default:
		PyErr_BadInternalCall();
		return NULL;
	}
	PyErr_Format(PyExc_TypeError, "%.200s() takes no keyword arguments",
		     f->m_ml->ml_name);
	return NULL;
}

/* Methods (the standard built-in methods, that is) */

static void
meth_dealloc(PyCFunctionObject *m)
{
	_PyObject_GC_UNTRACK(m);
	Py_XDECREF(m->m_self);
	Py_XDECREF(m->m_module);
	m->m_self = (PyObject *)free_list;
	free_list = m;
}

static PyObject *
meth_get__doc__(PyCFunctionObject *m, void *closure)
{
	char *doc = m->m_ml->ml_doc;

	if (doc != NULL)
		return PyString_FromString(doc);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
meth_get__name__(PyCFunctionObject *m, void *closure)
{
	return PyString_FromString(m->m_ml->ml_name);
}

static int
meth_traverse(PyCFunctionObject *m, visitproc visit, void *arg)
{
	int err;
	if (m->m_self != NULL) {
		err = visit(m->m_self, arg);
		if (err)
			return err;
	}
	if (m->m_module != NULL) {
		err = visit(m->m_module, arg);
		if (err)
			return err;
	}
	return 0;
}

static PyObject *
meth_get__self__(PyCFunctionObject *m, void *closure)
{
	PyObject *self;
	if (PyEval_GetRestricted()) {
		PyErr_SetString(PyExc_RuntimeError,
			"method.__self__ not accessible in restricted mode");
		return NULL;
	}
	self = m->m_self;
	if (self == NULL)
		self = Py_None;
	Py_INCREF(self);
	return self;
}

static PyGetSetDef meth_getsets [] = {
	{"__doc__",  (getter)meth_get__doc__,  NULL, NULL},
	{"__name__", (getter)meth_get__name__, NULL, NULL},
	{"__self__", (getter)meth_get__self__, NULL, NULL},
	{0}
};

#define OFF(x) offsetof(PyCFunctionObject, x)

static PyMemberDef meth_members[] = {
	{"__module__",    T_OBJECT,     OFF(m_module), WRITE_RESTRICTED},
	{NULL}
};

static PyObject *
meth_repr(PyCFunctionObject *m)
{
	if (m->m_self == NULL)
		return PyString_FromFormat("<built-in function %s>",
					   m->m_ml->ml_name);
	return PyString_FromFormat("<built-in method %s of %s object at %p>",
				   m->m_ml->ml_name,
				   m->m_self->ob_type->tp_name,
				   m->m_self);
}

static int
meth_compare(PyCFunctionObject *a, PyCFunctionObject *b)
{
	if (a->m_self != b->m_self)
		return (a->m_self < b->m_self) ? -1 : 1;
	if (a->m_ml->ml_meth == b->m_ml->ml_meth)
		return 0;
	if (strcmp(a->m_ml->ml_name, b->m_ml->ml_name) < 0)
		return -1;
	else
		return 1;
}

static long
meth_hash(PyCFunctionObject *a)
{
	long x,y;
	if (a->m_self == NULL)
		x = 0;
	else {
		x = PyObject_Hash(a->m_self);
		if (x == -1)
			return -1;
	}
	y = _Py_HashPointer((void*)(a->m_ml->ml_meth));
	if (y == -1)
		return -1;
	x ^= y;
	if (x == -1)
		x = -2;
	return x;
}


PyTypeObject PyCFunction_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"builtin_function_or_method",
	sizeof(PyCFunctionObject),
	0,
	(destructor)meth_dealloc, 		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	(cmpfunc)meth_compare,			/* tp_compare */
	(reprfunc)meth_repr,			/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	(hashfunc)meth_hash,			/* tp_hash */
	PyCFunction_Call,			/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
 	0,					/* tp_doc */
 	(traverseproc)meth_traverse,		/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	meth_members,				/* tp_members */
	meth_getsets,				/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
};

/* List all methods in a chain -- helper for findmethodinchain */

static PyObject *
listmethodchain(PyMethodChain *chain)
{
	PyMethodChain *c;
	PyMethodDef *ml;
	int i, n;
	PyObject *v;

	n = 0;
	for (c = chain; c != NULL; c = c->link) {
		for (ml = c->methods; ml->ml_name != NULL; ml++)
			n++;
	}
	v = PyList_New(n);
	if (v == NULL)
		return NULL;
	i = 0;
	for (c = chain; c != NULL; c = c->link) {
		for (ml = c->methods; ml->ml_name != NULL; ml++) {
			PyList_SetItem(v, i, PyString_FromString(ml->ml_name));
			i++;
		}
	}
	if (PyErr_Occurred()) {
		Py_DECREF(v);
		return NULL;
	}
	PyList_Sort(v);
	return v;
}

/* Find a method in a method chain */

PyObject *
Py_FindMethodInChain(PyMethodChain *chain, PyObject *self, char *name)
{
	if (name[0] == '_' && name[1] == '_') {
		if (strcmp(name, "__methods__") == 0)
			return listmethodchain(chain);
		if (strcmp(name, "__doc__") == 0) {
			char *doc = self->ob_type->tp_doc;
			if (doc != NULL)
				return PyString_FromString(doc);
		}
	}
	while (chain != NULL) {
		PyMethodDef *ml = chain->methods;
		for (; ml->ml_name != NULL; ml++) {
			if (name[0] == ml->ml_name[0] &&
			    strcmp(name+1, ml->ml_name+1) == 0)
				/* XXX */
				return PyCFunction_New(ml, self);
		}
		chain = chain->link;
	}
	PyErr_SetString(PyExc_AttributeError, name);
	return NULL;
}

/* Find a method in a single method list */

PyObject *
Py_FindMethod(PyMethodDef *methods, PyObject *self, char *name)
{
	PyMethodChain chain;
	chain.methods = methods;
	chain.link = NULL;
	return Py_FindMethodInChain(&chain, self, name);
}

/* Clear out the free list */

void
PyCFunction_Fini(void)
{
	while (free_list) {
		PyCFunctionObject *v = free_list;
		free_list = (PyCFunctionObject *)(v->m_self);
		PyObject_GC_Del(v);
	}
}

/* PyCFunction_New() is now just a macro that calls PyCFunction_NewEx(),
   but it's part of the API so we need to keep a function around that
   existing C extensions can call.
*/
   
#undef PyCFunction_New
PyAPI_FUNC(PyObject *) PyCFunction_New(PyMethodDef *, PyObject *);

PyObject *
PyCFunction_New(PyMethodDef *ml, PyObject *self)
{
	return PyCFunction_NewEx(ml, self, NULL);
}
