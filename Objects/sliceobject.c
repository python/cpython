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

static PyObject *
ellipsis_repr(PyObject *op)
{
	return PyString_FromString("Ellipsis");
}

static PyTypeObject PyEllipsis_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"ellipsis",
	0,
	0,
	0,		/*tp_dealloc*/ /*never called*/
	0,		/*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	(reprfunc)ellipsis_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	0,		/*tp_hash */
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
	PySliceObject *obj = PyObject_NEW(PySliceObject, &PySlice_Type);

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
	PyObject_DEL(r);
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


static PyObject *slice_getattr(PySliceObject *self, char *name)
{
	PyObject *ret;
  
	ret = NULL;
	if (strcmp(name, "start") == 0) {
		ret = self->start;
	}
	else if (strcmp(name, "stop") == 0) {
		ret = self->stop;
	}
	else if (strcmp(name, "step") == 0) {
		ret = self->step;
	}
	else if (strcmp(name, "__members__") == 0) {
		return Py_BuildValue("[sss]",
				     "start", "stop", "step");
	}
	else {
		PyErr_SetString(PyExc_AttributeError, name);
		return NULL;
	}
	Py_INCREF(ret);
	return ret;
}


PyTypeObject PySlice_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/* Number of items for varobject */
	"slice",		/* Name of this type */
	sizeof(PySliceObject),	/* Basic object size */
	0,			/* Item size for varobject */
	(destructor)slice_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)slice_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,		    /*tp_compare*/
	(reprfunc)slice_repr, /*tp_repr*/
	0,			/*tp_as_number*/
	0,	    	/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
};
