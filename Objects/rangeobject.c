
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
PyRange_New(long start, long len, long step, int reps)
{
	rangeobject *obj = PyObject_NEW(rangeobject, &PyRange_Type);

	obj->start = start;
	obj->len   = len;
	obj->step  = step;
	obj->reps  = reps;

	return (PyObject *) obj;
}

static void
range_dealloc(rangeobject *r)
{
	PyObject_DEL(r);
}

static PyObject *
range_item(rangeobject *r, int i)
{
	if (i < 0 || i >= r->len * r->reps) {
		PyErr_SetString(PyExc_IndexError,
				"xrange object index out of range");
		return NULL;
	}

	return PyInt_FromLong(r->start + (i % r->len) * r->step);
}

static int
range_length(rangeobject *r)
{
	return r->len * r->reps;
}

static PyObject *
range_repr(rangeobject *r)
{
	/* buffers must be big enough to hold 3 longs + an int +
	 * a bit of "(xrange(...) * ...)" text.
	 */
	char buf1[250];
	char buf2[250];

	if (r->start == 0 && r->step == 1)
		sprintf(buf1, "xrange(%ld)", r->start + r->len * r->step);

	else if (r->step == 1)
		sprintf(buf1, "xrange(%ld, %ld)",
			r->start,
			r->start + r->len * r->step);

	else
		sprintf(buf1, "xrange(%ld, %ld, %ld)",
			r->start,
			r->start + r->len * r->step,
			r->step);

	if (r->reps != 1)
		sprintf(buf2, "(%s * %d)", buf1, r->reps);

	return PyString_FromString(r->reps == 1 ? buf1 : buf2);
}

static PyObject *
range_concat(rangeobject *r, PyObject *obj)
{
	PyErr_SetString(PyExc_TypeError, "cannot concatenate xrange objects");
	return NULL;
}

static PyObject *
range_repeat(rangeobject *r, int n)
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
range_compare(rangeobject *r1, rangeobject *r2)
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
range_slice(rangeobject *r, int low, int high)
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
range_tolist(rangeobject *self, PyObject *args)
{
	PyObject *thelist;
	int j;
	int len = self->len * self->reps;

	if (! PyArg_ParseTuple(args, ":tolist"))
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
range_getattr(rangeobject *r, char *name)
{
	static PyMethodDef range_methods[] = {
		{"tolist",	(PyCFunction)range_tolist, METH_VARARGS,
                 "tolist() -> list\n"
                 "Return a list object with the same values."},
		{NULL,		NULL}
	};

	return Py_FindMethod(range_methods, (PyObject *) r, name);
}

static int
range_contains(rangeobject *r, PyObject *obj)
{
	long num = PyInt_AsLong(obj);

	if (num < 0 && PyErr_Occurred())
		return -1;

	if (num < r->start || (num - r->start) % r->step)
		return 0;
	if (num > (r->start + (r->len * r->step)))
		return 0;
	return 1;
}

static PySequenceMethods range_as_sequence = {
	(inquiry)range_length, /*sq_length*/
	(binaryfunc)range_concat, /*sq_concat*/
	(intargfunc)range_repeat, /*sq_repeat*/
	(intargfunc)range_item, /*sq_item*/
	(intintargfunc)range_slice, /*sq_slice*/
	0,		/*sq_ass_item*/
	0,		/*sq_ass_slice*/
	(objobjproc)range_contains, /*sq_contains*/
};

PyTypeObject PyRange_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/* Number of items for varobject */
	"xrange",		/* Name of this type */
	sizeof(rangeobject),	/* Basic object size */
	0,			/* Item size for varobject */
	(destructor)range_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)range_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	(cmpfunc)range_compare, /*tp_compare*/
	(reprfunc)range_repr,	/*tp_repr*/
	0,			/*tp_as_number*/
	&range_as_sequence,	/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,			/*tp_call*/
	0,			/*tp_str*/
	0,			/*tp_getattro*/
	0,			/*tp_setattro*/
	0,			/*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,	/*tp_flags*/
};
