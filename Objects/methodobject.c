
/* Method object implementation */

#include "Python.h"

#include "token.h"

static PyCFunctionObject *free_list = NULL;

PyObject *
PyCFunction_New(PyMethodDef *ml, PyObject *self)
{
	PyCFunctionObject *op;
	op = free_list;
	if (op != NULL) {
		free_list = (PyCFunctionObject *)(op->m_self);
		PyObject_INIT(op, &PyCFunction_Type);
	}
	else {
		op = PyObject_NEW(PyCFunctionObject, &PyCFunction_Type);
		if (op == NULL)
			return NULL;
	}
	op->m_ml = ml;
	Py_XINCREF(self);
	op->m_self = self;
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

/* Methods (the standard built-in methods, that is) */

static void
meth_dealloc(PyCFunctionObject *m)
{
	Py_XDECREF(m->m_self);
	m->m_self = (PyObject *)free_list;
	free_list = m;
}

static PyObject *
meth_getattr(PyCFunctionObject *m, char *name)
{
	if (strcmp(name, "__name__") == 0) {
		return PyString_FromString(m->m_ml->ml_name);
	}
	if (strcmp(name, "__doc__") == 0) {
		char *doc = m->m_ml->ml_doc;
		if (doc != NULL)
			return PyString_FromString(doc);
		Py_INCREF(Py_None);
		return Py_None;
	}
	if (strcmp(name, "__self__") == 0) {
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
	if (strcmp(name, "__members__") == 0) {
		return Py_BuildValue("[sss]",
				     "__doc__", "__name__", "__self__");
	}
	PyErr_SetString(PyExc_AttributeError, name);
	return NULL;
}

static PyObject *
meth_repr(PyCFunctionObject *m)
{
	char buf[200];
	if (m->m_self == NULL)
		sprintf(buf, "<built-in function %.80s>", m->m_ml->ml_name);
	else
		sprintf(buf,
			"<built-in method %.80s of %.80s object at %p>",
			m->m_ml->ml_name, m->m_self->ob_type->tp_name,
			m->m_self);
	return PyString_FromString(buf);
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
	(destructor)meth_dealloc, /*tp_dealloc*/
	0,		/*tp_print*/
	(getattrfunc)meth_getattr, /*tp_getattr*/
	0,		/*tp_setattr*/
	(cmpfunc)meth_compare, /*tp_compare*/
	(reprfunc)meth_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)meth_hash, /*tp_hash*/
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
		PyObject_DEL(v);
	}
}
