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

/* Method object implementation */

#include "Python.h"

#include "token.h"

static PyCFunctionObject *free_list = NULL;

PyObject *
PyCFunction_New(ml, self)
	PyMethodDef *ml;
	PyObject *self;
{
	PyCFunctionObject *op;
	op = free_list;
	if (op != NULL) {
		free_list = (PyCFunctionObject *)(op->m_self);
		op->ob_type = &PyCFunction_Type;
		_Py_NewReference(op);
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
PyCFunction_GetFunction(op)
	PyObject *op;
{
	if (!PyCFunction_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyCFunctionObject *)op) -> m_ml -> ml_meth;
}

PyObject *
PyCFunction_GetSelf(op)
	PyObject *op;
{
	if (!PyCFunction_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyCFunctionObject *)op) -> m_self;
}

int
PyCFunction_GetFlags(op)
	PyObject *op;
{
	if (!PyCFunction_Check(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return ((PyCFunctionObject *)op) -> m_ml -> ml_flags;
}

/* Methods (the standard built-in methods, that is) */

static void
meth_dealloc(m)
	PyCFunctionObject *m;
{
	Py_XDECREF(m->m_self);
	m->m_self = (PyObject *)free_list;
	free_list = m;
}

static PyObject *
meth_getattr(m, name)
	PyCFunctionObject *m;
	char *name;
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
meth_repr(m)
	PyCFunctionObject *m;
{
	char buf[200];
	if (m->m_self == NULL)
		sprintf(buf, "<built-in function %.80s>", m->m_ml->ml_name);
	else
		sprintf(buf,
			"<built-in method %.80s of %.80s object at %lx>",
			m->m_ml->ml_name, m->m_self->ob_type->tp_name,
			(long)m->m_self);
	return PyString_FromString(buf);
}

static int
meth_compare(a, b)
	PyCFunctionObject *a, *b;
{
	if (a->m_self != b->m_self)
		return PyObject_Compare(a->m_self, b->m_self);
	if (a->m_ml->ml_meth == b->m_ml->ml_meth)
		return 0;
	if (strcmp(a->m_ml->ml_name, b->m_ml->ml_name) < 0)
		return -1;
	else
		return 1;
}

static long
meth_hash(a)
	PyCFunctionObject *a;
{
	long x;
	if (a->m_self == NULL)
		x = 0;
	else {
		x = PyObject_Hash(a->m_self);
		if (x == -1)
			return -1;
	}
	return x ^ (long) a->m_ml->ml_meth;
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
listmethodchain(chain)
	PyMethodChain *chain;
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
Py_FindMethodInChain(chain, self, name)
	PyMethodChain *chain;
	PyObject *self;
	char *name;
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
Py_FindMethod(methods, self, name)
	PyMethodDef *methods;
	PyObject *self;
	char *name;
{
	PyMethodChain chain;
	chain.methods = methods;
	chain.link = NULL;
	return Py_FindMethodInChain(&chain, self, name);
}

/* Clear out the free list */

void
PyCFunction_Fini()
{
	while (free_list) {
		PyCFunctionObject *v = free_list;
		free_list = (PyCFunctionObject *)(v->m_self);
		PyMem_DEL(v);
	}
}
