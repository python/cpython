
/* List object implementation */

#include "Python.h"

#ifdef STDC_HEADERS
#include <stddef.h>
#else
#include <sys/types.h>		/* For size_t */
#endif

#define ROUNDUP(n, PyTryBlock) \
	((((n)+(PyTryBlock)-1)/(PyTryBlock))*(PyTryBlock))

static int
roundupsize(int n)
{
	if (n < 500)
		return ROUNDUP(n, 10);
	else
		return ROUNDUP(n, 100);
}

#define NRESIZE(var, type, nitems) PyMem_RESIZE(var, type, roundupsize(nitems))

PyObject *
PyList_New(int size)
{
	int i;
	PyListObject *op;
	size_t nbytes;
	if (size < 0) {
		PyErr_BadInternalCall();
		return NULL;
	}
	nbytes = size * sizeof(PyObject *);
	/* Check for overflow */
	if (nbytes / sizeof(PyObject *) != (size_t)size) {
		return PyErr_NoMemory();
	}
	/* PyObject_NewVar is inlined */
	op = (PyListObject *) PyObject_MALLOC(sizeof(PyListObject)
						+ PyGC_HEAD_SIZE);
	if (op == NULL) {
		return PyErr_NoMemory();
	}
	op = (PyListObject *) PyObject_FROM_GC(op);
	if (size <= 0) {
		op->ob_item = NULL;
	}
	else {
		op->ob_item = (PyObject **) PyMem_MALLOC(nbytes);
		if (op->ob_item == NULL) {
			PyObject_FREE(PyObject_AS_GC(op));
			return PyErr_NoMemory();
		}
	}
	PyObject_INIT_VAR(op, &PyList_Type, size);
	for (i = 0; i < size; i++)
		op->ob_item[i] = NULL;
	PyObject_GC_Init(op);
	return (PyObject *) op;
}

int
PyList_Size(PyObject *op)
{
	if (!PyList_Check(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	else
		return ((PyListObject *)op) -> ob_size;
}

static PyObject *indexerr;

PyObject *
PyList_GetItem(PyObject *op, int i)
{
	if (!PyList_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	if (i < 0 || i >= ((PyListObject *)op) -> ob_size) {
		if (indexerr == NULL)
			indexerr = PyString_FromString(
				"list index out of range");
		PyErr_SetObject(PyExc_IndexError, indexerr);
		return NULL;
	}
	return ((PyListObject *)op) -> ob_item[i];
}

int
PyList_SetItem(register PyObject *op, register int i,
               register PyObject *newitem)
{
	register PyObject *olditem;
	register PyObject **p;
	if (!PyList_Check(op)) {
		Py_XDECREF(newitem);
		PyErr_BadInternalCall();
		return -1;
	}
	if (i < 0 || i >= ((PyListObject *)op) -> ob_size) {
		Py_XDECREF(newitem);
		PyErr_SetString(PyExc_IndexError,
				"list assignment index out of range");
		return -1;
	}
	p = ((PyListObject *)op) -> ob_item + i;
	olditem = *p;
	*p = newitem;
	Py_XDECREF(olditem);
	return 0;
}

static int
ins1(PyListObject *self, int where, PyObject *v)
{
	int i;
	PyObject **items;
	if (v == NULL) {
		PyErr_BadInternalCall();
		return -1;
	}
	if (self->ob_size == INT_MAX) {
		PyErr_SetString(PyExc_OverflowError,
			"cannot add more objects to list");
		return -1;
	}
	items = self->ob_item;
	NRESIZE(items, PyObject *, self->ob_size+1);
	if (items == NULL) {
		PyErr_NoMemory();
		return -1;
	}
	if (where < 0)
		where = 0;
	if (where > self->ob_size)
		where = self->ob_size;
	for (i = self->ob_size; --i >= where; )
		items[i+1] = items[i];
	Py_INCREF(v);
	items[where] = v;
	self->ob_item = items;
	self->ob_size++;
	return 0;
}

int
PyList_Insert(PyObject *op, int where, PyObject *newitem)
{
	if (!PyList_Check(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return ins1((PyListObject *)op, where, newitem);
}

int
PyList_Append(PyObject *op, PyObject *newitem)
{
	if (!PyList_Check(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return ins1((PyListObject *)op,
		(int) ((PyListObject *)op)->ob_size, newitem);
}

/* Methods */

static void
list_dealloc(PyListObject *op)
{
	int i;
	Py_TRASHCAN_SAFE_BEGIN(op)
	PyObject_GC_Fini(op);
	if (op->ob_item != NULL) {
		/* Do it backwards, for Christian Tismer.
		   There's a simple test case where somehow this reduces
		   thrashing when a *very* large list is created and
		   immediately deleted. */
		i = op->ob_size;
		while (--i >= 0) {
			Py_XDECREF(op->ob_item[i]);
		}
		PyMem_FREE(op->ob_item);
	}
	op = (PyListObject *) PyObject_AS_GC(op);
	PyObject_DEL(op);
	Py_TRASHCAN_SAFE_END(op)
}

static int
list_print(PyListObject *op, FILE *fp, int flags)
{
	int i;

	i = Py_ReprEnter((PyObject*)op);
	if (i != 0) {
		if (i < 0)
			return i;
		fprintf(fp, "[...]");
		return 0;
	}
	fprintf(fp, "[");
	for (i = 0; i < op->ob_size; i++) {
		if (i > 0)
			fprintf(fp, ", ");
		if (PyObject_Print(op->ob_item[i], fp, 0) != 0) {
			Py_ReprLeave((PyObject *)op);
			return -1;
		}
	}
	fprintf(fp, "]");
	Py_ReprLeave((PyObject *)op);
	return 0;
}

static PyObject *
list_repr(PyListObject *v)
{
	PyObject *s, *comma;
	int i;

	i = Py_ReprEnter((PyObject*)v);
	if (i != 0) {
		if (i > 0)
			return PyString_FromString("[...]");
		return NULL;
	}
	s = PyString_FromString("[");
	comma = PyString_FromString(", ");
	for (i = 0; i < v->ob_size && s != NULL; i++) {
		if (i > 0)
			PyString_Concat(&s, comma);
		PyString_ConcatAndDel(&s, PyObject_Repr(v->ob_item[i]));
	}
	Py_XDECREF(comma);
	PyString_ConcatAndDel(&s, PyString_FromString("]"));
	Py_ReprLeave((PyObject *)v);
	return s;
}

static int
list_length(PyListObject *a)
{
	return a->ob_size;
}



static int
list_contains(PyListObject *a, PyObject *el)
{
	int i;

	for (i = 0; i < a->ob_size; ++i) {
		int cmp = PyObject_RichCompareBool(el, PyList_GET_ITEM(a, i),
						   Py_EQ);
		if (cmp > 0)
			return 1;
		else if (cmp < 0)
			return -1;
	}
	return 0;
}


static PyObject *
list_item(PyListObject *a, int i)
{
	if (i < 0 || i >= a->ob_size) {
		if (indexerr == NULL)
			indexerr = PyString_FromString(
				"list index out of range");
		PyErr_SetObject(PyExc_IndexError, indexerr);
		return NULL;
	}
	Py_INCREF(a->ob_item[i]);
	return a->ob_item[i];
}

static PyObject *
list_slice(PyListObject *a, int ilow, int ihigh)
{
	PyListObject *np;
	int i;
	if (ilow < 0)
		ilow = 0;
	else if (ilow > a->ob_size)
		ilow = a->ob_size;
	if (ihigh < ilow)
		ihigh = ilow;
	else if (ihigh > a->ob_size)
		ihigh = a->ob_size;
	np = (PyListObject *) PyList_New(ihigh - ilow);
	if (np == NULL)
		return NULL;
	for (i = ilow; i < ihigh; i++) {
		PyObject *v = a->ob_item[i];
		Py_INCREF(v);
		np->ob_item[i - ilow] = v;
	}
	return (PyObject *)np;
}

PyObject *
PyList_GetSlice(PyObject *a, int ilow, int ihigh)
{
	if (!PyList_Check(a)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return list_slice((PyListObject *)a, ilow, ihigh);
}

static PyObject *
list_concat(PyListObject *a, PyObject *bb)
{
	int size;
	int i;
	PyListObject *np;
	if (!PyList_Check(bb)) {
		PyErr_Format(PyExc_TypeError,
			  "can only concatenate list (not \"%.200s\") to list",
			  bb->ob_type->tp_name);
		return NULL;
	}
#define b ((PyListObject *)bb)
	size = a->ob_size + b->ob_size;
	np = (PyListObject *) PyList_New(size);
	if (np == NULL) {
		return NULL;
	}
	for (i = 0; i < a->ob_size; i++) {
		PyObject *v = a->ob_item[i];
		Py_INCREF(v);
		np->ob_item[i] = v;
	}
	for (i = 0; i < b->ob_size; i++) {
		PyObject *v = b->ob_item[i];
		Py_INCREF(v);
		np->ob_item[i + a->ob_size] = v;
	}
	return (PyObject *)np;
#undef b
}

static PyObject *
list_repeat(PyListObject *a, int n)
{
	int i, j;
	int size;
	PyListObject *np;
	PyObject **p;
	if (n < 0)
		n = 0;
	size = a->ob_size * n;
	np = (PyListObject *) PyList_New(size);
	if (np == NULL)
		return NULL;
	p = np->ob_item;
	for (i = 0; i < n; i++) {
		for (j = 0; j < a->ob_size; j++) {
			*p = a->ob_item[j];
			Py_INCREF(*p);
			p++;
		}
	}
	return (PyObject *) np;
}

static int
list_ass_slice(PyListObject *a, int ilow, int ihigh, PyObject *v)
{
	/* Because [X]DECREF can recursively invoke list operations on
	   this list, we must postpone all [X]DECREF activity until
	   after the list is back in its canonical shape.  Therefore
	   we must allocate an additional array, 'recycle', into which
	   we temporarily copy the items that are deleted from the
	   list. :-( */
	PyObject **recycle, **p;
	PyObject **item;
	int n; /* Size of replacement list */
	int d; /* Change in size */
	int k; /* Loop index */
#define b ((PyListObject *)v)
	if (v == NULL)
		n = 0;
	else if (PyList_Check(v)) {
		n = b->ob_size;
		if (a == b) {
			/* Special case "a[i:j] = a" -- copy b first */
			int ret;
			v = list_slice(b, 0, n);
			ret = list_ass_slice(a, ilow, ihigh, v);
			Py_DECREF(v);
			return ret;
		}
	}
	else {
		PyErr_Format(PyExc_TypeError,
			     "must assign list (not \"%.200s\") to slice",
			     v->ob_type->tp_name);
		return -1;
	}
	if (ilow < 0)
		ilow = 0;
	else if (ilow > a->ob_size)
		ilow = a->ob_size;
	if (ihigh < ilow)
		ihigh = ilow;
	else if (ihigh > a->ob_size)
		ihigh = a->ob_size;
	item = a->ob_item;
	d = n - (ihigh-ilow);
	if (ihigh > ilow)
		p = recycle = PyMem_NEW(PyObject *, (ihigh-ilow));
	else
		p = recycle = NULL;
	if (d <= 0) { /* Delete -d items; recycle ihigh-ilow items */
		for (k = ilow; k < ihigh; k++)
			*p++ = item[k];
		if (d < 0) {
			for (/*k = ihigh*/; k < a->ob_size; k++)
				item[k+d] = item[k];
			a->ob_size += d;
			NRESIZE(item, PyObject *, a->ob_size); /* Can't fail */
			a->ob_item = item;
		}
	}
	else { /* Insert d items; recycle ihigh-ilow items */
		NRESIZE(item, PyObject *, a->ob_size + d);
		if (item == NULL) {
			if (recycle != NULL)
				PyMem_DEL(recycle);
			PyErr_NoMemory();
			return -1;
		}
		for (k = a->ob_size; --k >= ihigh; )
			item[k+d] = item[k];
		for (/*k = ihigh-1*/; k >= ilow; --k)
			*p++ = item[k];
		a->ob_item = item;
		a->ob_size += d;
	}
	for (k = 0; k < n; k++, ilow++) {
		PyObject *w = b->ob_item[k];
		Py_XINCREF(w);
		item[ilow] = w;
	}
	if (recycle) {
		while (--p >= recycle)
			Py_XDECREF(*p);
		PyMem_DEL(recycle);
	}
	return 0;
#undef b
}

int
PyList_SetSlice(PyObject *a, int ilow, int ihigh, PyObject *v)
{
	if (!PyList_Check(a)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return list_ass_slice((PyListObject *)a, ilow, ihigh, v);
}

static PyObject *
list_inplace_repeat(PyListObject *self, int n)
{
	PyObject **items;
	int size, i, j;


	size = PyList_GET_SIZE(self);
	if (size == 0) {
		Py_INCREF(self);
		return (PyObject *)self;
	}

	items = self->ob_item;

	if (n < 1) {
		self->ob_item = NULL;
		self->ob_size = 0;
		for (i = 0; i < size; i++)
			Py_XDECREF(items[i]);
		PyMem_DEL(items);
		Py_INCREF(self);
		return (PyObject *)self;
	}

	NRESIZE(items, PyObject*, size*n);
	if (items == NULL) {
		PyErr_NoMemory();
		goto finally;
	}
	self->ob_item = items;
	for (i = 1; i < n; i++) { /* Start counting at 1, not 0 */
		for (j = 0; j < size; j++) {
			PyObject *o = PyList_GET_ITEM(self, j);
			Py_INCREF(o);
			PyList_SET_ITEM(self, self->ob_size++, o);
		}
	}
	Py_INCREF(self);
	return (PyObject *)self;
  finally:
  	return NULL;
}

static int
list_ass_item(PyListObject *a, int i, PyObject *v)
{
	PyObject *old_value;
	if (i < 0 || i >= a->ob_size) {
		PyErr_SetString(PyExc_IndexError,
				"list assignment index out of range");
		return -1;
	}
	if (v == NULL)
		return list_ass_slice(a, i, i+1, v);
	Py_INCREF(v);
	old_value = a->ob_item[i];
	a->ob_item[i] = v;
	Py_DECREF(old_value); 
	return 0;
}

static PyObject *
ins(PyListObject *self, int where, PyObject *v)
{
	if (ins1(self, where, v) != 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
listinsert(PyListObject *self, PyObject *args)
{
	int i;
	PyObject *v;
	if (!PyArg_ParseTuple(args, "iO:insert", &i, &v))
		return NULL;
	return ins(self, i, v);
}

/* Define NO_STRICT_LIST_APPEND to enable multi-argument append() */

#ifndef NO_STRICT_LIST_APPEND
#define PyArg_ParseTuple_Compat1 PyArg_ParseTuple
#else
#define PyArg_ParseTuple_Compat1(args, format, ret) \
( \
	PyTuple_GET_SIZE(args) > 1 ? (*ret = args, 1) : \
	PyTuple_GET_SIZE(args) == 1 ? (*ret = PyTuple_GET_ITEM(args, 0), 1) : \
	PyArg_ParseTuple(args, format, ret) \
)
#endif


static PyObject *
listappend(PyListObject *self, PyObject *args)
{
	PyObject *v;
	if (!PyArg_ParseTuple_Compat1(args, "O:append", &v))
		return NULL;
	return ins(self, (int) self->ob_size, v);
}

static int
listextend_internal(PyListObject *self, PyObject *b)
{
	PyObject **items;
	int selflen = PyList_GET_SIZE(self);
	int blen;
	register int i;

	if (PyObject_Size(b) == 0) {
		/* short circuit when b is empty */
		Py_DECREF(b);
		return 0;
	}

	if (self == (PyListObject*)b) {
		/* as in list_ass_slice() we must special case the
		 * situation: a.extend(a)
		 *
		 * XXX: I think this way ought to be faster than using
		 * list_slice() the way list_ass_slice() does.
		 */
		Py_DECREF(b);
		b = PyList_New(selflen);
		if (!b)
			return -1;
		for (i = 0; i < selflen; i++) {
			PyObject *o = PyList_GET_ITEM(self, i);
			Py_INCREF(o);
			PyList_SET_ITEM(b, i, o);
		}
	}

	blen = PyObject_Size(b);

	/* resize a using idiom */
	items = self->ob_item;
	NRESIZE(items, PyObject*, selflen + blen);
	if (items == NULL) {
		PyErr_NoMemory();
		Py_DECREF(b);
		return -1;
	}

	self->ob_item = items;

	/* populate the end of self with b's items */
	for (i = 0; i < blen; i++) {
		PyObject *o = PySequence_Fast_GET_ITEM(b, i);
		Py_INCREF(o);
		PyList_SET_ITEM(self, self->ob_size++, o);
	}
	Py_DECREF(b);
	return 0;
}


static PyObject *
list_inplace_concat(PyListObject *self, PyObject *other)
{
	other = PySequence_Fast(other, "argument to += must be a sequence");
	if (!other)
		return NULL;

	if (listextend_internal(self, other) < 0)
		return NULL;

	Py_INCREF(self);
	return (PyObject *)self;
}

static PyObject *
listextend(PyListObject *self, PyObject *args)
{

	PyObject *b;
	
	if (!PyArg_ParseTuple(args, "O:extend", &b))
		return NULL;

	b = PySequence_Fast(b, "list.extend() argument must be a sequence");
	if (!b)
		return NULL;

	if (listextend_internal(self, b) < 0)
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
listpop(PyListObject *self, PyObject *args)
{
	int i = -1;
	PyObject *v;
	if (!PyArg_ParseTuple(args, "|i:pop", &i))
		return NULL;
	if (self->ob_size == 0) {
		/* Special-case most common failure cause */
		PyErr_SetString(PyExc_IndexError, "pop from empty list");
		return NULL;
	}
	if (i < 0)
		i += self->ob_size;
	if (i < 0 || i >= self->ob_size) {
		PyErr_SetString(PyExc_IndexError, "pop index out of range");
		return NULL;
	}
	v = self->ob_item[i];
	Py_INCREF(v);
	if (list_ass_slice(self, i, i+1, (PyObject *)NULL) != 0) {
		Py_DECREF(v);
		return NULL;
	}
	return v;
}

/* New quicksort implementation for arrays of object pointers.
   Thanks to discussions with Tim Peters. */

/* CMPERROR is returned by our comparison function when an error
   occurred.  This is the largest negative integer (0x80000000 on a
   32-bit system). */
#define CMPERROR ( (int) ((unsigned int)1 << (8*sizeof(int) - 1)) )

/* Comparison function.  Takes care of calling a user-supplied
   comparison function (any callable Python object).  Calls the
   standard comparison function, PyObject_Compare(), if the user-
   supplied function is NULL. */

static int
docompare(PyObject *x, PyObject *y, PyObject *compare)
{
	PyObject *args, *res;
	int i;

	if (compare == NULL) {
		/* NOTE: we rely on the fact here that the sorting algorithm
		   only ever checks whether k<0, i.e., whether x<y.  So we
		   invoke the rich comparison function with Py_LT ('<'), and
		   return -1 when it returns true and 0 when it returns
		   false. */
		i = PyObject_RichCompareBool(x, y, Py_LT);
		if (i < 0)
			return CMPERROR;
		else
			return -i;
	}

	args = Py_BuildValue("(OO)", x, y);
	if (args == NULL)
		return CMPERROR;
	res = PyEval_CallObject(compare, args);
	Py_DECREF(args);
	if (res == NULL)
		return CMPERROR;
	if (!PyInt_Check(res)) {
		Py_DECREF(res);
		PyErr_SetString(PyExc_TypeError,
				"comparison function must return int");
		return CMPERROR;
	}
	i = PyInt_AsLong(res);
	Py_DECREF(res);
	if (i < 0)
		return -1;
	if (i > 0)
		return 1;
	return 0;
}

/* MINSIZE is the smallest array that will get a full-blown samplesort
   treatment; smaller arrays are sorted using binary insertion.  It must
   be at least 7 for the samplesort implementation to work.  Binary
   insertion does fewer compares, but can suffer O(N**2) data movement.
   The more expensive compares, the larger MINSIZE should be. */
#define MINSIZE 100

/* MINPARTITIONSIZE is the smallest array slice samplesort will bother to
   partition; smaller slices are passed to binarysort.  It must be at
   least 2, and no larger than MINSIZE.  Setting it higher reduces the #
   of compares slowly, but increases the amount of data movement quickly.
   The value here was chosen assuming a compare costs ~25x more than
   swapping a pair of memory-resident pointers -- but under that assumption,
   changing the value by a few dozen more or less has aggregate effect
   under 1%.  So the value is crucial, but not touchy <wink>. */
#define MINPARTITIONSIZE 40

/* MAXMERGE is the largest number of elements we'll always merge into
   a known-to-be sorted chunk via binary insertion, regardless of the
   size of that chunk.  Given a chunk of N sorted elements, and a group
   of K unknowns, the largest K for which it's better to do insertion
   (than a full-blown sort) is a complicated function of N and K mostly
   involving the expected number of compares and data moves under each
   approach, and the relative cost of those operations on a specific
   architecure.  The fixed value here is conservative, and should be a
   clear win regardless of architecture or N. */
#define MAXMERGE 15

/* STACKSIZE is the size of our work stack.  A rough estimate is that
   this allows us to sort arrays of size N where
   N / ln(N) = MINPARTITIONSIZE * 2**STACKSIZE, so 60 is more than enough
   for arrays of size 2**64.  Because we push the biggest partition
   first, the worst case occurs when all subarrays are always partitioned
   exactly in two. */
#define STACKSIZE 60


#define SETK(X,Y) if ((k = docompare(X,Y,compare))==CMPERROR) goto fail

/* binarysort is the best method for sorting small arrays: it does
   few compares, but can do data movement quadratic in the number of
   elements.
   [lo, hi) is a contiguous slice of a list, and is sorted via
   binary insertion.
   On entry, must have lo <= start <= hi, and that [lo, start) is already
   sorted (pass start == lo if you don't know!).
   If docompare complains (returns CMPERROR) return -1, else 0.
   Even in case of error, the output slice will be some permutation of
   the input (nothing is lost or duplicated).
*/

static int
binarysort(PyObject **lo, PyObject **hi, PyObject **start, PyObject *compare)
     /* compare -- comparison function object, or NULL for default */
{
	/* assert lo <= start <= hi
	   assert [lo, start) is sorted */
	register int k;
	register PyObject **l, **p, **r;
	register PyObject *pivot;

	if (lo == start)
		++start;
	for (; start < hi; ++start) {
		/* set l to where *start belongs */
		l = lo;
		r = start;
		pivot = *r;
		do {
			p = l + ((r - l) >> 1);
			SETK(pivot, *p);
			if (k < 0)
				r = p;
			else
				l = p + 1;
		} while (l < r);
		/* Pivot should go at l -- slide over to make room.
		   Caution: using memmove is much slower under MSVC 5;
		   we're not usually moving many slots. */
		for (p = start; p > l; --p)
			*p = *(p-1);
		*l = pivot;
	}
	return 0;

 fail:
	return -1;
}

/* samplesortslice is the sorting workhorse.
   [lo, hi) is a contiguous slice of a list, to be sorted in place.
   On entry, must have lo <= hi,
   If docompare complains (returns CMPERROR) return -1, else 0.
   Even in case of error, the output slice will be some permutation of
   the input (nothing is lost or duplicated).

   samplesort is basically quicksort on steroids:  a power of 2 close
   to n/ln(n) is computed, and that many elements (less 1) are picked at
   random from the array and sorted.  These 2**k - 1 elements are then
   used as preselected pivots for an equal number of quicksort
   partitioning steps, partitioning the slice into 2**k chunks each of
   size about ln(n).  These small final chunks are then usually handled
   by binarysort.  Note that when k=1, this is roughly the same as an
   ordinary quicksort using a random pivot, and when k=2 this is roughly
   a median-of-3 quicksort.  From that view, using k ~= lg(n/ln(n)) makes
   this a "median of n/ln(n)" quicksort.  You can also view it as a kind
   of bucket sort, where 2**k-1 bucket boundaries are picked dynamically.

   The large number of samples makes a quadratic-time case almost
   impossible, and asymptotically drives the average-case number of
   compares from quicksort's 2 N ln N (or 12/7 N ln N for the median-of-
   3 variant) down to N lg N.

   We also play lots of low-level tricks to cut the number of compares.
   
   Very obscure:  To avoid using extra memory, the PPs are stored in the
   array and shuffled around as partitioning proceeds.  At the start of a
   partitioning step, we'll have 2**m-1 (for some m) PPs in sorted order,
   adjacent (either on the left or the right!) to a chunk of X elements
   that are to be partitioned: P X or X P.  In either case we need to
   shuffle things *in place* so that the 2**(m-1) smaller PPs are on the
   left, followed by the PP to be used for this step (that's the middle
   of the PPs), followed by X, followed by the 2**(m-1) larger PPs:
       P X or X P -> Psmall pivot X Plarge
   and the order of the PPs must not be altered.  It can take a while
   to realize this isn't trivial!  It can take even longer <wink> to
   understand why the simple code below works, using only 2**(m-1) swaps.
   The key is that the order of the X elements isn't necessarily
   preserved:  X can end up as some cyclic permutation of its original
   order.  That's OK, because X is unsorted anyway.  If the order of X
   had to be preserved too, the simplest method I know of using O(1)
   scratch storage requires len(X) + 2**(m-1) swaps, spread over 2 passes.
   Since len(X) is typically several times larger than 2**(m-1), that
   would slow things down.
*/

struct SamplesortStackNode {
	/* Represents a slice of the array, from (& including) lo up
	   to (but excluding) hi.  "extra" additional & adjacent elements
	   are pre-selected pivots (PPs), spanning [lo-extra, lo) if
	   extra > 0, or [hi, hi-extra) if extra < 0.  The PPs are
	   already sorted, but nothing is known about the other elements
	   in [lo, hi). |extra| is always one less than a power of 2.
	   When extra is 0, we're out of PPs, and the slice must be
	   sorted by some other means. */
	PyObject **lo;
	PyObject **hi;
	int extra;
};

/* The number of PPs we want is 2**k - 1, where 2**k is as close to
   N / ln(N) as possible.  So k ~= lg(N / ln(N)).  Calling libm routines
   is undesirable, so cutoff values are canned in the "cutoff" table
   below:  cutoff[i] is the smallest N such that k == CUTOFFBASE + i. */
#define CUTOFFBASE 4
static long cutoff[] = {
	43,        /* smallest N such that k == 4 */
	106,       /* etc */
	250,
	576,
	1298,
	2885,
	6339,
	13805,
	29843,
	64116,
	137030,
	291554,
	617916,
	1305130,
	2748295,
	5771662,
	12091672,
	25276798,
	52734615,
	109820537,
	228324027,
	473977813,
	982548444,   /* smallest N such that k == 26 */
	2034159050   /* largest N that fits in signed 32-bit; k == 27 */
};

static int
samplesortslice(PyObject **lo, PyObject **hi, PyObject *compare)
     /* compare -- comparison function object, or NULL for default */
{
	register PyObject **l, **r;
	register PyObject *tmp, *pivot;
	register int k;
	int n, extra, top, extraOnRight;
	struct SamplesortStackNode stack[STACKSIZE];

	/* assert lo <= hi */
	n = hi - lo;

	/* ----------------------------------------------------------
	 * Special cases
	 * --------------------------------------------------------*/
	if (n < 2)
		return 0;

	/* Set r to the largest value such that [lo,r) is sorted.
	   This catches the already-sorted case, the all-the-same
	   case, and the appended-a-few-elements-to-a-sorted-list case.
	   If the array is unsorted, we're very likely to get out of
	   the loop fast, so the test is cheap if it doesn't pay off.
	*/
	/* assert lo < hi */
	for (r = lo+1; r < hi; ++r) {
		SETK(*r, *(r-1));
		if (k < 0)
			break;
	}
	/* [lo,r) is sorted, [r,hi) unknown.  Get out cheap if there are
	   few unknowns, or few elements in total. */
	if (hi - r <= MAXMERGE || n < MINSIZE)
		return binarysort(lo, hi, r, compare);

	/* Check for the array already being reverse-sorted.  Typical
	   benchmark-driven silliness <wink>. */
	/* assert lo < hi */
	for (r = lo+1; r < hi; ++r) {
		SETK(*(r-1), *r);
		if (k < 0)
			break;
	}
	if (hi - r <= MAXMERGE) {
		/* Reverse the reversed prefix, then insert the tail */
		PyObject **originalr = r;
		l = lo;
		do {
			--r;
			tmp = *l; *l = *r; *r = tmp;
			++l;
		} while (l < r);
		return binarysort(lo, hi, originalr, compare);
	}

	/* ----------------------------------------------------------
	 * Normal case setup: a large array without obvious pattern.
	 * --------------------------------------------------------*/

	/* extra := a power of 2 ~= n/ln(n), less 1.
	   First find the smallest extra s.t. n < cutoff[extra] */
	for (extra = 0;
	     extra < sizeof(cutoff) / sizeof(cutoff[0]);
	     ++extra) {
		if (n < cutoff[extra])
			break;
		/* note that if we fall out of the loop, the value of
		   extra still makes *sense*, but may be smaller than
		   we would like (but the array has more than ~= 2**31
		   elements in this case!) */ 
	}
	/* Now k == extra - 1 + CUTOFFBASE.  The smallest value k can
	   have is CUTOFFBASE-1, so
	   assert MINSIZE >= 2**(CUTOFFBASE-1) - 1 */
	extra = (1 << (extra - 1 + CUTOFFBASE)) - 1;
	/* assert extra > 0 and n >= extra */

	/* Swap that many values to the start of the array.  The
	   selection of elements is pseudo-random, but the same on
	   every run (this is intentional! timing algorithm changes is
	   a pain if timing varies across runs).  */
	{
		unsigned int seed = n / extra;  /* arbitrary */
		unsigned int i;
		for (i = 0; i < (unsigned)extra; ++i) {
			/* j := random int in [i, n) */
			unsigned int j;
			seed = seed * 69069 + 7;
			j = i + seed % (n - i);
			tmp = lo[i]; lo[i] = lo[j]; lo[j] = tmp;
		}
	}

	/* Recursively sort the preselected pivots. */
	if (samplesortslice(lo, lo + extra, compare) < 0)
		goto fail;

	top = 0;          /* index of available stack slot */
	lo += extra;      /* point to first unknown */
	extraOnRight = 0; /* the PPs are at the left end */

	/* ----------------------------------------------------------
	 * Partition [lo, hi), and repeat until out of work.
	 * --------------------------------------------------------*/
	for (;;) {
		/* assert lo <= hi, so n >= 0 */
		n = hi - lo;

		/* We may not want, or may not be able, to partition:
		   If n is small, it's quicker to insert.
		   If extra is 0, we're out of pivots, and *must* use
		   another method.
		*/
		if (n < MINPARTITIONSIZE || extra == 0) {
			if (n >= MINSIZE) {
				/* assert extra == 0
				   This is rare, since the average size
				   of a final block is only about
				   ln(original n). */
				if (samplesortslice(lo, hi, compare) < 0)
					goto fail;
			}
			else {
				/* Binary insertion should be quicker,
				   and we can take advantage of the PPs
				   already being sorted. */
				if (extraOnRight && extra) {
					/* swap the PPs to the left end */
					k = extra;
					do {
						tmp = *lo;
						*lo = *hi;
						*hi = tmp;
						++lo; ++hi;
					} while (--k);
				}
				if (binarysort(lo - extra, hi, lo,
					       compare) < 0)
					goto fail;
			}

			/* Find another slice to work on. */
			if (--top < 0)
				break;   /* no more -- done! */
			lo = stack[top].lo;
			hi = stack[top].hi;
			extra = stack[top].extra;
			extraOnRight = 0;
			if (extra < 0) {
				extraOnRight = 1;
				extra = -extra;
			}
			continue;
		}

		/* Pretend the PPs are indexed 0, 1, ..., extra-1.
		   Then our preselected pivot is at (extra-1)/2, and we
		   want to move the PPs before that to the left end of
		   the slice, and the PPs after that to the right end.
		   The following section changes extra, lo, hi, and the
		   slice such that:
		   [lo-extra, lo) contains the smaller PPs.
		   *lo == our PP.
		   (lo, hi) contains the unknown elements.
		   [hi, hi+extra) contains the larger PPs.
		*/
		k = extra >>= 1;  /* num PPs to move */ 
		if (extraOnRight) {
			/* Swap the smaller PPs to the left end.
			   Note that this loop actually moves k+1 items:
			   the last is our PP */
			do {
				tmp = *lo; *lo = *hi; *hi = tmp;
				++lo; ++hi;
			} while (k--);
		}
		else {
			/* Swap the larger PPs to the right end. */
			while (k--) {
				--lo; --hi;
				tmp = *lo; *lo = *hi; *hi = tmp;
			}
		}
		--lo;   /* *lo is now our PP */
		pivot = *lo;

		/* Now an almost-ordinary quicksort partition step.
		   Note that most of the time is spent here!
		   Only odd thing is that we partition into < and >=,
		   instead of the usual <= and >=.  This helps when
		   there are lots of duplicates of different values,
		   because it eventually tends to make subfiles
		   "pure" (all duplicates), and we special-case for
		   duplicates later. */
		l = lo + 1;
		r = hi - 1;
		/* assert lo < l < r < hi (small n weeded out above) */

		do {
			/* slide l right, looking for key >= pivot */
			do {
				SETK(*l, pivot);
				if (k < 0)
					++l;
				else
					break;
			} while (l < r);

			/* slide r left, looking for key < pivot */
			while (l < r) {
				register PyObject *rval = *r--;
				SETK(rval, pivot);
				if (k < 0) {
					/* swap and advance */
					r[1] = *l;
					*l++ = rval;
					break;
				}
			}

		} while (l < r);

		/* assert lo < r <= l < hi
		   assert r == l or r+1 == l
		   everything to the left of l is < pivot, and
		   everything to the right of r is >= pivot */

		if (l == r) {
			SETK(*r, pivot);
			if (k < 0)
				++l;
			else
				--r;
		}
		/* assert lo <= r and r+1 == l and l <= hi
		   assert r == lo or a[r] < pivot
		   assert a[lo] is pivot
		   assert l == hi or a[l] >= pivot
		   Swap the pivot into "the middle", so we can henceforth
		   ignore it.
		*/
		*lo = *r;
		*r = pivot;

		/* The following is true now, & will be preserved:
		   All in [lo,r) are < pivot
		   All in [r,l) == pivot (& so can be ignored)
		   All in [l,hi) are >= pivot */

		/* Check for duplicates of the pivot.  One compare is
		   wasted if there are no duplicates, but can win big
		   when there are.
		   Tricky: we're sticking to "<" compares, so deduce
		   equality indirectly.  We know pivot <= *l, so they're
		   equal iff not pivot < *l.
		*/
		while (l < hi) {
			/* pivot <= *l known */
			SETK(pivot, *l);
			if (k < 0)
				break;
			else
				/* <= and not < implies == */
				++l;
		}

		/* assert lo <= r < l <= hi
		   Partitions are [lo, r) and [l, hi) */

		/* push fattest first; remember we still have extra PPs
		   to the left of the left chunk and to the right of
		   the right chunk! */
		/* assert top < STACKSIZE */
		if (r - lo <= hi - l) {
			/* second is bigger */
			stack[top].lo = l;
			stack[top].hi = hi;
			stack[top].extra = -extra;
			hi = r;
			extraOnRight = 0;
		}
		else {
			/* first is bigger */
			stack[top].lo = lo;
			stack[top].hi = r;
			stack[top].extra = extra;
			lo = l;
			extraOnRight = 1;
		}
		++top;

	}   /* end of partitioning loop */

	return 0;

 fail:
	return -1;
}

#undef SETK

staticforward PyTypeObject immutable_list_type;

static PyObject *
listsort(PyListObject *self, PyObject *args)
{
	int err;
	PyObject *compare = NULL;

	if (args != NULL) {
		if (!PyArg_ParseTuple(args, "|O:sort", &compare))
			return NULL;
	}
	self->ob_type = &immutable_list_type;
	err = samplesortslice(self->ob_item,
			      self->ob_item + self->ob_size,
			      compare);
	self->ob_type = &PyList_Type;
	if (err < 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

int
PyList_Sort(PyObject *v)
{
	if (v == NULL || !PyList_Check(v)) {
		PyErr_BadInternalCall();
		return -1;
	}
	v = listsort((PyListObject *)v, (PyObject *)NULL);
	if (v == NULL)
		return -1;
	Py_DECREF(v);
	return 0;
}

static void
_listreverse(PyListObject *self)
{
	register PyObject **p, **q;
	register PyObject *tmp;
	
	if (self->ob_size > 1) {
		for (p = self->ob_item, q = self->ob_item + self->ob_size - 1;
		     p < q;
		     p++, q--)
		{
			tmp = *p;
			*p = *q;
			*q = tmp;
		}
	}
}

static PyObject *
listreverse(PyListObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":reverse"))
		return NULL;
	_listreverse(self);
	Py_INCREF(Py_None);
	return Py_None;
}

int
PyList_Reverse(PyObject *v)
{
	if (v == NULL || !PyList_Check(v)) {
		PyErr_BadInternalCall();
		return -1;
	}
	_listreverse((PyListObject *)v);
	return 0;
}

PyObject *
PyList_AsTuple(PyObject *v)
{
	PyObject *w;
	PyObject **p;
	int n;
	if (v == NULL || !PyList_Check(v)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	n = ((PyListObject *)v)->ob_size;
	w = PyTuple_New(n);
	if (w == NULL)
		return NULL;
	p = ((PyTupleObject *)w)->ob_item;
	memcpy((void *)p,
	       (void *)((PyListObject *)v)->ob_item,
	       n*sizeof(PyObject *));
	while (--n >= 0) {
		Py_INCREF(*p);
		p++;
	}
	return w;
}

static PyObject *
listindex(PyListObject *self, PyObject *args)
{
	int i;
	PyObject *v;

	if (!PyArg_ParseTuple_Compat1(args, "O:index", &v))
		return NULL;
	for (i = 0; i < self->ob_size; i++) {
		int cmp = PyObject_RichCompareBool(self->ob_item[i], v, Py_EQ);
		if (cmp > 0)
			return PyInt_FromLong((long)i);
		else if (cmp < 0)
			return NULL;
	}
	PyErr_SetString(PyExc_ValueError, "list.index(x): x not in list");
	return NULL;
}

static PyObject *
listcount(PyListObject *self, PyObject *args)
{
	int count = 0;
	int i;
	PyObject *v;

	if (!PyArg_ParseTuple_Compat1(args, "O:count", &v))
		return NULL;
	for (i = 0; i < self->ob_size; i++) {
		int cmp = PyObject_RichCompareBool(self->ob_item[i], v, Py_EQ);
		if (cmp > 0)
			count++;
		else if (cmp < 0)
			return NULL;
	}
	return PyInt_FromLong((long)count);
}

static PyObject *
listremove(PyListObject *self, PyObject *args)
{
	int i;
	PyObject *v;

	if (!PyArg_ParseTuple_Compat1(args, "O:remove", &v))
		return NULL;
	for (i = 0; i < self->ob_size; i++) {
		int cmp = PyObject_RichCompareBool(self->ob_item[i], v, Py_EQ);
		if (cmp > 0) {
			if (list_ass_slice(self, i, i+1,
					   (PyObject *)NULL) != 0)
				return NULL;
			Py_INCREF(Py_None);
			return Py_None;
		}
		else if (cmp < 0)
			return NULL;
	}
	PyErr_SetString(PyExc_ValueError, "list.remove(x): x not in list");
	return NULL;
}

static int
list_traverse(PyListObject *o, visitproc visit, void *arg)
{
	int i, err;
	PyObject *x;

	for (i = o->ob_size; --i >= 0; ) {
		x = o->ob_item[i];
		if (x != NULL) {
			err = visit(x, arg);
			if (err)
				return err;
		}
	}
	return 0;
}

static int
list_clear(PyListObject *lp)
{
	(void) PyList_SetSlice((PyObject *)lp, 0, lp->ob_size, 0);
	return 0;
}

static PyObject *
list_richcompare(PyObject *v, PyObject *w, int op)
{
	PyListObject *vl, *wl;
	int i;

	if (!PyList_Check(v) || !PyList_Check(w)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	vl = (PyListObject *)v;
	wl = (PyListObject *)w;

	if (vl->ob_size != wl->ob_size && (op == Py_EQ || op == Py_NE)) {
		/* Shortcut: if the lengths differ, the lists differ */
		PyObject *res;
		if (op == Py_EQ)
			res = Py_False;
		else
			res = Py_True;
		Py_INCREF(res);
		return res;
	}

	/* Search for the first index where items are different */
	for (i = 0; i < vl->ob_size && i < wl->ob_size; i++) {
		int k = PyObject_RichCompareBool(vl->ob_item[i],
						 wl->ob_item[i], Py_EQ);
		if (k < 0)
			return NULL;
		if (!k)
			break;
	}

	if (i >= vl->ob_size || i >= wl->ob_size) {
		/* No more items to compare -- compare sizes */
		int vs = vl->ob_size;
		int ws = wl->ob_size;
		int cmp;
		PyObject *res;
		switch (op) {
		case Py_LT: cmp = vs <  ws; break;
		case Py_LE: cmp = vs <= ws; break;
		case Py_EQ: cmp = vs == ws; break;
		case Py_NE: cmp = vs != ws; break;
		case Py_GT: cmp = vs >  ws; break;
		case Py_GE: cmp = vs >= ws; break;
		default: return NULL; /* cannot happen */
		}
		if (cmp)
			res = Py_True;
		else
			res = Py_False;
		Py_INCREF(res);
		return res;
	}

	/* We have an item that differs -- shortcuts for EQ/NE */
	if (op == Py_EQ) {
		Py_INCREF(Py_False);
		return Py_False;
	}
	if (op == Py_NE) {
		Py_INCREF(Py_True);
		return Py_True;
	}

	/* Compare the final item again using the proper operator */
	return PyObject_RichCompare(vl->ob_item[i], wl->ob_item[i], op);
}

static char append_doc[] =
"L.append(object) -- append object to end";
static char extend_doc[] =
"L.extend(list) -- extend list by appending list elements";
static char insert_doc[] =
"L.insert(index, object) -- insert object before index";
static char pop_doc[] =
"L.pop([index]) -> item -- remove and return item at index (default last)";
static char remove_doc[] =
"L.remove(value) -- remove first occurrence of value";
static char index_doc[] =
"L.index(value) -> integer -- return index of first occurrence of value";
static char count_doc[] =
"L.count(value) -> integer -- return number of occurrences of value";
static char reverse_doc[] =
"L.reverse() -- reverse *IN PLACE*";
static char sort_doc[] =
"L.sort([cmpfunc]) -- sort *IN PLACE*; if given, cmpfunc(x, y) -> -1, 0, 1";

static PyMethodDef list_methods[] = {
	{"append",	(PyCFunction)listappend,  METH_VARARGS, append_doc},
	{"insert",	(PyCFunction)listinsert,  METH_VARARGS, insert_doc},
	{"extend",      (PyCFunction)listextend,  METH_VARARGS, extend_doc},
	{"pop",		(PyCFunction)listpop, 	  METH_VARARGS, pop_doc},
	{"remove",	(PyCFunction)listremove,  METH_VARARGS, remove_doc},
	{"index",	(PyCFunction)listindex,   METH_VARARGS, index_doc},
	{"count",	(PyCFunction)listcount,   METH_VARARGS, count_doc},
	{"reverse",	(PyCFunction)listreverse, METH_VARARGS, reverse_doc},
	{"sort",	(PyCFunction)listsort, 	  METH_VARARGS, sort_doc},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
list_getattr(PyListObject *f, char *name)
{
	return Py_FindMethod(list_methods, (PyObject *)f, name);
}

static PySequenceMethods list_as_sequence = {
	(inquiry)list_length,			/* sq_length */
	(binaryfunc)list_concat,		/* sq_concat */
	(intargfunc)list_repeat,		/* sq_repeat */
	(intargfunc)list_item,			/* sq_item */
	(intintargfunc)list_slice,		/* sq_slice */
	(intobjargproc)list_ass_item,		/* sq_ass_item */
	(intintobjargproc)list_ass_slice,	/* sq_ass_slice */
	(objobjproc)list_contains,		/* sq_contains */
	(binaryfunc)list_inplace_concat,	/* sq_inplace_concat */
	(intargfunc)list_inplace_repeat,	/* sq_inplace_repeat */
};

PyTypeObject PyList_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"list",
	sizeof(PyListObject) + PyGC_HEAD_SIZE,
	0,
	(destructor)list_dealloc,		/* tp_dealloc */
	(printfunc)list_print,			/* tp_print */
	(getattrfunc)list_getattr,		/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)list_repr,			/* tp_repr */
	0,					/* tp_as_number */
	&list_as_sequence,			/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_GC,	/* tp_flags */
 	0,					/* tp_doc */
 	(traverseproc)list_traverse,		/* tp_traverse */
 	(inquiry)list_clear,			/* tp_clear */
	list_richcompare,			/* tp_richcompare */
};


/* During a sort, we really can't have anyone modifying the list; it could
   cause core dumps.  Thus, we substitute a dummy type that raises an
   explanatory exception when a modifying operation is used.  Caveat:
   comparisons may behave differently; but I guess it's a bad idea anyway to
   compare a list that's being sorted... */

static PyObject *
immutable_list_op(void)
{
	PyErr_SetString(PyExc_TypeError,
			"a list cannot be modified while it is being sorted");
	return NULL;
}

static PyMethodDef immutable_list_methods[] = {
	{"append",	(PyCFunction)immutable_list_op},
	{"insert",	(PyCFunction)immutable_list_op},
	{"remove",	(PyCFunction)immutable_list_op},
	{"index",	(PyCFunction)listindex},
	{"count",	(PyCFunction)listcount},
	{"reverse",	(PyCFunction)immutable_list_op},
	{"sort",	(PyCFunction)immutable_list_op},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
immutable_list_getattr(PyListObject *f, char *name)
{
	return Py_FindMethod(immutable_list_methods, (PyObject *)f, name);
}

static int
immutable_list_ass(void)
{
	immutable_list_op();
	return -1;
}

static PySequenceMethods immutable_list_as_sequence = {
	(inquiry)list_length,			/* sq_length */
	(binaryfunc)list_concat,		/* sq_concat */
	(intargfunc)list_repeat,		/* sq_repeat */
	(intargfunc)list_item,			/* sq_item */
	(intintargfunc)list_slice,		/* sq_slice */
	(intobjargproc)immutable_list_ass,	/* sq_ass_item */
	(intintobjargproc)immutable_list_ass,	/* sq_ass_slice */
	(objobjproc)list_contains,		/* sq_contains */
};

static PyTypeObject immutable_list_type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"list (immutable, during sort)",
	sizeof(PyListObject) + PyGC_HEAD_SIZE,
	0,
	0, /* Cannot happen */			/* tp_dealloc */
	(printfunc)list_print,			/* tp_print */
	(getattrfunc)immutable_list_getattr,	/* tp_getattr */
	0,					/* tp_setattr */
	0, /* Won't be called */		/* tp_compare */
	(reprfunc)list_repr,			/* tp_repr */
	0,					/* tp_as_number */
	&immutable_list_as_sequence,		/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_GC,	/* tp_flags */
 	0,					/* tp_doc */
 	(traverseproc)list_traverse,		/* tp_traverse */
	0,					/* tp_clear */
	list_richcompare,			/* tp_richcompare */
	/* NOTE: This is *not* the standard list_type struct! */
};
