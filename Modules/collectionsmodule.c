#include "Python.h"
#include "structmember.h"

/* collections module implementation of a deque() datatype
   Written and maintained by Raymond D. Hettinger <python@rcn.com>
   Copyright (c) 2004 Python Software Foundation.
   All rights reserved.
*/

/* The block length may be set to any number over 1.  Larger numbers
 * reduce the number of calls to the memory allocator but take more
 * memory.  Ideally, BLOCKLEN should be set with an eye to the
 * length of a cache line.  
 */

#define BLOCKLEN 62
#define CENTER ((BLOCKLEN - 1) / 2)

/* A `dequeobject` is composed of a doubly-linked list of `block` nodes.
 * This list is not circular (the leftmost block has leftlink==NULL,
 * and the rightmost block has rightlink==NULL).  A deque d's first
 * element is at d.leftblock[leftindex] and its last element is at
 * d.rightblock[rightindex]; note that, unlike as for Python slice
 * indices, these indices are inclusive on both ends.  By being inclusive
 * on both ends, algorithms for left and right operations become 
 * symmetrical which simplifies the design.
 * 
 * The list of blocks is never empty, so d.leftblock and d.rightblock
 * are never equal to NULL.
 *
 * The indices, d.leftindex and d.rightindex are always in the range
 *     0 <= index < BLOCKLEN.
 * Their exact relationship is:
 *     (d.leftindex + d.len - 1) % BLOCKLEN == d.rightindex.
 *
 * Empty deques have d.len == 0; d.leftblock==d.rightblock;
 * d.leftindex == CENTER+1; and d.rightindex == CENTER.
 * Checking for d.len == 0 is the intended way to see whether d is empty.
 *
 * Whenever d.leftblock == d.rightblock, 
 *     d.leftindex + d.len - 1 == d.rightindex.
 * 
 * However, when d.leftblock != d.rightblock, d.leftindex and d.rightindex
 * become indices into distinct blocks and either may be larger than the 
 * other.
 */

typedef struct BLOCK {
	struct BLOCK *leftlink;
	struct BLOCK *rightlink;
	PyObject *data[BLOCKLEN];
} block;

static block *
newblock(block *leftlink, block *rightlink, int len) {
	block *b;
	/* To prevent len from overflowing INT_MAX on 64-bit machines, we
	 * refuse to allocate new blocks if the current len is dangerously
	 * close.  There is some extra margin to prevent spurious arithmetic
	 * overflows at various places.  The following check ensures that
	 * the blocks allocated to the deque, in the worst case, can only
	 * have INT_MAX-2 entries in total.
	 */
	if (len >= INT_MAX - 2*BLOCKLEN) {
		PyErr_SetString(PyExc_OverflowError,
				"cannot add more blocks to the deque");
		return NULL;
	}
	b = PyMem_Malloc(sizeof(block));
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
	long state;	/* incremented whenever the indices move */
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

	b = newblock(NULL, NULL, 0);
	if (b == NULL) {
		Py_DECREF(deque);
		return NULL;
	}

	assert(BLOCKLEN >= 2);
	deque->leftblock = b;
	deque->rightblock = b;
	deque->leftindex = CENTER + 1;
	deque->rightindex = CENTER;
	deque->len = 0;
	deque->state = 0;
	deque->weakreflist = NULL;

	return (PyObject *)deque;
}

static PyObject *
deque_append(dequeobject *deque, PyObject *item)
{
	deque->state++;
	if (deque->rightindex == BLOCKLEN-1) {
		block *b = newblock(deque->rightblock, NULL, deque->len);
		if (b == NULL)
			return NULL;
		assert(deque->rightblock->rightlink == NULL);
		deque->rightblock->rightlink = b;
		deque->rightblock = b;
		deque->rightindex = -1;
	}
	Py_INCREF(item);
	deque->len++;
	deque->rightindex++;
	deque->rightblock->data[deque->rightindex] = item;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(append_doc, "Add an element to the right side of the deque.");

static PyObject *
deque_appendleft(dequeobject *deque, PyObject *item)
{
	deque->state++;
	if (deque->leftindex == 0) {
		block *b = newblock(NULL, deque->leftblock, deque->len);
		if (b == NULL)
			return NULL;
		assert(deque->leftblock->leftlink == NULL);
		deque->leftblock->leftlink = b;
		deque->leftblock = b;
		deque->leftindex = BLOCKLEN;
	}
	Py_INCREF(item);
	deque->len++;
	deque->leftindex--;
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
	deque->state++;

	if (deque->rightindex == -1) {
		if (deque->len == 0) {
			assert(deque->leftblock == deque->rightblock);
			assert(deque->leftindex == deque->rightindex+1);
			/* re-center instead of freeing a block */
			deque->leftindex = CENTER + 1;
			deque->rightindex = CENTER;
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
	deque->state++;

	if (deque->leftindex == BLOCKLEN) {
		if (deque->len == 0) {
			assert(deque->leftblock == deque->rightblock);
			assert(deque->leftindex == deque->rightindex+1);
			/* re-center instead of freeing a block */
			deque->leftindex = CENTER + 1;
			deque->rightindex = CENTER;
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
		deque->state++;
		if (deque->rightindex == BLOCKLEN-1) {
			block *b = newblock(deque->rightblock, NULL,
					    deque->len);
			if (b == NULL) {
				Py_DECREF(item);
				Py_DECREF(it);
				return NULL;
			}
			assert(deque->rightblock->rightlink == NULL);
			deque->rightblock->rightlink = b;
			deque->rightblock = b;
			deque->rightindex = -1;
		}
		deque->len++;
		deque->rightindex++;
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
		deque->state++;
		if (deque->leftindex == 0) {
			block *b = newblock(NULL, deque->leftblock,
					    deque->len);
			if (b == NULL) {
				Py_DECREF(item);
				Py_DECREF(it);
				return NULL;
			}
			assert(deque->leftblock->leftlink == NULL);
			deque->leftblock->leftlink = b;
			deque->leftblock = b;
			deque->leftindex = BLOCKLEN;
		}
		deque->len++;
		deque->leftindex--;
		deque->leftblock->data[deque->leftindex] = item;
	}
	Py_DECREF(it);
	if (PyErr_Occurred())
		return NULL;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(extendleft_doc,
"Extend the left side of the deque with elements from the iterable");

static int
_deque_rotate(dequeobject *deque, int n)
{
	int i, len=deque->len, halflen=(len+1)>>1;
	PyObject *item, *rv;

	if (len == 0)
		return 0;
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
			return -1;
		Py_DECREF(rv);
	}
	for (i=0 ; i>n ; i--) {
		item = deque_popleft(deque, NULL);
		assert (item != NULL);
		rv = deque_append(deque, item);
		Py_DECREF(item);
		if (rv == NULL)
			return -1;
		Py_DECREF(rv);
	}
	return 0;
}

static PyObject *
deque_rotate(dequeobject *deque, PyObject *args)
{
	int n=1;

	if (!PyArg_ParseTuple(args, "|i:rotate", &n))
		return NULL;
	if (_deque_rotate(deque, n) == 0)
		Py_RETURN_NONE;
	return NULL;
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

	while (deque->len) {
		item = deque_pop(deque, NULL);
		assert (item != NULL);
		Py_DECREF(item);
	}
	assert(deque->leftblock == deque->rightblock &&
	       deque->leftindex - 1 == deque->rightindex &&
	       deque->len == 0);
	return 0;
}

static PyObject *
deque_item(dequeobject *deque, int i)
{
	block *b;
	PyObject *item;
	int n, index=i;

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
		if (index < (deque->len >> 1)) {
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
	PyObject *item;

	assert (i >= 0 && i < deque->len);
	if (_deque_rotate(deque, -i) == -1)
		return -1;

	item = deque_popleft(deque, NULL);
	assert (item != NULL);
	Py_DECREF(item);

	return _deque_rotate(deque, i);
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
	block *b;
	PyObject *item;
	int index;
	int indexlo = deque->leftindex;

	for (b = deque->leftblock; b != NULL; b = b->rightlink) {
		const int indexhi = b == deque->rightblock ?
					 deque->rightindex :
				    	 BLOCKLEN - 1;

		for (index = indexlo; index <= indexhi; ++index) {
			item = b->data[index];
			Py_VISIT(item);
		}
		indexlo = 0;
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
	PyObject *dict, *result, *it;

	dict = PyObject_GetAttrString((PyObject *)deque, "__dict__");
	if (dict == NULL) {
		PyErr_Clear();
		dict = Py_None;
		Py_INCREF(dict);
	}
	it = PyObject_GetIter((PyObject *)deque);
	if (it == NULL) {
		Py_DECREF(dict);
		return NULL;
	}
	result = Py_BuildValue("O()ON", deque->ob_type, dict, it);
	Py_DECREF(dict);
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
	int b, vs, ws, cmp=-1;

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
	for (;;) {
		x = PyIter_Next(it1);
		if (x == NULL && PyErr_Occurred())
			goto done;
		y = PyIter_Next(it2);
		if (x == NULL || y == NULL)
			break;
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
	/* We reached the end of one deque or both */
	Py_XDECREF(x);
	Py_XDECREF(y);
	if (PyErr_Occurred())
		goto done;
	switch (op) {
	case Py_LT: cmp = y != NULL; break;  /* if w was longer */
	case Py_LE: cmp = x == NULL; break;  /* if v was not longer */
	case Py_EQ: cmp = x == y;    break;  /* if we reached the end of both */
	case Py_NE: cmp = x != y;    break;  /* if one deque continues */
	case Py_GT: cmp = x != NULL; break;  /* if v was longer */
	case Py_GE: cmp = y == NULL; break;  /* if w was not longer */
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
	long state;	/* state when the iterator is created */
	int counter;    /* number of items remaining for iteration */
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
	it->state = deque->state;
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

	if (it->counter == 0)
		return NULL;

	if (it->deque->state != it->state) {
		it->counter = 0;
		PyErr_SetString(PyExc_RuntimeError,
				"deque mutated during iteration");
		return NULL;
	}
	assert (!(it->b == it->deque->rightblock && 
		  it->index > it->deque->rightindex));

	item = it->b->data[it->index];
	it->index++;
	it->counter--;
	if (it->index == BLOCKLEN && it->counter > 0) {
		assert (it->b->rightlink != NULL);
		it->b = it->b->rightlink;
		it->index = 0;
	}
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
	it->state = deque->state;
	it->counter = deque->len;
	return (PyObject *)it;
}

static PyObject *
dequereviter_next(dequeiterobject *it)
{
	PyObject *item;
	if (it->counter == 0)
		return NULL;

	if (it->deque->state != it->state) {
		it->counter = 0;
		PyErr_SetString(PyExc_RuntimeError,
				"deque mutated during iteration");
		return NULL;
	}
	assert (!(it->b == it->deque->leftblock && 
		  it->index < it->deque->leftindex));

	item = it->b->data[it->index];
	it->index--;
	it->counter--;
	if (it->index == -1 && it->counter > 0) {
		assert (it->b->leftlink != NULL);
		it->b = it->b->leftlink;
		it->index = BLOCKLEN - 1;
	}
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
