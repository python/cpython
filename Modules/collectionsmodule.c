#include "Python.h"

/* collections module implementation of a deque() datatype
   Written and maintained by Raymond D. Hettinger <python@rcn.com>
   Copyright (c) 2004 Python Software Foundation.
   All rights reserved.
*/

#define BLOCKLEN 46

typedef struct BLOCK {
	struct BLOCK *leftlink;
	struct BLOCK *rightlink;
	PyObject *data[BLOCKLEN];
} block;

static block *newblock(block *leftlink, block *rightlink) {
	block *b = PyMem_Malloc(sizeof(block));
	if (b == NULL) {
		PyErr_NoMemory();
		return NULL;
	}
	b->leftlink = leftlink;
	b->rightlink = rightlink;
	return b;
}

typedef struct {
	PyObject_HEAD
	block *leftblock;
	block *rightblock;
	int leftindex;
	int rightindex;
	int len;
} dequeobject;

static PyObject *
deque_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	dequeobject *deque;
	block *b;

	/* create dequeobject structure */
	deque = (dequeobject *)type->tp_alloc(type, 0);
	if (deque == NULL)
		return NULL;
	
	b = newblock(NULL, NULL);
	if (b == NULL) {
		Py_DECREF(deque);
		return NULL;
	}

	deque->leftblock = b;
	deque->rightblock = b;
	deque->leftindex = BLOCKLEN / 2 + 1;
	deque->rightindex = BLOCKLEN / 2;
	deque->len = 0;

	return (PyObject *)deque;
}

static PyObject *
deque_append(dequeobject *deque, PyObject *item)
{
	deque->rightindex++;
	deque->len++;
	if (deque->rightindex == BLOCKLEN) {
		block *b = newblock(deque->rightblock, NULL);
		if (b == NULL)
			return NULL;
		assert(deque->rightblock->rightlink == NULL);
		deque->rightblock->rightlink = b;
		deque->rightblock = b;
		deque->rightindex = 0;
	}
	Py_INCREF(item);
	deque->rightblock->data[deque->rightindex] = item;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(append_doc, "Add an element to the right side of the deque.");

static PyObject *
deque_appendleft(dequeobject *deque, PyObject *item)
{
	deque->leftindex--;
	deque->len++;
	if (deque->leftindex == -1) {
		block *b = newblock(NULL, deque->leftblock);
		if (b == NULL)
			return NULL;
		assert(deque->leftblock->leftlink == NULL);
		deque->leftblock->leftlink = b;
		deque->leftblock = b;
		deque->leftindex = BLOCKLEN - 1;
	}
	Py_INCREF(item);
	deque->leftblock->data[deque->leftindex] = item;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(appendleft_doc, "Add an element to the left side of the deque.");

static PyObject *
deque_pop(dequeobject *deque, PyObject *unused)
{
	PyObject *item;
	block *prevblock;

	if (deque->len == 0) {
		PyErr_SetString(PyExc_LookupError, "pop from an empty deque");
		return NULL;
	}
	item = deque->rightblock->data[deque->rightindex];
	deque->rightindex--;
	deque->len--;

	if (deque->rightindex == -1) {
		if (deque->len == 0) {
			assert(deque->leftblock == deque->rightblock);
			assert(deque->leftindex == deque->rightindex+1);
			/* re-center instead of freeing a block */
			deque->leftindex = BLOCKLEN / 2 + 1;
			deque->rightindex = BLOCKLEN / 2;
		} else {
			prevblock = deque->rightblock->leftlink;
			assert(deque->leftblock != deque->rightblock);
			PyMem_Free(deque->rightblock);
			prevblock->rightlink = NULL;
			deque->rightblock = prevblock;
			deque->rightindex = BLOCKLEN - 1;
		}
	}
	return item;
}

PyDoc_STRVAR(pop_doc, "Remove and return the rightmost element.");

static PyObject *
deque_popleft(dequeobject *deque, PyObject *unused)
{
	PyObject *item;
	block *prevblock;

	if (deque->len == 0) {
		PyErr_SetString(PyExc_LookupError, "pop from an empty deque");
		return NULL;
	}
	item = deque->leftblock->data[deque->leftindex];
	deque->leftindex++;
	deque->len--;

	if (deque->leftindex == BLOCKLEN) {
		if (deque->len == 0) {
			assert(deque->leftblock == deque->rightblock);
			assert(deque->leftindex == deque->rightindex+1);
			/* re-center instead of freeing a block */
			deque->leftindex = BLOCKLEN / 2 + 1;
			deque->rightindex = BLOCKLEN / 2;
		} else {
			assert(deque->leftblock != deque->rightblock);
			prevblock = deque->leftblock->rightlink;
			assert(deque->leftblock != NULL);
			PyMem_Free(deque->leftblock);
			assert(prevblock != NULL);
			prevblock->leftlink = NULL;
			deque->leftblock = prevblock;
			deque->leftindex = 0;
		}
	}
	return item;
}

PyDoc_STRVAR(popleft_doc, "Remove and return the leftmost element.");

static int
deque_len(dequeobject *deque)
{
	return deque->len;
}

static int
deque_clear(dequeobject *deque)
{
	PyObject *item;

	while (deque_len(deque)) {
		item = deque_pop(deque, NULL);
		if (item == NULL)
			return -1;
		Py_DECREF(item);
	}
	assert(deque->leftblock == deque->rightblock &&
		deque->leftindex > deque->rightindex);
	return 0;
}

static PyObject *
deque_clearmethod(dequeobject *deque)
{
	if (deque_clear(deque) == -1)
		return NULL;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(clear_doc, "Remove all elements from the deque.");

static void
deque_dealloc(dequeobject *deque)
{
	PyObject_GC_UnTrack(deque);
	if (deque->leftblock != NULL) {
		int err = deque_clear(deque);
		assert(err == 0);
		assert(deque->leftblock != NULL);
		PyMem_Free(deque->leftblock);
	}
	deque->leftblock = NULL;
	deque->rightblock = NULL;
	deque->ob_type->tp_free(deque);
}

static int
set_traverse(dequeobject *deque, visitproc visit, void *arg)
{
	block * b = deque->leftblock;
	int index = deque->leftindex;
	int err;
	PyObject *item;

	while (b != deque->rightblock || index <= deque->rightindex) {
		item = b->data[index];
		index++;
		if (index == BLOCKLEN && b->rightlink != NULL) {
			b = b->rightlink;
			index = 0;
		}
		err = visit(item, arg);
		if (err) 
			return err;
	}
	return 0;
}

static long
deque_nohash(PyObject *self)
{
	PyErr_SetString(PyExc_TypeError, "deque objects are unhashable");
	return -1;
}

static PyObject *
deque_copy(PyObject *deque)
{
	return PyObject_CallFunctionObjArgs((PyObject *)(deque->ob_type), 
		deque, NULL);
}

PyDoc_STRVAR(copy_doc, "Return a shallow copy of a deque.");

static PyObject *
deque_reduce(dequeobject *deque)
{
	PyObject *seq, *args, *result;

	seq = PySequence_Tuple((PyObject *)deque);
	if (seq == NULL)
		return NULL;
	args = PyTuple_Pack(1, seq);
	if (args == NULL) {
		Py_DECREF(seq);
		return NULL;
	}
	result = PyTuple_Pack(2, deque->ob_type, args);
	Py_DECREF(seq);
	Py_DECREF(args);
	return result;
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyObject *
deque_repr(PyObject *deque)
{
	PyObject *aslist, *result, *fmt;
	int i;

	i = Py_ReprEnter(deque);
	if (i != 0) {
		if (i < 0)
			return NULL;
		return PyString_FromString("[...]");
	}

	aslist = PySequence_List(deque);
	if (aslist == NULL) {
		Py_ReprLeave(deque);
		return NULL;
	}

	fmt = PyString_FromString("deque(%r)");
	if (fmt == NULL) {
		Py_DECREF(aslist);
		Py_ReprLeave(deque);
		return NULL;
	}
	result = PyString_Format(fmt, aslist);
	Py_DECREF(fmt);
	Py_DECREF(aslist);
	Py_ReprLeave(deque);
	return result;
}

static int
deque_tp_print(PyObject *deque, FILE *fp, int flags)
{
	PyObject *it, *item;
	int pos=0;
	char *emit = "";	/* No separator emitted on first pass */
	char *separator = ", ";
	int i;

	i = Py_ReprEnter(deque);
	if (i != 0) {
		if (i < 0)
			return i;
		fputs("[...]", fp);
		return 0;
	}

	it = PyObject_GetIter(deque);
	if (it == NULL)
		return -1;

	fputs("deque([", fp);
	while ((item = PyIter_Next(it)) != NULL) {
		fputs(emit, fp);
		emit = separator;
		if (PyObject_Print(item, fp, 0) != 0) {
			Py_DECREF(item);
			Py_DECREF(it);
			Py_ReprLeave(deque);
			return -1;
		}
		Py_DECREF(item);
	}
	Py_ReprLeave(deque);
	Py_DECREF(it);
	if (PyErr_Occurred()) 
		return -1;
	fputs("])", fp);
	return 0;
}

static int
deque_init(dequeobject *deque, PyObject *args, PyObject *kwds)
{
	PyObject *iterable = NULL, *it, *item;

	if (!PyArg_UnpackTuple(args, "deque", 0, 1, &iterable))
		return -1;

	if (iterable != NULL) {
		it = PyObject_GetIter(iterable);
		if (it == NULL)
			return -1;

		while ((item = PyIter_Next(it)) != NULL) {
			deque->rightindex++;
			deque->len++;
			if (deque->rightindex == BLOCKLEN) {
				block *b = newblock(deque->rightblock, NULL);
				if (b == NULL) {
					Py_DECREF(it);
					Py_DECREF(item);
					return -1;
				}
				deque->rightblock->rightlink = b;
				deque->rightblock = b;
				deque->rightindex = 0;
			}
			deque->rightblock->data[deque->rightindex] = item;
		}
		Py_DECREF(it);
		if (PyErr_Occurred()) 
			return -1;
	}
	return 0;
}

static PySequenceMethods deque_as_sequence = {
	(inquiry)deque_len,		/* sq_length */
	0,				/* sq_concat */
};

/* deque object ********************************************************/

static PyObject *deque_iter(dequeobject *deque);

static PyMethodDef deque_methods[] = {
	{"append",		(PyCFunction)deque_append,	
		METH_O,		 append_doc},
	{"appendleft",		(PyCFunction)deque_appendleft,	
		METH_O,		 appendleft_doc},
	{"clear",		(PyCFunction)deque_clearmethod,	
		METH_NOARGS,	 clear_doc},
	{"__copy__",		(PyCFunction)deque_copy,	
		METH_NOARGS,	 copy_doc},
	{"pop",			(PyCFunction)deque_pop,	
		METH_NOARGS,	 pop_doc},
	{"popleft",		(PyCFunction)deque_popleft,	
		METH_NOARGS,	 popleft_doc},
	{"__reduce__",	(PyCFunction)deque_reduce,	
		METH_NOARGS,	 reduce_doc},
	{NULL,		NULL}	/* sentinel */
};

PyDoc_STRVAR(deque_doc,
"deque(iterable) --> deque object\n\
\n\
Build an ordered collection accessible from endpoints only.");

PyTypeObject deque_type = {
	PyObject_HEAD_INIT(NULL)
	0,				/* ob_size */
	"collections.deque",		/* tp_name */
	sizeof(dequeobject),		/* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)deque_dealloc,	/* tp_dealloc */
	(printfunc)deque_tp_print,	/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	(reprfunc)deque_repr,		/* tp_repr */
	0,				/* tp_as_number */
	&deque_as_sequence,		/* tp_as_sequence */
	0,				/* tp_as_mapping */
	deque_nohash,			/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,	/* tp_flags */
	deque_doc,			/* tp_doc */
	(traverseproc)set_traverse,	/* tp_traverse */
	(inquiry)deque_clear,		/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset*/
	(getiterfunc)deque_iter,	/* tp_iter */
	0,				/* tp_iternext */
	deque_methods,			/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	(initproc)deque_init,		/* tp_init */
	PyType_GenericAlloc,		/* tp_alloc */
	deque_new,			/* tp_new */
	PyObject_GC_Del,		/* tp_free */
};

/*********************** Deque Iterator **************************/

typedef struct {
	PyObject_HEAD
	int index;
	block *b;
	dequeobject *deque;
	int len;
} dequeiterobject;

PyTypeObject dequeiter_type;

static PyObject *
deque_iter(dequeobject *deque)
{
	dequeiterobject *it;

	it = PyObject_New(dequeiterobject, &dequeiter_type);
	if (it == NULL)
		return NULL;
	it->b = deque->leftblock;
	it->index = deque->leftindex;
	Py_INCREF(deque);
	it->deque = deque;
	it->len = deque->len;
	return (PyObject *)it;
}

static void
dequeiter_dealloc(dequeiterobject *dio)
{
	Py_XDECREF(dio->deque);
	dio->ob_type->tp_free(dio);
}

static PyObject *
dequeiter_next(dequeiterobject *it)
{
	PyObject *item;
	if (it->b == it->deque->rightblock && it->index > it->deque->rightindex)
		return NULL;

	if (it->len != it->deque->len) {
		it->len = -1; /* Make this state sticky */
		PyErr_SetString(PyExc_RuntimeError,
				"deque changed size during iteration");
		return NULL;
	}

	item = it->b->data[it->index];
	it->index++;
	if (it->index == BLOCKLEN && it->b->rightlink != NULL) {
		it->b = it->b->rightlink;
		it->index = 0;
	}
	Py_INCREF(item);
	return item;
}

PyTypeObject dequeiter_type = {
	PyObject_HEAD_INIT(NULL)
	0,					/* ob_size */
	"deque_iterator",			/* tp_name */
	sizeof(dequeiterobject),		/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	(destructor)dequeiter_dealloc,		/* tp_dealloc */
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
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
	0,					/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	PyObject_SelfIter,			/* tp_iter */
	(iternextfunc)dequeiter_next,		/* tp_iternext */
	0,
};

/* module level code ********************************************************/

PyDoc_STRVAR(module_doc,
"High performance data structures\n\
");

PyMODINIT_FUNC
initcollections(void)
{
	PyObject *m;

	m = Py_InitModule3("collections", NULL, module_doc);

	if (PyType_Ready(&deque_type) < 0)
		return;
	Py_INCREF(&deque_type);
	PyModule_AddObject(m, "deque", (PyObject *)&deque_type);

	if (PyType_Ready(&dequeiter_type) < 0)
		return;	

	return;
}
