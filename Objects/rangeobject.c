/* Range object implementation */

#include "Python.h"

typedef struct {
	PyObject_HEAD
	long	start;
	long	step;
	long	len;
} rangeobject;

/* XXX PyRange_New should be deprecated.  It's not documented.  It's not
 * used in the core.  Its error-checking is akin to Swiss cheese:  accepts
 * step == 0; accepts len < 0; ignores that (len - 1) * step may overflow;
 * raises a baffling "integer addition" exception if it thinks the last
 * item is "too big"; and doesn't compute whether "last item is too big"
 * correctly even if the multiplication doesn't overflow.
 */
PyObject *
PyRange_New(long start, long len, long step, int reps)
{
	rangeobject *obj;

	if (reps != 1) {
		PyErr_SetString(PyExc_ValueError,
			"PyRange_New's 'repetitions' argument must be 1");
		return NULL;
	}

	obj = PyObject_New(rangeobject, &PyRange_Type);
	if (obj == NULL)
		return NULL;

	if (len == 0) {
		start = 0;
		len = 0;
		step = 1;
	}
	else {
		long last = start + (len - 1) * step;
		if ((step > 0) ?
		    (last > (PyInt_GetMax() - step)) :
		    (last < (-1 - PyInt_GetMax() - step))) {
			PyErr_SetString(PyExc_OverflowError,
					"integer addition");
			Py_DECREF(obj);
			return NULL;
		}
	}
	obj->start = start;
	obj->len   = len;
	obj->step  = step;

	return (PyObject *) obj;
}

/* Return number of items in range/xrange (lo, hi, step).  step > 0
 * required.  Return a value < 0 if & only if the true value is too
 * large to fit in a signed long.
 */
static long
get_len_of_range(long lo, long hi, long step)
{
	/* -------------------------------------------------------------
	If lo >= hi, the range is empty.
	Else if n values are in the range, the last one is
	lo + (n-1)*step, which must be <= hi-1.  Rearranging,
	n <= (hi - lo - 1)/step + 1, so taking the floor of the RHS gives
	the proper value.  Since lo < hi in this case, hi-lo-1 >= 0, so
	the RHS is non-negative and so truncation is the same as the
	floor.  Letting M be the largest positive long, the worst case
	for the RHS numerator is hi=M, lo=-M-1, and then
	hi-lo-1 = M-(-M-1)-1 = 2*M.  Therefore unsigned long has enough
	precision to compute the RHS exactly.
	---------------------------------------------------------------*/
	long n = 0;
	if (lo < hi) {
		unsigned long uhi = (unsigned long)hi;
		unsigned long ulo = (unsigned long)lo;
		unsigned long diff = uhi - ulo - 1;
		n = (long)(diff / (unsigned long)step + 1);
	}
	return n;
}

static PyObject *
range_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
	rangeobject *obj;
	long ilow = 0, ihigh = 0, istep = 1;
	long n;

	if (PyTuple_Size(args) <= 1) {
		if (!PyArg_ParseTuple(args,
				"l;xrange() requires 1-3 int arguments",
				&ihigh))
			return NULL;
	}
	else {
		if (!PyArg_ParseTuple(args,
				"ll|l;xrange() requires 1-3 int arguments",
				&ilow, &ihigh, &istep))
			return NULL;
	}
	if (istep == 0) {
		PyErr_SetString(PyExc_ValueError, "xrange() arg 3 must not be zero");
		return NULL;
	}
	if (istep > 0)
		n = get_len_of_range(ilow, ihigh, istep);
	else
		n = get_len_of_range(ihigh, ilow, -istep);
	if (n < 0) {
		PyErr_SetString(PyExc_OverflowError,
				"xrange() result has too many items");
		return NULL;
	}

	obj = PyObject_New(rangeobject, &PyRange_Type);
	if (obj == NULL)
		return NULL;
	obj->start = ilow;
	obj->len   = n;
	obj->step  = istep;
	return (PyObject *) obj;
}

PyDoc_STRVAR(range_doc,
"xrange([start,] stop[, step]) -> xrange object\n\
\n\
Like range(), but instead of returning a list, returns an object that\n\
generates the numbers in the range on demand.  For looping, this is \n\
slightly faster than range() and more memory efficient.");

static PyObject *
range_item(rangeobject *r, int i)
{
	if (i < 0 || i >= r->len) {
		PyErr_SetString(PyExc_IndexError,
				"xrange object index out of range");
		return NULL;
	}
	return PyInt_FromLong(r->start + (i % r->len) * r->step);
}

static int
range_length(rangeobject *r)
{
#if LONG_MAX != INT_MAX
	if (r->len > INT_MAX) {
		PyErr_SetString(PyExc_ValueError,
				"xrange object size cannot be reported");
		return -1;
	}
#endif
	return (int)(r->len);
}

static PyObject *
range_repr(rangeobject *r)
{
	PyObject *rtn;

	if (r->start == 0 && r->step == 1)
		rtn = PyString_FromFormat("xrange(%ld)",
					  r->start + r->len * r->step);

	else if (r->step == 1)
		rtn = PyString_FromFormat("xrange(%ld, %ld)",
					  r->start,
					  r->start + r->len * r->step);

	else
		rtn = PyString_FromFormat("xrange(%ld, %ld, %ld)",
					  r->start,
					  r->start + r->len * r->step,
					  r->step);
	return rtn;
}

static PySequenceMethods range_as_sequence = {
	(inquiry)range_length,	/* sq_length */
	0,			/* sq_concat */
	0,			/* sq_repeat */
	(intargfunc)range_item, /* sq_item */
	0,			/* sq_slice */
};

static PyObject * range_iter(PyObject *seq);
static PyObject * range_reverse(PyObject *seq);

PyDoc_STRVAR(reverse_doc,
"Returns a reverse iterator.");

static PyMethodDef range_methods[] = {
	{"__reversed__",	(PyCFunction)range_reverse, METH_NOARGS, reverse_doc},
 	{NULL,		NULL}		/* sentinel */
};

PyTypeObject PyRange_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/* Number of items for varobject */
	"xrange",			/* Name of this type */
	sizeof(rangeobject),		/* Basic object size */
	0,				/* Item size for varobject */
	(destructor)PyObject_Del,	/* tp_dealloc */
	0,				/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	(reprfunc)range_repr,		/* tp_repr */
	0,				/* tp_as_number */
	&range_as_sequence,		/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
	range_doc,			/* tp_doc */
	0,				/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	(getiterfunc)range_iter,	/* tp_iter */
	0,				/* tp_iternext */
	range_methods,			/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	0,				/* tp_alloc */
	range_new,			/* tp_new */
};

/*********************** Xrange Iterator **************************/

typedef struct {
	PyObject_HEAD
	long	index;
	long	start;
	long	step;
	long	len;
} rangeiterobject;

static PyTypeObject Pyrangeiter_Type;

static PyObject *
range_iter(PyObject *seq)
{
	rangeiterobject *it;

	if (!PyRange_Check(seq)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	it = PyObject_New(rangeiterobject, &Pyrangeiter_Type);
	if (it == NULL)
		return NULL;
	it->index = 0;
	it->start = ((rangeobject *)seq)->start;
	it->step = ((rangeobject *)seq)->step;
	it->len = ((rangeobject *)seq)->len;
	return (PyObject *)it;
}

static PyObject *
range_reverse(PyObject *seq)
{
	rangeiterobject *it;
	long start, step, len;

	if (!PyRange_Check(seq)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	it = PyObject_New(rangeiterobject, &Pyrangeiter_Type);
	if (it == NULL)
		return NULL;

	start = ((rangeobject *)seq)->start;
	step = ((rangeobject *)seq)->step;
	len = ((rangeobject *)seq)->len;

	it->index = 0;
	it->start = start + (len-1) * step;
	it->step = -step;
	it->len = len;

	return (PyObject *)it;
}

static PyObject *
rangeiter_next(rangeiterobject *r)
{
	if (r->index < r->len)
		return PyInt_FromLong(r->start + (r->index++) * r->step);
	return NULL;
}

static int
rangeiter_len(rangeiterobject *r)
{
	return r->len - r->index;
}

static PySequenceMethods rangeiter_as_sequence = {
	(inquiry)rangeiter_len,		/* sq_length */
	0,				/* sq_concat */
};


static PyTypeObject Pyrangeiter_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,                                      /* ob_size */
	"rangeiterator",                        /* tp_name */
	sizeof(rangeiterobject),                /* tp_basicsize */
	0,                                      /* tp_itemsize */
	/* methods */
	(destructor)PyObject_Del,		/* tp_dealloc */
	0,                                      /* tp_print */
	0,                                      /* tp_getattr */
	0,                                      /* tp_setattr */
	0,                                      /* tp_compare */
	0,                                      /* tp_repr */
	0,                                      /* tp_as_number */
	&rangeiter_as_sequence,			/* tp_as_sequence */
	0,                                      /* tp_as_mapping */
	0,                                      /* tp_hash */
	0,                                      /* tp_call */
	0,                                      /* tp_str */
	PyObject_GenericGetAttr,                /* tp_getattro */
	0,                                      /* tp_setattro */
	0,                                      /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
	0,                                      /* tp_doc */
	0,					/* tp_traverse */
	0,                                      /* tp_clear */
	0,                                      /* tp_richcompare */
	0,                                      /* tp_weaklistoffset */
	PyObject_SelfIter,			/* tp_iter */
	(iternextfunc)rangeiter_next,		/* tp_iternext */
	0,					/* tp_methods */
};
