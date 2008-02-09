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

#define MAXFREEBLOCKS 10
static int numfreeblocks = 0;
static block *freeblocks[MAXFREEBLOCKS];

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
	if (numfreeblocks) {
		numfreeblocks -= 1;
		b = freeblocks[numfreeblocks];
	} else {
		b = PyMem_Malloc(sizeof(block));
		if (b == NULL) {
			PyErr_NoMemory();
			return NULL;
		}
	}
	b->leftlink = leftlink;
	b->rightlink = rightlink;
	return b;
}

void
freeblock(block *b)
{
	if (numfreeblocks < MAXFREEBLOCKS) {
		freeblocks[numfreeblocks] = b;
		numfreeblocks++;
	} else {
		PyMem_Free(b);
	}
}

typedef struct {
	PyObject_HEAD
	block *leftblock;
	block *rightblock;
	int leftindex;	/* in range(BLOCKLEN) */
	int rightindex;	/* in range(BLOCKLEN) */
	int len;
	int maxlen;
	long state;	/* incremented whenever the indices move */
	PyObject *weakreflist; /* List of weak references */
} dequeobject;

/* The deque's size limit is d.maxlen.  The limit can be zero or positive.
 * If there is no limit, then d.maxlen == -1.
 * 
 * After an item is added to a deque, we check to see if the size has grown past
 * the limit. If it has, we get the size back down to the limit by popping an
 * item off of the opposite end.  The methods that can trigger this are append(),
 * appendleft(), extend(), and extendleft().
 */

#define TRIM(d, popfunction)                               	\
    if (d->maxlen != -1 && d->len > d->maxlen) {              	\
            PyObject *rv = popfunction(d, NULL);                \
            assert(rv != NULL  &&  d->len <= d->maxlen);        \
            Py_DECREF(rv);                                      \
    }

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
	deque->maxlen = -1;

	return (PyObject *)deque;
}

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
			freeblock(deque->rightblock);
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
	assert(deque->leftblock != NULL);
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
			freeblock(deque->leftblock);
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
	TRIM(deque, deque_popleft);
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
	TRIM(deque, deque_pop);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(appendleft_doc, "Add an element to the left side of the deque.");

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
		TRIM(deque, deque_popleft);               
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
		TRIM(deque, deque_pop);               
	}
	Py_DECREF(it);
	if (PyErr_Occurred())
		return NULL;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(extendleft_doc,
"Extend the left side of the deque with elements from the iterable");

static int
_deque_rotate(dequeobject *deque, Py_ssize_t n)
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

static Py_ssize_t
deque_len(dequeobject *deque)
{
	return deque->len;
}

static PyObject *
deque_remove(dequeobject *deque, PyObject *value)
{
	Py_ssize_t i, n=deque->len;

	for (i=0 ; i<n ; i++) {
		PyObject *item = deque->leftblock->data[deque->leftindex];
		int cmp = PyObject_RichCompareBool(item, value, Py_EQ);

		if (deque->len != n) {
			PyErr_SetString(PyExc_IndexError,
				"deque mutated during remove().");
			return NULL;
		}
		if (cmp > 0) {
			PyObject *tgt = deque_popleft(deque, NULL);
			assert (tgt != NULL);
			Py_DECREF(tgt);
			if (_deque_rotate(deque, i) == -1)
				return NULL;
			Py_RETURN_NONE;
		}
		else if (cmp < 0) {
			_deque_rotate(deque, i);
			return NULL;
		}
		_deque_rotate(deque, -1);
	}
	PyErr_SetString(PyExc_ValueError, "deque.remove(x): x not in deque");
	return NULL;
}

PyDoc_STRVAR(remove_doc,
"D.remove(value) -- remove first occurrence of value.");

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
deque_del_item(dequeobject *deque, Py_ssize_t i)
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
deque_ass_item(dequeobject *deque, Py_ssize_t i, PyObject *v)
{
	PyObject *old_value;
	block *b;
	Py_ssize_t n, len=deque->len, halflen=(len+1)>>1, index=i;

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
		freeblock(deque->leftblock);
	}
	deque->leftblock = NULL;
	deque->rightblock = NULL;
	Py_TYPE(deque)->tp_free(deque);
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
	if (((dequeobject *)deque)->maxlen == -1)
		return PyObject_CallFunction((PyObject *)(Py_TYPE(deque)), "O", deque, NULL);
	else
		return PyObject_CallFunction((PyObject *)(Py_TYPE(deque)), "Oi",
			deque, ((dequeobject *)deque)->maxlen, NULL);
}

PyDoc_STRVAR(copy_doc, "Return a shallow copy of a deque.");

static PyObject *
deque_reduce(dequeobject *deque)
{
	PyObject *dict, *result, *aslist;

	dict = PyObject_GetAttrString((PyObject *)deque, "__dict__");
	if (dict == NULL)
		PyErr_Clear();
	aslist = PySequence_List((PyObject *)deque);
	if (aslist == NULL) {
		Py_XDECREF(dict);
		return NULL;
	}
	if (dict == NULL) {
		if (deque->maxlen == -1)
			result = Py_BuildValue("O(O)", Py_TYPE(deque), aslist);
		else
			result = Py_BuildValue("O(Oi)", Py_TYPE(deque), aslist, deque->maxlen);
	} else {
		if (deque->maxlen == -1)
			result = Py_BuildValue("O(OO)O", Py_TYPE(deque), aslist, Py_None, dict);
		else
			result = Py_BuildValue("O(Oi)O", Py_TYPE(deque), aslist, deque->maxlen, dict);
	}
	Py_XDECREF(dict);
	Py_DECREF(aslist);
	return result;
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyObject *
deque_repr(PyObject *deque)
{
	PyObject *aslist, *result;
	int i;

	i = Py_ReprEnter(deque);
	if (i != 0) {
		if (i < 0)
			return NULL;
		return PyUnicode_FromString("[...]");
	}

	aslist = PySequence_List(deque);
	if (aslist == NULL) {
		Py_ReprLeave(deque);
		return NULL;
	}
	if (((dequeobject *)deque)->maxlen != -1)
		result = PyUnicode_FromFormat("deque(%R, maxlen=%i)", aslist,
                                              ((dequeobject *)deque)->maxlen);
	else
		result = PyUnicode_FromFormat("deque(%R)", aslist);
	Py_DECREF(aslist);
	Py_ReprLeave(deque);
	return result;
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
deque_init(dequeobject *deque, PyObject *args, PyObject *kwdargs)
{
	PyObject *iterable = NULL;
	PyObject *maxlenobj = NULL;
	int maxlen = -1;
	char *kwlist[] = {"iterable", "maxlen", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwdargs, "|OO:deque", kwlist, &iterable, &maxlenobj))
		return -1;
	if (maxlenobj != NULL && maxlenobj != Py_None) {
		maxlen = PyLong_AsLong(maxlenobj);
		if (maxlen == -1 && PyErr_Occurred())
			return -1;
		if (maxlen < 0) {
			PyErr_SetString(PyExc_ValueError, "maxlen must be non-negative");
			return -1;
		}
	}
	deque->maxlen = maxlen;
	deque_clear(deque);
	if (iterable != NULL) {
		PyObject *rv = deque_extend(deque, iterable);
		if (rv == NULL)
			return -1;
		Py_DECREF(rv);
	}
	return 0;
}

static PySequenceMethods deque_as_sequence = {
	(lenfunc)deque_len,		/* sq_length */
	0,				/* sq_concat */
	0,				/* sq_repeat */
	(ssizeargfunc)deque_item,	/* sq_item */
	0,				/* sq_slice */
	(ssizeobjargproc)deque_ass_item,	/* sq_ass_item */
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
	{"extendleft",		(PyCFunction)deque_extendleft,
		METH_O,		 extendleft_doc},
	{"pop",			(PyCFunction)deque_pop,
		METH_NOARGS,	 pop_doc},
	{"popleft",		(PyCFunction)deque_popleft,
		METH_NOARGS,	 popleft_doc},
	{"__reduce__",	(PyCFunction)deque_reduce,
		METH_NOARGS,	 reduce_doc},
	{"remove",		(PyCFunction)deque_remove,
		METH_O,		 remove_doc},
	{"__reversed__",	(PyCFunction)deque_reviter,
		METH_NOARGS,	 reversed_doc},
	{"rotate",		(PyCFunction)deque_rotate,
		METH_VARARGS,	rotate_doc},
	{NULL,		NULL}	/* sentinel */
};

PyDoc_STRVAR(deque_doc,
"deque(iterable[, maxlen]) --> deque object\n\
\n\
Build an ordered collection accessible from endpoints only.");

static PyTypeObject deque_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"collections.deque",		/* tp_name */
	sizeof(dequeobject),		/* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)deque_dealloc,	/* tp_dealloc */
	0,				/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	deque_repr,			/* tp_repr */
	0,				/* tp_as_number */
	&deque_as_sequence,		/* tp_as_sequence */
	0,				/* tp_as_mapping */
	deque_nohash,			/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
	                                /* tp_flags */
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
	Py_TYPE(dio)->tp_free(dio);
}

static PyObject *
dequeiter_next(dequeiterobject *it)
{
	PyObject *item;

	if (it->deque->state != it->state) {
		it->counter = 0;
		PyErr_SetString(PyExc_RuntimeError,
				"deque mutated during iteration");
		return NULL;
	}
	if (it->counter == 0)
		return NULL;        
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

static PyObject *
dequeiter_len(dequeiterobject *it)
{
	return PyLong_FromLong(it->counter);
}

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

static PyMethodDef dequeiter_methods[] = {
	{"__length_hint__", (PyCFunction)dequeiter_len, METH_NOARGS, length_hint_doc},
 	{NULL,		NULL}		/* sentinel */
};

PyTypeObject dequeiter_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
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
	dequeiter_methods,			/* tp_methods */
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
	PyVarObject_HEAD_INIT(NULL, 0)
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
	(iternextfunc)dequereviter_next,	/* tp_iternext */
	dequeiter_methods,			/* tp_methods */
	0,
};

/* defaultdict type *********************************************************/

typedef struct {
	PyDictObject dict;
	PyObject *default_factory;
} defdictobject;

static PyTypeObject defdict_type; /* Forward */

PyDoc_STRVAR(defdict_missing_doc,
"__missing__(key) # Called by __getitem__ for missing key; pseudo-code:\n\
  if self.default_factory is None: raise KeyError((key,))\n\
  self[key] = value = self.default_factory()\n\
  return value\n\
");

static PyObject *
defdict_missing(defdictobject *dd, PyObject *key)
{
	PyObject *factory = dd->default_factory;
	PyObject *value;
	if (factory == NULL || factory == Py_None) {
		/* XXX Call dict.__missing__(key) */
		PyObject *tup;
		tup = PyTuple_Pack(1, key);
		if (!tup) return NULL;
		PyErr_SetObject(PyExc_KeyError, tup);
		Py_DECREF(tup);
		return NULL;
	}
	value = PyEval_CallObject(factory, NULL);
	if (value == NULL)
		return value;
	if (PyObject_SetItem((PyObject *)dd, key, value) < 0) {
		Py_DECREF(value);
		return NULL;
	}
	return value;
}

PyDoc_STRVAR(defdict_copy_doc, "D.copy() -> a shallow copy of D.");

static PyObject *
defdict_copy(defdictobject *dd)
{
	/* This calls the object's class.  That only works for subclasses
	   whose class constructor has the same signature.  Subclasses that
	   define a different constructor signature must override __copy__().
	*/
	return PyObject_CallFunctionObjArgs((PyObject*)Py_TYPE(dd),
					    dd->default_factory, dd, NULL);
}

static PyObject *
defdict_reduce(defdictobject *dd)
{
	/* __reduce__ must return a 5-tuple as follows:

	   - factory function
	   - tuple of args for the factory function
	   - additional state (here None)
	   - sequence iterator (here None)
	   - dictionary iterator (yielding successive (key, value) pairs

	   This API is used by pickle.py and copy.py.

	   For this to be useful with pickle.py, the default_factory
	   must be picklable; e.g., None, a built-in, or a global
	   function in a module or package.

	   Both shallow and deep copying are supported, but for deep
	   copying, the default_factory must be deep-copyable; e.g. None,
	   or a built-in (functions are not copyable at this time).

	   This only works for subclasses as long as their constructor
	   signature is compatible; the first argument must be the
	   optional default_factory, defaulting to None.
	*/
	PyObject *args;
	PyObject *items;
	PyObject *result;
	if (dd->default_factory == NULL || dd->default_factory == Py_None)
		args = PyTuple_New(0);
	else
		args = PyTuple_Pack(1, dd->default_factory);
	if (args == NULL)
		return NULL;
	items = PyObject_CallMethod((PyObject *)dd, "items", "()");
	if (items == NULL) {
		Py_DECREF(args);
		return NULL;
	}
	result = PyTuple_Pack(5, Py_TYPE(dd), args,
			      Py_None, Py_None, items);
	Py_DECREF(items);
	Py_DECREF(args);
	return result;
}

static PyMethodDef defdict_methods[] = {
	{"__missing__", (PyCFunction)defdict_missing, METH_O,
	 defdict_missing_doc},
	{"__copy__", (PyCFunction)defdict_copy, METH_NOARGS,
	 defdict_copy_doc},
	{"__reduce__", (PyCFunction)defdict_reduce, METH_NOARGS,
	 reduce_doc},
	{NULL}
};

static PyMemberDef defdict_members[] = {
	{"default_factory", T_OBJECT,
	 offsetof(defdictobject, default_factory), 0,
	 PyDoc_STR("Factory for default value called by __missing__().")},
	{NULL}
};

static void
defdict_dealloc(defdictobject *dd)
{
	Py_CLEAR(dd->default_factory);
	PyDict_Type.tp_dealloc((PyObject *)dd);
}

static PyObject *
defdict_repr(defdictobject *dd)
{
	PyObject *baserepr;
	PyObject *defrepr;
	PyObject *result;
	baserepr = PyDict_Type.tp_repr((PyObject *)dd);
	if (baserepr == NULL)
		return NULL;
	if (dd->default_factory == NULL)
		defrepr = PyUnicode_FromString("None");
	else
	{
		int status = Py_ReprEnter(dd->default_factory);
		if (status != 0) {
			if (status < 0)
				return NULL;
			defrepr = PyUnicode_FromString("...");
		}
		else
			defrepr = PyObject_Repr(dd->default_factory);
		Py_ReprLeave(dd->default_factory);
	}
	if (defrepr == NULL) {
		Py_DECREF(baserepr);
		return NULL;
	}
	result = PyUnicode_FromFormat("defaultdict(%U, %U)",
				      defrepr, baserepr);
	Py_DECREF(defrepr);
	Py_DECREF(baserepr);
	return result;
}

static int
defdict_traverse(PyObject *self, visitproc visit, void *arg)
{
	Py_VISIT(((defdictobject *)self)->default_factory);
	return PyDict_Type.tp_traverse(self, visit, arg);
}

static int
defdict_tp_clear(defdictobject *dd)
{
	Py_CLEAR(dd->default_factory);
	return PyDict_Type.tp_clear((PyObject *)dd);
}

static int
defdict_init(PyObject *self, PyObject *args, PyObject *kwds)
{
	defdictobject *dd = (defdictobject *)self;
	PyObject *olddefault = dd->default_factory;
	PyObject *newdefault = NULL;
	PyObject *newargs;
	int result;
	if (args == NULL || !PyTuple_Check(args))
		newargs = PyTuple_New(0);
	else {
		Py_ssize_t n = PyTuple_GET_SIZE(args);
		if (n > 0) {
			newdefault = PyTuple_GET_ITEM(args, 0);
			if (!PyCallable_Check(newdefault)) {
				PyErr_SetString(PyExc_TypeError,
					"first argument must be callable");                           
				return -1;
			}
		}
		newargs = PySequence_GetSlice(args, 1, n);
	}
	if (newargs == NULL)
		return -1;
	Py_XINCREF(newdefault);
	dd->default_factory = newdefault;
	result = PyDict_Type.tp_init(self, newargs, kwds);
	Py_DECREF(newargs);
	Py_XDECREF(olddefault);
	return result;
}

PyDoc_STRVAR(defdict_doc,
"defaultdict(default_factory) --> dict with default factory\n\
\n\
The default factory is called without arguments to produce\n\
a new value when a key is not present, in __getitem__ only.\n\
A defaultdict compares equal to a dict with the same items.\n\
");

/* See comment in xxsubtype.c */
#define DEFERRED_ADDRESS(ADDR) 0

static PyTypeObject defdict_type = {
	PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
	"collections.defaultdict",	/* tp_name */
	sizeof(defdictobject),		/* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)defdict_dealloc,	/* tp_dealloc */
	0,				/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	(reprfunc)defdict_repr,		/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,	       			/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
                                        /* tp_flags */
	defdict_doc,			/* tp_doc */
	defdict_traverse,		/* tp_traverse */
	(inquiry)defdict_tp_clear,	/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset*/
	0,				/* tp_iter */
	0,				/* tp_iternext */
	defdict_methods,		/* tp_methods */
	defdict_members,		/* tp_members */
	0,				/* tp_getset */
	DEFERRED_ADDRESS(&PyDict_Type),	/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	defdict_init,			/* tp_init */
	PyType_GenericAlloc,		/* tp_alloc */
	0,				/* tp_new */
	PyObject_GC_Del,		/* tp_free */
};

/* module level code ********************************************************/

PyDoc_STRVAR(module_doc,
"High performance data structures.\n\
- deque:        ordered collection accessible from endpoints only\n\
- defaultdict:  dict subclass with a default value factory\n\
");

PyMODINIT_FUNC
init_collections(void)
{
	PyObject *m;

	m = Py_InitModule3("_collections", NULL, module_doc);
	if (m == NULL)
		return;

	if (PyType_Ready(&deque_type) < 0)
		return;
	Py_INCREF(&deque_type);
	PyModule_AddObject(m, "deque", (PyObject *)&deque_type);

	defdict_type.tp_base = &PyDict_Type;
	if (PyType_Ready(&defdict_type) < 0)
		return;
	Py_INCREF(&defdict_type);
	PyModule_AddObject(m, "defaultdict", (PyObject *)&defdict_type);

	if (PyType_Ready(&dequeiter_type) < 0)
		return;

	if (PyType_Ready(&dequereviter_type) < 0)
		return;

	return;
}
