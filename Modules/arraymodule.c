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

/* Array object implementation */

/* An array is a uniform list -- all items have the same type.
   The item type is restricted to simple C types like int or float */

#include "Python.h"

#ifdef STDC_HEADERS
#include <stddef.h>
#else
#include <sys/types.h>		/* For size_t */
#endif

struct arrayobject; /* Forward */

struct arraydescr {
	int typecode;
	int itemsize;
	PyObject * (*getitem) Py_FPROTO((struct arrayobject *, int));
	int (*setitem) Py_FPROTO((struct arrayobject *, int, PyObject *));
};

typedef struct arrayobject {
	PyObject_VAR_HEAD
	char *ob_item;
	struct arraydescr *ob_descr;
} arrayobject;

staticforward PyTypeObject Arraytype;

#define is_arrayobject(op) ((op)->ob_type == &Arraytype)

/* Forward */
static PyObject *newarrayobject Py_PROTO((int, struct arraydescr *));
#if 0
static int getarraysize Py_PROTO((PyObject *));
#endif
static PyObject *getarrayitem Py_PROTO((PyObject *, int));
static int setarrayitem Py_PROTO((PyObject *, int, PyObject *));
#if 0
static int insarrayitem Py_PROTO((PyObject *, int, PyObject *));
static int addarrayitem Py_PROTO((PyObject *, PyObject *));
#endif

static PyObject *
c_getitem(ap, i)
	arrayobject *ap;
	int i;
{
	return PyString_FromStringAndSize(&((char *)ap->ob_item)[i], 1);
}

static int
c_setitem(ap, i, v)
	arrayobject *ap;
	int i;
	PyObject *v;
{
	char x;
	if (!PyArg_Parse(v, "c;array item must be char", &x))
		return -1;
	if (i >= 0)
		     ((char *)ap->ob_item)[i] = x;
	return 0;
}

static PyObject *
b_getitem(ap, i)
	arrayobject *ap;
	int i;
{
	long x = ((char *)ap->ob_item)[i];
	if (x >= 128)
		x -= 256;
	return PyInt_FromLong(x);
}

static int
b_setitem(ap, i, v)
	arrayobject *ap;
	int i;
	PyObject *v;
{
	char x;
	if (!PyArg_Parse(v, "b;array item must be integer", &x))
		return -1;
	if (i >= 0)
		     ((char *)ap->ob_item)[i] = x;
	return 0;
}

static PyObject *
BB_getitem(ap, i)
	arrayobject *ap;
	int i;
{
	long x = ((unsigned char *)ap->ob_item)[i];
	return PyInt_FromLong(x);
}

#define BB_setitem b_setitem

static PyObject *
h_getitem(ap, i)
	arrayobject *ap;
	int i;
{
	return PyInt_FromLong((long) ((short *)ap->ob_item)[i]);
}

static int
h_setitem(ap, i, v)
	arrayobject *ap;
	int i;
	PyObject *v;
{
	short x;
	if (!PyArg_Parse(v, "h;array item must be integer", &x))
		return -1;
	if (i >= 0)
		     ((short *)ap->ob_item)[i] = x;
	return 0;
}

static PyObject *
HH_getitem(ap, i)
	arrayobject *ap;
	int i;
{
	return PyInt_FromLong((long) ((unsigned short *)ap->ob_item)[i]);
}

#define HH_setitem h_setitem

static PyObject *
i_getitem(ap, i)
	arrayobject *ap;
	int i;
{
	return PyInt_FromLong((long) ((int *)ap->ob_item)[i]);
}

static int
i_setitem(ap, i, v)
	arrayobject *ap;
	int i;
	PyObject *v;
{
	int x;
	if (!PyArg_Parse(v, "i;array item must be integer", &x))
		return -1;
	if (i >= 0)
		     ((int *)ap->ob_item)[i] = x;
	return 0;
}

static PyObject *
II_getitem(ap, i)
	arrayobject *ap;
	int i;
{
	return PyLong_FromUnsignedLong(
		(unsigned long) ((unsigned int *)ap->ob_item)[i]);
}

static int
II_setitem(ap, i, v)
	arrayobject *ap;
	int i;
	PyObject *v;
{
	unsigned long x;
	if (PyLong_Check(v)) {
		x = PyLong_AsUnsignedLong(v);
		if (x == (unsigned long) -1 && PyErr_Occurred())
			return -1;
	}
	else {
		if (!PyArg_Parse(v, "l;array item must be integer", &x))
			return -1;
	}
	if (i >= 0)
		((unsigned int *)ap->ob_item)[i] = x;
	return 0;
}

static PyObject *
l_getitem(ap, i)
	arrayobject *ap;
	int i;
{
	return PyInt_FromLong(((long *)ap->ob_item)[i]);
}

static int
l_setitem(ap, i, v)
	arrayobject *ap;
	int i;
	PyObject *v;
{
	long x;
	if (!PyArg_Parse(v, "l;array item must be integer", &x))
		return -1;
	if (i >= 0)
		     ((long *)ap->ob_item)[i] = x;
	return 0;
}

static PyObject *
LL_getitem(ap, i)
	arrayobject *ap;
	int i;
{
	return PyLong_FromUnsignedLong(((unsigned long *)ap->ob_item)[i]);
}

static int
LL_setitem(ap, i, v)
	arrayobject *ap;
	int i;
	PyObject *v;
{
	unsigned long x;
	if (PyLong_Check(v)) {
		x = PyLong_AsUnsignedLong(v);
		if (x == (unsigned long) -1 && PyErr_Occurred())
			return -1;
	}
	else {
		if (!PyArg_Parse(v, "l;array item must be integer", &x))
			return -1;
	}
	if (i >= 0)
		((unsigned long *)ap->ob_item)[i] = x;
	return 0;
}

static PyObject *
f_getitem(ap, i)
	arrayobject *ap;
	int i;
{
	return PyFloat_FromDouble((double) ((float *)ap->ob_item)[i]);
}

static int
f_setitem(ap, i, v)
	arrayobject *ap;
	int i;
	PyObject *v;
{
	float x;
	if (!PyArg_Parse(v, "f;array item must be float", &x))
		return -1;
	if (i >= 0)
		     ((float *)ap->ob_item)[i] = x;
	return 0;
}

static PyObject *
d_getitem(ap, i)
	arrayobject *ap;
	int i;
{
	return PyFloat_FromDouble(((double *)ap->ob_item)[i]);
}

static int
d_setitem(ap, i, v)
	arrayobject *ap;
	int i;
	PyObject *v;
{
	double x;
	if (!PyArg_Parse(v, "d;array item must be float", &x))
		return -1;
	if (i >= 0)
		     ((double *)ap->ob_item)[i] = x;
	return 0;
}

/* Description of types */
static struct arraydescr descriptors[] = {
	{'c', sizeof(char), c_getitem, c_setitem},
	{'b', sizeof(char), b_getitem, b_setitem},
	{'B', sizeof(char), BB_getitem, BB_setitem},
	{'h', sizeof(short), h_getitem, h_setitem},
	{'H', sizeof(short), HH_getitem, HH_setitem},
	{'i', sizeof(int), i_getitem, i_setitem},
	{'I', sizeof(int), II_getitem, II_setitem},
	{'l', sizeof(long), l_getitem, l_setitem},
	{'L', sizeof(long), LL_getitem, LL_setitem},
	{'f', sizeof(float), f_getitem, f_setitem},
	{'d', sizeof(double), d_getitem, d_setitem},
	{'\0', 0, 0, 0} /* Sentinel */
};
/* If we ever allow items larger than double, we must change reverse()! */
	

static PyObject *
newarrayobject(size, descr)
	int size;
	struct arraydescr *descr;
{
	arrayobject *op;
	size_t nbytes;
	if (size < 0) {
		PyErr_BadInternalCall();
		return NULL;
	}
	nbytes = size * descr->itemsize;
	/* Check for overflow */
	if (nbytes / descr->itemsize != (size_t)size) {
		return PyErr_NoMemory();
	}
	op = PyMem_NEW(arrayobject, 1);
	if (op == NULL) {
		return PyErr_NoMemory();
	}
	if (size <= 0) {
		op->ob_item = NULL;
	}
	else {
		op->ob_item = PyMem_NEW(char, nbytes);
		if (op->ob_item == NULL) {
			PyMem_DEL(op);
			return PyErr_NoMemory();
		}
	}
	op->ob_type = &Arraytype;
	op->ob_size = size;
	op->ob_descr = descr;
	_Py_NewReference(op);
	return (PyObject *) op;
}

#if 0
static int
getarraysize(op)
	PyObject *op;
{
	if (!is_arrayobject(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return ((arrayobject *)op) -> ob_size;
}
#endif

static PyObject *
getarrayitem(op, i)
	PyObject *op;
	int i;
{
	register arrayobject *ap;
	if (!is_arrayobject(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	ap = (arrayobject *)op;
	if (i < 0 || i >= ap->ob_size) {
		PyErr_SetString(PyExc_IndexError, "array index out of range");
		return NULL;
	}
	return (*ap->ob_descr->getitem)(ap, i);
}

static int
ins1(self, where, v)
	arrayobject *self;
	int where;
	PyObject *v;
{
	char *items;
	if (v == NULL) {
		PyErr_BadInternalCall();
		return -1;
	}
	if ((*self->ob_descr->setitem)(self, -1, v) < 0)
		return -1;
	items = self->ob_item;
	PyMem_RESIZE(items, char,
		     (self->ob_size+1) * self->ob_descr->itemsize);
	if (items == NULL) {
		PyErr_NoMemory();
		return -1;
	}
	if (where < 0)
		where = 0;
	if (where > self->ob_size)
		where = self->ob_size;
	memmove(items + (where+1)*self->ob_descr->itemsize,
		items + where*self->ob_descr->itemsize,
		(self->ob_size-where)*self->ob_descr->itemsize);
	self->ob_item = items;
	self->ob_size++;
	return (*self->ob_descr->setitem)(self, where, v);
}

#if 0
static int
insarrayitem(op, where, newitem)
	PyObject *op;
	int where;
	PyObject *newitem;
{
	if (!is_arrayobject(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return ins1((arrayobject *)op, where, newitem);
}

static int
addarrayitem(op, newitem)
	PyObject *op;
	PyObject *newitem;
{
	if (!is_arrayobject(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return ins1((arrayobject *)op,
		(int) ((arrayobject *)op)->ob_size, newitem);
}
#endif

/* Methods */

static void
array_dealloc(op)
	arrayobject *op;
{
	if (op->ob_item != NULL)
		PyMem_DEL(op->ob_item);
	PyMem_DEL(op);
}

static int
array_compare(v, w)
	arrayobject *v, *w;
{
	int len = (v->ob_size < w->ob_size) ? v->ob_size : w->ob_size;
	int i;
	for (i = 0; i < len; i++) {
		PyObject *ai, *bi;
		int cmp;
		ai = getarrayitem((PyObject *)v, i);
		bi = getarrayitem((PyObject *)w, i);
		if (ai && bi)
			cmp = PyObject_Compare(ai, bi);
		else
			cmp = -1;
		Py_XDECREF(ai);
		Py_XDECREF(bi);
		if (cmp != 0)
			return cmp;
	}
	return v->ob_size - w->ob_size;
}

static int
array_length(a)
	arrayobject *a;
{
	return a->ob_size;
}

static PyObject *
array_item(a, i)
	arrayobject *a;
	int i;
{
	if (i < 0 || i >= a->ob_size) {
		PyErr_SetString(PyExc_IndexError, "array index out of range");
		return NULL;
	}
	return getarrayitem((PyObject *)a, i);
}

static PyObject *
array_slice(a, ilow, ihigh)
	arrayobject *a;
	int ilow, ihigh;
{
	arrayobject *np;
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
	np = (arrayobject *) newarrayobject(ihigh - ilow, a->ob_descr);
	if (np == NULL)
		return NULL;
	memcpy(np->ob_item, a->ob_item + ilow * a->ob_descr->itemsize,
	       (ihigh-ilow) * a->ob_descr->itemsize);
	return (PyObject *)np;
}

static PyObject *
array_concat(a, bb)
	arrayobject *a;
	PyObject *bb;
{
	int size;
	arrayobject *np;
	if (!is_arrayobject(bb)) {
		PyErr_BadArgument();
		return NULL;
	}
#define b ((arrayobject *)bb)
	if (a->ob_descr != b->ob_descr) {
		PyErr_BadArgument();
		return NULL;
	}
	size = a->ob_size + b->ob_size;
	np = (arrayobject *) newarrayobject(size, a->ob_descr);
	if (np == NULL) {
		return NULL;
	}
	memcpy(np->ob_item, a->ob_item, a->ob_size*a->ob_descr->itemsize);
	memcpy(np->ob_item + a->ob_size*a->ob_descr->itemsize,
	       b->ob_item, b->ob_size*b->ob_descr->itemsize);
	return (PyObject *)np;
#undef b
}

static PyObject *
array_repeat(a, n)
	arrayobject *a;
	int n;
{
	int i;
	int size;
	arrayobject *np;
	char *p;
	int nbytes;
	if (n < 0)
		n = 0;
	size = a->ob_size * n;
	np = (arrayobject *) newarrayobject(size, a->ob_descr);
	if (np == NULL)
		return NULL;
	p = np->ob_item;
	nbytes = a->ob_size * a->ob_descr->itemsize;
	for (i = 0; i < n; i++) {
		memcpy(p, a->ob_item, nbytes);
		p += nbytes;
	}
	return (PyObject *) np;
}

static int
array_ass_slice(a, ilow, ihigh, v)
	arrayobject *a;
	int ilow, ihigh;
	PyObject *v;
{
	char *item;
	int n; /* Size of replacement array */
	int d; /* Change in size */
#define b ((arrayobject *)v)
	if (v == NULL)
		n = 0;
	else if (is_arrayobject(v)) {
		n = b->ob_size;
		if (a == b) {
			/* Special case "a[i:j] = a" -- copy b first */
			int ret;
			v = array_slice(b, 0, n);
			ret = array_ass_slice(a, ilow, ihigh, v);
			Py_DECREF(v);
			return ret;
		}
		if (b->ob_descr != a->ob_descr) {
			PyErr_BadArgument();
			return -1;
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
	if (d < 0) { /* Delete -d items */
		memmove(item + (ihigh+d)*a->ob_descr->itemsize,
			item + ihigh*a->ob_descr->itemsize,
			(a->ob_size-ihigh)*a->ob_descr->itemsize);
		a->ob_size += d;
		PyMem_RESIZE(item, char, a->ob_size*a->ob_descr->itemsize);
						/* Can't fail */
		a->ob_item = item;
	}
	else if (d > 0) { /* Insert d items */
		PyMem_RESIZE(item, char,
			     (a->ob_size + d)*a->ob_descr->itemsize);
		if (item == NULL) {
			PyErr_NoMemory();
			return -1;
		}
		memmove(item + (ihigh+d)*a->ob_descr->itemsize,
			item + ihigh*a->ob_descr->itemsize,
			(a->ob_size-ihigh)*a->ob_descr->itemsize);
		a->ob_item = item;
		a->ob_size += d;
	}
	if (n > 0)
		memcpy(item + ilow*a->ob_descr->itemsize, b->ob_item,
		       n*b->ob_descr->itemsize);
	return 0;
#undef b
}

static int
array_ass_item(a, i, v)
	arrayobject *a;
	int i;
	PyObject *v;
{
	if (i < 0 || i >= a->ob_size) {
		PyErr_SetString(PyExc_IndexError,
			         "array assignment index out of range");
		return -1;
	}
	if (v == NULL)
		return array_ass_slice(a, i, i+1, v);
	return (*a->ob_descr->setitem)(a, i, v);
}

static int
setarrayitem(a, i, v)
	PyObject *a;
	int i;
	PyObject *v;
{
	if (!is_arrayobject(a)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return array_ass_item((arrayobject *)a, i, v);
}

static PyObject *
ins(self, where, v)
	arrayobject *self;
	int where;
	PyObject *v;
{
	if (ins1(self, where, v) != 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
array_insert(self, args)
	arrayobject *self;
	PyObject *args;
{
	int i;
	PyObject *v;
	if (!PyArg_Parse(args, "(iO)", &i, &v))
		return NULL;
	return ins(self, i, v);
}

static char insert_doc [] =
"insert (i,x)\n\
\n\
Insert a new item x into the array before position i.";


static PyObject *
array_buffer_info(self, args)
	arrayobject *self;
	PyObject *args;
{
	return Py_BuildValue("ll",
			     (long)(self->ob_item), (long)(self->ob_size));
}

static char buffer_info_doc [] =
"buffer_info -> (address, length)\n\
\n\
Return a tuple (address, length) giving the current memory address and\n\
the length in bytes of the buffer used to hold array's contents.";


static PyObject *
array_append(self, args)
	arrayobject *self;
	PyObject *args;
{
	PyObject *v;
	if (!PyArg_Parse(args, "O", &v))
		return NULL;
	return ins(self, (int) self->ob_size, v);
}

static char append_doc [] =
"append(x)\n\
\n\
Append new value x to the end of the array.";


static PyObject *
array_byteswap(self, args)
	arrayobject *self;
	PyObject *args;
{
	char *p;
	int i;
	switch (self->ob_descr->itemsize) {
	case 1:
		break;
	case 2:
		for (p = self->ob_item, i = self->ob_size; --i >= 0; p += 2) {
			char p0 = p[0];
			p[0] = p[1];
			p[1] = p0;
		}
		break;
	case 4:
		for (p = self->ob_item, i = self->ob_size; --i >= 0; p += 4) {
			char p0 = p[0];
			char p1 = p[1];
			p[0] = p[3];
			p[1] = p[2];
			p[2] = p1;
			p[3] = p0;
		}
		break;
	case 8:
		for (p = self->ob_item, i = self->ob_size; --i >= 0; p += 8) {
			char p0 = p[0];
			char p1 = p[1];
			char p2 = p[2];
			char p3 = p[3];
			p[0] = p[7];
			p[1] = p[6];
			p[2] = p[5];
			p[3] = p[4];
			p[4] = p3;
			p[5] = p2;
			p[6] = p1;
			p[7] = p0;
		}
		break;
	default:
		PyErr_SetString(PyExc_RuntimeError,
			   "don't know how to byteswap this array type");
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char byteswap_doc [] =
"byteswap(x)\n\
\n\
Byteswap all items of the array.  This is only supported for integer\n\
values of x, which determines the size of the blocks swapped.";

static PyObject *
array_reverse(self, args)
	arrayobject *self;
	PyObject *args;
{
	register int itemsize = self->ob_descr->itemsize;
	register char *p, *q;
	char tmp[sizeof(double)]; /* Assume that's the max item size */

	if (args != NULL) {
		PyErr_BadArgument();
		return NULL;
	}

	if (self->ob_size > 1) {
		for (p = self->ob_item,
		     q = self->ob_item + (self->ob_size - 1)*itemsize;
		     p < q;
		     p += itemsize, q -= itemsize) {
			memmove(tmp, p, itemsize);
			memmove(p, q, itemsize);
			memmove(q, tmp, itemsize);
		}
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

static char reverse_doc [] =
"reverse()\n\
\n\
Reverse the order of the items in the array.";

/* The following routines were adapted from listobject.c but not converted.
   To make them work you will have to work! */

#if 0
static PyObject *
array_index(self, args)
	arrayobject *self;
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
		/* XXX PyErr_Occurred */
	}
	PyErr_SetString(PyExc_ValueError, "array.index(x): x not in array");
	return NULL;
}
#endif

#if 0
static PyObject *
array_count(self, args)
	arrayobject *self;
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
		/* XXX PyErr_Occurred */
	}
	return PyInt_FromLong((long)count);
}
#endif

#if 0
static PyObject *
array_remove(self, args)
	arrayobject *self;
	PyObject *args;
{
	int i;
	
	if (args == NULL) {
		PyErr_BadArgument();
		return NULL;
	}
	for (i = 0; i < self->ob_size; i++) {
		if (PyObject_Compare(self->ob_item[i], args) == 0) {
			if (array_ass_slice(self, i, i+1,
					    (PyObject *)NULL) != 0)
				return NULL;
			Py_INCREF(Py_None);
			return Py_None;
		}
		/* XXX PyErr_Occurred */
	}
	PyErr_SetString(PyExc_ValueError, "array.remove(x): x not in array");
	return NULL;
}
#endif

static PyObject *
array_fromfile(self, args)
	arrayobject *self;
	PyObject *args;
{
	PyObject *f;
	int n;
	FILE *fp;
	if (!PyArg_Parse(args, "(Oi)", &f, &n))
		return NULL;
	fp = PyFile_AsFile(f);
	if (fp == NULL) {
		PyErr_SetString(PyExc_TypeError, "arg1 must be open file");
		return NULL;
	}
	if (n > 0) {
		char *item = self->ob_item;
		int itemsize = self->ob_descr->itemsize;
		int nread;
		PyMem_RESIZE(item, char, (self->ob_size + n) * itemsize);
		if (item == NULL) {
			PyErr_NoMemory();
			return NULL;
		}
		self->ob_item = item;
		self->ob_size += n;
		nread = fread(item + (self->ob_size - n) * itemsize,
			      itemsize, n, fp);
		if (nread < n) {
			self->ob_size -= (n - nread);
			PyMem_RESIZE(item, char, self->ob_size*itemsize);
			self->ob_item = item;
			PyErr_SetString(PyExc_EOFError,
				         "not enough items in file");
			return NULL;
		}
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char fromfile_doc [] =
"fromfile(f, n)\n\
\n\
Read n objects from the file object f and append them to the end of the\n\
array.  Also called as read.";


static PyObject *
array_tofile(self, args)
	arrayobject *self;
	PyObject *args;
{
	PyObject *f;
	FILE *fp;
	if (!PyArg_Parse(args, "O", &f))
		return NULL;
	fp = PyFile_AsFile(f);
	if (fp == NULL) {
		PyErr_SetString(PyExc_TypeError, "arg must be open file");
		return NULL;
	}
	if (self->ob_size > 0) {
		if ((int)fwrite(self->ob_item, self->ob_descr->itemsize,
			   self->ob_size, fp) != self->ob_size) {
			PyErr_SetFromErrno(PyExc_IOError);
			clearerr(fp);
			return NULL;
		}
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char tofile_doc [] =
"tofile(f)\n\
\n\
Write all items (as machine values) to the file object f.  Also called as\n\
write.";


static PyObject *
array_fromlist(self, args)
	arrayobject *self;
	PyObject *args;
{
	int n;
	PyObject *list;
	int itemsize = self->ob_descr->itemsize;
	if (!PyArg_Parse(args, "O", &list))
		return NULL;
	if (!PyList_Check(list)) {
		PyErr_SetString(PyExc_TypeError, "arg must be list");
		return NULL;
	}
	n = PyList_Size(list);
	if (n > 0) {
		char *item = self->ob_item;
		int i;
		PyMem_RESIZE(item, char, (self->ob_size + n) * itemsize);
		if (item == NULL) {
			PyErr_NoMemory();
			return NULL;
		}
		self->ob_item = item;
		self->ob_size += n;
		for (i = 0; i < n; i++) {
			PyObject *v = PyList_GetItem(list, i);
			if ((*self->ob_descr->setitem)(self,
					self->ob_size - n + i, v) != 0) {
				self->ob_size -= n;
				PyMem_RESIZE(item, char,
					          self->ob_size * itemsize);
				self->ob_item = item;
				return NULL;
			}
		}
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char fromlist_doc [] =
"fromlist(list)\n\
\n\
Append items to array from list.";


static PyObject *
array_tolist(self, args)
	arrayobject *self;
	PyObject *args;
{
	PyObject *list = PyList_New(self->ob_size);
	int i;
	if (list == NULL)
		return NULL;
	for (i = 0; i < self->ob_size; i++) {
		PyObject *v = getarrayitem((PyObject *)self, i);
		if (v == NULL) {
			Py_DECREF(list);
			return NULL;
		}
		PyList_SetItem(list, i, v);
	}
	return list;
}

static char tolist_doc [] =
"tolist() -> list\n\
\n\
Convert array to an ordinary list with the same items.";


static PyObject *
array_fromstring(self, args)
	arrayobject *self;
	PyObject *args;
{
	char *str;
	int n;
	int itemsize = self->ob_descr->itemsize;
	if (!PyArg_Parse(args, "s#", &str, &n))
		return NULL;
	if (n % itemsize != 0) {
		PyErr_SetString(PyExc_ValueError,
			   "string length not a multiple of item size");
		return NULL;
	}
	n = n / itemsize;
	if (n > 0) {
		char *item = self->ob_item;
		PyMem_RESIZE(item, char, (self->ob_size + n) * itemsize);
		if (item == NULL) {
			PyErr_NoMemory();
			return NULL;
		}
		self->ob_item = item;
		self->ob_size += n;
		memcpy(item + (self->ob_size - n) * itemsize,
		       str, itemsize*n);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char fromstring_doc [] =
"fromstring(string)\n\
\n\
Appends items from the string, interpreting it as an array of machine\n\
values,as if it had been read from a file using the fromfile() method).";


static PyObject *
array_tostring(self, args)
	arrayobject *self;
	PyObject *args;
{
	if (!PyArg_Parse(args, ""))
		return NULL;
	return PyString_FromStringAndSize(self->ob_item,
				    self->ob_size * self->ob_descr->itemsize);
}

static char tostring_doc [] =
"tostring() -> string\n\
\n\
Convert the array to an array of machine values and return the string\n\
representation.";

PyMethodDef array_methods[] = {
	{"append",	(PyCFunction)array_append, 0, append_doc},
	{"buffer_info", (PyCFunction)array_buffer_info, 0, buffer_info_doc},
	{"byteswap",	(PyCFunction)array_byteswap, 0, byteswap_doc},
/*	{"count",	(method)array_count},*/
	{"fromfile",	(PyCFunction)array_fromfile, 0, fromfile_doc},
	{"fromlist",	(PyCFunction)array_fromlist, 0, fromlist_doc},
	{"fromstring",	(PyCFunction)array_fromstring, 0, fromstring_doc},
/*	{"index",	(method)array_index},*/
	{"insert",	(PyCFunction)array_insert, 0, insert_doc},
	{"read",	(PyCFunction)array_fromfile, 0, fromfile_doc},
/*	{"remove",	(method)array_remove},*/
	{"reverse",	(PyCFunction)array_reverse, 0, reverse_doc},
/*	{"sort",	(method)array_sort},*/
	{"tofile",	(PyCFunction)array_tofile, 0, tofile_doc},
	{"tolist",	(PyCFunction)array_tolist, 0, tolist_doc},
	{"tostring",	(PyCFunction)array_tostring, 0, tostring_doc},
	{"write",	(PyCFunction)array_tofile, 0, tofile_doc},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
array_getattr(a, name)
	arrayobject *a;
	char *name;
{
	if (strcmp(name, "typecode") == 0) {
		char tc = a->ob_descr->typecode;
		return PyString_FromStringAndSize(&tc, 1);
	}
	if (strcmp(name, "itemsize") == 0) {
		return PyInt_FromLong((long)a->ob_descr->itemsize);
	}
	if (strcmp(name, "__members__") == 0) {
		PyObject *list = PyList_New(2);
		if (list) {
			PyList_SetItem(list, 0,
				       PyString_FromString("typecode"));
			PyList_SetItem(list, 1,
				       PyString_FromString("itemsize"));
			if (PyErr_Occurred()) {
				Py_DECREF(list);
				list = NULL;
			}
		}
		return list;
	}
	return Py_FindMethod(array_methods, (PyObject *)a, name);
}

static int
array_print(a, fp, flags)
	arrayobject *a;
	FILE *fp;
	int flags;
{
	int ok = 0;
	int i, len;
	PyObject *v;
	len = a->ob_size;
	if (len == 0) {
		fprintf(fp, "array('%c')", a->ob_descr->typecode);
		return ok;
	}
	if (a->ob_descr->typecode == 'c') {
		fprintf(fp, "array('c', ");
		v = array_tostring(a, (PyObject *)NULL);
		ok = PyObject_Print(v, fp, 0);
		Py_XDECREF(v);
		fprintf(fp, ")");
		return ok;
	}
	fprintf(fp, "array('%c', [", a->ob_descr->typecode);
	for (i = 0; i < len && ok == 0; i++) {
		if (i > 0)
			fprintf(fp, ", ");
		v = (a->ob_descr->getitem)(a, i);
		ok = PyObject_Print(v, fp, 0);
		Py_XDECREF(v);
	}
	fprintf(fp, "])");
	return ok;
}

static PyObject *
array_repr(a)
	arrayobject *a;
{
	char buf[256];
	PyObject *s, *t, *comma, *v;
	int i, len;
	len = a->ob_size;
	if (len == 0) {
		sprintf(buf, "array('%c')", a->ob_descr->typecode);
		return PyString_FromString(buf);
	}
	if (a->ob_descr->typecode == 'c') {
		sprintf(buf, "array('c', ");
		s = PyString_FromString(buf);
		v = array_tostring(a, (PyObject *)NULL);
		t = PyObject_Repr(v);
		Py_XDECREF(v);
		PyString_ConcatAndDel(&s, t);
		PyString_ConcatAndDel(&s, PyString_FromString(")"));
		return s;
	}
	sprintf(buf, "array('%c', [", a->ob_descr->typecode);
	s = PyString_FromString(buf);
	comma = PyString_FromString(", ");
	for (i = 0; i < len && !PyErr_Occurred(); i++) {
		if (i > 0)
			PyString_Concat(&s, comma);
		v = (a->ob_descr->getitem)(a, i);
		t = PyObject_Repr(v);
		Py_XDECREF(v);
		PyString_ConcatAndDel(&s, t);
	}
	Py_XDECREF(comma);
	PyString_ConcatAndDel(&s, PyString_FromString("])"));
	return s;
}

static int
array_buffer_getreadbuf(self, index, ptr)
	arrayobject *self;
	int index;
	const void **ptr;
{
	if ( index != 0 ) {
		PyErr_SetString(PyExc_SystemError,
				"Accessing non-existent array segment");
		return -1;
	}
	*ptr = (void *)self->ob_item;
	return self->ob_size*self->ob_descr->itemsize;
}

static int
array_buffer_getwritebuf(self, index, ptr)
	arrayobject *self;
	int index;
	const void **ptr;
{
	if ( index != 0 ) {
		PyErr_SetString(PyExc_SystemError,
				"Accessing non-existent array segment");
		return -1;
	}
	*ptr = (void *)self->ob_item;
	return self->ob_size*self->ob_descr->itemsize;
}

static int
array_buffer_getsegcount(self, lenp)
	arrayobject *self;
	int *lenp;
{
	if ( lenp )
		*lenp = self->ob_size*self->ob_descr->itemsize;
	return 1;
}

static PySequenceMethods array_as_sequence = {
	(inquiry)array_length,		        /*sq_length*/
	(binaryfunc)array_concat,               /*sq_concat*/
	(intargfunc)array_repeat,		/*sq_repeat*/
	(intargfunc)array_item,		        /*sq_item*/
	(intintargfunc)array_slice,		/*sq_slice*/
	(intobjargproc)array_ass_item,		/*sq_ass_item*/
	(intintobjargproc)array_ass_slice,	/*sq_ass_slice*/
};

static PyBufferProcs array_as_buffer = {
	(getreadbufferproc)array_buffer_getreadbuf,
	(getwritebufferproc)array_buffer_getwritebuf,
	(getsegcountproc)array_buffer_getsegcount,
};




static PyObject *
a_array(self, args)
	PyObject *self;
	PyObject *args;
{
	char c;
	PyObject *initial = NULL;
	struct arraydescr *descr;
	if (!PyArg_Parse(args, "c", &c)) {
		PyErr_Clear();
		if (!PyArg_Parse(args, "(cO)", &c, &initial))
			return NULL;
		if (!PyList_Check(initial) && !PyString_Check(initial)) {
			PyErr_SetString(PyExc_TypeError,
				 "array initializer must be list or string");
			return NULL;
		}
	}
	for (descr = descriptors; descr->typecode != '\0'; descr++) {
		if (descr->typecode == c) {
			PyObject *a;
			int len;
			if (initial == NULL || !PyList_Check(initial))
				len = 0;
			else
				len = PyList_Size(initial);
			a = newarrayobject(len, descr);
			if (a == NULL)
				return NULL;
			if (len > 0) {
				int i;
				for (i = 0; i < len; i++) {
					PyObject *v =
					         PyList_GetItem(initial, i);
					if (setarrayitem(a, i, v) != 0) {
						Py_DECREF(a);
						return NULL;
					}
				}
			}
			if (initial != NULL && PyString_Check(initial)) {
				PyObject *v =
				  array_fromstring((arrayobject *)a, initial);
				if (v == NULL) {
					Py_DECREF(a);
					return NULL;
				}
				Py_DECREF(v);
			}
			return a;
		}
	}
	PyErr_SetString(PyExc_ValueError,
		"bad typecode (must be c, b, B, h, H, i, I, l, L, f or d)");
	return NULL;
}

static char a_array_doc [] =
"array(typecode [, initializer]) -> array\n\
\n\
Return a new array whose items are restricted by typecode, and\n\
initialized from the optional initializer value, which must be a list\n\
or a string.";

static PyMethodDef a_methods[] = {
	{"array",	a_array, 0, a_array_doc},
	{NULL,		NULL}		/* sentinel */
};

static char module_doc [] =
"This module defines a new object type which can efficiently represent\n\
an array of basic values: characters, integers, floating point\n\
numbers.  Arrays are sequence types and behave very much like lists,\n\
except that the type of objects stored in them is constrained.  The\n\
type is specified at object creation time by using a type code, which\n\
is a single character.  The following type codes are defined:\n\
\n\
    Type code   C Type             Minimum size in bytes \n\
    'c'         character          1 \n\
    'b'         signed integer     1 \n\
    'B'         unsigned integer   1 \n\
    'h'         signed integer     2 \n\
    'H'         unsigned integer   2 \n\
    'i'         signed integer     2 \n\
    'I'         unsigned integer   2 \n\
    'l'         signed integer     4 \n\
    'L'         unsigned integer   4 \n\
    'f'         floating point     4 \n\
    'd'         floating point     8 \n\
\n\
Functions:\n\
\n\
array(typecode [, initializer]) -- create a new array\n\
\n\
Special Objects:\n\
\n\
ArrayType -- type object for array objects\n\
";

static char arraytype_doc [] =
"An array represents basic values and behave very much like lists, except\n\
the type of objects stored in them is constrained.\n\
\n\
Methods:\n\
\n\
append() -- append a new item to the end of the array\n\
buffer_info() -- return information giving the current memory info\n\
byteswap() -- byteswap all the items of the array\n\
fromfile() -- read items from a file object\n\
fromlist() -- append items from the list\n\
fromstring() -- append items from the string\n\
insert() -- insert a new item into the array at a provided position\n\
read() -- DEPRECATED, use fromfile()\n\
reverse() -- reverse the order of the items in the array\n\
tofile() -- write all items to a file object\n\
tolist() -- return the array converted to an ordinary list\n\
tostring() -- return the array converted to a string\n\
write() -- DEPRECATED, use tofile()\n\
\n\
Variables:\n\
\n\
typecode -- the typecode character used to create the array\n\
itemsize -- the length in bytes of one array item\n\
";

statichere PyTypeObject Arraytype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"array",
	sizeof(arrayobject),
	0,
	(destructor)array_dealloc,	/*tp_dealloc*/
	(printfunc)array_print,		/*tp_print*/
	(getattrfunc)array_getattr,	/*tp_getattr*/
	0,				/*tp_setattr*/
	(cmpfunc)array_compare,		/*tp_compare*/
	(reprfunc)array_repr,		/*tp_repr*/
	0,				/*tp_as_number*/
	&array_as_sequence,		/*tp_as_sequence*/
	0,				/*tp_as_mapping*/
	0, 				/*tp_hash*/
	0,				/*tp_call*/
	0,				/*tp_str*/
	0,				/*tp_getattro*/
	0,				/*tp_setattro*/
	&array_as_buffer,		/*tp_as_buffer*/
	0,				/*tp_xxx4*/
	arraytype_doc,			/*tp_doc*/
};

void
initarray()
{
	PyObject *m, *d;
	m = Py_InitModule3("array", a_methods, module_doc);
	d = PyModule_GetDict(m);
	if (PyDict_SetItemString(d, "ArrayType",
				 (PyObject *)&Arraytype) != 0)
		Py_FatalError("can't define array.ArrayType");
}
