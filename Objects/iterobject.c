/* Iterator objects */

#include "Python.h"

typedef struct {
	PyObject_HEAD
	long      it_index;
	PyObject *it_seq; /* Set to NULL when iterator is exhausted */
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
	Py_XDECREF(it->it_seq);
	PyObject_GC_Del(it);
}

static int
iter_traverse(seqiterobject *it, visitproc visit, void *arg)
{
	if (it->it_seq == NULL)
		return 0;
	return visit(it->it_seq, arg);
}

static PyObject *
iter_iternext(PyObject *iterator)
{
	seqiterobject *it;
	PyObject *seq;
	PyObject *result;

	assert(PySeqIter_Check(iterator));
	it = (seqiterobject *)iterator;
	seq = it->it_seq;
	if (seq == NULL)
		return NULL;

	result = PySequence_GetItem(seq, it->it_index);
	if (result != NULL) {
		it->it_index++;
		return result;
	}
	if (PyErr_ExceptionMatches(PyExc_IndexError) ||
	    PyErr_ExceptionMatches(PyExc_StopIteration))
	{
		PyErr_Clear();
		Py_DECREF(seq);
		it->it_seq = NULL;
	}
	return NULL;
}

static int
iter_len(seqiterobject *it)
{
	int seqsize, len;

	if (it->it_seq) {
		seqsize = PySequence_Size(it->it_seq);
		if (seqsize == -1)
			return -1;
		len = seqsize - it->it_index;
		if (len >= 0)
			return len;
	}
	return 0;
}

static PySequenceMethods iter_as_sequence = {
	(inquiry)iter_len,		/* sq_length */
	0,				/* sq_concat */
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
	&iter_as_sequence,			/* tp_as_sequence */
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
	PyObject_SelfIter,			/* tp_iter */
	(iternextfunc)iter_iternext,		/* tp_iternext */
	0,					/* tp_methods */
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
	PyObject *it_callable; /* Set to NULL when iterator is exhausted */
	PyObject *it_sentinel; /* Set to NULL when iterator is exhausted */
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
	Py_XDECREF(it->it_callable);
	Py_XDECREF(it->it_sentinel);
	PyObject_GC_Del(it);
}

static int
calliter_traverse(calliterobject *it, visitproc visit, void *arg)
{
	int err;
	if (it->it_callable != NULL && (err = visit(it->it_callable, arg)))
		return err;
	if (it->it_sentinel != NULL && (err = visit(it->it_sentinel, arg)))
		return err;
	return 0;
}

static PyObject *
calliter_iternext(calliterobject *it)
{
	if (it->it_callable != NULL) {
		PyObject *args = PyTuple_New(0);
		PyObject *result;
		if (args == NULL)
			return NULL;
		result = PyObject_Call(it->it_callable, args, NULL);
		Py_DECREF(args);
		if (result != NULL) {
			int ok;
			ok = PyObject_RichCompareBool(result,
						      it->it_sentinel,
						      Py_EQ);
			if (ok == 0)
				return result; /* Common case, fast path */
			Py_DECREF(result);
			if (ok > 0) {
				Py_CLEAR(it->it_callable);
				Py_CLEAR(it->it_sentinel);
			}
		}
		else if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
			PyErr_Clear();
			Py_CLEAR(it->it_callable);
			Py_CLEAR(it->it_sentinel);
		}
	}
	return NULL;
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
	PyObject_SelfIter,			/* tp_iter */
	(iternextfunc)calliter_iternext,	/* tp_iternext */
	0,					/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
};
