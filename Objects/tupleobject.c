
/* Tuple object implementation */

#include "Python.h"

/* Speed optimization to avoid frequent malloc/free of small tuples */
#ifndef MAXSAVESIZE
#define MAXSAVESIZE	20  /* Largest tuple to save on free list */
#endif
#ifndef MAXSAVEDTUPLES 
#define MAXSAVEDTUPLES  2000  /* Maximum number of tuples of each size to save */
#endif

#if MAXSAVESIZE > 0
/* Entries 1 up to MAXSAVESIZE are free lists, entry 0 is the empty
   tuple () of which at most one instance will be allocated.
*/
static PyTupleObject *free_tuples[MAXSAVESIZE];
static int num_free_tuples[MAXSAVESIZE];
#endif
#ifdef COUNT_ALLOCS
int fast_tuple_allocs;
int tuple_zero_allocs;
#endif

PyObject *
PyTuple_New(register int size)
{
	register PyTupleObject *op;
	if (size < 0) {
		PyErr_BadInternalCall();
		return NULL;
	}
#if MAXSAVESIZE > 0
	if (size == 0 && free_tuples[0]) {
		op = free_tuples[0];
		Py_INCREF(op);
#ifdef COUNT_ALLOCS
		tuple_zero_allocs++;
#endif
		return (PyObject *) op;
	}
	if (0 < size && size < MAXSAVESIZE &&
	    (op = free_tuples[size]) != NULL)
	{
		free_tuples[size] = (PyTupleObject *) op->ob_item[0];
		num_free_tuples[size]--;
#ifdef COUNT_ALLOCS
		fast_tuple_allocs++;
#endif
		/* PyObject_InitVar is inlined */
#ifdef Py_TRACE_REFS
		op->ob_size = size;
		op->ob_type = &PyTuple_Type;
#endif
		_Py_NewReference((PyObject *)op);
	}
	else
#endif
	{
		int nbytes = size * sizeof(PyObject *);
		/* Check for overflow */
		if (nbytes / sizeof(PyObject *) != (size_t)size ||
		    (nbytes += sizeof(PyTupleObject) - sizeof(PyObject *))
		    <= 0)
		{
			return PyErr_NoMemory();
		}
		op = PyObject_GC_NewVar(PyTupleObject, &PyTuple_Type, size);
		if (op == NULL)
			return NULL;
	}
	memset(op->ob_item, 0, sizeof(*op->ob_item) * size);
#if MAXSAVESIZE > 0
	if (size == 0) {
		free_tuples[0] = op;
		++num_free_tuples[0];
		Py_INCREF(op);	/* extra INCREF so that this is never freed */
	}
#endif
	_PyObject_GC_TRACK(op);
	return (PyObject *) op;
}

int
PyTuple_Size(register PyObject *op)
{
	if (!PyTuple_Check(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	else
		return ((PyTupleObject *)op)->ob_size;
}

PyObject *
PyTuple_GetItem(register PyObject *op, register int i)
{
	if (!PyTuple_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	if (i < 0 || i >= ((PyTupleObject *)op) -> ob_size) {
		PyErr_SetString(PyExc_IndexError, "tuple index out of range");
		return NULL;
	}
	return ((PyTupleObject *)op) -> ob_item[i];
}

int
PyTuple_SetItem(register PyObject *op, register int i, PyObject *newitem)
{
	register PyObject *olditem;
	register PyObject **p;
	if (!PyTuple_Check(op) || op->ob_refcnt != 1) {
		Py_XDECREF(newitem);
		PyErr_BadInternalCall();
		return -1;
	}
	if (i < 0 || i >= ((PyTupleObject *)op) -> ob_size) {
		Py_XDECREF(newitem);
		PyErr_SetString(PyExc_IndexError,
				"tuple assignment index out of range");
		return -1;
	}
	p = ((PyTupleObject *)op) -> ob_item + i;
	olditem = *p;
	*p = newitem;
	Py_XDECREF(olditem);
	return 0;
}

/* Methods */

static void
tupledealloc(register PyTupleObject *op)
{
	register int i;
	register int len =  op->ob_size;
	PyObject_GC_UnTrack(op);
	Py_TRASHCAN_SAFE_BEGIN(op)
	if (len > 0) {
		i = len;
		while (--i >= 0)
			Py_XDECREF(op->ob_item[i]);
#if MAXSAVESIZE > 0
		if (len < MAXSAVESIZE &&
		    num_free_tuples[len] < MAXSAVEDTUPLES &&
		    op->ob_type == &PyTuple_Type)
		{
			op->ob_item[0] = (PyObject *) free_tuples[len];
			num_free_tuples[len]++;
			free_tuples[len] = op;
			goto done; /* return */
		}
#endif
	}
	op->ob_type->tp_free((PyObject *)op);
done:
	Py_TRASHCAN_SAFE_END(op)
}

static int
tupleprint(PyTupleObject *op, FILE *fp, int flags)
{
	int i;
	fprintf(fp, "(");
	for (i = 0; i < op->ob_size; i++) {
		if (i > 0)
			fprintf(fp, ", ");
		if (PyObject_Print(op->ob_item[i], fp, 0) != 0)
			return -1;
	}
	if (op->ob_size == 1)
		fprintf(fp, ",");
	fprintf(fp, ")");
	return 0;
}

static PyObject *
tuplerepr(PyTupleObject *v)
{
	int i, n;
	PyObject *s, *temp;
	PyObject *pieces, *result = NULL;

	n = v->ob_size;
	if (n == 0)
		return PyString_FromString("()");

	pieces = PyTuple_New(n);
	if (pieces == NULL)
		return NULL;

	/* Do repr() on each element. */
	for (i = 0; i < n; ++i) {
		s = PyObject_Repr(v->ob_item[i]);
		if (s == NULL)
			goto Done;
		PyTuple_SET_ITEM(pieces, i, s);
	}

	/* Add "()" decorations to the first and last items. */
	assert(n > 0);
	s = PyString_FromString("(");
	if (s == NULL)
		goto Done;
	temp = PyTuple_GET_ITEM(pieces, 0);
	PyString_ConcatAndDel(&s, temp);
	PyTuple_SET_ITEM(pieces, 0, s);
	if (s == NULL)
		goto Done;

	s = PyString_FromString(n == 1 ? ",)" : ")");
	if (s == NULL)
		goto Done;
	temp = PyTuple_GET_ITEM(pieces, n-1);
	PyString_ConcatAndDel(&temp, s);
	PyTuple_SET_ITEM(pieces, n-1, temp);
	if (temp == NULL)
		goto Done;

	/* Paste them all together with ", " between. */
	s = PyString_FromString(", ");
	if (s == NULL)
		goto Done;
	result = _PyString_Join(s, pieces);
	Py_DECREF(s);	

Done:
	Py_DECREF(pieces);
	return result;
}

static long
tuplehash(PyTupleObject *v)
{
	register long x, y;
	register int len = v->ob_size;
	register PyObject **p;
	x = 0x345678L;
	p = v->ob_item;
	while (--len >= 0) {
		y = PyObject_Hash(*p++);
		if (y == -1)
			return -1;
		x = (1000003*x) ^ y;
	}
	x ^= v->ob_size;
	if (x == -1)
		x = -2;
	return x;
}

static int
tuplelength(PyTupleObject *a)
{
	return a->ob_size;
}

static int
tuplecontains(PyTupleObject *a, PyObject *el)
{
	int i, cmp;

	for (i = 0; i < a->ob_size; ++i) {
		cmp = PyObject_RichCompareBool(el, PyTuple_GET_ITEM(a, i),
					       Py_EQ);
		if (cmp > 0)
			return 1;
		else if (cmp < 0)
			return -1;
	}
	return 0;
}

static PyObject *
tupleitem(register PyTupleObject *a, register int i)
{
	if (i < 0 || i >= a->ob_size) {
		PyErr_SetString(PyExc_IndexError, "tuple index out of range");
		return NULL;
	}
	Py_INCREF(a->ob_item[i]);
	return a->ob_item[i];
}

static PyObject *
tupleslice(register PyTupleObject *a, register int ilow, register int ihigh)
{
	register PyTupleObject *np;
	register int i;
	if (ilow < 0)
		ilow = 0;
	if (ihigh > a->ob_size)
		ihigh = a->ob_size;
	if (ihigh < ilow)
		ihigh = ilow;
	if (ilow == 0 && ihigh == a->ob_size && PyTuple_CheckExact(a)) {
		Py_INCREF(a);
		return (PyObject *)a;
	}
	np = (PyTupleObject *)PyTuple_New(ihigh - ilow);
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
PyTuple_GetSlice(PyObject *op, int i, int j)
{
	if (op == NULL || !PyTuple_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return tupleslice((PyTupleObject *)op, i, j);
}

static PyObject *
tupleconcat(register PyTupleObject *a, register PyObject *bb)
{
	register int size;
	register int i;
	PyTupleObject *np;
	if (!PyTuple_Check(bb)) {
		PyErr_Format(PyExc_TypeError,
       		     "can only concatenate tuple (not \"%.200s\") to tuple",
			     bb->ob_type->tp_name);
		return NULL;
	}
#define b ((PyTupleObject *)bb)
	size = a->ob_size + b->ob_size;
	np = (PyTupleObject *) PyTuple_New(size);
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
tuplerepeat(PyTupleObject *a, int n)
{
	int i, j;
	int size;
	PyTupleObject *np;
	PyObject **p;
	if (n < 0)
		n = 0;
	if (a->ob_size == 0 || n == 1) {
		if (PyTuple_CheckExact(a)) {
			/* Since tuples are immutable, we can return a shared
			   copy in this case */
			Py_INCREF(a);
			return (PyObject *)a;
		}
		if (a->ob_size == 0)
			return PyTuple_New(0);
	}
	size = a->ob_size * n;
	if (size/a->ob_size != n)
		return PyErr_NoMemory();
	np = (PyTupleObject *) PyTuple_New(size);
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
tupletraverse(PyTupleObject *o, visitproc visit, void *arg)
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

static PyObject *
tuplerichcompare(PyObject *v, PyObject *w, int op)
{
	PyTupleObject *vt, *wt;
	int i;
	int vlen, wlen;

	if (!PyTuple_Check(v) || !PyTuple_Check(w)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	vt = (PyTupleObject *)v;
	wt = (PyTupleObject *)w;

	vlen = vt->ob_size;
	wlen = wt->ob_size;

	/* Note:  the corresponding code for lists has an "early out" test
	 * here when op is EQ or NE and the lengths differ.  That pays there,
	 * but Tim was unable to find any real code where EQ/NE tuple
	 * compares don't have the same length, so testing for it here would
	 * have cost without benefit.
	 */

	/* Search for the first index where items are different.
	 * Note that because tuples are immutable, it's safe to reuse
	 * vlen and wlen across the comparison calls.
	 */
	for (i = 0; i < vlen && i < wlen; i++) {
		int k = PyObject_RichCompareBool(vt->ob_item[i],
						 wt->ob_item[i], Py_EQ);
		if (k < 0)
			return NULL;
		if (!k)
			break;
	}

	if (i >= vlen || i >= wlen) {
		/* No more items to compare -- compare sizes */
		int cmp;
		PyObject *res;
		switch (op) {
		case Py_LT: cmp = vlen <  wlen; break;
		case Py_LE: cmp = vlen <= wlen; break;
		case Py_EQ: cmp = vlen == wlen; break;
		case Py_NE: cmp = vlen != wlen; break;
		case Py_GT: cmp = vlen >  wlen; break;
		case Py_GE: cmp = vlen >= wlen; break;
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
	return PyObject_RichCompare(vt->ob_item[i], wt->ob_item[i], op);
}

staticforward PyObject *
tuple_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static PyObject *
tuple_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *arg = NULL;
	static char *kwlist[] = {"sequence", 0};

	if (type != &PyTuple_Type)
		return tuple_subtype_new(type, args, kwds);
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O:tuple", kwlist, &arg))
		return NULL;

	if (arg == NULL)
		return PyTuple_New(0);
	else
		return PySequence_Tuple(arg);
}

static PyObject *
tuple_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *tmp, *new, *item;
	int i, n;

	assert(PyType_IsSubtype(type, &PyTuple_Type));
	tmp = tuple_new(&PyTuple_Type, args, kwds);
	if (tmp == NULL)
		return NULL;
	assert(PyTuple_Check(tmp));
	new = type->tp_alloc(type, n = PyTuple_GET_SIZE(tmp));
	if (new == NULL)
		return NULL;
	for (i = 0; i < n; i++) {
		item = PyTuple_GET_ITEM(tmp, i);
		Py_INCREF(item);
		PyTuple_SET_ITEM(new, i, item);
	}
	Py_DECREF(tmp);
	return new;
}

PyDoc_STRVAR(tuple_doc,
"tuple() -> an empty tuple\n"
"tuple(sequence) -> tuple initialized from sequence's items\n"
"\n"
"If the argument is a tuple, the return value is the same object.");

static PySequenceMethods tuple_as_sequence = {
	(inquiry)tuplelength,			/* sq_length */
	(binaryfunc)tupleconcat,		/* sq_concat */
	(intargfunc)tuplerepeat,		/* sq_repeat */
	(intargfunc)tupleitem,			/* sq_item */
	(intintargfunc)tupleslice,		/* sq_slice */
	0,					/* sq_ass_item */
	0,					/* sq_ass_slice */
	(objobjproc)tuplecontains,		/* sq_contains */
};

static PyObject*
tuplesubscript(PyTupleObject* self, PyObject* item)
{
	if (PyInt_Check(item)) {
		long i = PyInt_AS_LONG(item);
		if (i < 0)
			i += PyTuple_GET_SIZE(self);
		return tupleitem(self, i);
	}
	else if (PyLong_Check(item)) {
		long i = PyLong_AsLong(item);
		if (i == -1 && PyErr_Occurred())
			return NULL;
		if (i < 0)
			i += PyTuple_GET_SIZE(self);
		return tupleitem(self, i);
	}
	else if (PySlice_Check(item)) {
		int start, stop, step, slicelength, cur, i;
		PyObject* result;
		PyObject* it;

		if (PySlice_GetIndicesEx((PySliceObject*)item,
				 PyTuple_GET_SIZE(self),
				 &start, &stop, &step, &slicelength) < 0) {
			return NULL;
		}

		if (slicelength <= 0) {
			return PyTuple_New(0);
		}
		else {
			result = PyTuple_New(slicelength);

			for (cur = start, i = 0; i < slicelength; 
			     cur += step, i++) {
				it = PyTuple_GET_ITEM(self, cur);
				Py_INCREF(it);
				PyTuple_SET_ITEM(result, i, it);
			}
			
			return result;
		}
	}
	else {
		PyErr_SetString(PyExc_TypeError, 
				"tuple indices must be integers");
		return NULL;
	}
}

static PyMappingMethods tuple_as_mapping = {
	(inquiry)tuplelength,
	(binaryfunc)tuplesubscript,
	0
};

PyTypeObject PyTuple_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"tuple",
	sizeof(PyTupleObject) - sizeof(PyObject *),
	sizeof(PyObject *),
	(destructor)tupledealloc,		/* tp_dealloc */
	(printfunc)tupleprint,			/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)tuplerepr,			/* tp_repr */
	0,					/* tp_as_number */
	&tuple_as_sequence,			/* tp_as_sequence */
	&tuple_as_mapping,			/* tp_as_mapping */
	(hashfunc)tuplehash,			/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
		Py_TPFLAGS_BASETYPE,		/* tp_flags */
	tuple_doc,				/* tp_doc */
 	(traverseproc)tupletraverse,		/* tp_traverse */
	0,					/* tp_clear */
	tuplerichcompare,			/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	tuple_new,				/* tp_new */
	PyObject_GC_Del,        		/* tp_free */
};

/* The following function breaks the notion that tuples are immutable:
   it changes the size of a tuple.  We get away with this only if there
   is only one module referencing the object.  You can also think of it
   as creating a new tuple object and destroying the old one, only more
   efficiently.  In any case, don't use this if the tuple may already be
   known to some other part of the code. */

int
_PyTuple_Resize(PyObject **pv, int newsize)
{
	register PyTupleObject *v;
	register PyTupleObject *sv;
	int i;
	int oldsize;

	v = (PyTupleObject *) *pv;
	if (v == NULL || v->ob_type != &PyTuple_Type ||
	    (v->ob_size != 0 && v->ob_refcnt != 1)) {
		*pv = 0;
		Py_XDECREF(v);
		PyErr_BadInternalCall();
		return -1;
	}
	oldsize = v->ob_size;
	if (oldsize == newsize)
		return 0;

	if (oldsize == 0) {
		/* Empty tuples are often shared, so we should never 
		   resize them in-place even if we do own the only
		   (current) reference */
		Py_DECREF(v);
		*pv = PyTuple_New(newsize);
		return *pv == NULL ? -1 : 0;
	}

	/* XXX UNREF/NEWREF interface should be more symmetrical */
#ifdef Py_REF_DEBUG
	--_Py_RefTotal;
#endif
	_PyObject_GC_UNTRACK(v);
	_Py_ForgetReference((PyObject *) v);
	/* DECREF items deleted by shrinkage */
	for (i = newsize; i < oldsize; i++) {
		Py_XDECREF(v->ob_item[i]);
		v->ob_item[i] = NULL;
	}
	sv = PyObject_GC_Resize(PyTupleObject, v, newsize);
	if (sv == NULL) {
		*pv = NULL;
		PyObject_GC_Del(v);
		return -1;
	}
	_Py_NewReference((PyObject *) sv);
	/* Zero out items added by growing */
	if (newsize > oldsize)
		memset(&sv->ob_item[oldsize], 0,
		       sizeof(*sv->ob_item) * (newsize - oldsize));
	*pv = (PyObject *) sv;
	_PyObject_GC_TRACK(sv);
	return 0;
}

void
PyTuple_Fini(void)
{
#if MAXSAVESIZE > 0
	int i;

	Py_XDECREF(free_tuples[0]);
	free_tuples[0] = NULL;

	for (i = 1; i < MAXSAVESIZE; i++) {
		PyTupleObject *p, *q;
		p = free_tuples[i];
		free_tuples[i] = NULL;
		while (p) {
			q = p;
			p = (PyTupleObject *)(p->ob_item[0]);
			PyObject_GC_Del(q);
		}
	}
#endif
}
