/* Iterator objects */

#include "Python.h"

typedef struct {
	PyObject_HEAD
	long      it_index;
	PyObject *it_seq;
} seqiterobject;

PyObject *
PySeqIter_New(PyObject *seq)
{
	seqiterobject *it;

	if (!PySequence_Check(seq)) {
		PyErr_BadInternalCall();
		return NULL;
	}	
	it = PyObject_GC_New(seqiterobject, &PySeqIter_Type);
	if (it == NULL)
		return NULL;
	it->it_index = 0;
	Py_INCREF(seq);
	it->it_seq = seq;
	_PyObject_GC_TRACK(it);
	return (PyObject *)it;
}
static void
iter_dealloc(seqiterobject *it)
{
	_PyObject_GC_UNTRACK(it);
	Py_DECREF(it->it_seq);
	PyObject_GC_Del(it);
}

static int
iter_traverse(seqiterobject *it, visitproc visit, void *arg)
{
	return visit(it->it_seq, arg);
}

static PyObject *
iter_next(seqiterobject *it)
{
	PyObject *seq = it->it_seq;
	PyObject *result = PySequence_GetItem(seq, it->it_index++);

	if (result == NULL && PyErr_ExceptionMatches(PyExc_IndexError))
		PyErr_SetObject(PyExc_StopIteration, Py_None);
	return result;
}

static PyObject *
iter_getiter(PyObject *it)
{
	Py_INCREF(it);
	return it;
}

static PyObject *
iter_iternext(PyObject *iterator)
{
	seqiterobject *it;
	PyObject *seq;

	assert(PySeqIter_Check(iterator));
	it = (seqiterobject *)iterator;
	seq = it->it_seq;

	if (PyTuple_CheckExact(seq)) {
		if (it->it_index < PyTuple_GET_SIZE(seq)) {
			PyObject *item;
			item = PyTuple_GET_ITEM(seq, it->it_index);
			it->it_index++;
			Py_INCREF(item);
			return item;
		}
		return NULL;
	}
	else {
		PyObject *result = PySequence_ITEM(seq, it->it_index);
		it->it_index++;
		if (result != NULL) {
			return result;
		}
		if (PyErr_ExceptionMatches(PyExc_IndexError) ||
			PyErr_ExceptionMatches(PyExc_StopIteration)) {
			PyErr_Clear();
			return NULL;
		}
		else {
			return NULL;
		}
	}
}

static PyMethodDef iter_methods[] = {
	{"next",	(PyCFunction)iter_next,	METH_NOARGS,
	 "it.next() -- get the next value, or raise StopIteration"},
	{NULL,		NULL}		/* sentinel */
};

PyTypeObject PySeqIter_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,					/* ob_size */
	"iterator",				/* tp_name */
	sizeof(seqiterobject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	(destructor)iter_dealloc, 		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
 	0,					/* tp_doc */
 	(traverseproc)iter_traverse,		/* tp_traverse */
 	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	(getiterfunc)iter_getiter,		/* tp_iter */
	(iternextfunc)iter_iternext,		/* tp_iternext */
	iter_methods,				/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
};

/* -------------------------------------- */

typedef struct {
	PyObject_HEAD
	PyObject *it_callable;
	PyObject *it_sentinel;
} calliterobject;

PyObject *
PyCallIter_New(PyObject *callable, PyObject *sentinel)
{
	calliterobject *it;
	it = PyObject_GC_New(calliterobject, &PyCallIter_Type);
	if (it == NULL)
		return NULL;
	Py_INCREF(callable);
	it->it_callable = callable;
	Py_INCREF(sentinel);
	it->it_sentinel = sentinel;
	_PyObject_GC_TRACK(it);
	return (PyObject *)it;
}
static void
calliter_dealloc(calliterobject *it)
{
	_PyObject_GC_UNTRACK(it);
	Py_DECREF(it->it_callable);
	Py_DECREF(it->it_sentinel);
	PyObject_GC_Del(it);
}

static int
calliter_traverse(calliterobject *it, visitproc visit, void *arg)
{
	int err;
	if ((err = visit(it->it_callable, arg)))
		return err;
	if ((err = visit(it->it_sentinel, arg)))
		return err;
	return 0;
}

static PyObject *
calliter_next(calliterobject *it, PyObject *args)
{
	PyObject *result = PyObject_CallObject(it->it_callable, NULL);
	if (result != NULL) {
		if (PyObject_RichCompareBool(result, it->it_sentinel, Py_EQ)) {
			PyErr_SetObject(PyExc_StopIteration, Py_None);
			Py_DECREF(result);
			result = NULL;
		}
	}
	return result;
}

static PyMethodDef calliter_methods[] = {
	{"next",	(PyCFunction)calliter_next,	METH_VARARGS,
	 "it.next() -- get the next value, or raise StopIteration"},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
calliter_iternext(calliterobject *it)
{
	PyObject *result = PyObject_CallObject(it->it_callable, NULL);
	if (result != NULL) {
		if (PyObject_RichCompareBool(result, it->it_sentinel, Py_EQ)) {
			Py_DECREF(result);
			result = NULL;
		}
	}
	else if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
		PyErr_Clear();
	}
	return result;
}

PyTypeObject PyCallIter_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,					/* ob_size */
	"callable-iterator",			/* tp_name */
	sizeof(calliterobject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	(destructor)calliter_dealloc, 		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
 	0,					/* tp_doc */
 	(traverseproc)calliter_traverse,	/* tp_traverse */
 	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	(getiterfunc)iter_getiter,		/* tp_iter */
	(iternextfunc)calliter_iternext,	/* tp_iternext */
	calliter_methods,			/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
};
