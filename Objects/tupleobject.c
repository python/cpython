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

/* Tuple object implementation */

#include "Python.h"

#ifndef MAXSAVESIZE
#define MAXSAVESIZE	20
#endif

#if MAXSAVESIZE > 0
/* Entries 1 upto MAXSAVESIZE are free lists, entry 0 is the empty
   tuple () of which at most one instance will be allocated.
*/
static PyTupleObject *free_tuples[MAXSAVESIZE];
#endif
#ifdef COUNT_ALLOCS
int fast_tuple_allocs;
int tuple_zero_allocs;
#endif

PyObject *
PyTuple_New(size)
	register int size;
{
	register int i;
	register PyTupleObject *op;
	if (size < 0) {
		PyErr_BadInternalCall();
		return NULL;
	}
#if MAXSAVESIZE > 0
	if (size == 0 && free_tuples[0]) {
		op = free_tuples[0];
		Py_INCREF(op);
#ifdef COUNT_ALLOCS
		tuple_zero_allocs++;
#endif
		return (PyObject *) op;
	}
	if (0 < size && size < MAXSAVESIZE &&
	    (op = free_tuples[size]) != NULL)
	{
		free_tuples[size] = (PyTupleObject *) op->ob_item[0];
#ifdef COUNT_ALLOCS
		fast_tuple_allocs++;
#endif
	} else
#endif
	{
		op = (PyTupleObject *) malloc(
			sizeof(PyTupleObject) + size * sizeof(PyObject *));
		if (op == NULL)
			return PyErr_NoMemory();
	}
	op->ob_type = &PyTuple_Type;
	op->ob_size = size;
	for (i = 0; i < size; i++)
		op->ob_item[i] = NULL;
	_Py_NewReference(op);
#if MAXSAVESIZE > 0
	if (size == 0) {
		free_tuples[0] = op;
		Py_INCREF(op);	/* extra INCREF so that this is never freed */
	}
#endif
	return (PyObject *) op;
}

int
PyTuple_Size(op)
	register PyObject *op;
{
	if (!PyTuple_Check(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	else
		return ((PyTupleObject *)op)->ob_size;
}

PyObject *
PyTuple_GetItem(op, i)
	register PyObject *op;
	register int i;
{
	if (!PyTuple_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	if (i < 0 || i >= ((PyTupleObject *)op) -> ob_size) {
		PyErr_SetString(PyExc_IndexError, "tuple index out of range");
		return NULL;
	}
	return ((PyTupleObject *)op) -> ob_item[i];
}

int
PyTuple_SetItem(op, i, newitem)
	register PyObject *op;
	register int i;
	PyObject *newitem;
{
	register PyObject *olditem;
	register PyObject **p;
	if (!PyTuple_Check(op)) {
		Py_XDECREF(newitem);
		PyErr_BadInternalCall();
		return -1;
	}
	if (i < 0 || i >= ((PyTupleObject *)op) -> ob_size) {
		Py_XDECREF(newitem);
		PyErr_SetString(PyExc_IndexError,
				"tuple assignment index out of range");
		return -1;
	}
	p = ((PyTupleObject *)op) -> ob_item + i;
	olditem = *p;
	*p = newitem;
	Py_XDECREF(olditem);
	return 0;
}

/* Methods */

static void
tupledealloc(op)
	register PyTupleObject *op;
{
	register int i;
	for (i = 0; i < op->ob_size; i++)
		Py_XDECREF(op->ob_item[i]);
#if MAXSAVESIZE > 0
	if (0 < op->ob_size && op->ob_size < MAXSAVESIZE) {
		op->ob_item[0] = (PyObject *) free_tuples[op->ob_size];
		free_tuples[op->ob_size] = op;
	} else
#endif
		free((ANY *)op);
}

static int
tupleprint(op, fp, flags)
	PyTupleObject *op;
	FILE *fp;
	int flags;
{
	int i;
	fprintf(fp, "(");
	for (i = 0; i < op->ob_size; i++) {
		if (i > 0)
			fprintf(fp, ", ");
		if (PyObject_Print(op->ob_item[i], fp, 0) != 0)
			return -1;
	}
	if (op->ob_size == 1)
		fprintf(fp, ",");
	fprintf(fp, ")");
	return 0;
}

static PyObject *
tuplerepr(v)
	PyTupleObject *v;
{
	PyObject *s, *comma;
	int i;
	s = PyString_FromString("(");
	comma = PyString_FromString(", ");
	for (i = 0; i < v->ob_size && s != NULL; i++) {
		if (i > 0)
			PyString_Concat(&s, comma);
		PyString_ConcatAndDel(&s, PyObject_Repr(v->ob_item[i]));
	}
	Py_DECREF(comma);
	if (v->ob_size == 1)
		PyString_ConcatAndDel(&s, PyString_FromString(","));
	PyString_ConcatAndDel(&s, PyString_FromString(")"));
	return s;
}

static int
tuplecompare(v, w)
	register PyTupleObject *v, *w;
{
	register int len =
		(v->ob_size < w->ob_size) ? v->ob_size : w->ob_size;
	register int i;
	for (i = 0; i < len; i++) {
		int cmp = PyObject_Compare(v->ob_item[i], w->ob_item[i]);
		if (cmp != 0)
			return cmp;
	}
	return v->ob_size - w->ob_size;
}

static long
tuplehash(v)
	PyTupleObject *v;
{
	register long x, y;
	register int len = v->ob_size;
	register PyObject **p;
	x = 0x345678L;
	p = v->ob_item;
	while (--len >= 0) {
		y = PyObject_Hash(*p++);
		if (y == -1)
			return -1;
		x = (1000003*x) ^ y;
	}
	x ^= v->ob_size;
	if (x == -1)
		x = -2;
	return x;
}

static int
tuplelength(a)
	PyTupleObject *a;
{
	return a->ob_size;
}

static PyObject *
tupleitem(a, i)
	register PyTupleObject *a;
	register int i;
{
	if (i < 0 || i >= a->ob_size) {
		PyErr_SetString(PyExc_IndexError, "tuple index out of range");
		return NULL;
	}
	Py_INCREF(a->ob_item[i]);
	return a->ob_item[i];
}

static PyObject *
tupleslice(a, ilow, ihigh)
	register PyTupleObject *a;
	register int ilow, ihigh;
{
	register PyTupleObject *np;
	register int i;
	if (ilow < 0)
		ilow = 0;
	if (ihigh > a->ob_size)
		ihigh = a->ob_size;
	if (ihigh < ilow)
		ihigh = ilow;
	if (ilow == 0 && ihigh == a->ob_size) {
		/* XXX can only do this if tuples are immutable! */
		Py_INCREF(a);
		return (PyObject *)a;
	}
	np = (PyTupleObject *)PyTuple_New(ihigh - ilow);
	if (np == NULL)
		return NULL;
	for (i = ilow; i < ihigh; i++) {
		PyObject *v = a->ob_item[i];
		Py_INCREF(v);
		np->ob_item[i - ilow] = v;
	}
	return (PyObject *)np;
}

PyObject *
PyTuple_GetSlice(op, i, j)
	PyObject *op;
	int i, j;
{
	if (op == NULL || !PyTuple_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return tupleslice((PyTupleObject *)op, i, j);
}

static PyObject *
tupleconcat(a, bb)
	register PyTupleObject *a;
	register PyObject *bb;
{
	register int size;
	register int i;
	PyTupleObject *np;
	if (!PyTuple_Check(bb)) {
		PyErr_BadArgument();
		return NULL;
	}
#define b ((PyTupleObject *)bb)
	size = a->ob_size + b->ob_size;
	np = (PyTupleObject *) PyTuple_New(size);
	if (np == NULL) {
		return NULL;
	}
	for (i = 0; i < a->ob_size; i++) {
		PyObject *v = a->ob_item[i];
		Py_INCREF(v);
		np->ob_item[i] = v;
	}
	for (i = 0; i < b->ob_size; i++) {
		PyObject *v = b->ob_item[i];
		Py_INCREF(v);
		np->ob_item[i + a->ob_size] = v;
	}
	return (PyObject *)np;
#undef b
}

static PyObject *
tuplerepeat(a, n)
	PyTupleObject *a;
	int n;
{
	int i, j;
	int size;
	PyTupleObject *np;
	PyObject **p;
	if (n < 0)
		n = 0;
	if (a->ob_size*n == a->ob_size) {
		/* Since tuples are immutable, we can return a shared
		   copy in this case */
		Py_INCREF(a);
		return (PyObject *)a;
	}
	size = a->ob_size * n;
	np = (PyTupleObject *) PyTuple_New(size);
	if (np == NULL)
		return NULL;
	p = np->ob_item;
	for (i = 0; i < n; i++) {
		for (j = 0; j < a->ob_size; j++) {
			*p = a->ob_item[j];
			Py_INCREF(*p);
			p++;
		}
	}
	return (PyObject *) np;
}

static PySequenceMethods tuple_as_sequence = {
	(inquiry)tuplelength, /*sq_length*/
	(binaryfunc)tupleconcat, /*sq_concat*/
	(intargfunc)tuplerepeat, /*sq_repeat*/
	(intargfunc)tupleitem, /*sq_item*/
	(intintargfunc)tupleslice, /*sq_slice*/
	0,		/*sq_ass_item*/
	0,		/*sq_ass_slice*/
};

PyTypeObject PyTuple_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"tuple",
	sizeof(PyTupleObject) - sizeof(PyObject *),
	sizeof(PyObject *),
	(destructor)tupledealloc, /*tp_dealloc*/
	(printfunc)tupleprint, /*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	(cmpfunc)tuplecompare, /*tp_compare*/
	(reprfunc)tuplerepr, /*tp_repr*/
	0,		/*tp_as_number*/
	&tuple_as_sequence,	/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)tuplehash, /*tp_hash*/
};

/* The following function breaks the notion that tuples are immutable:
   it changes the size of a tuple.  We get away with this only if there
   is only one module referencing the object.  You can also think of it
   as creating a new tuple object and destroying the old one, only
   more efficiently.  In any case, don't use this if the tuple may
   already be known to some other part of the code...
   If last_is_sticky is set, the tuple will grow or shrink at the
   front, otherwise it will grow or shrink at the end. */

int
_PyTuple_Resize(pv, newsize, last_is_sticky)
	PyObject **pv;
	int newsize;
	int last_is_sticky;
{
	register PyTupleObject *v;
	register PyTupleObject *sv;
	int i;
	int sizediff;

	v = (PyTupleObject *) *pv;
	if (v == NULL || !PyTuple_Check(v) || v->ob_refcnt != 1) {
		*pv = 0;
		Py_DECREF(v);
		PyErr_BadInternalCall();
		return -1;
	}
	sizediff = newsize - v->ob_size;
	if (sizediff == 0)
		return 0;
	/* XXX UNREF/NEWREF interface should be more symmetrical */
#ifdef Py_REF_DEBUG
	--_Py_RefTotal;
#endif
	_Py_ForgetReference(v);
	if (last_is_sticky && sizediff < 0) {
		/* shrinking:
		   move entries to the front and zero moved entries */
		for (i = 0; i < newsize; i++) {
			Py_XDECREF(v->ob_item[i]);
			v->ob_item[i] = v->ob_item[i - sizediff];
			v->ob_item[i - sizediff] = NULL;
		}
	}
	for (i = newsize; i < v->ob_size; i++) {
		Py_XDECREF(v->ob_item[i]);
		v->ob_item[i] = NULL;
	}
	sv = (PyTupleObject *)
		realloc((char *)v,
			sizeof(PyTupleObject) + newsize * sizeof(PyObject *));
	*pv = (PyObject *) sv;
	if (sv == NULL) {
		PyMem_DEL(v);
		PyErr_NoMemory();
		return -1;
	}
	_Py_NewReference(sv);
	for (i = sv->ob_size; i < newsize; i++)
		sv->ob_item[i] = NULL;
	if (last_is_sticky && sizediff > 0) {
		/* growing: move entries to the end and zero moved entries */
		for (i = newsize - 1; i >= sizediff; i--) {
			sv->ob_item[i] = sv->ob_item[i - sizediff];
			sv->ob_item[i - sizediff] = NULL;
		}
	}
	sv->ob_size = newsize;
	return 0;
}
