
/* Range object implementation */

#include "Python.h"

typedef struct {
	PyObject_HEAD
	long	start;
	long	step;
	long	len;
} rangeobject;

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
			return NULL;
		}			
	}
	obj->start = start;
	obj->len   = len;
	obj->step  = step;

	return (PyObject *) obj;
}

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
	return r->len;
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

staticforward PyObject * range_iter(PyObject *seq);

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
	0,				/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
	0,				/* tp_doc */
	0,				/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	(getiterfunc)range_iter,	/* tp_iter */	
};

/*********************** Xrange Iterator **************************/

typedef struct {
	PyObject_HEAD
	long	index;
	long	start;
	long	step;
	long	len;
} rangeiterobject;

PyTypeObject Pyrangeiter_Type;

PyObject *
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
rangeiter_getiter(PyObject *it)
{
	Py_INCREF(it);
	return it;
}

static PyObject *
rangeiter_next(rangeiterobject *r)
{
	if (r->index < r->len) 
		return PyInt_FromLong(r->start + (r->index++) * r->step);
	PyErr_SetObject(PyExc_StopIteration, Py_None);
	return NULL;
}

static PyMethodDef rangeiter_methods[] = {
	{"next",        (PyCFunction)rangeiter_next,     METH_NOARGS,
	 "it.next() -- get the next value, or raise StopIteration"},
	{NULL,          NULL}           /* sentinel */
};

PyTypeObject Pyrangeiter_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,                                      /* ob_size */
	"rangeiterator",                         /* tp_name */
	sizeof(rangeiterobject),                 /* tp_basicsize */
	0,                                      /* tp_itemsize */
	/* methods */
	(destructor)PyObject_Del,		/* tp_dealloc */
	0,                                      /* tp_print */
	0,                                      /* tp_getattr */
	0,                                      /* tp_setattr */
	0,                                      /* tp_compare */
	0,                                      /* tp_repr */
	0,                                      /* tp_as_number */
	0,                                      /* tp_as_sequence */
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
	(getiterfunc)rangeiter_getiter,		/* tp_iter */
	(iternextfunc)rangeiter_next,		/* tp_iternext */
	rangeiter_methods,			/* tp_methods */
};

