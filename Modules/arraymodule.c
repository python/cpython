/* Array object implementation */

/* An array is a uniform list -- all items have the same type.
   The item type is restricted to simple C types like int or float */

#include "Python.h"

#ifdef STDC_HEADERS
#include <stddef.h>
#else /* !STDC_HEADERS */
#ifndef DONT_HAVE_SYS_TYPES_H
#include <sys/types.h>		/* For size_t */
#endif /* DONT_HAVE_SYS_TYPES_H */
#endif /* !STDC_HEADERS */

struct arrayobject; /* Forward */

/* All possible arraydescr values are defined in the vector "descriptors"
 * below.  That's defined later because the appropriate get and set
 * functions aren't visible yet.
 */
struct arraydescr {
	int typecode;
	int itemsize;
	PyObject * (*getitem)(struct arrayobject *, int);
	int (*setitem)(struct arrayobject *, int, PyObject *);
};

typedef struct arrayobject {
	PyObject_VAR_HEAD
	char *ob_item;
	struct arraydescr *ob_descr;
} arrayobject;

staticforward PyTypeObject Arraytype;

#define is_arrayobject(op) ((op)->ob_type == &Arraytype)

/****************************************************************************
Get and Set functions for each type.
A Get function takes an arrayobject* and an integer index, returning the
array value at that index wrapped in an appropriate PyObject*.
A Set function takes an arrayobject, integer index, and PyObject*; sets
the array value at that index to the raw C data extracted from the PyObject*,
and returns 0 if successful, else nonzero on failure (PyObject* not of an
appropriate type or value).
Note that the basic Get and Set functions do NOT check that the index is
in bounds; that's the responsibility of the caller.
****************************************************************************/

static PyObject *
c_getitem(arrayobject *ap, int i)
{
	return PyString_FromStringAndSize(&((char *)ap->ob_item)[i], 1);
}

static int
c_setitem(arrayobject *ap, int i, PyObject *v)
{
	char x;
	if (!PyArg_Parse(v, "c;array item must be char", &x))
		return -1;
	if (i >= 0)
		     ((char *)ap->ob_item)[i] = x;
	return 0;
}

static PyObject *
b_getitem(arrayobject *ap, int i)
{
	long x = ((char *)ap->ob_item)[i];
	if (x >= 128)
		x -= 256;
	return PyInt_FromLong(x);
}

static int
b_setitem(arrayobject *ap, int i, PyObject *v)
{
	short x;
	/* PyArg_Parse's 'b' formatter is for an unsigned char, therefore
	   must use the next size up that is signed ('h') and manually do
	   the overflow checking */
	if (!PyArg_Parse(v, "h;array item must be integer", &x))
		return -1;
	else if (x < -128) {
		PyErr_SetString(PyExc_OverflowError,
			"signed char is less than minimum");
		return -1;
	}
	else if (x > 127) {
		PyErr_SetString(PyExc_OverflowError,
			"signed char is greater than maximum");
		return -1;
	}
	if (i >= 0)
		((char *)ap->ob_item)[i] = (char)x;
	return 0;
}

static PyObject *
BB_getitem(arrayobject *ap, int i)
{
	long x = ((unsigned char *)ap->ob_item)[i];
	return PyInt_FromLong(x);
}

static int
BB_setitem(arrayobject *ap, int i, PyObject *v)
{
	unsigned char x;
	/* 'B' == unsigned char, maps to PyArg_Parse's 'b' formatter */
	if (!PyArg_Parse(v, "b;array item must be integer", &x))
		return -1;
	if (i >= 0)
		     ((char *)ap->ob_item)[i] = x;
	return 0;
}

static PyObject *
h_getitem(arrayobject *ap, int i)
{
	return PyInt_FromLong((long) ((short *)ap->ob_item)[i]);
}

static int
h_setitem(arrayobject *ap, int i, PyObject *v)
{
	short x;
	/* 'h' == signed short, maps to PyArg_Parse's 'h' formatter */
	if (!PyArg_Parse(v, "h;array item must be integer", &x))
		return -1;
	if (i >= 0)
		     ((short *)ap->ob_item)[i] = x;
	return 0;
}

static PyObject *
HH_getitem(arrayobject *ap, int i)
{
	return PyInt_FromLong((long) ((unsigned short *)ap->ob_item)[i]);
}

static int
HH_setitem(arrayobject *ap, int i, PyObject *v)
{
	int x;
	/* PyArg_Parse's 'h' formatter is for a signed short, therefore
	   must use the next size up and manually do the overflow checking */
	if (!PyArg_Parse(v, "i;array item must be integer", &x))
		return -1;
	else if (x < 0) {
		PyErr_SetString(PyExc_OverflowError,
			"unsigned short is less than minimum");
		return -1;
	}
	else if (x > USHRT_MAX) {
		PyErr_SetString(PyExc_OverflowError,
			"unsigned short is greater than maximum");
		return -1;
	}
	if (i >= 0)
		((short *)ap->ob_item)[i] = (short)x;
	return 0;
}

static PyObject *
i_getitem(arrayobject *ap, int i)
{
	return PyInt_FromLong((long) ((int *)ap->ob_item)[i]);
}

static int
i_setitem(arrayobject *ap, int i, PyObject *v)
{
	int x;
	/* 'i' == signed int, maps to PyArg_Parse's 'i' formatter */
	if (!PyArg_Parse(v, "i;array item must be integer", &x))
		return -1;
	if (i >= 0)
		     ((int *)ap->ob_item)[i] = x;
	return 0;
}

static PyObject *
II_getitem(arrayobject *ap, int i)
{
	return PyLong_FromUnsignedLong(
		(unsigned long) ((unsigned int *)ap->ob_item)[i]);
}

static int
II_setitem(arrayobject *ap, int i, PyObject *v)
{
	unsigned long x;
	if (PyLong_Check(v)) {
		x = PyLong_AsUnsignedLong(v);
		if (x == (unsigned long) -1 && PyErr_Occurred())
			return -1;
	}
	else {
		long y;
		if (!PyArg_Parse(v, "l;array item must be integer", &y))
			return -1;
		if (y < 0) {
			PyErr_SetString(PyExc_OverflowError,
				"unsigned int is less than minimum");
			return -1;
		}
		x = (unsigned long)y;

	}
	if (x > UINT_MAX) {
		PyErr_SetString(PyExc_OverflowError,
			"unsigned int is greater than maximum");
		return -1;
	}

	if (i >= 0)
		((unsigned int *)ap->ob_item)[i] = (unsigned int)x;
	return 0;
}

static PyObject *
l_getitem(arrayobject *ap, int i)
{
	return PyInt_FromLong(((long *)ap->ob_item)[i]);
}

static int
l_setitem(arrayobject *ap, int i, PyObject *v)
{
	long x;
	if (!PyArg_Parse(v, "l;array item must be integer", &x))
		return -1;
	if (i >= 0)
		     ((long *)ap->ob_item)[i] = x;
	return 0;
}

static PyObject *
LL_getitem(arrayobject *ap, int i)
{
	return PyLong_FromUnsignedLong(((unsigned long *)ap->ob_item)[i]);
}

static int
LL_setitem(arrayobject *ap, int i, PyObject *v)
{
	unsigned long x;
	if (PyLong_Check(v)) {
		x = PyLong_AsUnsignedLong(v);
		if (x == (unsigned long) -1 && PyErr_Occurred())
			return -1;
	}
	else {
		long y;
		if (!PyArg_Parse(v, "l;array item must be integer", &y))
			return -1;
		if (y < 0) {
			PyErr_SetString(PyExc_OverflowError,
				"unsigned long is less than minimum");
			return -1;
		}
		x = (unsigned long)y;

	}
	if (x > ULONG_MAX) {
		PyErr_SetString(PyExc_OverflowError,
			"unsigned long is greater than maximum");
		return -1;
	}

	if (i >= 0)
		((unsigned long *)ap->ob_item)[i] = x;
	return 0;
}

static PyObject *
f_getitem(arrayobject *ap, int i)
{
	return PyFloat_FromDouble((double) ((float *)ap->ob_item)[i]);
}

static int
f_setitem(arrayobject *ap, int i, PyObject *v)
{
	float x;
	if (!PyArg_Parse(v, "f;array item must be float", &x))
		return -1;
	if (i >= 0)
		     ((float *)ap->ob_item)[i] = x;
	return 0;
}

static PyObject *
d_getitem(arrayobject *ap, int i)
{
	return PyFloat_FromDouble(((double *)ap->ob_item)[i]);
}

static int
d_setitem(arrayobject *ap, int i, PyObject *v)
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

/****************************************************************************
Implementations of array object methods.
****************************************************************************/

static PyObject *
newarrayobject(int size, struct arraydescr *descr)
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
	op = PyObject_NewVar(arrayobject, &Arraytype, size);
	if (op == NULL) {
		return PyErr_NoMemory();
	}
	if (size <= 0) {
		op->ob_item = NULL;
	}
	else {
		op->ob_item = PyMem_NEW(char, nbytes);
		if (op->ob_item == NULL) {
			PyObject_Del(op);
			return PyErr_NoMemory();
		}
	}
	op->ob_descr = descr;
	return (PyObject *) op;
}

static PyObject *
getarrayitem(PyObject *op, int i)
{
	register arrayobject *ap;
	assert(is_arrayobject(op));
	ap = (arrayobject *)op;
	if (i < 0 || i >= ap->ob_size) {
		PyErr_SetString(PyExc_IndexError, "array index out of range");
		return NULL;
	}
	return (*ap->ob_descr->getitem)(ap, i);
}

static int
ins1(arrayobject *self, int where, PyObject *v)
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

/* Methods */

static void
array_dealloc(arrayobject *op)
{
	if (op->ob_item != NULL)
		PyMem_DEL(op->ob_item);
	PyObject_Del(op);
}

static int
array_compare(arrayobject *v, arrayobject *w)
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
array_length(arrayobject *a)
{
	return a->ob_size;
}

static PyObject *
array_item(arrayobject *a, int i)
{
	if (i < 0 || i >= a->ob_size) {
		PyErr_SetString(PyExc_IndexError, "array index out of range");
		return NULL;
	}
	return getarrayitem((PyObject *)a, i);
}

static PyObject *
array_slice(arrayobject *a, int ilow, int ihigh)
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
array_concat(arrayobject *a, PyObject *bb)
{
	int size;
	arrayobject *np;
	if (!is_arrayobject(bb)) {
		PyErr_Format(PyExc_TypeError,
		     "can only append array (not \"%.200s\") to array",
			     bb->ob_type->tp_name);
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
array_repeat(arrayobject *a, int n)
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
array_ass_slice(arrayobject *a, int ilow, int ihigh, PyObject *v)
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
		PyErr_Format(PyExc_TypeError,
	     "can only assign array (not \"%.200s\") to array slice",
			     v->ob_type->tp_name);
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
array_ass_item(arrayobject *a, int i, PyObject *v)
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
setarrayitem(PyObject *a, int i, PyObject *v)
{
	assert(is_arrayobject(a));
	return array_ass_item((arrayobject *)a, i, v);
}

static PyObject *
ins(arrayobject *self, int where, PyObject *v)
{
	if (ins1(self, where, v) != 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
array_count(arrayobject *self, PyObject *args)
{
	int count = 0;
	int i;
	PyObject *v;

        if (!PyArg_ParseTuple(args, "O:count", &v))
		return NULL;
	for (i = 0; i < self->ob_size; i++) {
		PyObject *selfi = getarrayitem((PyObject *)self, i);
		if (PyObject_Compare(selfi, v) == 0)
			count++;
		Py_DECREF(selfi);
		if (PyErr_Occurred())
			return NULL;
	}
	return PyInt_FromLong((long)count);
}

static char count_doc [] =
"count(x)\n\
\n\
Return number of occurences of x in the array.";

static PyObject *
array_index(arrayobject *self, PyObject *args)
{
	int i;
	PyObject *v;

	if (!PyArg_ParseTuple(args, "O:index", &v))
		return NULL;
	for (i = 0; i < self->ob_size; i++) {
		PyObject *selfi = getarrayitem((PyObject *)self, i);
		if (PyObject_Compare(selfi, v) == 0) {
			Py_DECREF(selfi);
			return PyInt_FromLong((long)i);
		}
		Py_DECREF(selfi);
		if (PyErr_Occurred())
			return NULL;
	}
	PyErr_SetString(PyExc_ValueError, "array.index(x): x not in list");
	return NULL;
}

static char index_doc [] =
"index(x)\n\
\n\
Return index of first occurence of x in the array.";

static PyObject *
array_remove(arrayobject *self, PyObject *args)
{
	int i;
	PyObject *v;

        if (!PyArg_ParseTuple(args, "O:remove", &v))
		return NULL;
	for (i = 0; i < self->ob_size; i++) {
		PyObject *selfi = getarrayitem((PyObject *)self,i);
		if (PyObject_Compare(selfi, v) == 0) {
			Py_DECREF(selfi);
			if (array_ass_slice(self, i, i+1,
					   (PyObject *)NULL) != 0)
				return NULL;
			Py_INCREF(Py_None);
			return Py_None;
		}
		Py_DECREF(selfi);
		if (PyErr_Occurred())
			return NULL;
	}
	PyErr_SetString(PyExc_ValueError, "array.remove(x): x not in list");
	return NULL;
}

static char remove_doc [] =
"remove(x)\n\
\n\
Remove the first occurence of x in the array.";

static PyObject *
array_pop(arrayobject *self, PyObject *args)
{
	int i = -1;
	PyObject *v;
	if (!PyArg_ParseTuple(args, "|i:pop", &i))
		return NULL;
	if (self->ob_size == 0) {
		/* Special-case most common failure cause */
		PyErr_SetString(PyExc_IndexError, "pop from empty array");
		return NULL;
	}
	if (i < 0)
		i += self->ob_size;
	if (i < 0 || i >= self->ob_size) {
		PyErr_SetString(PyExc_IndexError, "pop index out of range");
		return NULL;
	}
	v = getarrayitem((PyObject *)self,i);
	if (array_ass_slice(self, i, i+1, (PyObject *)NULL) != 0) {
		Py_DECREF(v);
		return NULL;
	}
	return v;
}

static char pop_doc [] =
"pop([i])\n\
\n\
Return the i-th element and delete it from the array. i defaults to -1.";

static PyObject *
array_extend(arrayobject *self, PyObject *args)
{
	int size;
        PyObject    *bb;

	if (!PyArg_ParseTuple(args, "O:extend", &bb))
            return NULL;

	if (!is_arrayobject(bb)) {
		PyErr_Format(PyExc_TypeError,
			     "can only extend array with array (not \"%.200s\")",
			     bb->ob_type->tp_name);
		return NULL;
	}
#define b ((arrayobject *)bb)
	if (self->ob_descr != b->ob_descr) {
		PyErr_SetString(PyExc_TypeError,
			     "can only extend with array of same kind");
		return NULL;
	}
	size = self->ob_size + b->ob_size;
        PyMem_RESIZE(self->ob_item, char, size*self->ob_descr->itemsize);
        if (self->ob_item == NULL) {
                PyObject_Del(self);
                return PyErr_NoMemory();
        }
	memcpy(self->ob_item + self->ob_size*self->ob_descr->itemsize,
               b->ob_item, b->ob_size*b->ob_descr->itemsize);
        self->ob_size = size;
        Py_INCREF(Py_None);
	return Py_None;
#undef b
}

static char extend_doc [] =
"extend(array)\n\
\n\
 Append array items to the end of the array.";

static PyObject *
array_insert(arrayobject *self, PyObject *args)
{
	int i;
	PyObject *v;
        if (!PyArg_ParseTuple(args, "iO:insert", &i, &v))
		return NULL;
	return ins(self, i, v);
}

static char insert_doc [] =
"insert(i,x)\n\
\n\
Insert a new item x into the array before position i.";


static PyObject *
array_buffer_info(arrayobject *self, PyObject *args)
{
	PyObject* retval = NULL;
        if (!PyArg_ParseTuple(args, ":buffer_info"))
                return NULL;
	retval = PyTuple_New(2);
	if (!retval)
		return NULL;

	PyTuple_SET_ITEM(retval, 0, PyLong_FromVoidPtr(self->ob_item));
	PyTuple_SET_ITEM(retval, 1, PyInt_FromLong((long)(self->ob_size)));

	return retval;
}

static char buffer_info_doc [] =
"buffer_info() -> (address, length)\n\
\n\
Return a tuple (address, length) giving the current memory address and\n\
the length in bytes of the buffer used to hold array's contents.";


static PyObject *
array_append(arrayobject *self, PyObject *args)
{
	PyObject *v;
        if (!PyArg_ParseTuple(args, "O:append", &v))
		return NULL;
	return ins(self, (int) self->ob_size, v);
}

static char append_doc [] =
"append(x)\n\
\n\
Append new value x to the end of the array.";


static PyObject *
array_byteswap(arrayobject *self, PyObject *args)
{
	char *p;
	int i;

        if (!PyArg_ParseTuple(args, ":byteswap"))
                return NULL;

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
"byteswap()\n\
\n\
Byteswap all items of the array.  If the items in the array are not 1, 2,\n\
4, or 8 bytes in size, RuntimeError is raised.";

static PyObject *
array_reverse(arrayobject *self, PyObject *args)
{
	register int itemsize = self->ob_descr->itemsize;
	register char *p, *q;
	/* little buffer to hold items while swapping */
	char tmp[256];	/* 8 is probably enough -- but why skimp */
	assert(itemsize <= sizeof(tmp));

        if (!PyArg_ParseTuple(args, ":reverse"))
                return NULL;

	if (self->ob_size > 1) {
		for (p = self->ob_item,
		     q = self->ob_item + (self->ob_size - 1)*itemsize;
		     p < q;
		     p += itemsize, q -= itemsize) {
			/* memory areas guaranteed disjoint, so memcpy
			 * is safe (& memmove may be slower).
			 */
			memcpy(tmp, p, itemsize);
			memcpy(p, q, itemsize);
			memcpy(q, tmp, itemsize);
		}
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static char reverse_doc [] =
"reverse()\n\
\n\
Reverse the order of the items in the array.";

static PyObject *
array_fromfile(arrayobject *self, PyObject *args)
{
	PyObject *f;
	int n;
	FILE *fp;
        if (!PyArg_ParseTuple(args, "Oi:fromfile", &f, &n))
		return NULL;
	fp = PyFile_AsFile(f);
	if (fp == NULL) {
		PyErr_SetString(PyExc_TypeError, "arg1 must be open file");
		return NULL;
	}
	if (n > 0) {
		char *item = self->ob_item;
		int itemsize = self->ob_descr->itemsize;
		size_t nread;
		int newlength;
		size_t newbytes;
		/* Be careful here about overflow */
		if ((newlength = self->ob_size + n) <= 0 ||
		    (newbytes = newlength * itemsize) / itemsize !=
		    (size_t)newlength)
			goto nomem;
		PyMem_RESIZE(item, char, newbytes);
		if (item == NULL) {
		  nomem:
			PyErr_NoMemory();
			return NULL;
		}
		self->ob_item = item;
		self->ob_size += n;
		nread = fread(item + (self->ob_size - n) * itemsize,
			      itemsize, n, fp);
		if (nread < (size_t)n) {
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
array_tofile(arrayobject *self, PyObject *args)
{
	PyObject *f;
	FILE *fp;
        if (!PyArg_ParseTuple(args, "O:tofile", &f))
		return NULL;
	fp = PyFile_AsFile(f);
	if (fp == NULL) {
		PyErr_SetString(PyExc_TypeError, "arg must be open file");
		return NULL;
	}
	if (self->ob_size > 0) {
		if (fwrite(self->ob_item, self->ob_descr->itemsize,
			   self->ob_size, fp) != (size_t)self->ob_size) {
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
array_fromlist(arrayobject *self, PyObject *args)
{
	int n;
	PyObject *list;
	int itemsize = self->ob_descr->itemsize;
        if (!PyArg_ParseTuple(args, "O:fromlist", &list))
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
array_tolist(arrayobject *self, PyObject *args)
{
	PyObject *list = PyList_New(self->ob_size);
	int i;
        if (!PyArg_ParseTuple(args, ":tolist"))
                return NULL;
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
array_fromstring(arrayobject *self, PyObject *args)
{
	char *str;
	int n;
	int itemsize = self->ob_descr->itemsize;
        if (!PyArg_ParseTuple(args, "s#:fromstring", &str, &n))
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
array_tostring(arrayobject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":tostring"))
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
	{"append",	(PyCFunction)array_append, METH_VARARGS, append_doc},
	{"buffer_info", (PyCFunction)array_buffer_info, METH_VARARGS, buffer_info_doc},
	{"byteswap",	(PyCFunction)array_byteswap, METH_VARARGS, byteswap_doc},
	{"count",	(PyCFunction)array_count, METH_VARARGS, count_doc},
	{"extend",      (PyCFunction)array_extend, METH_VARARGS, extend_doc},
	{"fromfile",	(PyCFunction)array_fromfile, METH_VARARGS, fromfile_doc},
	{"fromlist",	(PyCFunction)array_fromlist, METH_VARARGS, fromlist_doc},
	{"fromstring",	(PyCFunction)array_fromstring, METH_VARARGS, fromstring_doc},
	{"index",	(PyCFunction)array_index, METH_VARARGS, index_doc},
	{"insert",	(PyCFunction)array_insert, METH_VARARGS, insert_doc},
	{"pop",		(PyCFunction)array_pop, METH_VARARGS, pop_doc},
	{"read",	(PyCFunction)array_fromfile, METH_VARARGS, fromfile_doc},
	{"remove",	(PyCFunction)array_remove, METH_VARARGS, remove_doc},
	{"reverse",	(PyCFunction)array_reverse, METH_VARARGS, reverse_doc},
/*	{"sort",	(PyCFunction)array_sort, METH_VARARGS, sort_doc},*/
	{"tofile",	(PyCFunction)array_tofile, METH_VARARGS, tofile_doc},
	{"tolist",	(PyCFunction)array_tolist, METH_VARARGS, tolist_doc},
	{"tostring",	(PyCFunction)array_tostring, METH_VARARGS, tostring_doc},
	{"write",	(PyCFunction)array_tofile, METH_VARARGS, tofile_doc},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
array_getattr(arrayobject *a, char *name)
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
array_print(arrayobject *a, FILE *fp, int flags)
{
	int ok = 0;
	int i, len;
	PyObject *t_empty = PyTuple_New(0);
	PyObject *v;
	len = a->ob_size;
	if (len == 0) {
		fprintf(fp, "array('%c')", a->ob_descr->typecode);
		return ok;
	}
	if (a->ob_descr->typecode == 'c') {
		fprintf(fp, "array('c', ");
		v = array_tostring(a, t_empty);
		Py_DECREF(t_empty);;
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
array_repr(arrayobject *a)
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
array_buffer_getreadbuf(arrayobject *self, int index, const void **ptr)
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
array_buffer_getwritebuf(arrayobject *self, int index, const void **ptr)
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
array_buffer_getsegcount(arrayobject *self, int *lenp)
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
a_array(PyObject *self, PyObject *args)
{
	char c;
	PyObject *initial = NULL;
	struct arraydescr *descr;
        if (!PyArg_ParseTuple(args, "c:array", &c)) {
		PyErr_Clear();
                if (!PyArg_ParseTuple(args, "cO:array", &c, &initial))
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
				PyObject *t_initial = Py_BuildValue("(O)", initial);
				PyObject *v =
					array_fromstring((arrayobject *)a, t_initial);
                                Py_DECREF(t_initial);
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
	{"array",	a_array, METH_VARARGS, a_array_doc},
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
count() -- return number of occurences of an object\n\
extend() -- extend array by appending array elements\n\
fromfile() -- read items from a file object\n\
fromlist() -- append items from the list\n\
fromstring() -- append items from the string\n\
index() -- return index of first occurence of an object\n\
insert() -- insert a new item into the array at a provided position\n\
pop() -- remove and return item (default last)\n\
read() -- DEPRECATED, use fromfile()\n\
remove() -- remove first occurence of an object\n\
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
	PyObject_HEAD_INIT(NULL)
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

DL_EXPORT(void)
initarray(void)
{
	PyObject *m, *d;

        Arraytype.ob_type = &PyType_Type;
	m = Py_InitModule3("array", a_methods, module_doc);
	d = PyModule_GetDict(m);
	PyDict_SetItemString(d, "ArrayType", (PyObject *)&Arraytype);
	/* No need to check the error here, the caller will do that */
}
