#include "Python.h"
#include "structmember.h"

/* collections module implementation of a deque() datatype
   Written and maintained by Raymond D. Hettinger <python@rcn.com>
   Copyright (c) 2004 Python Software Foundation.
   All rights reserved.
*/

#define BLOCKLEN 46

/* A `dequeobject` is composed of a doubly-linked list of `block` nodes.
 * This list is not circular (the leftmost block has leftlink==NULL,
 * and the rightmost block has rightlink==NULL).  A deque d's first
 * element is at d.leftblock[leftindex] and its last element is at
 * d.rightblock[rightindex]; note that, unlike as for Python slice
 * indices, these indices are inclusive on both ends.
 * The list of blocks is never empty.  An empty deque d has
 * d.leftblock == d.rightblock != NULL; d.len == 0; and
 * d.leftindex > d.rightindex; checking for d.len == 0 is the intended
 * way to see whether d is empty.
 * Note that since d.leftindex and d.rightindex may be indices into
 * distinct blocks (and certainly are, for any d with len(d) > BLOCKLEN),
 * it's not generally true that d.leftindex <= d.rightindex.
 */

typedef struct BLOCK {
	struct BLOCK *leftlink;
	struct BLOCK *rightlink;
	PyObject *data[BLOCKLEN];
} block;

static block *
newblock(block *leftlink, block *rightlink) {
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
	int leftindex;	/* in range(BLOCKLEN) */
	int rightindex;	/* in range(BLOCKLEN) */
	int len;
	PyObject *weakreflist; /* List of weak references */
} dequeobject;

static PyTypeObject deque_type;

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
	deque->weakreflist = NULL;

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
		PyErr_SetString(PyExc_IndexError, "pop from an empty deque");
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
		PyErr_SetString(PyExc_IndexError, "pop from an empty deque");
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

static PyObject *
deque_extend(dequeobject *deque, PyObject *iterable)
{
	PyObject *it, *item;

	it = PyObject_GetIter(iterable);
	if (it == NULL)
		return NULL;

	while ((item = PyIter_Next(it)) != NULL) {
		deque->rightindex++;
		deque->len++;
		if (deque->rightindex == BLOCKLEN) {
			block *b = newblock(deque->rightblock, NULL);
			if (b == NULL) {
				Py_DECREF(item);
				Py_DECREF(it);
				return NULL;
			}
			assert(deque->rightblock->rightlink == NULL);
			deque->rightblock->rightlink = b;
			deque->rightblock = b;
			deque->rightindex = 0;
		}
		deque->rightblock->data[deque->rightindex] = item;
	}
	Py_DECREF(it);
	if (PyErr_Occurred())
		return NULL;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(extend_doc,
"Extend the right side of the deque with elements from the iterable");

static PyObject *
deque_extendleft(dequeobject *deque, PyObject *iterable)
{
	PyObject *it, *item;

	it = PyObject_GetIter(iterable);
	if (it == NULL)
		return NULL;

	while ((item = PyIter_Next(it)) != NULL) {
		deque->leftindex--;
		deque->len++;
		if (deque->leftindex == -1) {
			block *b = newblock(NULL, deque->leftblock);
			if (b == NULL) {
				Py_DECREF(item);
				Py_DECREF(it);
				return NULL;
			}
			assert(deque->leftblock->leftlink == NULL);
			deque->leftblock->leftlink = b;
			deque->leftblock = b;
			deque->leftindex = BLOCKLEN - 1;
		}
		deque->leftblock->data[deque->leftindex] = item;
	}
	Py_DECREF(it);
	if (PyErr_Occurred())
		return NULL;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(extendleft_doc,
"Extend the left side of the deque with elements from the iterable");

static PyObject *
deque_rotate(dequeobject *deque, PyObject *args)
{
	int i, n=1, len=deque->len, halflen=(len+1)>>1;
	PyObject *item, *rv;

	if (!PyArg_ParseTuple(args, "|i:rotate", &n))
		return NULL;

	if (len == 0)
		Py_RETURN_NONE;
	if (n > halflen || n < -halflen) {
		n %= len;
		if (n > halflen)
			n -= len;
		else if (n < -halflen)
			n += len;
	}

	for (i=0 ; i<n ; i++) {
		item = deque_pop(deque, NULL);
		assert (item != NULL);
		rv = deque_appendleft(deque, item);
		Py_DECREF(item);
		if (rv == NULL)
			return NULL;
		Py_DECREF(rv);
	}
	for (i=0 ; i>n ; i--) {
		item = deque_popleft(deque, NULL);
		assert (item != NULL);
		rv = deque_append(deque, item);
		Py_DECREF(item);
		if (rv == NULL)
			return NULL;
		Py_DECREF(rv);
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR(rotate_doc,
"Rotate the deque n steps to the right (default n=1).  If n is negative, rotates left.");

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
		assert (item != NULL);
		Py_DECREF(item);
	}
	assert(deque->leftblock == deque->rightblock &&
		deque->leftindex > deque->rightindex);
	return 0;
}

static PyObject *
deque_item(dequeobject *deque, int i)
{
	block *b;
	PyObject *item;
	int n;

	if (i < 0 || i >= deque->len) {
		PyErr_SetString(PyExc_IndexError,
				"deque index out of range");
		return NULL;
	}

	if (i == 0) {
		i = deque->leftindex;
		b = deque->leftblock;
	} else if (i == deque->len - 1) {
		i = deque->rightindex;
		b = deque->rightblock;
	} else {
		i += deque->leftindex;
		n = i / BLOCKLEN;
		i %= BLOCKLEN;
		if (i < (deque->len >> 1)) {
			b = deque->leftblock;
			while (n--)
				b = b->rightlink;
		} else {
			n = (deque->leftindex + deque->len - 1) / BLOCKLEN - n;
			b = deque->rightblock;
			while (n--)
				b = b->leftlink;
		}
	}
	item = b->data[i];
	Py_INCREF(item);
	return item;
}

/* delitem() implemented in terms of rotate for simplicity and reasonable
   performance near the end points.  If for some reason this method becomes
   popular, it is not hard to re-implement this using direct data movement
   (similar to code in list slice assignment) and achieve a two or threefold
   performance boost.
*/

static int
deque_del_item(dequeobject *deque, int i)
{
	PyObject *item=NULL, *minus_i=NULL, *plus_i=NULL;
	int rv = -1;

	assert (i >= 0 && i < deque->len);

	minus_i = Py_BuildValue("(i)", -i);
	if (minus_i == NULL)
		goto fail;

	plus_i = Py_BuildValue("(i)", i);
	if (plus_i == NULL)
		goto fail;

	item = deque_rotate(deque, minus_i);
	if (item == NULL)
		goto fail;
	Py_DECREF(item);

	item = deque_popleft(deque, NULL);
	assert (item != NULL);
	Py_DECREF(item);

	item = deque_rotate(deque, plus_i);
	if (item == NULL)
		goto fail;

	rv = 0;
fail:
	Py_XDECREF(item);
	Py_XDECREF(minus_i);
	Py_XDECREF(plus_i);
	return rv;
}

static int
deque_ass_item(dequeobject *deque, int i, PyObject *v)
{
	PyObject *old_value;
	block *b;
	int n, len=deque->len, halflen=(len+1)>>1, index=i;

	if (i < 0 || i >= len) {
		PyErr_SetString(PyExc_IndexError,
				"deque index out of range");
		return -1;
	}
	if (v == NULL)
		return deque_del_item(deque, i);

	i += deque->leftindex;
	n = i / BLOCKLEN;
	i %= BLOCKLEN;
	if (index <= halflen) {
		b = deque->leftblock;
		while (n--)
			b = b->rightlink;
	} else {
		n = (deque->leftindex + len - 1) / BLOCKLEN - n;
		b = deque->rightblock;
		while (n--)
			b = b->leftlink;
	}
	Py_INCREF(v);
	old_value = b->data[i];
	b->data[i] = v;
	Py_DECREF(old_value);
	return 0;
}

static PyObject *
deque_clearmethod(dequeobject *deque)
{
	int rv;

	rv = deque_clear(deque);
	assert (rv != -1);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(clear_doc, "Remove all elements from the deque.");

static void
deque_dealloc(dequeobject *deque)
{
	PyObject_GC_UnTrack(deque);
	if (deque->weakreflist != NULL)
		PyObject_ClearWeakRefs((PyObject *) deque);
	if (deque->leftblock != NULL) {
		deque_clear(deque);
		assert(deque->leftblock != NULL);
		PyMem_Free(deque->leftblock);
	}
	deque->leftblock = NULL;
	deque->rightblock = NULL;
	deque->ob_type->tp_free(deque);
}

static int
deque_traverse(dequeobject *deque, visitproc visit, void *arg)
{
	block * b = deque->leftblock;
	int index = deque->leftindex;
	PyObject *item;

	while (b != deque->rightblock || index <= deque->rightindex) {
		item = b->data[index];
		index++;
		if (index == BLOCKLEN ) {
			assert(b->rightlink != NULL);
			b = b->rightlink;
			index = 0;
		}
		Py_VISIT(item);
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

static PyObject *
deque_richcompare(PyObject *v, PyObject *w, int op)
{
	PyObject *it1=NULL, *it2=NULL, *x, *y;
	int i, b, vs, ws, minlen, cmp=-1;

	if (!PyObject_TypeCheck(v, &deque_type) ||
	    !PyObject_TypeCheck(w, &deque_type)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	/* Shortcuts */
	vs = ((dequeobject *)v)->len;
	ws = ((dequeobject *)w)->len;
	if (op == Py_EQ) {
		if (v == w)
			Py_RETURN_TRUE;
		if (vs != ws)
			Py_RETURN_FALSE;
	}
	if (op == Py_NE) {
		if (v == w)
			Py_RETURN_FALSE;
		if (vs != ws)
			Py_RETURN_TRUE;
	}

	/* Search for the first index where items are different */
	it1 = PyObject_GetIter(v);
	if (it1 == NULL)
		goto done;
	it2 = PyObject_GetIter(w);
	if (it2 == NULL)
		goto done;
	minlen = (vs < ws)  ?  vs  :  ws;
	for (i=0 ; i < minlen ; i++) {
		x = PyIter_Next(it1);
		if (x == NULL)
			goto done;
		y = PyIter_Next(it2);
		if (y == NULL) {
			Py_DECREF(x);
			goto done;
		}
		b = PyObject_RichCompareBool(x, y, Py_EQ);
		if (b == 0) {
			cmp = PyObject_RichCompareBool(x, y, op);
			Py_DECREF(x);
			Py_DECREF(y);
			goto done;
		}
		Py_DECREF(x);
		Py_DECREF(y);
		if (b == -1)
			goto done;
	}
	/* Elements are equal through minlen.  The longest input is the greatest */
	switch (op) {
	case Py_LT: cmp = vs <  ws; break;
	case Py_LE: cmp = vs <= ws; break;
	case Py_EQ: cmp = vs == ws; break;
	case Py_NE: cmp = vs != ws; break;
	case Py_GT: cmp = vs >  ws; break;
	case Py_GE: cmp = vs >= ws; break;
	}

done:
	Py_XDECREF(it1);
	Py_XDECREF(it2);
	if (cmp == 1)
		Py_RETURN_TRUE;
	if (cmp == 0)
		Py_RETURN_FALSE;
	return NULL;
}

static int
deque_init(dequeobject *deque, PyObject *args, PyObject *kwds)
{
	PyObject *iterable = NULL;

	if (!PyArg_UnpackTuple(args, "deque", 0, 1, &iterable))
		return -1;

	if (iterable != NULL) {
		PyObject *rv = deque_extend(deque, iterable);
		if (rv == NULL)
			return -1;
		Py_DECREF(rv);
	}
	return 0;
}

static PySequenceMethods deque_as_sequence = {
	(inquiry)deque_len,		/* sq_length */
	0,				/* sq_concat */
	0,				/* sq_repeat */
	(intargfunc)deque_item,		/* sq_item */
	0,				/* sq_slice */
	(intobjargproc)deque_ass_item,	/* sq_ass_item */
};

/* deque object ********************************************************/

static PyObject *deque_iter(dequeobject *deque);
static PyObject *deque_reviter(dequeobject *deque);
PyDoc_STRVAR(reversed_doc,
	"D.__reversed__() -- return a reverse iterator over the deque");

static PyMethodDef deque_methods[] = {
	{"append",		(PyCFunction)deque_append,
		METH_O,		 append_doc},
	{"appendleft",		(PyCFunction)deque_appendleft,
		METH_O,		 appendleft_doc},
	{"clear",		(PyCFunction)deque_clearmethod,
		METH_NOARGS,	 clear_doc},
	{"__copy__",		(PyCFunction)deque_copy,
		METH_NOARGS,	 copy_doc},
	{"extend",		(PyCFunction)deque_extend,
		METH_O,		 extend_doc},
	{"extendleft",	(PyCFunction)deque_extendleft,
		METH_O,		 extendleft_doc},
	{"pop",			(PyCFunction)deque_pop,
		METH_NOARGS,	 pop_doc},
	{"popleft",		(PyCFunction)deque_popleft,
		METH_NOARGS,	 popleft_doc},
	{"__reduce__",	(PyCFunction)deque_reduce,
		METH_NOARGS,	 reduce_doc},
	{"__reversed__",	(PyCFunction)deque_reviter,
		METH_NOARGS,	 reversed_doc},
	{"rotate",		(PyCFunction)deque_rotate,
		METH_VARARGS,	rotate_doc},
	{NULL,		NULL}	/* sentinel */
};

PyDoc_STRVAR(deque_doc,
"deque(iterable) --> deque object\n\
\n\
Build an ordered collection accessible from endpoints only.");

static PyTypeObject deque_type = {
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
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_HAVE_WEAKREFS,	/* tp_flags */
	deque_doc,			/* tp_doc */
	(traverseproc)deque_traverse,	/* tp_traverse */
	(inquiry)deque_clear,		/* tp_clear */
	(richcmpfunc)deque_richcompare,	/* tp_richcompare */
	offsetof(dequeobject, weakreflist),	/* tp_weaklistoffset*/
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
	int counter;
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
	it->counter = deque->len;
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
		it->counter = 0;
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
	it->counter--;
	Py_INCREF(item);
	return item;
}

static int
dequeiter_len(dequeiterobject *it)
{
	return it->counter;
}

static PySequenceMethods dequeiter_as_sequence = {
	(inquiry)dequeiter_len,		/* sq_length */
	0,				/* sq_concat */
};

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
	&dequeiter_as_sequence,			/* tp_as_sequence */
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

/*********************** Deque Reverse Iterator **************************/

PyTypeObject dequereviter_type;

static PyObject *
deque_reviter(dequeobject *deque)
{
	dequeiterobject *it;

	it = PyObject_New(dequeiterobject, &dequereviter_type);
	if (it == NULL)
		return NULL;
	it->b = deque->rightblock;
	it->index = deque->rightindex;
	Py_INCREF(deque);
	it->deque = deque;
	it->len = deque->len;
	it->counter = deque->len;
	return (PyObject *)it;
}

static PyObject *
dequereviter_next(dequeiterobject *it)
{
	PyObject *item;
	if (it->b == it->deque->leftblock && it->index < it->deque->leftindex)
		return NULL;

	if (it->len != it->deque->len) {
		it->len = -1; /* Make this state sticky */
		it->counter = 0;
		PyErr_SetString(PyExc_RuntimeError,
				"deque changed size during iteration");
		return NULL;
	}

	item = it->b->data[it->index];
	it->index--;
	if (it->index == -1 && it->b->leftlink != NULL) {
		it->b = it->b->leftlink;
		it->index = BLOCKLEN - 1;
	}
	it->counter--;
	Py_INCREF(item);
	return item;
}

PyTypeObject dequereviter_type = {
	PyObject_HEAD_INIT(NULL)
	0,					/* ob_size */
	"deque_reverse_iterator",		/* tp_name */
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
	&dequeiter_as_sequence,			/* tp_as_sequence */
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
	(iternextfunc)dequereviter_next,	/* tp_iternext */
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

	if (PyType_Ready(&dequereviter_type) < 0)
		return;

	return;
}
