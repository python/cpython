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
	Py_VISIT(it->it_seq);
	return 0;
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

static PyObject *
iter_len(seqiterobject *it)
{
	Py_ssize_t seqsize, len;

	if (it->it_seq) {
		seqsize = PySequence_Size(it->it_seq);
		if (seqsize == -1)
			return NULL;
		len = seqsize - it->it_index;
		if (len >= 0)
			return PyLong_FromSsize_t(len);
	}
	return PyLong_FromLong(0);
}

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

static PyMethodDef seqiter_methods[] = {
	{"__length_hint__", (PyCFunction)iter_len, METH_NOARGS, length_hint_doc},
 	{NULL,		NULL}		/* sentinel */
};

PyTypeObject PySeqIter_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
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
	PyObject_SelfIter,			/* tp_iter */
	iter_iternext,				/* tp_iternext */
	seqiter_methods,			/* tp_methods */
	0,					/* tp_members */
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
	Py_VISIT(it->it_callable);
	Py_VISIT(it->it_sentinel);
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
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"callable_iterator",			/* tp_name */
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
};


/*********************** Zip Iterator **************************/
/* Largely copied from itertools.c by Brian Holmes */

typedef struct zipiterobject_t {
	PyObject_HEAD
	PyTupleObject *it_tuple;  /* Set to NULL when iterator is exhausted */
        Py_ssize_t resultsize;	
	PyTupleObject *result;	/* Reusable tuple for optimization */
} zipiterobject;

 /* Forward */

PyObject *
_PyZip_CreateIter(PyObject* args)
{
	Py_ssize_t i;
        Py_ssize_t tuplesize;
	PyObject* ziptuple;
	PyObject* result;
	struct zipiterobject_t* zipiter;
        
        assert(PyTuple_Check(args));

	if (Py_TYPE(&PyZipIter_Type) == NULL) {
		if (PyType_Ready(&PyZipIter_Type) < 0)
			return NULL;
	}

	tuplesize = PySequence_Length((PyObject*) args);

	ziptuple = PyTuple_New(tuplesize);
	if (ziptuple == NULL)
		return NULL;

	for (i = 0; i < tuplesize; i++) {
		PyObject *o = PyTuple_GET_ITEM(args, i);
		PyObject *it = PyObject_GetIter(o);
		if (it == NULL) {
			/* XXX Should we do this?
			if (PyErr_ExceptionMatches(PyExc_TypeError))
				PyErr_Format(PyExc_TypeError, 
				  "zip argument #%zd must support iteration",
					I+1);
			*/
			Py_DECREF(ziptuple);
			return NULL;
		}
		PyTuple_SET_ITEM(ziptuple, i, it);
	}

        /* create a reusable result holder */
        result = PyTuple_New(tuplesize);
        if (result == NULL) {
                Py_DECREF(ziptuple);
                return NULL;
        }
        for (i = 0; i < tuplesize; i++) {
                Py_INCREF(Py_None);
                PyTuple_SET_ITEM(result, i, Py_None);
        }
	
	zipiter = PyObject_GC_New(zipiterobject, &PyZipIter_Type);
	if (zipiter == NULL) {
		Py_DECREF(ziptuple);
		Py_DECREF(result);
		return NULL;
	}

	zipiter->result = (PyTupleObject*) result;
        zipiter->resultsize = tuplesize;
	zipiter->it_tuple = (PyTupleObject *) ziptuple;
	_PyObject_GC_TRACK(zipiter);
	return (PyObject *)zipiter;
}

static void
zipiter_dealloc(zipiterobject *it)
{
	_PyObject_GC_UNTRACK(it);
	Py_XDECREF(it->it_tuple);
	Py_XDECREF(it->result);
	PyObject_GC_Del(it);
}

static int
zipiter_traverse(zipiterobject *it, visitproc visit, void *arg)
{
	Py_VISIT(it->it_tuple);
	Py_VISIT(it->result);
	return 0;
}

static PyObject *
zipiter_next(zipiterobject *zit)
{
        Py_ssize_t i;
        Py_ssize_t tuplesize = zit->resultsize;
        PyObject *result = (PyObject*) zit->result;
        PyObject *olditem;

        if (tuplesize == 0)
                return NULL;

        if (result->ob_refcnt == 1) {
		Py_INCREF(result);
		for (i = 0; i < tuplesize; i++) {
			PyObject *it = PyTuple_GET_ITEM(zit->it_tuple, i);
			PyObject *item;
			assert(PyIter_Check(it));
			item = (*it->ob_type->tp_iternext)(it);
			if (item == NULL) {
				Py_DECREF(result);
				return NULL;
			}
			olditem = PyTuple_GET_ITEM(result, i);
			PyTuple_SET_ITEM(result, i, item);
			Py_DECREF(olditem);
		}
	} else {
		result = PyTuple_New(tuplesize);
		if (result == NULL)
			return NULL;
		for (i = 0; i < tuplesize; i++) {
			PyObject *it = PyTuple_GET_ITEM(zit->it_tuple, i);
			PyObject *item;
			assert(PyIter_Check(it));
			item = (*it->ob_type->tp_iternext)(it);
			if (item == NULL) {
				Py_DECREF(result);
				return NULL;
			}
			PyTuple_SET_ITEM(result, i, item);
		}
	}
	return result;
}

PyTypeObject PyZipIter_Type = {
	PyVarObject_HEAD_INIT(0, 0)
	"zip_iterator",				/* tp_name */
	sizeof(zipiterobject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	(destructor)zipiter_dealloc,		/* tp_dealloc */
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
	(traverseproc)zipiter_traverse,		/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weakzipoffset */
	PyObject_SelfIter,			/* tp_iter */
	(iternextfunc)zipiter_next,		/* tp_iternext */
	0,					/* tp_methods */
	0,					/* tp_members */
};
