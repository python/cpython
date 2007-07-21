/*
Written by Jim Hugunin and Chris Chase.

This includes both the singular ellipsis object and slice objects.

Guido, feel free to do whatever you want in the way of copyrights
for this file.
*/

/* 
Py_Ellipsis encodes the '...' rubber index token. It is similar to
the Py_NoneStruct in that there is no way to create other objects of
this type and there is exactly one in existence.
*/

#include "Python.h"
#include "structmember.h"

static PyObject *
ellipsis_repr(PyObject *op)
{
	return PyUnicode_FromString("Ellipsis");
}

static PyTypeObject PyEllipsis_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"ellipsis",			/* tp_name */
	0,				/* tp_basicsize */
	0,				/* tp_itemsize */
	0, /*never called*/		/* tp_dealloc */
	0,				/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	ellipsis_repr,			/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
};

PyObject _Py_EllipsisObject = {
	_PyObject_EXTRA_INIT
	1, &PyEllipsis_Type
};


/* Slice object implementation

   start, stop, and step are python objects with None indicating no
   index is present.
*/

PyObject *
PySlice_New(PyObject *start, PyObject *stop, PyObject *step)
{
	PySliceObject *obj = PyObject_New(PySliceObject, &PySlice_Type);

	if (obj == NULL)
		return NULL;

	if (step == NULL) step = Py_None;
	Py_INCREF(step);
	if (start == NULL) start = Py_None;
	Py_INCREF(start);
	if (stop == NULL) stop = Py_None;
	Py_INCREF(stop);

	obj->step = step;
	obj->start = start;
	obj->stop = stop;

	return (PyObject *) obj;
}

PyObject *
_PySlice_FromIndices(Py_ssize_t istart, Py_ssize_t istop)
{
	PyObject *start, *end, *slice;
	start = PyInt_FromSsize_t(istart);
	if (!start)
		return NULL;
	end = PyInt_FromSsize_t(istop);
	if (!end) {
		Py_DECREF(start);
		return NULL;
	}

	slice = PySlice_New(start, end, NULL);
	Py_DECREF(start);
	Py_DECREF(end);
	return slice;
}

int
PySlice_GetIndices(PySliceObject *r, Py_ssize_t length,
                   Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t *step)
{
	/* XXX support long ints */
	if (r->step == Py_None) {
		*step = 1;
	} else {
		if (!PyInt_Check(r->step) && !PyLong_Check(r->step)) return -1;
		*step = PyInt_AsSsize_t(r->step);
	}
	if (r->start == Py_None) {
		*start = *step < 0 ? length-1 : 0;
	} else {
		if (!PyInt_Check(r->start) && !PyLong_Check(r->step)) return -1;
		*start = PyInt_AsSsize_t(r->start);
		if (*start < 0) *start += length;
	}
	if (r->stop == Py_None) {
		*stop = *step < 0 ? -1 : length;
	} else {
		if (!PyInt_Check(r->stop) && !PyLong_Check(r->step)) return -1;
		*stop = PyInt_AsSsize_t(r->stop);
		if (*stop < 0) *stop += length;
	}
	if (*stop > length) return -1;
	if (*start >= length) return -1;
	if (*step == 0) return -1;
	return 0;
}

int
PySlice_GetIndicesEx(PySliceObject *r, Py_ssize_t length,
		     Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t *step, Py_ssize_t *slicelength)
{
	/* this is harder to get right than you might think */

	Py_ssize_t defstart, defstop;

	if (r->step == Py_None) {
		*step = 1;
	} 
	else {
		if (!_PyEval_SliceIndex(r->step, step)) return -1;
		if (*step == 0) {
			PyErr_SetString(PyExc_ValueError,
					"slice step cannot be zero");
			return -1;
		}
	}

	defstart = *step < 0 ? length-1 : 0;
	defstop = *step < 0 ? -1 : length;

	if (r->start == Py_None) {
		*start = defstart;
	}
	else {
		if (!_PyEval_SliceIndex(r->start, start)) return -1;
		if (*start < 0) *start += length;
		if (*start < 0) *start = (*step < 0) ? -1 : 0;
		if (*start >= length) 
			*start = (*step < 0) ? length - 1 : length;
	}

	if (r->stop == Py_None) {
		*stop = defstop;
	}
	else {
		if (!_PyEval_SliceIndex(r->stop, stop)) return -1;
		if (*stop < 0) *stop += length;
		if (*stop < 0) *stop = -1;
		if (*stop > length) *stop = length;
	}

	if ((*step < 0 && *stop >= *start) 
	    || (*step > 0 && *start >= *stop)) {
		*slicelength = 0;
	}
	else if (*step < 0) {
		*slicelength = (*stop-*start+1)/(*step)+1;
	}
	else {
		*slicelength = (*stop-*start-1)/(*step)+1;
	}

	return 0;
}

static PyObject *
slice_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
	PyObject *start, *stop, *step;

	start = stop = step = NULL;

	if (!_PyArg_NoKeywords("slice()", kw))
		return NULL;

	if (!PyArg_UnpackTuple(args, "slice", 1, 3, &start, &stop, &step))
		return NULL;

	/* This swapping of stop and start is to maintain similarity with
	   range(). */
	if (stop == NULL) {
		stop = start;
		start = NULL;
	}
	return PySlice_New(start, stop, step);
}

PyDoc_STRVAR(slice_doc,
"slice([start,] stop[, step])\n\
\n\
Create a slice object.  This is used for extended slicing (e.g. a[0:10:2]).");

static void
slice_dealloc(PySliceObject *r)
{
	Py_DECREF(r->step);
	Py_DECREF(r->start);
	Py_DECREF(r->stop);
	PyObject_Del(r);
}

static PyObject *
slice_repr(PySliceObject *r)
{
	return PyUnicode_FromFormat("slice(%R, %R, %R)", r->start, r->stop, r->step);
}

static PyMemberDef slice_members[] = {
	{"start", T_OBJECT, offsetof(PySliceObject, start), READONLY},
	{"stop", T_OBJECT, offsetof(PySliceObject, stop), READONLY},
	{"step", T_OBJECT, offsetof(PySliceObject, step), READONLY},
	{0}
};

static PyObject*
slice_indices(PySliceObject* self, PyObject* len)
{
	Py_ssize_t ilen, start, stop, step, slicelength;

	ilen = PyNumber_AsSsize_t(len, PyExc_OverflowError);

	if (ilen == -1 && PyErr_Occurred()) {
		return NULL;
	}

	if (PySlice_GetIndicesEx(self, ilen, &start, &stop, 
				 &step, &slicelength) < 0) {
		return NULL;
	}

	return Py_BuildValue("(nnn)", start, stop, step);
}

PyDoc_STRVAR(slice_indices_doc,
"S.indices(len) -> (start, stop, stride)\n\
\n\
Assuming a sequence of length len, calculate the start and stop\n\
indices, and the stride length of the extended slice described by\n\
S. Out of bounds indices are clipped in a manner consistent with the\n\
handling of normal slices.");

static PyObject *
slice_reduce(PySliceObject* self)
{
	return Py_BuildValue("O(OOO)", Py_Type(self), self->start, self->stop, self->step);
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyMethodDef slice_methods[] = {
	{"indices",	(PyCFunction)slice_indices,
	 METH_O,	slice_indices_doc},
	{"__reduce__",	(PyCFunction)slice_reduce,
	 METH_NOARGS,	reduce_doc},
	{NULL, NULL}
};

static PyObject *
slice_richcompare(PyObject *v, PyObject *w, int op)
{
	PyObject *t1;
	PyObject *t2;
	PyObject *res;

	if (v == w) {
		/* XXX Do we really need this shortcut?
		   There's a unit test for it, but is that fair? */
		switch (op) {
		case Py_EQ: 
		case Py_LE:
		case Py_GE:
			res = Py_True; 
			break;
		default:
			res = Py_False; 
			break;
		}
		Py_INCREF(res);
		return res;
	}

	t1 = PyTuple_New(3);
	t2 = PyTuple_New(3);
	if (t1 == NULL || t2 == NULL)
		return NULL;

	PyTuple_SET_ITEM(t1, 0, ((PySliceObject *)v)->start);
	PyTuple_SET_ITEM(t1, 1, ((PySliceObject *)v)->stop);
	PyTuple_SET_ITEM(t1, 2, ((PySliceObject *)v)->step);
	PyTuple_SET_ITEM(t2, 0, ((PySliceObject *)w)->start);
	PyTuple_SET_ITEM(t2, 1, ((PySliceObject *)w)->stop);
	PyTuple_SET_ITEM(t2, 2, ((PySliceObject *)w)->step);

	res = PyObject_RichCompare(t1, t2, op);

	PyTuple_SET_ITEM(t1, 0, NULL);
	PyTuple_SET_ITEM(t1, 1, NULL);
	PyTuple_SET_ITEM(t1, 2, NULL);
	PyTuple_SET_ITEM(t2, 0, NULL);
	PyTuple_SET_ITEM(t2, 1, NULL);
	PyTuple_SET_ITEM(t2, 2, NULL);

	Py_DECREF(t1);
	Py_DECREF(t2);

	return res;
}

static long
slice_hash(PySliceObject *v)
{
	PyErr_SetString(PyExc_TypeError, "unhashable type");
	return -1L;
}

PyTypeObject PySlice_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"slice",		/* Name of this type */
	sizeof(PySliceObject),	/* Basic object size */
	0,			/* Item size for varobject */
	(destructor)slice_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,			 		/* tp_compare */
	(reprfunc)slice_repr,   		/* tp_repr */
	0,					/* tp_as_number */
	0,	    				/* tp_as_sequence */
	0,					/* tp_as_mapping */
	(hashfunc)slice_hash,			/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
	slice_doc,				/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	slice_richcompare,			/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	slice_methods,				/* tp_methods */
	slice_members,				/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	slice_new,				/* tp_new */
};
