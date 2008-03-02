/* List object implementation */

#include "Python.h"

#ifdef STDC_HEADERS
#include <stddef.h>
#else
#include <sys/types.h>		/* For size_t */
#endif

static int
roundupsize(int n)
{
	unsigned int nbits = 0;
	unsigned int n2 = (unsigned int)n >> 5;

	/* Round up:
	 * If n <       256, to a multiple of        8.
	 * If n <      2048, to a multiple of       64.
	 * If n <     16384, to a multiple of      512.
	 * If n <    131072, to a multiple of     4096.
	 * If n <   1048576, to a multiple of    32768.
	 * If n <   8388608, to a multiple of   262144.
	 * If n <  67108864, to a multiple of  2097152.
	 * If n < 536870912, to a multiple of 16777216.
	 * ...
	 * If n < 2**(5+3*i), to a multiple of 2**(3*i).
	 *
	 * This over-allocates proportional to the list size, making room
	 * for additional growth.  The over-allocation is mild, but is
	 * enough to give linear-time amortized behavior over a long
	 * sequence of appends() in the presence of a poorly-performing
	 * system realloc() (which is a reality, e.g., across all flavors
	 * of Windows, with Win9x behavior being particularly bad -- and
	 * we've still got address space fragmentation problems on Win9x
	 * even with this scheme, although it requires much longer lists to
	 * provoke them than it used to).
	 */
	do {
		n2 >>= 3;
		nbits += 3;
	} while (n2);
	return ((n >> nbits) + 1) << nbits;
 }

#define NRESIZE(var, type, nitems)				\
do {								\
	size_t _new_size = roundupsize(nitems);			\
	if (_new_size <= ((~(size_t)0) / sizeof(type)))		\
		PyMem_RESIZE(var, type, _new_size);		\
	else							\
		var = NULL;					\
} while (0)

PyObject *
PyList_New(int size)
{
	PyListObject *op;
	size_t nbytes;
	if (size < 0) {
		PyErr_BadInternalCall();
		return NULL;
	}
	nbytes = size * sizeof(PyObject *);
	/* Check for overflow without an actual overflow,
	 *  which can cause compiler to optimise out */
	if (size > PY_SIZE_MAX / sizeof(PyObject *)) {
		return PyErr_NoMemory();
	}
	op = PyObject_GC_New(PyListObject, &PyList_Type);
	if (op == NULL) {
		return NULL;
	}
	if (size <= 0) {
		op->ob_item = NULL;
	}
	else {
		op->ob_item = (PyObject **) PyMem_MALLOC(nbytes);
		if (op->ob_item == NULL) {
			return PyErr_NoMemory();
		}
		memset(op->ob_item, 0, sizeof(*op->ob_item) * size);
	}
	op->ob_size = size;
	_PyObject_GC_TRACK(op);
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
	if (where < 0) {
		where += self->ob_size;
		if (where < 0)
			where = 0;
	}
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
	PyObject_GC_UnTrack(op);
	Py_TRASHCAN_SAFE_BEGIN(op)
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
	op->ob_type->tp_free((PyObject *)op);
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
	int i;
	PyObject *s, *temp;
	PyObject *pieces = NULL, *result = NULL;

	i = Py_ReprEnter((PyObject*)v);
	if (i != 0) {
		return i > 0 ? PyString_FromString("[...]") : NULL;
	}

	if (v->ob_size == 0) {
		result = PyString_FromString("[]");
		goto Done;
	}

	pieces = PyList_New(0);
	if (pieces == NULL)
		goto Done;

	/* Do repr() on each element.  Note that this may mutate the list,
	   so must refetch the list size on each iteration. */
	for (i = 0; i < v->ob_size; ++i) {
		int status;
		s = PyObject_Repr(v->ob_item[i]);
		if (s == NULL)
			goto Done;
		status = PyList_Append(pieces, s);
		Py_DECREF(s);  /* append created a new ref */
		if (status < 0)
			goto Done;
	}

	/* Add "[]" decorations to the first and last items. */
	assert(PyList_GET_SIZE(pieces) > 0);
	s = PyString_FromString("[");
	if (s == NULL)
		goto Done;
	temp = PyList_GET_ITEM(pieces, 0);
	PyString_ConcatAndDel(&s, temp);
	PyList_SET_ITEM(pieces, 0, s);
	if (s == NULL)
		goto Done;

	s = PyString_FromString("]");
	if (s == NULL)
		goto Done;
	temp = PyList_GET_ITEM(pieces, PyList_GET_SIZE(pieces) - 1);
	PyString_ConcatAndDel(&temp, s);
	PyList_SET_ITEM(pieces, PyList_GET_SIZE(pieces) - 1, temp);
	if (temp == NULL)
		goto Done;

	/* Paste them all together with ", " between. */
	s = PyString_FromString(", ");
	if (s == NULL)
		goto Done;
	result = _PyString_Join(s, pieces);
	Py_DECREF(s);

Done:
	Py_XDECREF(pieces);
	Py_ReprLeave((PyObject *)v);
	return result;
}

static int
list_length(PyListObject *a)
{
	return a->ob_size;
}



static int
list_contains(PyListObject *a, PyObject *el)
{
	int i, cmp;

	for (i = 0, cmp = 0 ; cmp == 0 && i < a->ob_size; ++i)
		cmp = PyObject_RichCompareBool(el, PyList_GET_ITEM(a, i),
						   Py_EQ);
	return cmp;
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
	if (size < 0)
		return PyErr_NoMemory();
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
	PyObject *elem;
	if (n < 0)
		n = 0;
	size = a->ob_size * n;
	if (size == 0)
              return PyList_New(0);
	if (n && size/n != a->ob_size)
		return PyErr_NoMemory();
	np = (PyListObject *) PyList_New(size);
	if (np == NULL)
		return NULL;

	if (a->ob_size == 1) {
		elem = a->ob_item[0];
		for (i = 0; i < n; i++) {
			np->ob_item[i] = elem;
			Py_INCREF(elem);
		}
		return (PyObject *) np;
	}
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
	PyObject *v_as_SF = NULL; /* PySequence_Fast(v) */
	int n; /* Size of replacement list */
	int d; /* Change in size */
	int k; /* Loop index */
#define b ((PyListObject *)v)
	if (v == NULL)
		n = 0;
	else {
		char msg[256];
		if (a == b) {
			/* Special case "a[i:j] = a" -- copy b first */
			int ret;
			v = list_slice(b, 0, b->ob_size);
			if (v == NULL)
				return -1;
			ret = list_ass_slice(a, ilow, ihigh, v);
			Py_DECREF(v);
			return ret;
		}

		PyOS_snprintf(msg, sizeof(msg),
			      "must assign sequence"
			      " (not \"%.200s\") to slice",
			      v->ob_type->tp_name);
		v_as_SF = PySequence_Fast(v, msg);
		if(v_as_SF == NULL)
			return -1;
		n = PySequence_Fast_GET_SIZE(v_as_SF);
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
	if (ihigh > ilow) {
		p = recycle = PyMem_NEW(PyObject *, (ihigh-ilow));
		if (recycle == NULL) {
			PyErr_NoMemory();
			return -1;
		}
	}
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
		PyObject *w = PySequence_Fast_GET_ITEM(v_as_SF, k);
		Py_XINCREF(w);
		item[ilow] = w;
	}
	if (recycle) {
		while (--p >= recycle)
			Py_XDECREF(*p);
		PyMem_DEL(recycle);
	}
	if (a->ob_size == 0 && a->ob_item != NULL) {
		PyMem_FREE(a->ob_item);
		a->ob_item = NULL;
	}
	Py_XDECREF(v_as_SF);
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

static PyObject *
listappend(PyListObject *self, PyObject *v)
{
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
	other = PySequence_Fast(other, "argument to += must be iterable");
	if (!other)
		return NULL;

	if (listextend_internal(self, other) < 0)
		return NULL;

	Py_INCREF(self);
	return (PyObject *)self;
}

static PyObject *
listextend(PyListObject *self, PyObject *b)
{

	b = PySequence_Fast(b, "list.extend() argument must be iterable");
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

/* Reverse a slice of a list in place, from lo up to (exclusive) hi. */
static void
reverse_slice(PyObject **lo, PyObject **hi)
{
	assert(lo && hi);

	--hi;
	while (lo < hi) {
		PyObject *t = *lo;
		*lo = *hi;
		*hi = t;
		++lo;
		--hi;
	}
}

/* Lots of code for an adaptive, stable, natural mergesort.  There are many
 * pieces to this algorithm; read listsort.txt for overviews and details.
 */

/* Comparison function.  Takes care of calling a user-supplied
 * comparison function (any callable Python object), which must not be
 * NULL (use the ISLT macro if you don't know, or call PyObject_RichCompareBool
 * with Py_LT if you know it's NULL).
 * Returns -1 on error, 1 if x < y, 0 if x >= y.
 */
static int
islt(PyObject *x, PyObject *y, PyObject *compare)
{
	PyObject *res;
	PyObject *args;
	int i;

	assert(compare != NULL);
	/* Call the user's comparison function and translate the 3-way
	 * result into true or false (or error).
	 */
	args = PyTuple_New(2);
	if (args == NULL)
		return -1;
	Py_INCREF(x);
	Py_INCREF(y);
	PyTuple_SET_ITEM(args, 0, x);
	PyTuple_SET_ITEM(args, 1, y);
	res = PyObject_Call(compare, args, NULL);
	Py_DECREF(args);
	if (res == NULL)
		return -1;
	if (!PyInt_Check(res)) {
		Py_DECREF(res);
		PyErr_SetString(PyExc_TypeError,
				"comparison function must return int");
		return -1;
	}
	i = PyInt_AsLong(res);
	Py_DECREF(res);
	return i < 0;
}

/* If COMPARE is NULL, calls PyObject_RichCompareBool with Py_LT, else calls
 * islt.  This avoids a layer of function call in the usual case, and
 * sorting does many comparisons.
 * Returns -1 on error, 1 if x < y, 0 if x >= y.
 */
#define ISLT(X, Y, COMPARE) ((COMPARE) == NULL ?			\
			     PyObject_RichCompareBool(X, Y, Py_LT) :	\
			     islt(X, Y, COMPARE))

/* Compare X to Y via "<".  Goto "fail" if the comparison raises an
   error.  Else "k" is set to true iff X<Y, and an "if (k)" block is
   started.  It makes more sense in context <wink>.  X and Y are PyObject*s.
*/
#define IFLT(X, Y) if ((k = ISLT(X, Y, compare)) < 0) goto fail;  \
		   if (k)

/* binarysort is the best method for sorting small arrays: it does
   few compares, but can do data movement quadratic in the number of
   elements.
   [lo, hi) is a contiguous slice of a list, and is sorted via
   binary insertion.  This sort is stable.
   On entry, must have lo <= start <= hi, and that [lo, start) is already
   sorted (pass start == lo if you don't know!).
   If islt() complains return -1, else 0.
   Even in case of error, the output slice will be some permutation of
   the input (nothing is lost or duplicated).
*/
static int
binarysort(PyObject **lo, PyObject **hi, PyObject **start, PyObject *compare)
     /* compare -- comparison function object, or NULL for default */
{
	register int k;
	register PyObject **l, **p, **r;
	register PyObject *pivot;

	assert(lo <= start && start <= hi);
	/* assert [lo, start) is sorted */
	if (lo == start)
		++start;
	for (; start < hi; ++start) {
		/* set l to where *start belongs */
		l = lo;
		r = start;
		pivot = *r;
		/* Invariants:
		 * pivot >= all in [lo, l).
		 * pivot  < all in [r, start).
		 * The second is vacuously true at the start.
		 */
		assert(l < r);
		do {
			p = l + ((r - l) >> 1);
			IFLT(pivot, *p)
				r = p;
			else
				l = p+1;
		} while (l < r);
		assert(l == r);
		/* The invariants still hold, so pivot >= all in [lo, l) and
		   pivot < all in [l, start), so pivot belongs at l.  Note
		   that if there are elements equal to pivot, l points to the
		   first slot after them -- that's why this sort is stable.
		   Slide over to make room.
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

/*
Return the length of the run beginning at lo, in the slice [lo, hi).  lo < hi
is required on entry.  "A run" is the longest ascending sequence, with

    lo[0] <= lo[1] <= lo[2] <= ...

or the longest descending sequence, with

    lo[0] > lo[1] > lo[2] > ...

Boolean *descending is set to 0 in the former case, or to 1 in the latter.
For its intended use in a stable mergesort, the strictness of the defn of
"descending" is needed so that the caller can safely reverse a descending
sequence without violating stability (strict > ensures there are no equal
elements to get out of order).

Returns -1 in case of error.
*/
static int
count_run(PyObject **lo, PyObject **hi, PyObject *compare, int *descending)
{
	int k;
	int n;

	assert(lo < hi);
	*descending = 0;
	++lo;
	if (lo == hi)
		return 1;

	n = 2;
	IFLT(*lo, *(lo-1)) {
		*descending = 1;
		for (lo = lo+1; lo < hi; ++lo, ++n) {
			IFLT(*lo, *(lo-1))
				;
			else
				break;
		}
	}
	else {
		for (lo = lo+1; lo < hi; ++lo, ++n) {
			IFLT(*lo, *(lo-1))
				break;
		}
	}

	return n;
fail:
	return -1;
}

/*
Locate the proper position of key in a sorted vector; if the vector contains
an element equal to key, return the position immediately to the left of
the leftmost equal element.  [gallop_right() does the same except returns
the position to the right of the rightmost equal element (if any).]

"a" is a sorted vector with n elements, starting at a[0].  n must be > 0.

"hint" is an index at which to begin the search, 0 <= hint < n.  The closer
hint is to the final result, the faster this runs.

The return value is the int k in 0..n such that

    a[k-1] < key <= a[k]

pretending that *(a-1) is minus infinity and a[n] is plus infinity.  IOW,
key belongs at index k; or, IOW, the first k elements of a should precede
key, and the last n-k should follow key.

Returns -1 on error.  See listsort.txt for info on the method.
*/
static int
gallop_left(PyObject *key, PyObject **a, int n, int hint, PyObject *compare)
{
	int ofs;
	int lastofs;
	int k;

	assert(key && a && n > 0 && hint >= 0 && hint < n);

	a += hint;
	lastofs = 0;
	ofs = 1;
	IFLT(*a, key) {
		/* a[hint] < key -- gallop right, until
		 * a[hint + lastofs] < key <= a[hint + ofs]
		 */
		const int maxofs = n - hint;	/* &a[n-1] is highest */
		while (ofs < maxofs) {
			IFLT(a[ofs], key) {
				lastofs = ofs;
				ofs = (ofs << 1) + 1;
				if (ofs <= 0)	/* int overflow */
					ofs = maxofs;
			}
 			else	/* key <= a[hint + ofs] */
				break;
		}
		if (ofs > maxofs)
			ofs = maxofs;
		/* Translate back to offsets relative to &a[0]. */
		lastofs += hint;
		ofs += hint;
	}
	else {
		/* key <= a[hint] -- gallop left, until
		 * a[hint - ofs] < key <= a[hint - lastofs]
		 */
		const int maxofs = hint + 1;	/* &a[0] is lowest */
		while (ofs < maxofs) {
			IFLT(*(a-ofs), key)
				break;
			/* key <= a[hint - ofs] */
			lastofs = ofs;
			ofs = (ofs << 1) + 1;
			if (ofs <= 0)	/* int overflow */
				ofs = maxofs;
		}
		if (ofs > maxofs)
			ofs = maxofs;
		/* Translate back to positive offsets relative to &a[0]. */
		k = lastofs;
		lastofs = hint - ofs;
		ofs = hint - k;
	}
	a -= hint;

	assert(-1 <= lastofs && lastofs < ofs && ofs <= n);
	/* Now a[lastofs] < key <= a[ofs], so key belongs somewhere to the
	 * right of lastofs but no farther right than ofs.  Do a binary
	 * search, with invariant a[lastofs-1] < key <= a[ofs].
	 */
	++lastofs;
	while (lastofs < ofs) {
		int m = lastofs + ((ofs - lastofs) >> 1);

		IFLT(a[m], key)
			lastofs = m+1;	/* a[m] < key */
		else
			ofs = m;	/* key <= a[m] */
	}
	assert(lastofs == ofs);		/* so a[ofs-1] < key <= a[ofs] */
	return ofs;

fail:
	return -1;
}

/*
Exactly like gallop_left(), except that if key already exists in a[0:n],
finds the position immediately to the right of the rightmost equal value.

The return value is the int k in 0..n such that

    a[k-1] <= key < a[k]

or -1 if error.

The code duplication is massive, but this is enough different given that
we're sticking to "<" comparisons that it's much harder to follow if
written as one routine with yet another "left or right?" flag.
*/
static int
gallop_right(PyObject *key, PyObject **a, int n, int hint, PyObject *compare)
{
	int ofs;
	int lastofs;
	int k;

	assert(key && a && n > 0 && hint >= 0 && hint < n);

	a += hint;
	lastofs = 0;
	ofs = 1;
	IFLT(key, *a) {
		/* key < a[hint] -- gallop left, until
		 * a[hint - ofs] <= key < a[hint - lastofs]
		 */
		const int maxofs = hint + 1;	/* &a[0] is lowest */
		while (ofs < maxofs) {
			IFLT(key, *(a-ofs)) {
				lastofs = ofs;
				ofs = (ofs << 1) + 1;
				if (ofs <= 0)	/* int overflow */
					ofs = maxofs;
			}
			else	/* a[hint - ofs] <= key */
				break;
		}
		if (ofs > maxofs)
			ofs = maxofs;
		/* Translate back to positive offsets relative to &a[0]. */
		k = lastofs;
		lastofs = hint - ofs;
		ofs = hint - k;
	}
	else {
		/* a[hint] <= key -- gallop right, until
		 * a[hint + lastofs] <= key < a[hint + ofs]
		*/
		const int maxofs = n - hint;	/* &a[n-1] is highest */
		while (ofs < maxofs) {
			IFLT(key, a[ofs])
				break;
			/* a[hint + ofs] <= key */
			lastofs = ofs;
			ofs = (ofs << 1) + 1;
			if (ofs <= 0)	/* int overflow */
				ofs = maxofs;
		}
		if (ofs > maxofs)
			ofs = maxofs;
		/* Translate back to offsets relative to &a[0]. */
		lastofs += hint;
		ofs += hint;
	}
	a -= hint;

	assert(-1 <= lastofs && lastofs < ofs && ofs <= n);
	/* Now a[lastofs] <= key < a[ofs], so key belongs somewhere to the
	 * right of lastofs but no farther right than ofs.  Do a binary
	 * search, with invariant a[lastofs-1] <= key < a[ofs].
	 */
	++lastofs;
	while (lastofs < ofs) {
		int m = lastofs + ((ofs - lastofs) >> 1);

		IFLT(key, a[m])
			ofs = m;	/* key < a[m] */
		else
			lastofs = m+1;	/* a[m] <= key */
	}
	assert(lastofs == ofs);		/* so a[ofs-1] <= key < a[ofs] */
	return ofs;

fail:
	return -1;
}

/* The maximum number of entries in a MergeState's pending-runs stack.
 * This is enough to sort arrays of size up to about
 *     32 * phi ** MAX_MERGE_PENDING
 * where phi ~= 1.618.  85 is ridiculouslylarge enough, good for an array
 * with 2**64 elements.
 */
#define MAX_MERGE_PENDING 85

/* When we get into galloping mode, we stay there until both runs win less
 * often than MIN_GALLOP consecutive times.  See listsort.txt for more info.
 */
#define MIN_GALLOP 7

/* Avoid malloc for small temp arrays. */
#define MERGESTATE_TEMP_SIZE 256

/* One MergeState exists on the stack per invocation of mergesort.  It's just
 * a convenient way to pass state around among the helper functions.
 */
struct s_slice {
	PyObject **base;
	int len;
};

typedef struct s_MergeState {
	/* The user-supplied comparison function. or NULL if none given. */
	PyObject *compare;

	/* This controls when we get *into* galloping mode.  It's initialized
	 * to MIN_GALLOP.  merge_lo and merge_hi tend to nudge it higher for
	 * random data, and lower for highly structured data.
	 */
	int min_gallop;

	/* 'a' is temp storage to help with merges.  It contains room for
	 * alloced entries.
	 */
	PyObject **a;	/* may point to temparray below */
	int alloced;

	/* A stack of n pending runs yet to be merged.  Run #i starts at
	 * address base[i] and extends for len[i] elements.  It's always
	 * true (so long as the indices are in bounds) that
	 *
	 *     pending[i].base + pending[i].len == pending[i+1].base
	 *
	 * so we could cut the storage for this, but it's a minor amount,
	 * and keeping all the info explicit simplifies the code.
	 */
	int n;
	struct s_slice pending[MAX_MERGE_PENDING];

	/* 'a' points to this when possible, rather than muck with malloc. */
	PyObject *temparray[MERGESTATE_TEMP_SIZE];
} MergeState;

/* Conceptually a MergeState's constructor. */
static void
merge_init(MergeState *ms, PyObject *compare)
{
	assert(ms != NULL);
	ms->compare = compare;
	ms->a = ms->temparray;
	ms->alloced = MERGESTATE_TEMP_SIZE;
	ms->n = 0;
	ms->min_gallop = MIN_GALLOP;
}

/* Free all the temp memory owned by the MergeState.  This must be called
 * when you're done with a MergeState, and may be called before then if
 * you want to free the temp memory early.
 */
static void
merge_freemem(MergeState *ms)
{
	assert(ms != NULL);
	if (ms->a != ms->temparray)
		PyMem_Free(ms->a);
	ms->a = ms->temparray;
	ms->alloced = MERGESTATE_TEMP_SIZE;
}

/* Ensure enough temp memory for 'need' array slots is available.
 * Returns 0 on success and -1 if the memory can't be gotten.
 */
static int
merge_getmem(MergeState *ms, int need)
{
	assert(ms != NULL);
	if (need <= ms->alloced)
		return 0;
	/* Don't realloc!  That can cost cycles to copy the old data, but
	 * we don't care what's in the block.
	 */
	merge_freemem(ms);
	if (need > INT_MAX / sizeof(PyObject*)) {
		PyErr_NoMemory();
		return -1;
	}
	ms->a = (PyObject **)PyMem_Malloc(need * sizeof(PyObject*));
	if (ms->a) {
		ms->alloced = need;
		return 0;
	}
	PyErr_NoMemory();
	merge_freemem(ms);	/* reset to sane state */
	return -1;
}
#define MERGE_GETMEM(MS, NEED) ((NEED) <= (MS)->alloced ? 0 :	\
				merge_getmem(MS, NEED))

/* Merge the na elements starting at pa with the nb elements starting at pb
 * in a stable way, in-place.  na and nb must be > 0, and pa + na == pb.
 * Must also have that *pb < *pa, that pa[na-1] belongs at the end of the
 * merge, and should have na <= nb.  See listsort.txt for more info.
 * Return 0 if successful, -1 if error.
 */
static int
merge_lo(MergeState *ms, PyObject **pa, int na, PyObject **pb, int nb)
{
	int k;
	PyObject *compare;
	PyObject **dest;
	int result = -1;	/* guilty until proved innocent */
	int min_gallop = ms->min_gallop;

	assert(ms && pa && pb && na > 0 && nb > 0 && pa + na == pb);
	if (MERGE_GETMEM(ms, na) < 0)
		return -1;
	memcpy(ms->a, pa, na * sizeof(PyObject*));
	dest = pa;
	pa = ms->a;

	*dest++ = *pb++;
	--nb;
	if (nb == 0)
		goto Succeed;
	if (na == 1)
		goto CopyB;

	compare = ms->compare;
	for (;;) {
		int acount = 0;	/* # of times A won in a row */
		int bcount = 0;	/* # of times B won in a row */

		/* Do the straightforward thing until (if ever) one run
		 * appears to win consistently.
		 */
 		for (;;) {
 			assert(na > 1 && nb > 0);
	 		k = ISLT(*pb, *pa, compare);
			if (k) {
				if (k < 0)
					goto Fail;
				*dest++ = *pb++;
				++bcount;
				acount = 0;
				--nb;
				if (nb == 0)
					goto Succeed;
				if (bcount >= min_gallop)
					break;
			}
			else {
				*dest++ = *pa++;
				++acount;
				bcount = 0;
				--na;
				if (na == 1)
					goto CopyB;
				if (acount >= min_gallop)
					break;
			}
 		}

		/* One run is winning so consistently that galloping may
		 * be a huge win.  So try that, and continue galloping until
		 * (if ever) neither run appears to be winning consistently
		 * anymore.
		 */
		++min_gallop;
		do {
 			assert(na > 1 && nb > 0);
			min_gallop -= min_gallop > 1;
	 		ms->min_gallop = min_gallop;
			k = gallop_right(*pb, pa, na, 0, compare);
			acount = k;
			if (k) {
				if (k < 0)
					goto Fail;
				memcpy(dest, pa, k * sizeof(PyObject *));
				dest += k;
				pa += k;
				na -= k;
				if (na == 1)
					goto CopyB;
				/* na==0 is impossible now if the comparison
				 * function is consistent, but we can't assume
				 * that it is.
				 */
				if (na == 0)
					goto Succeed;
			}
			*dest++ = *pb++;
			--nb;
			if (nb == 0)
				goto Succeed;

 			k = gallop_left(*pa, pb, nb, 0, compare);
 			bcount = k;
			if (k) {
				if (k < 0)
					goto Fail;
				memmove(dest, pb, k * sizeof(PyObject *));
				dest += k;
				pb += k;
				nb -= k;
				if (nb == 0)
					goto Succeed;
			}
			*dest++ = *pa++;
			--na;
			if (na == 1)
				goto CopyB;
 		} while (acount >= MIN_GALLOP || bcount >= MIN_GALLOP);
 		++min_gallop;	/* penalize it for leaving galloping mode */
 		ms->min_gallop = min_gallop;
 	}
Succeed:
	result = 0;
Fail:
	if (na)
		memcpy(dest, pa, na * sizeof(PyObject*));
	return result;
CopyB:
	assert(na == 1 && nb > 0);
	/* The last element of pa belongs at the end of the merge. */
	memmove(dest, pb, nb * sizeof(PyObject *));
	dest[nb] = *pa;
	return 0;
}

/* Merge the na elements starting at pa with the nb elements starting at pb
 * in a stable way, in-place.  na and nb must be > 0, and pa + na == pb.
 * Must also have that *pb < *pa, that pa[na-1] belongs at the end of the
 * merge, and should have na >= nb.  See listsort.txt for more info.
 * Return 0 if successful, -1 if error.
 */
static int
merge_hi(MergeState *ms, PyObject **pa, int na, PyObject **pb, int nb)
{
	int k;
	PyObject *compare;
	PyObject **dest;
	int result = -1;	/* guilty until proved innocent */
	PyObject **basea;
	PyObject **baseb;
	int min_gallop = ms->min_gallop;

	assert(ms && pa && pb && na > 0 && nb > 0 && pa + na == pb);
	if (MERGE_GETMEM(ms, nb) < 0)
		return -1;
	dest = pb + nb - 1;
	memcpy(ms->a, pb, nb * sizeof(PyObject*));
	basea = pa;
	baseb = ms->a;
	pb = ms->a + nb - 1;
	pa += na - 1;

	*dest-- = *pa--;
	--na;
	if (na == 0)
		goto Succeed;
	if (nb == 1)
		goto CopyA;

	compare = ms->compare;
	for (;;) {
		int acount = 0;	/* # of times A won in a row */
		int bcount = 0;	/* # of times B won in a row */

		/* Do the straightforward thing until (if ever) one run
		 * appears to win consistently.
		 */
 		for (;;) {
 			assert(na > 0 && nb > 1);
	 		k = ISLT(*pb, *pa, compare);
			if (k) {
				if (k < 0)
					goto Fail;
				*dest-- = *pa--;
				++acount;
				bcount = 0;
				--na;
				if (na == 0)
					goto Succeed;
				if (acount >= min_gallop)
					break;
			}
			else {
				*dest-- = *pb--;
				++bcount;
				acount = 0;
				--nb;
				if (nb == 1)
					goto CopyA;
				if (bcount >= min_gallop)
					break;
			}
 		}

		/* One run is winning so consistently that galloping may
		 * be a huge win.  So try that, and continue galloping until
		 * (if ever) neither run appears to be winning consistently
		 * anymore.
		 */
		++min_gallop;
		do {
 			assert(na > 0 && nb > 1);
			min_gallop -= min_gallop > 1;
	 		ms->min_gallop = min_gallop;
			k = gallop_right(*pb, basea, na, na-1, compare);
			if (k < 0)
				goto Fail;
			k = na - k;
			acount = k;
			if (k) {
				dest -= k;
				pa -= k;
				memmove(dest+1, pa+1, k * sizeof(PyObject *));
				na -= k;
				if (na == 0)
					goto Succeed;
			}
			*dest-- = *pb--;
			--nb;
			if (nb == 1)
				goto CopyA;

 			k = gallop_left(*pa, baseb, nb, nb-1, compare);
			if (k < 0)
				goto Fail;
			k = nb - k;
			bcount = k;
			if (k) {
				dest -= k;
				pb -= k;
				memcpy(dest+1, pb+1, k * sizeof(PyObject *));
				nb -= k;
				if (nb == 1)
					goto CopyA;
				/* nb==0 is impossible now if the comparison
				 * function is consistent, but we can't assume
				 * that it is.
				 */
				if (nb == 0)
					goto Succeed;
			}
			*dest-- = *pa--;
			--na;
			if (na == 0)
				goto Succeed;
 		} while (acount >= MIN_GALLOP || bcount >= MIN_GALLOP);
 		++min_gallop;	/* penalize it for leaving galloping mode */
 		ms->min_gallop = min_gallop;
 	}
Succeed:
	result = 0;
Fail:
	if (nb)
		memcpy(dest-(nb-1), baseb, nb * sizeof(PyObject*));
	return result;
CopyA:
	assert(nb == 1 && na > 0);
	/* The first element of pb belongs at the front of the merge. */
	dest -= na;
	pa -= na;
	memmove(dest+1, pa+1, na * sizeof(PyObject *));
	*dest = *pb;
	return 0;
}

/* Merge the two runs at stack indices i and i+1.
 * Returns 0 on success, -1 on error.
 */
static int
merge_at(MergeState *ms, int i)
{
	PyObject **pa, **pb;
	int na, nb;
	int k;
	PyObject *compare;

	assert(ms != NULL);
	assert(ms->n >= 2);
	assert(i >= 0);
	assert(i == ms->n - 2 || i == ms->n - 3);

	pa = ms->pending[i].base;
	na = ms->pending[i].len;
	pb = ms->pending[i+1].base;
	nb = ms->pending[i+1].len;
	assert(na > 0 && nb > 0);
	assert(pa + na == pb);

	/* Record the length of the combined runs; if i is the 3rd-last
	 * run now, also slide over the last run (which isn't involved
	 * in this merge).  The current run i+1 goes away in any case.
	 */
	ms->pending[i].len = na + nb;
	if (i == ms->n - 3)
		ms->pending[i+1] = ms->pending[i+2];
	--ms->n;

	/* Where does b start in a?  Elements in a before that can be
	 * ignored (already in place).
	 */
	compare = ms->compare;
	k = gallop_right(*pb, pa, na, 0, compare);
	if (k < 0)
		return -1;
	pa += k;
	na -= k;
	if (na == 0)
		return 0;

	/* Where does a end in b?  Elements in b after that can be
	 * ignored (already in place).
	 */
	nb = gallop_left(pa[na-1], pb, nb, nb-1, compare);
	if (nb <= 0)
		return nb;

	/* Merge what remains of the runs, using a temp array with
	 * min(na, nb) elements.
	 */
	if (na <= nb)
		return merge_lo(ms, pa, na, pb, nb);
	else
		return merge_hi(ms, pa, na, pb, nb);
}

/* Examine the stack of runs waiting to be merged, merging adjacent runs
 * until the stack invariants are re-established:
 *
 * 1. len[-3] > len[-2] + len[-1]
 * 2. len[-2] > len[-1]
 *
 * See listsort.txt for more info.
 *
 * Returns 0 on success, -1 on error.
 */
static int
merge_collapse(MergeState *ms)
{
	struct s_slice *p = ms->pending;

	assert(ms);
	while (ms->n > 1) {
		int n = ms->n - 2;
		if (n > 0 && p[n-1].len <= p[n].len + p[n+1].len) {
		    	if (p[n-1].len < p[n+1].len)
		    		--n;
			if (merge_at(ms, n) < 0)
				return -1;
		}
		else if (p[n].len <= p[n+1].len) {
			 if (merge_at(ms, n) < 0)
			 	return -1;
		}
		else
			break;
	}
	return 0;
}

/* Regardless of invariants, merge all runs on the stack until only one
 * remains.  This is used at the end of the mergesort.
 *
 * Returns 0 on success, -1 on error.
 */
static int
merge_force_collapse(MergeState *ms)
{
	struct s_slice *p = ms->pending;

	assert(ms);
	while (ms->n > 1) {
		int n = ms->n - 2;
		if (n > 0 && p[n-1].len < p[n+1].len)
			--n;
		if (merge_at(ms, n) < 0)
			return -1;
	}
	return 0;
}

/* Compute a good value for the minimum run length; natural runs shorter
 * than this are boosted artificially via binary insertion.
 *
 * If n < 64, return n (it's too small to bother with fancy stuff).
 * Else if n is an exact power of 2, return 32.
 * Else return an int k, 32 <= k <= 64, such that n/k is close to, but
 * strictly less than, an exact power of 2.
 *
 * See listsort.txt for more info.
 */
static int
merge_compute_minrun(int n)
{
	int r = 0;	/* becomes 1 if any 1 bits are shifted off */

	assert(n >= 0);
	while (n >= 64) {
		r |= n & 1;
		n >>= 1;
	}
	return n + r;
}

/* An adaptive, stable, natural mergesort.  See listsort.txt.
 * Returns Py_None on success, NULL on error.  Even in case of error, the
 * list will be some permutation of its input state (nothing is lost or
 * duplicated).
 */
static PyObject *
listsort(PyListObject *self, PyObject *args)
{
	MergeState ms;
	PyObject **lo, **hi;
	int nremaining;
	int minrun;
	int saved_ob_size;
	PyObject **saved_ob_item;
	PyObject **empty_ob_item;
	PyObject *compare = NULL;
	PyObject *result = NULL;	/* guilty until proved innocent */

	assert(self != NULL);
	if (args != NULL) {
		if (!PyArg_UnpackTuple(args, "sort", 0, 1, &compare))
			return NULL;
	}
	if (compare == Py_None)
		compare = NULL;

	merge_init(&ms, compare);

	/* The list is temporarily made empty, so that mutations performed
	 * by comparison functions can't affect the slice of memory we're
	 * sorting (allowing mutations during sorting is a core-dump
	 * factory, since ob_item may change).
	 */
	saved_ob_size = self->ob_size;
	saved_ob_item = self->ob_item;
	self->ob_size = 0;
	self->ob_item = empty_ob_item = PyMem_NEW(PyObject *, 0);

	nremaining = saved_ob_size;
	if (nremaining < 2)
		goto succeed;

	/* March over the array once, left to right, finding natural runs,
	 * and extending short natural runs to minrun elements.
	 */
	lo = saved_ob_item;
	hi = lo + nremaining;
	minrun = merge_compute_minrun(nremaining);
	do {
		int descending;
		int n;

		/* Identify next run. */
		n = count_run(lo, hi, compare, &descending);
		if (n < 0)
			goto fail;
		if (descending)
			reverse_slice(lo, lo + n);
		/* If short, extend to min(minrun, nremaining). */
		if (n < minrun) {
			const int force = nremaining <= minrun ?
	 			  	  nremaining : minrun;
			if (binarysort(lo, lo + force, lo + n, compare) < 0)
				goto fail;
			n = force;
		}
		/* Push run onto pending-runs stack, and maybe merge. */
		assert(ms.n < MAX_MERGE_PENDING);
		ms.pending[ms.n].base = lo;
		ms.pending[ms.n].len = n;
		++ms.n;
		if (merge_collapse(&ms) < 0)
			goto fail;
		/* Advance to find next run. */
		lo += n;
		nremaining -= n;
	} while (nremaining);
	assert(lo == hi);

	if (merge_force_collapse(&ms) < 0)
		goto fail;
	assert(ms.n == 1);
	assert(ms.pending[0].base == saved_ob_item);
	assert(ms.pending[0].len == saved_ob_size);

succeed:
	result = Py_None;
fail:
	if (self->ob_item != empty_ob_item || self->ob_size) {
		/* The user mucked with the list during the sort. */
		(void)list_ass_slice(self, 0, self->ob_size, (PyObject *)NULL);
		if (result != NULL) {
			PyErr_SetString(PyExc_ValueError,
					"list modified during sort");
			result = NULL;
		}
	}
	if (self->ob_item == empty_ob_item)
		PyMem_FREE(empty_ob_item);
	self->ob_size = saved_ob_size;
	self->ob_item = saved_ob_item;
	merge_freemem(&ms);
	Py_XINCREF(result);
	return result;
}
#undef IFLT
#undef ISLT

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

static PyObject *
listreverse(PyListObject *self)
{
	if (self->ob_size > 1)
		reverse_slice(self->ob_item, self->ob_item + self->ob_size);
	Py_INCREF(Py_None);
	return Py_None;
}

int
PyList_Reverse(PyObject *v)
{
	PyListObject *self = (PyListObject *)v;

	if (v == NULL || !PyList_Check(v)) {
		PyErr_BadInternalCall();
		return -1;
	}
	if (self->ob_size > 1)
		reverse_slice(self->ob_item, self->ob_item + self->ob_size);
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
	int i, start=0, stop=self->ob_size;
	PyObject *v;

	if (!PyArg_ParseTuple(args, "O|O&O&:index", &v,
	                            _PyEval_SliceIndex, &start,
	                            _PyEval_SliceIndex, &stop))
		return NULL;
	if (start < 0) {
		start += self->ob_size;
		if (start < 0)
			start = 0;
	}
	if (stop < 0) {
		stop += self->ob_size;
		if (stop < 0)
			stop = 0;
	}
	else if (stop > self->ob_size)
		stop = self->ob_size;
	for (i = start; i < stop; i++) {
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
listcount(PyListObject *self, PyObject *v)
{
	int count = 0;
	int i;

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
listremove(PyListObject *self, PyObject *v)
{
	int i;

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

/* Adapted from newer code by Tim */
static int
list_fill(PyListObject *result, PyObject *v)
{
	PyObject *it;      /* iter(v) */
	int n;		   /* guess for result list size */
	int i;

	n = result->ob_size;

	/* Special-case list(a_list), for speed. */
	if (PyList_Check(v)) {
		if (v == (PyObject *)result)
			return 0; /* source is destination, we're done */
		return list_ass_slice(result, 0, n, v);
	}

	/* Empty previous contents */
	if (n != 0) {
		if (list_ass_slice(result, 0, n, (PyObject *)NULL) != 0)
			return -1;
	}

	/* Get iterator.  There may be some low-level efficiency to be gained
	 * by caching the tp_iternext slot instead of using PyIter_Next()
	 * later, but premature optimization is the root etc.
	 */
	it = PyObject_GetIter(v);
	if (it == NULL)
		return -1;

	/* Guess a result list size. */
	n = -1;	 /* unknown */
	if (PySequence_Check(v) &&
	    v->ob_type->tp_as_sequence->sq_length) {
		n = PySequence_Size(v);
		if (n < 0)
			PyErr_Clear();
	}
	if (n < 0)
		n = 8;	/* arbitrary */
	NRESIZE(result->ob_item, PyObject*, n);
	if (result->ob_item == NULL) {
		PyErr_NoMemory();
		goto error;
	}
	memset(result->ob_item, 0, sizeof(*result->ob_item) * n);
	result->ob_size = n;

	/* Run iterator to exhaustion. */
	for (i = 0; ; i++) {
		PyObject *item = PyIter_Next(it);
		if (item == NULL) {
			if (PyErr_Occurred())
				goto error;
			break;
		}
		if (i < n)
			PyList_SET_ITEM(result, i, item); /* steals ref */
		else {
			int status = ins1(result, result->ob_size, item);
			Py_DECREF(item);  /* append creates a new ref */
			if (status < 0)
				goto error;
		}
	}

	/* Cut back result list if initial guess was too large. */
	if (i < n && result != NULL) {
		if (list_ass_slice(result, i, n, (PyObject *)NULL) != 0)
			goto error;
	}
	Py_DECREF(it);
	return 0;

  error:
	Py_DECREF(it);
	return -1;
}

static int
list_init(PyListObject *self, PyObject *args, PyObject *kw)
{
	PyObject *arg = NULL;
	static char *kwlist[] = {"sequence", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kw, "|O:list", kwlist, &arg))
		return -1;
	if (arg != NULL)
		return list_fill(self, arg);
	if (self->ob_size > 0)
		return list_ass_slice(self, 0, self->ob_size, (PyObject*)NULL);
	return 0;
}

static long
list_nohash(PyObject *self)
{
	PyErr_SetString(PyExc_TypeError, "list objects are unhashable");
	return -1;
}

PyDoc_STRVAR(append_doc,
"L.append(object) -- append object to end");
PyDoc_STRVAR(extend_doc,
"L.extend(iterable) -- extend list by appending elements from the iterable");
PyDoc_STRVAR(insert_doc,
"L.insert(index, object) -- insert object before index");
PyDoc_STRVAR(pop_doc,
"L.pop([index]) -> item -- remove and return item at index (default last)");
PyDoc_STRVAR(remove_doc,
"L.remove(value) -- remove first occurrence of value");
PyDoc_STRVAR(index_doc,
"L.index(value, [start, [stop]]) -> integer -- return first index of value");
PyDoc_STRVAR(count_doc,
"L.count(value) -> integer -- return number of occurrences of value");
PyDoc_STRVAR(reverse_doc,
"L.reverse() -- reverse *IN PLACE*");
PyDoc_STRVAR(sort_doc,
"L.sort(cmpfunc=None) -- stable sort *IN PLACE*; cmpfunc(x, y) -> -1, 0, 1");

static PyMethodDef list_methods[] = {
	{"append",	(PyCFunction)listappend,  METH_O, append_doc},
	{"insert",	(PyCFunction)listinsert,  METH_VARARGS, insert_doc},
	{"extend",      (PyCFunction)listextend,  METH_O, extend_doc},
	{"pop",		(PyCFunction)listpop, 	  METH_VARARGS, pop_doc},
	{"remove",	(PyCFunction)listremove,  METH_O, remove_doc},
	{"index",	(PyCFunction)listindex,   METH_VARARGS, index_doc},
	{"count",	(PyCFunction)listcount,   METH_O, count_doc},
	{"reverse",	(PyCFunction)listreverse, METH_NOARGS, reverse_doc},
	{"sort",	(PyCFunction)listsort, 	  METH_VARARGS, sort_doc},
 	{NULL,		NULL}		/* sentinel */
};

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

PyDoc_STRVAR(list_doc,
"list() -> new list\n"
"list(sequence) -> new list initialized from sequence's items");

static PyObject *list_iter(PyObject *seq);

static PyObject *
list_subscript(PyListObject* self, PyObject* item)
{
	if (PyInt_Check(item)) {
		long i = PyInt_AS_LONG(item);
		if (i < 0)
			i += PyList_GET_SIZE(self);
		return list_item(self, i);
	}
	else if (PyLong_Check(item)) {
		long i = PyLong_AsLong(item);
		if (i == -1 && PyErr_Occurred())
			return NULL;
		if (i < 0)
			i += PyList_GET_SIZE(self);
		return list_item(self, i);
	}
	else if (PySlice_Check(item)) {
		int start, stop, step, slicelength, cur, i;
		PyObject* result;
		PyObject* it;

		if (PySlice_GetIndicesEx((PySliceObject*)item, self->ob_size,
				 &start, &stop, &step, &slicelength) < 0) {
			return NULL;
		}

		if (slicelength <= 0) {
			return PyList_New(0);
		}
		else {
			result = PyList_New(slicelength);
			if (!result) return NULL;

			for (cur = start, i = 0; i < slicelength;
			     cur += step, i++) {
				it = PyList_GET_ITEM(self, cur);
				Py_INCREF(it);
				PyList_SET_ITEM(result, i, it);
			}

			return result;
		}
	}
	else {
		PyErr_SetString(PyExc_TypeError,
				"list indices must be integers");
		return NULL;
	}
}

static int
list_ass_subscript(PyListObject* self, PyObject* item, PyObject* value)
{
	if (PyInt_Check(item)) {
		long i = PyInt_AS_LONG(item);
		if (i < 0)
			i += PyList_GET_SIZE(self);
		return list_ass_item(self, i, value);
	}
	else if (PyLong_Check(item)) {
		long i = PyLong_AsLong(item);
		if (i == -1 && PyErr_Occurred())
			return -1;
		if (i < 0)
			i += PyList_GET_SIZE(self);
		return list_ass_item(self, i, value);
	}
	else if (PySlice_Check(item)) {
		int start, stop, step, slicelength;

		if (PySlice_GetIndicesEx((PySliceObject*)item, self->ob_size,
				 &start, &stop, &step, &slicelength) < 0) {
			return -1;
		}

		/* treat L[slice(a,b)] = v _exactly_ like L[a:b] = v */
		if (step == 1 && ((PySliceObject*)item)->step == Py_None)
			return list_ass_slice(self, start, stop, value);

		if (value == NULL) {
			/* delete slice */
			PyObject **garbage, **it;
			int cur, i, j;

			if (slicelength <= 0)
				return 0;

			if (step < 0) {
				stop = start + 1;
				start = stop + step*(slicelength - 1) - 1;
				step = -step;
			}

			garbage = (PyObject**)
				PyMem_MALLOC(slicelength*sizeof(PyObject*));

			/* drawing pictures might help
			   understand these for loops */
			for (cur = start, i = 0;
			     cur < stop;
			     cur += step, i++) {
				int lim = step;

				garbage[i] = PyList_GET_ITEM(self, cur);

				if (cur + step >= self->ob_size) {
					lim = self->ob_size - cur - 1;
				}

				for (j = 0; j < lim; j++) {
					PyList_SET_ITEM(self, cur + j - i,
						PyList_GET_ITEM(self,
								cur + j + 1));
				}
			}
			for (cur = start + slicelength*step + 1;
			     cur < self->ob_size; cur++) {
				PyList_SET_ITEM(self, cur - slicelength,
						PyList_GET_ITEM(self, cur));
			}
			self->ob_size -= slicelength;
			it = self->ob_item;
			NRESIZE(it, PyObject*, self->ob_size);
			self->ob_item = it;

			for (i = 0; i < slicelength; i++) {
				Py_DECREF(garbage[i]);
			}
			PyMem_FREE(garbage);

			return 0;
		}
		else {
			/* assign slice */
			PyObject **garbage, *ins, *seq;
			int cur, i;

			/* protect against a[::-1] = a */
			if (self == (PyListObject*)value) {
				seq = list_slice((PyListObject*)value, 0,
						   PyList_GET_SIZE(value));
			}
			else {
				char msg[256];
				PyOS_snprintf(msg, sizeof(msg),
		      "must assign sequence (not \"%.200s\") to extended slice",
					      value->ob_type->tp_name);
				seq = PySequence_Fast(value, msg);
				if (!seq)
					return -1;
			}

			if (PySequence_Fast_GET_SIZE(seq) != slicelength) {
				PyErr_Format(PyExc_ValueError,
            "attempt to assign sequence of size %d to extended slice of size %d",
					     PySequence_Fast_GET_SIZE(seq),
					     slicelength);
				Py_DECREF(seq);
				return -1;
			}

			if (!slicelength) {
				Py_DECREF(seq);
				return 0;
			}

			assert(slicelength <= PY_SIZE_MAX / sizeof(PyObject*));

			garbage = (PyObject**)
				PyMem_MALLOC(slicelength*sizeof(PyObject*));

			for (cur = start, i = 0; i < slicelength;
			     cur += step, i++) {
				garbage[i] = PyList_GET_ITEM(self, cur);

				ins = PySequence_Fast_GET_ITEM(seq, i);
				Py_INCREF(ins);
				PyList_SET_ITEM(self, cur, ins);
			}

			for (i = 0; i < slicelength; i++) {
				Py_DECREF(garbage[i]);
			}

			PyMem_FREE(garbage);
			Py_DECREF(seq);

			return 0;
		}
	}
	else {
		PyErr_SetString(PyExc_TypeError,
				"list indices must be integers");
		return -1;
	}
}

static PyMappingMethods list_as_mapping = {
	(inquiry)list_length,
	(binaryfunc)list_subscript,
	(objobjargproc)list_ass_subscript
};

PyTypeObject PyList_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"list",
	sizeof(PyListObject),
	0,
	(destructor)list_dealloc,		/* tp_dealloc */
	(printfunc)list_print,			/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)list_repr,			/* tp_repr */
	0,					/* tp_as_number */
	&list_as_sequence,			/* tp_as_sequence */
	&list_as_mapping,			/* tp_as_mapping */
	list_nohash,				/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,		/* tp_flags */
 	list_doc,				/* tp_doc */
 	(traverseproc)list_traverse,		/* tp_traverse */
 	(inquiry)list_clear,			/* tp_clear */
	list_richcompare,			/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	list_iter,				/* tp_iter */
	0,					/* tp_iternext */
	list_methods,				/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	(initproc)list_init,			/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	PyType_GenericNew,			/* tp_new */
	PyObject_GC_Del,			/* tp_free */
};


/*********************** List Iterator **************************/

typedef struct {
	PyObject_HEAD
	long it_index;
	PyListObject *it_seq; /* Set to NULL when iterator is exhausted */
} listiterobject;

PyTypeObject PyListIter_Type;

static PyObject *
list_iter(PyObject *seq)
{
	listiterobject *it;

	if (!PyList_Check(seq)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	it = PyObject_GC_New(listiterobject, &PyListIter_Type);
	if (it == NULL)
		return NULL;
	it->it_index = 0;
	Py_INCREF(seq);
	it->it_seq = (PyListObject *)seq;
	_PyObject_GC_TRACK(it);
	return (PyObject *)it;
}

static void
listiter_dealloc(listiterobject *it)
{
	_PyObject_GC_UNTRACK(it);
	Py_XDECREF(it->it_seq);
	PyObject_GC_Del(it);
}

static int
listiter_traverse(listiterobject *it, visitproc visit, void *arg)
{
	if (it->it_seq == NULL)
		return 0;
	return visit((PyObject *)it->it_seq, arg);
}

static PyObject *
listiter_next(listiterobject *it)
{
	PyListObject *seq;
	PyObject *item;

	assert(it != NULL);
	seq = it->it_seq;
	if (seq == NULL)
		return NULL;
	assert(PyList_Check(seq));

	if (it->it_index < PyList_GET_SIZE(seq)) {
		item = PyList_GET_ITEM(seq, it->it_index);
		++it->it_index;
		Py_INCREF(item);
		return item;
	}

	Py_DECREF(seq);
	it->it_seq = NULL;
	return NULL;
}

PyTypeObject PyListIter_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,					/* ob_size */
	"listiterator",				/* tp_name */
	sizeof(listiterobject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	(destructor)listiter_dealloc,		/* tp_dealloc */
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
	(traverseproc)listiter_traverse,	/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	PyObject_SelfIter,			/* tp_iter */
	(iternextfunc)listiter_next,		/* tp_iternext */
	0,					/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
};
