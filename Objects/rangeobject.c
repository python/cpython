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

/* Range object implementation */

#include "Python.h"

typedef struct {
	PyObject_HEAD
	long	start;
	long	step;
	long	len;
	int	reps;
} rangeobject;


PyObject *
PyRange_New(start, len, step, reps)
	long start, len, step;
	int reps;
{
	rangeobject *obj = PyObject_NEW(rangeobject, &PyRange_Type);

	obj->start = start;
	obj->len   = len;
	obj->step  = step;
	obj->reps  = reps;

	return (PyObject *) obj;
}

static void
range_dealloc(r)
	rangeobject *r;
{
	PyMem_DEL(r);
}

static PyObject *
range_item(r, i)
	rangeobject *r;
	int i;
{
	if (i < 0 || i >= r->len * r->reps) {
		PyErr_SetString(PyExc_IndexError,
				"xrange object index out of range");
		return NULL;
	}

	return PyInt_FromLong(r->start + (i % r->len) * r->step);
}

static int
range_length(r)
	rangeobject *r;
{
	return r->len * r->reps;
}

static int
range_print(r, fp, flags)
	rangeobject *r;
	FILE *fp;
	int flags;
{
	int i, j;

	fprintf(fp, "(");
	for (i = 0; i < r->reps; ++i)
		for (j = 0; j < r->len; ++j) {
			if (j > 0 || i > 0)
				fprintf(fp, ", ");

			fprintf(fp, "%ld", r->start + j * r->step);
		}

	if (r->len == 1 && r->reps == 1)
		fprintf(fp, ",");
	fprintf(fp, ")");
	return 0;
}

static PyObject *
range_repr(r)
	rangeobject *r;
{
	char buf[80];
	sprintf(buf, "(xrange(%ld, %ld, %ld) * %d)",
			r->start,
			r->start + r->len * r->step,
			r->step,
			r->reps);
	return PyString_FromString(buf);
}

static PyObject *
range_concat(r, obj)
	rangeobject *r;
	PyObject *obj;
{
	PyErr_SetString(PyExc_TypeError, "cannot concatenate xrange objects");
	return NULL;
}

static PyObject *
range_repeat(r, n)
	rangeobject *r;
	int n;
{
	if (n < 0)
		return (PyObject *) PyRange_New(0, 0, 1, 1);

	else if (n == 1) {
		Py_INCREF(r);
		return (PyObject *) r;
	}

	else
		return (PyObject *) PyRange_New(
						r->start,
						r->len,
						r->step,
						r->reps * n);
}

static int
range_compare(r1, r2)
	rangeobject *r1, *r2;
{
	if (r1->start != r2->start)
		return r1->start - r2->start;

	else if (r1->step != r2->step)
		return r1->step - r2->step;

	else if (r1->len != r2->len)
		return r1->len - r2->len;

	else
		return r1->reps - r2->reps;
}

static PyObject *
range_slice(r, low, high)
	rangeobject *r;
	int low, high;
{
	if (r->reps != 1) {
		PyErr_SetString(PyExc_TypeError,
				"cannot slice a replicated xrange");
		return NULL;
	}
	if (low < 0)
		low = 0;
	else if (low > r->len)
		low = r->len;
	if (high < 0)
		high = 0;
	if (high < low)
		high = low;
	else if (high > r->len)
		high = r->len;

	if (low == 0 && high == r->len) {
		Py_INCREF(r);
		return (PyObject *) r;
	}

	return (PyObject *) PyRange_New(
				low * r->step + r->start,
				high - low,
				r->step,
				1);
}

static PyObject *
range_tolist(self, args)
rangeobject *self;
PyObject *args;
{
	PyObject *thelist;
	int j;
	int len = self->len * self->reps;

	if (! PyArg_Parse(args, ""))
		return NULL;

	if ((thelist = PyList_New(len)) == NULL)
		return NULL;

	for (j = 0; j < len; ++j)
		if ((PyList_SetItem(thelist, j, (PyObject *) PyInt_FromLong(
			self->start + (j % self->len) * self->step))) < 0)
			return NULL;

	return thelist;
}

static PyObject *
range_getattr(r, name)
	rangeobject *r;
	char *name;
{
	static PyMethodDef range_methods[] = {
		{"tolist",	(PyCFunction)range_tolist},
		{NULL,		NULL}
	};

	return Py_FindMethod(range_methods, (PyObject *) r, name);
}

static PySequenceMethods range_as_sequence = {
	(inquiry)range_length, /*sq_length*/
	(binaryfunc)range_concat, /*sq_concat*/
	(intargfunc)range_repeat, /*sq_repeat*/
	(intargfunc)range_item, /*sq_item*/
	(intintargfunc)range_slice, /*sq_slice*/
	0,		/*sq_ass_item*/
	0,		/*sq_ass_slice*/
};

PyTypeObject PyRange_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/* Number of items for varobject */
	"xrange",		/* Name of this type */
	sizeof(rangeobject),	/* Basic object size */
	0,			/* Item size for varobject */
	(destructor)range_dealloc, /*tp_dealloc*/
	(printfunc)range_print, /*tp_print*/
	(getattrfunc)range_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	(cmpfunc)range_compare, /*tp_compare*/
	(reprfunc)range_repr, /*tp_repr*/
	0,			/*tp_as_number*/
	&range_as_sequence,	/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
};
