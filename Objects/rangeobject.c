
/* Range object implementation */

#include "Python.h"
#include "structmember.h"
#include <string.h>

typedef struct {
	PyObject_HEAD
	long	start;
	long	step;
	long	len;
} rangeobject;

PyObject *
PyRange_New(long start, long len, long step)
{
	rangeobject *obj = PyObject_NEW(rangeobject, &PyRange_Type);

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
		    (last > (PyInt_GetMax() - step))
		    :(last < (-1 - PyInt_GetMax() - step))) {
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

static void
range_dealloc(rangeobject *r)
{
	PyObject_DEL(r);
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
	/* buffers must be big enough to hold 3 longs + an int +
	 * a bit of "(xrange(...) * ...)" text.
	 */
	char buf1[250];

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

	return PyString_FromString(buf1);
}

static PySequenceMethods range_as_sequence = {
	(inquiry)range_length,	/*sq_length*/
	0,			/*sq_concat*/
	0,			/*sq_repeat*/
	(intargfunc)range_item,	/*sq_item*/
	0,			/*sq_slice*/
	0,			/*sq_ass_item*/
	0,			/*sq_ass_slice*/
	0,			/*sq_contains*/
};

PyTypeObject PyRange_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/* Number of items for varobject */
	"xrange",		/* Name of this type */
	sizeof(rangeobject),	/* Basic object size */
	0,			/* Item size for varobject */
	(destructor)range_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
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
