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

/* List object implementation */

#include "Python.h"

#ifdef STDC_HEADERS
#include <stddef.h>
#else
#include <sys/types.h>		/* For size_t */
#endif

#define ROUNDUP(n, PyTryBlock) \
	((((n)+(PyTryBlock)-1)/(PyTryBlock))*(PyTryBlock))

static int
roundupsize(n)
	int n;
{
	if (n < 500)
		return ROUNDUP(n, 10);
	else
		return ROUNDUP(n, 100);
}

#define NRESIZE(var, type, nitems) PyMem_RESIZE(var, type, roundupsize(nitems))

PyObject *
PyList_New(size)
	int size;
{
	int i;
	PyListObject *op;
	size_t nbytes;
	if (size < 0) {
		PyErr_BadInternalCall();
		return NULL;
	}
	nbytes = size * sizeof(PyObject *);
	/* Check for overflow */
	if (nbytes / sizeof(PyObject *) != (size_t)size) {
		return PyErr_NoMemory();
	}
	op = (PyListObject *) malloc(sizeof(PyListObject));
	if (op == NULL) {
		return PyErr_NoMemory();
	}
	if (size <= 0) {
		op->ob_item = NULL;
	}
	else {
		op->ob_item = (PyObject **) malloc(nbytes);
		if (op->ob_item == NULL) {
			free((ANY *)op);
			return PyErr_NoMemory();
		}
	}
	op->ob_type = &PyList_Type;
	op->ob_size = size;
	for (i = 0; i < size; i++)
		op->ob_item[i] = NULL;
	_Py_NewReference(op);
	return (PyObject *) op;
}

int
PyList_Size(op)
	PyObject *op;
{
	if (!PyList_Check(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	else
		return ((PyListObject *)op) -> ob_size;
}

static PyObject *indexerr;

PyObject *
PyList_GetItem(op, i)
	PyObject *op;
	int i;
{
	if (!PyList_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	if (i < 0 || i >= ((PyListObject *)op) -> ob_size) {
		if (indexerr == NULL)
			indexerr = PyString_FromString(
				"list index out of range");
		PyErr_SetObject(PyExc_IndexError, indexerr);
		return NULL;
	}
	return ((PyListObject *)op) -> ob_item[i];
}

int
PyList_SetItem(op, i, newitem)
	register PyObject *op;
	register int i;
	register PyObject *newitem;
{
	register PyObject *olditem;
	register PyObject **p;
	if (!PyList_Check(op)) {
		Py_XDECREF(newitem);
		PyErr_BadInternalCall();
		return -1;
	}
	if (i < 0 || i >= ((PyListObject *)op) -> ob_size) {
		Py_XDECREF(newitem);
		PyErr_SetString(PyExc_IndexError,
				"list assignment index out of range");
		return -1;
	}
	p = ((PyListObject *)op) -> ob_item + i;
	olditem = *p;
	*p = newitem;
	Py_XDECREF(olditem);
	return 0;
}

static int
ins1(self, where, v)
	PyListObject *self;
	int where;
	PyObject *v;
{
	int i;
	PyObject **items;
	if (v == NULL) {
		PyErr_BadInternalCall();
		return -1;
	}
	items = self->ob_item;
	NRESIZE(items, PyObject *, self->ob_size+1);
	if (items == NULL) {
		PyErr_NoMemory();
		return -1;
	}
	if (where < 0)
		where = 0;
	if (where > self->ob_size)
		where = self->ob_size;
	for (i = self->ob_size; --i >= where; )
		items[i+1] = items[i];
	Py_INCREF(v);
	items[where] = v;
	self->ob_item = items;
	self->ob_size++;
	return 0;
}

int
PyList_Insert(op, where, newitem)
	PyObject *op;
	int where;
	PyObject *newitem;
{
	if (!PyList_Check(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return ins1((PyListObject *)op, where, newitem);
}

int
PyList_Append(op, newitem)
	PyObject *op;
	PyObject *newitem;
{
	if (!PyList_Check(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return ins1((PyListObject *)op,
		(int) ((PyListObject *)op)->ob_size, newitem);
}

/* Methods */

static void
list_dealloc(op)
	PyListObject *op;
{
	int i;
	if (op->ob_item != NULL) {
		for (i = 0; i < op->ob_size; i++) {
			Py_XDECREF(op->ob_item[i]);
		}
		free((ANY *)op->ob_item);
	}
	free((ANY *)op);
}

static int
list_print(op, fp, flags)
	PyListObject *op;
	FILE *fp;
	int flags;
{
	int i;

	i = Py_ReprEnter((PyObject*)op);
	if (i != 0) {
		if (i < 0)
			return i;
		fprintf(fp, "[...]");
		return 0;
	}
	fprintf(fp, "[");
	for (i = 0; i < op->ob_size; i++) {
		if (i > 0)
			fprintf(fp, ", ");
		if (PyObject_Print(op->ob_item[i], fp, 0) != 0) {
			Py_ReprLeave((PyObject *)op);
			return -1;
		}
	}
	fprintf(fp, "]");
	Py_ReprLeave((PyObject *)op);
	return 0;
}

static PyObject *
list_repr(v)
	PyListObject *v;
{
	PyObject *s, *comma;
	int i;

	i = Py_ReprEnter((PyObject*)v);
	if (i != 0) {
		if (i > 0)
			return PyString_FromString("[...]");
		return NULL;
	}
	s = PyString_FromString("[");
	comma = PyString_FromString(", ");
	for (i = 0; i < v->ob_size && s != NULL; i++) {
		if (i > 0)
			PyString_Concat(&s, comma);
		PyString_ConcatAndDel(&s, PyObject_Repr(v->ob_item[i]));
	}
	Py_XDECREF(comma);
	PyString_ConcatAndDel(&s, PyString_FromString("]"));
	Py_ReprLeave((PyObject *)v);
	return s;
}

static int
list_compare(v, w)
	PyListObject *v, *w;
{
	int len = (v->ob_size < w->ob_size) ? v->ob_size : w->ob_size;
	int i;
	for (i = 0; i < len; i++) {
		int cmp = PyObject_Compare(v->ob_item[i], w->ob_item[i]);
		if (cmp != 0)
			return cmp;
	}
	return v->ob_size - w->ob_size;
}

static int
list_length(a)
	PyListObject *a;
{
	return a->ob_size;
}

static PyObject *
list_item(a, i)
	PyListObject *a;
	int i;
{
	if (i < 0 || i >= a->ob_size) {
		if (indexerr == NULL)
			indexerr = PyString_FromString(
				"list index out of range");
		PyErr_SetObject(PyExc_IndexError, indexerr);
		return NULL;
	}
	Py_INCREF(a->ob_item[i]);
	return a->ob_item[i];
}

static PyObject *
list_slice(a, ilow, ihigh)
	PyListObject *a;
	int ilow, ihigh;
{
	PyListObject *np;
	int i;
	if (ilow < 0)
		ilow = 0;
	else if (ilow > a->ob_size)
		ilow = a->ob_size;
	if (ihigh < 0)
		ihigh = 0;
	if (ihigh < ilow)
		ihigh = ilow;
	else if (ihigh > a->ob_size)
		ihigh = a->ob_size;
	np = (PyListObject *) PyList_New(ihigh - ilow);
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
PyList_GetSlice(a, ilow, ihigh)
	PyObject *a;
	int ilow, ihigh;
{
	if (!PyList_Check(a)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return list_slice((PyListObject *)a, ilow, ihigh);
}

static PyObject *
list_concat(a, bb)
	PyListObject *a;
	PyObject *bb;
{
	int size;
	int i;
	PyListObject *np;
	if (!PyList_Check(bb)) {
		PyErr_BadArgument();
		return NULL;
	}
#define b ((PyListObject *)bb)
	size = a->ob_size + b->ob_size;
	np = (PyListObject *) PyList_New(size);
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
list_repeat(a, n)
	PyListObject *a;
	int n;
{
	int i, j;
	int size;
	PyListObject *np;
	PyObject **p;
	if (n < 0)
		n = 0;
	size = a->ob_size * n;
	np = (PyListObject *) PyList_New(size);
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

static int
list_ass_slice(a, ilow, ihigh, v)
	PyListObject *a;
	int ilow, ihigh;
	PyObject *v;
{
	/* Because [X]DECREF can recursively invoke list operations on
	   this list, we must postpone all [X]DECREF activity until
	   after the list is back in its canonical shape.  Therefore
	   we must allocate an additional array, 'recycle', into which
	   we temporarily copy the items that are deleted from the
	   list. :-( */
	PyObject **recycle, **p;
	PyObject **item;
	int n; /* Size of replacement list */
	int d; /* Change in size */
	int k; /* Loop index */
#define b ((PyListObject *)v)
	if (v == NULL)
		n = 0;
	else if (PyList_Check(v)) {
		n = b->ob_size;
		if (a == b) {
			/* Special case "a[i:j] = a" -- copy b first */
			int ret;
			v = list_slice(b, 0, n);
			ret = list_ass_slice(a, ilow, ihigh, v);
			Py_DECREF(v);
			return ret;
		}
	}
	else {
		PyErr_BadArgument();
		return -1;
	}
	if (ilow < 0)
		ilow = 0;
	else if (ilow > a->ob_size)
		ilow = a->ob_size;
	if (ihigh < 0)
		ihigh = 0;
	if (ihigh < ilow)
		ihigh = ilow;
	else if (ihigh > a->ob_size)
		ihigh = a->ob_size;
	item = a->ob_item;
	d = n - (ihigh-ilow);
	if (ihigh > ilow)
		p = recycle = PyMem_NEW(PyObject *, (ihigh-ilow));
	else
		p = recycle = NULL;
	if (d <= 0) { /* Delete -d items; recycle ihigh-ilow items */
		for (k = ilow; k < ihigh; k++)
			*p++ = item[k];
		if (d < 0) {
			for (/*k = ihigh*/; k < a->ob_size; k++)
				item[k+d] = item[k];
			a->ob_size += d;
			NRESIZE(item, PyObject *, a->ob_size); /* Can't fail */
			a->ob_item = item;
		}
	}
	else { /* Insert d items; recycle ihigh-ilow items */
		NRESIZE(item, PyObject *, a->ob_size + d);
		if (item == NULL) {
			PyMem_XDEL(recycle);
			PyErr_NoMemory();
			return -1;
		}
		for (k = a->ob_size; --k >= ihigh; )
			item[k+d] = item[k];
		for (/*k = ihigh-1*/; k >= ilow; --k)
			*p++ = item[k];
		a->ob_item = item;
		a->ob_size += d;
	}
	for (k = 0; k < n; k++, ilow++) {
		PyObject *w = b->ob_item[k];
		Py_XINCREF(w);
		item[ilow] = w;
	}
	if (recycle) {
		while (--p >= recycle)
			Py_XDECREF(*p);
		PyMem_DEL(recycle);
	}
	return 0;
#undef b
}

int
PyList_SetSlice(a, ilow, ihigh, v)
	PyObject *a;
	int ilow, ihigh;
	PyObject *v;
{
	if (!PyList_Check(a)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return list_ass_slice((PyListObject *)a, ilow, ihigh, v);
}

static int
list_ass_item(a, i, v)
	PyListObject *a;
	int i;
	PyObject *v;
{
	PyObject *old_value;
	if (i < 0 || i >= a->ob_size) {
		PyErr_SetString(PyExc_IndexError,
				"list assignment index out of range");
		return -1;
	}
	if (v == NULL)
		return list_ass_slice(a, i, i+1, v);
	Py_INCREF(v);
	old_value = a->ob_item[i];
	a->ob_item[i] = v;
	Py_DECREF(old_value); 
	return 0;
}

static PyObject *
ins(self, where, v)
	PyListObject *self;
	int where;
	PyObject *v;
{
	if (ins1(self, where, v) != 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
listinsert(self, args)
	PyListObject *self;
	PyObject *args;
{
	int i;
	PyObject *v;
	if (!PyArg_Parse(args, "(iO)", &i, &v))
		return NULL;
	return ins(self, i, v);
}

static PyObject *
listappend(self, args)
	PyListObject *self;
	PyObject *args;
{
	PyObject *v;
	if (!PyArg_Parse(args, "O", &v))
		return NULL;
	return ins(self, (int) self->ob_size, v);
}

/* New quicksort implementation for arrays of object pointers.
   Thanks to discussions with Tim Peters. */

/* CMPERROR is returned by our comparison function when an error
   occurred.  This is the largest negative integer (0x80000000 on a
   32-bit system). */
#define CMPERROR ( (int) ((unsigned int)1 << (8*sizeof(int) - 1)) )

/* Comparison function.  Takes care of calling a user-supplied
   comparison function (any callable Python object).  Calls the
   standard comparison function, cmpobject(), if the user-supplied
   function is NULL. */

static int
docompare(x, y, compare)
	PyObject *x;
	PyObject *y;
	PyObject *compare;
{
	PyObject *args, *res;
	int i;

	if (compare == NULL) {
		i = PyObject_Compare(x, y);
		if (i && PyErr_Occurred())
			i = CMPERROR;
		return i;
	}

	args = Py_BuildValue("(OO)", x, y);
	if (args == NULL)
		return CMPERROR;
	res = PyEval_CallObject(compare, args);
	Py_DECREF(args);
	if (res == NULL)
		return CMPERROR;
	if (!PyInt_Check(res)) {
		Py_DECREF(res);
		PyErr_SetString(PyExc_TypeError,
				"comparison function should return int");
		return CMPERROR;
	}
	i = PyInt_AsLong(res);
	Py_DECREF(res);
	if (i < 0)
		return -1;
	if (i > 0)
		return 1;
	return 0;
}

/* MINSIZE is the smallest array we care to partition; smaller arrays
   are sorted using a straight insertion sort (above).  It must be at
   least 3 for the quicksort implementation to work.  Assuming that
   comparisons are more expensive than everything else (and this is a
   good assumption for Python), it should be 10, which is the cutoff
   point: quicksort requires more comparisons than insertion sort for
   smaller arrays. */
#define MINSIZE 12

/* Straight insertion sort.  More efficient for sorting small arrays. */

static int
insertionsort(array, size, compare)
	PyObject **array;	/* Start of array to sort */
	int size;	/* Number of elements to sort */
	PyObject *compare;/* Comparison function object, or NULL => default */
{
	register PyObject **a = array;
	register PyObject **end = array+size;
	register PyObject **p;

	for (p = a+1; p < end; p++) {
		register PyObject *key = *p;
		register PyObject **q = p;
		while (--q >= a) {
			register int k = docompare(key, *q, compare);
			/* if (p-q >= MINSIZE)
			   fprintf(stderr, "OUCH! %d\n", p-q); */
			if (k == CMPERROR)
				return -1;
			if (k < 0) {
				*(q+1) = *q;
				*q = key; /* For consistency */
			}
			else
				break;
		}
	}

	return 0;
}

/* STACKSIZE is the size of our work stack.  A rough estimate is that
   this allows us to sort arrays of MINSIZE * 2**STACKSIZE, or large
   enough.  (Because of the way we push the biggest partition first,
   the worst case occurs when all subarrays are always partitioned
   exactly in two.) */
#define STACKSIZE 64

/* Quicksort algorithm.  Return -1 if an exception occurred; in this
   case we leave the array partly sorted but otherwise in good health
   (i.e. no items have been removed or duplicated). */

static int
quicksort(array, size, compare)
	PyObject **array;	/* Start of array to sort */
	int size;	/* Number of elements to sort */
	PyObject *compare;/* Comparison function object, or NULL for default */
{
	register PyObject *tmp, *pivot;
	register PyObject **l, **r, **p;
	register PyObject **lo, **hi;
	int top, k, n;
	PyObject **lostack[STACKSIZE];
	PyObject **histack[STACKSIZE];

	/* Start out with the whole array on the work stack */
	lostack[0] = array;
	histack[0] = array+size;
	top = 1;

	/* Repeat until the work stack is empty */
	while (--top >= 0) {
		lo = lostack[top];
		hi = histack[top];

		/* If it's a small one, use straight insertion sort */
		n = hi - lo;
		if (n < MINSIZE)
			continue;

		/* Choose median of first, middle and last as pivot;
		   these 3 are reverse-sorted in the process; the ends
		   will be swapped on the first do-loop iteration.
		*/
		l = lo;		 /* First  */
		p = lo + (n>>1); /* Middle */
		r = hi - 1;	 /* Last   */

		k = docompare(*l, *p, compare);
		if (k == CMPERROR)
			return -1;
		if (k < 0)
			{ tmp = *l; *l = *p; *p = tmp; }

		k = docompare(*p, *r, compare);
		if (k == CMPERROR)
			return -1;
		if (k < 0)
			{ tmp = *p; *p = *r; *r = tmp; }

		k = docompare(*l, *p, compare);
		if (k == CMPERROR)
			return -1;
		if (k < 0)
			{ tmp = *l; *l = *p; *p = tmp; }

		pivot = *p;

		/* Partition the array */
		do {
			tmp = *l; *l = *r; *r = tmp;
			if (l == p) {
				p = r;
				l++;
			}
			else if (r == p) {
				p = l;
				r--;
			}
			else {
				l++;
				r--;
			}

			/* Move left index to element >= pivot */
			while (l < p) {
				k = docompare(*l, pivot, compare);
				if (k == CMPERROR)
					return -1;
				if (k < 0)
					l++;
				else
					break;
			}
			/* Move right index to element <= pivot */
			while (r > p) {
				k = docompare(pivot, *r, compare);
				if (k == CMPERROR)
					return -1;
				if (k < 0)
					r--;
				else
					break;
			}

		} while (l < r);

		/* lo < l == p == r < hi-1
		   *p == pivot

		   All in [lo,p)   are <= pivot
		   At p                == pivot
		   All in [p+1,hi) are >= pivot

		   Now extend as far as possible (around p) so that:
		   All in [lo,r) are <= pivot
		   All in [r,l)  are == pivot
		   All in [l,hi) are >= pivot
		   This wastes two compares if no elements are == to the
		   pivot, but can win big when there are duplicates.
		   Mildly tricky: continue using only "<" -- we deduce
		   equality indirectly.
		*/
		while (r > lo) {
			/* because r-1 < p, *(r-1) <= pivot is known */
			k = docompare(*(r-1), pivot, compare);
			if (k == CMPERROR)
				return -1;
			if (k < 0)
				break;
			/* <= and not < implies == */
			r--;
		}

		l++;
		while (l < hi) {
			/* because l > p, pivot <= *l is known */
			k = docompare(pivot, *l, compare);
			if (k == CMPERROR)
				return -1;
			if (k < 0)
				break;
			/* <= and not < implies == */
			l++;
		}

		/* Push biggest partition first */
		if (r - lo >= hi - l) {
			/* First one is bigger */
			lostack[top] = lo;
			histack[top++] = r;
			lostack[top] = l;
			histack[top++] = hi;
		} else {
			/* Second one is bigger */
			lostack[top] = l;
			histack[top++] = hi;
			lostack[top] = lo;
			histack[top++] = r;
		}
		/* Should assert top <= STACKSIZE */
	}

	/*
	 * Ouch - even if I screwed up the quicksort above, the
	 * insertionsort below will cover up the problem - just a
	 * performance hit would be noticable.
	 */

	/* insertionsort is pretty fast on the partially sorted list */

	if (insertionsort(array, size, compare) < 0)
		return -1;

	/* Success */
	return 0;
}

static PyObject *
listsort(self, compare)
	PyListObject *self;
	PyObject *compare;
{
	/* XXX Don't you *dare* changing the list's length in compare()! */
	if (quicksort(self->ob_item, self->ob_size, compare) < 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
listreverse(self, args)
	PyListObject *self;
	PyObject *args;
{
	register PyObject **p, **q;
	register PyObject *tmp;
	
	if (args != NULL) {
		PyErr_BadArgument();
		return NULL;
	}

	if (self->ob_size > 1) {
		for (p = self->ob_item, q = self->ob_item + self->ob_size - 1;
						p < q; p++, q--) {
			tmp = *p;
			*p = *q;
			*q = tmp;
		}
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

int
PyList_Reverse(v)
	PyObject *v;
{
	if (v == NULL || !PyList_Check(v)) {
		PyErr_BadInternalCall();
		return -1;
	}
	v = listreverse((PyListObject *)v, (PyObject *)NULL);
	if (v == NULL)
		return -1;
	Py_DECREF(v);
	return 0;
}

int
PyList_Sort(v)
	PyObject *v;
{
	if (v == NULL || !PyList_Check(v)) {
		PyErr_BadInternalCall();
		return -1;
	}
	v = listsort((PyListObject *)v, (PyObject *)NULL);
	if (v == NULL)
		return -1;
	Py_DECREF(v);
	return 0;
}

PyObject *
PyList_AsTuple(v)
	PyObject *v;
{
	PyObject *w;
	PyObject **p;
	int n;
	if (v == NULL || !PyList_Check(v)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	n = ((PyListObject *)v)->ob_size;
	w = PyTuple_New(n);
	if (w == NULL)
		return NULL;
	p = ((PyTupleObject *)w)->ob_item;
	memcpy((ANY *)p,
	       (ANY *)((PyListObject *)v)->ob_item,
	       n*sizeof(PyObject *));
	while (--n >= 0) {
		Py_INCREF(*p);
		p++;
	}
	return w;
}

static PyObject *
listindex(self, args)
	PyListObject *self;
	PyObject *args;
{
	int i;
	
	if (args == NULL) {
		PyErr_BadArgument();
		return NULL;
	}
	for (i = 0; i < self->ob_size; i++) {
		if (PyObject_Compare(self->ob_item[i], args) == 0)
			return PyInt_FromLong((long)i);
		if (PyErr_Occurred())
			return NULL;
	}
	PyErr_SetString(PyExc_ValueError, "list.index(x): x not in list");
	return NULL;
}

static PyObject *
listcount(self, args)
	PyListObject *self;
	PyObject *args;
{
	int count = 0;
	int i;
	
	if (args == NULL) {
		PyErr_BadArgument();
		return NULL;
	}
	for (i = 0; i < self->ob_size; i++) {
		if (PyObject_Compare(self->ob_item[i], args) == 0)
			count++;
		if (PyErr_Occurred())
			return NULL;
	}
	return PyInt_FromLong((long)count);
}

static PyObject *
listremove(self, args)
	PyListObject *self;
	PyObject *args;
{
	int i;
	
	if (args == NULL) {
		PyErr_BadArgument();
		return NULL;
	}
	for (i = 0; i < self->ob_size; i++) {
		if (PyObject_Compare(self->ob_item[i], args) == 0) {
			if (list_ass_slice(self, i, i+1,
					   (PyObject *)NULL) != 0)
				return NULL;
			Py_INCREF(Py_None);
			return Py_None;
		}
		if (PyErr_Occurred())
			return NULL;
	}
	PyErr_SetString(PyExc_ValueError, "list.remove(x): x not in list");
	return NULL;
}

static PyMethodDef list_methods[] = {
	{"append",	(PyCFunction)listappend},
	{"insert",	(PyCFunction)listinsert},
	{"remove",	(PyCFunction)listremove},
	{"index",	(PyCFunction)listindex},
	{"count",	(PyCFunction)listcount},
	{"reverse",	(PyCFunction)listreverse},
	{"sort",	(PyCFunction)listsort, 0},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
list_getattr(f, name)
	PyListObject *f;
	char *name;
{
	return Py_FindMethod(list_methods, (PyObject *)f, name);
}

static PySequenceMethods list_as_sequence = {
	(inquiry)list_length, /*sq_length*/
	(binaryfunc)list_concat, /*sq_concat*/
	(intargfunc)list_repeat, /*sq_repeat*/
	(intargfunc)list_item, /*sq_item*/
	(intintargfunc)list_slice, /*sq_slice*/
	(intobjargproc)list_ass_item, /*sq_ass_item*/
	(intintobjargproc)list_ass_slice, /*sq_ass_slice*/
};

PyTypeObject PyList_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"list",
	sizeof(PyListObject),
	0,
	(destructor)list_dealloc, /*tp_dealloc*/
	(printfunc)list_print, /*tp_print*/
	(getattrfunc)list_getattr, /*tp_getattr*/
	0,		/*tp_setattr*/
	(cmpfunc)list_compare, /*tp_compare*/
	(reprfunc)list_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	&list_as_sequence,	/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};
