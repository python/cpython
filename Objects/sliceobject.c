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
	return PyString_FromString("Ellipsis");
}

static PyTypeObject PyEllipsis_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,					/* ob_size */
	"ellipsis",				/* tp_name */
	0,					/* tp_basicsize */
	0,					/* tp_itemsize */
	0, /*never called*/			/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)ellipsis_repr,		/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
};

PyObject _Py_EllipsisObject = {
	PyObject_HEAD_INIT(&PyEllipsis_Type)
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

int
PySlice_GetIndices(PySliceObject *r, int length,
                   int *start, int *stop, int *step)
{
	if (r->step == Py_None) {
		*step = 1;
	} else {
		if (!PyInt_Check(r->step)) return -1;
		*step = PyInt_AsLong(r->step);
	}
	if (r->start == Py_None) {
		*start = *step < 0 ? length-1 : 0;
	} else {
		if (!PyInt_Check(r->start)) return -1;
		*start = PyInt_AsLong(r->start);
		if (*start < 0) *start += length;
	}
	if (r->stop == Py_None) {
		*stop = *step < 0 ? -1 : length;
	} else {
		if (!PyInt_Check(r->stop)) return -1;
		*stop = PyInt_AsLong(r->stop);
		if (*stop < 0) *stop += length;
	}
	if (*stop > length) return -1;
	if (*start >= length) return -1;
	if (*step == 0) return -1;
	return 0;
}

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
	PyObject *s, *comma;

	s = PyString_FromString("slice(");
	comma = PyString_FromString(", ");
	PyString_ConcatAndDel(&s, PyObject_Repr(r->start));
	PyString_Concat(&s, comma);
	PyString_ConcatAndDel(&s, PyObject_Repr(r->stop));
	PyString_Concat(&s, comma);
	PyString_ConcatAndDel(&s, PyObject_Repr(r->step));
	PyString_ConcatAndDel(&s, PyString_FromString(")"));
	Py_DECREF(comma);
	return s;
}

static PyMemberDef slice_members[] = {
	{"start", T_OBJECT, offsetof(PySliceObject, start), READONLY},
	{"stop", T_OBJECT, offsetof(PySliceObject, stop), READONLY},
	{"step", T_OBJECT, offsetof(PySliceObject, step), READONLY},
	{0}
};

static int
slice_compare(PySliceObject *v, PySliceObject *w)
{
	int result = 0;

        if (v == w)
		return 0;

	if (PyObject_Cmp(v->start, w->start, &result) < 0)
	    return -2;
	if (result != 0)
		return result;
	if (PyObject_Cmp(v->stop, w->stop, &result) < 0)
	    return -2;
	if (result != 0)
		return result;
	if (PyObject_Cmp(v->step, w->step, &result) < 0)
	    return -2;
	return result;
}

PyTypeObject PySlice_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/* Number of items for varobject */
	"slice",		/* Name of this type */
	sizeof(PySliceObject),	/* Basic object size */
	0,			/* Item size for varobject */
	(destructor)slice_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	(cmpfunc)slice_compare, 		/* tp_compare */
	(reprfunc)slice_repr,   		/* tp_repr */
	0,					/* tp_as_number */
	0,	    				/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
	0,					/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	slice_members,				/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
};
