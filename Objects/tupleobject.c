
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
PyTuple_New(register Py_ssize_t size)
{
	register PyTupleObject *op;
	Py_ssize_t i;
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
	if (size < MAXSAVESIZE && (op = free_tuples[size]) != NULL) {
		free_tuples[size] = (PyTupleObject *) op->ob_item[0];
		num_free_tuples[size]--;
#ifdef COUNT_ALLOCS
		fast_tuple_allocs++;
#endif
		/* Inline PyObject_InitVar */
#ifdef Py_TRACE_REFS
		Py_Size(op) = size;
		Py_Type(op) = &PyTuple_Type;
#endif
		_Py_NewReference((PyObject *)op);
	}
	else
#endif
	{
		Py_ssize_t nbytes = size * sizeof(PyObject *);
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
	for (i=0; i < size; i++)
		op->ob_item[i] = NULL;
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

Py_ssize_t
PyTuple_Size(register PyObject *op)
{
	if (!PyTuple_Check(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	else
		return Py_Size(op);
}

PyObject *
PyTuple_GetItem(register PyObject *op, register Py_ssize_t i)
{
	if (!PyTuple_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	if (i < 0 || i >= Py_Size(op)) {
		PyErr_SetString(PyExc_IndexError, "tuple index out of range");
		return NULL;
	}
	return ((PyTupleObject *)op) -> ob_item[i];
}

int
PyTuple_SetItem(register PyObject *op, register Py_ssize_t i, PyObject *newitem)
{
	register PyObject *olditem;
	register PyObject **p;
	if (!PyTuple_Check(op) || op->ob_refcnt != 1) {
		Py_XDECREF(newitem);
		PyErr_BadInternalCall();
		return -1;
	}
	if (i < 0 || i >= Py_Size(op)) {
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

PyObject *
PyTuple_Pack(Py_ssize_t n, ...)
{
	Py_ssize_t i;
	PyObject *o;
	PyObject *result;
	PyObject **items;
	va_list vargs;

	va_start(vargs, n);
	result = PyTuple_New(n);
	if (result == NULL)
		return NULL;
	items = ((PyTupleObject *)result)->ob_item;
	for (i = 0; i < n; i++) {
		o = va_arg(vargs, PyObject *);
		Py_INCREF(o);
		items[i] = o;
	}
	va_end(vargs);
	return result;
}


/* Methods */

static void
tupledealloc(register PyTupleObject *op)
{
	register Py_ssize_t i;
	register Py_ssize_t len =  Py_Size(op);
	PyObject_GC_UnTrack(op);
	Py_TRASHCAN_SAFE_BEGIN(op)
	if (len > 0) {
		i = len;
		while (--i >= 0)
			Py_XDECREF(op->ob_item[i]);
#if MAXSAVESIZE > 0
		if (len < MAXSAVESIZE &&
		    num_free_tuples[len] < MAXSAVEDTUPLES &&
		    Py_Type(op) == &PyTuple_Type)
		{
			op->ob_item[0] = (PyObject *) free_tuples[len];
			num_free_tuples[len]++;
			free_tuples[len] = op;
			goto done; /* return */
		}
#endif
	}
	Py_Type(op)->tp_free((PyObject *)op);
done:
	Py_TRASHCAN_SAFE_END(op)
}

static int
tupleprint(PyTupleObject *op, FILE *fp, int flags)
{
	Py_ssize_t i;
	fprintf(fp, "(");
	for (i = 0; i < Py_Size(op); i++) {
		if (i > 0)
			fprintf(fp, ", ");
		if (PyObject_Print(op->ob_item[i], fp, 0) != 0)
			return -1;
	}
	if (Py_Size(op) == 1)
		fprintf(fp, ",");
	fprintf(fp, ")");
	return 0;
}

static PyObject *
tuplerepr(PyTupleObject *v)
{
	Py_ssize_t i, n;
	PyObject *s, *temp;
	PyObject *pieces, *result = NULL;

	n = Py_Size(v);
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

/* The addend 82520, was selected from the range(0, 1000000) for 
   generating the greatest number of prime multipliers for tuples 
   upto length eight:

     1082527, 1165049, 1082531, 1165057, 1247581, 1330103, 1082533, 
     1330111, 1412633, 1165069, 1247599, 1495177, 1577699
*/

static long
tuplehash(PyTupleObject *v)
{
	register long x, y;
	register Py_ssize_t len = Py_Size(v);
	register PyObject **p;
	long mult = 1000003L;
	x = 0x345678L;
	p = v->ob_item;
	while (--len >= 0) {
		y = PyObject_Hash(*p++);
		if (y == -1)
			return -1;
		x = (x ^ y) * mult;
		/* the cast might truncate len; that doesn't change hash stability */
		mult += (long)(82520L + len + len);
	}
	x += 97531L;
	if (x == -1)
		x = -2;
	return x;
}

static Py_ssize_t
tuplelength(PyTupleObject *a)
{
	return Py_Size(a);
}

static int
tuplecontains(PyTupleObject *a, PyObject *el)
{
	Py_ssize_t i;
	int cmp;

	for (i = 0, cmp = 0 ; cmp == 0 && i < Py_Size(a); ++i)
		cmp = PyObject_RichCompareBool(el, PyTuple_GET_ITEM(a, i),
						   Py_EQ);
	return cmp;
}

static PyObject *
tupleitem(register PyTupleObject *a, register Py_ssize_t i)
{
	if (i < 0 || i >= Py_Size(a)) {
		PyErr_SetString(PyExc_IndexError, "tuple index out of range");
		return NULL;
	}
	Py_INCREF(a->ob_item[i]);
	return a->ob_item[i];
}

static PyObject *
tupleslice(register PyTupleObject *a, register Py_ssize_t ilow, 
	   register Py_ssize_t ihigh)
{
	register PyTupleObject *np;
	PyObject **src, **dest;
	register Py_ssize_t i;
	Py_ssize_t len;
	if (ilow < 0)
		ilow = 0;
	if (ihigh > Py_Size(a))
		ihigh = Py_Size(a);
	if (ihigh < ilow)
		ihigh = ilow;
	if (ilow == 0 && ihigh == Py_Size(a) && PyTuple_CheckExact(a)) {
		Py_INCREF(a);
		return (PyObject *)a;
	}
	len = ihigh - ilow;
	np = (PyTupleObject *)PyTuple_New(len);
	if (np == NULL)
		return NULL;
	src = a->ob_item + ilow;
	dest = np->ob_item;
	for (i = 0; i < len; i++) {
		PyObject *v = src[i];
		Py_INCREF(v);
		dest[i] = v;
	}
	return (PyObject *)np;
}

PyObject *
PyTuple_GetSlice(PyObject *op, Py_ssize_t i, Py_ssize_t j)
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
	register Py_ssize_t size;
	register Py_ssize_t i;
	PyObject **src, **dest;
	PyTupleObject *np;
	if (!PyTuple_Check(bb)) {
		PyErr_Format(PyExc_TypeError,
       		     "can only concatenate tuple (not \"%.200s\") to tuple",
			     Py_Type(bb)->tp_name);
		return NULL;
	}
#define b ((PyTupleObject *)bb)
	size = Py_Size(a) + Py_Size(b);
	if (size < 0)
		return PyErr_NoMemory();
	np = (PyTupleObject *) PyTuple_New(size);
	if (np == NULL) {
		return NULL;
	}
	src = a->ob_item;
	dest = np->ob_item;
	for (i = 0; i < Py_Size(a); i++) {
		PyObject *v = src[i];
		Py_INCREF(v);
		dest[i] = v;
	}
	src = b->ob_item;
	dest = np->ob_item + Py_Size(a);
	for (i = 0; i < Py_Size(b); i++) {
		PyObject *v = src[i];
		Py_INCREF(v);
		dest[i] = v;
	}
	return (PyObject *)np;
#undef b
}

static PyObject *
tuplerepeat(PyTupleObject *a, Py_ssize_t n)
{
	Py_ssize_t i, j;
	Py_ssize_t size;
	PyTupleObject *np;
	PyObject **p, **items;
	if (n < 0)
		n = 0;
	if (Py_Size(a) == 0 || n == 1) {
		if (PyTuple_CheckExact(a)) {
			/* Since tuples are immutable, we can return a shared
			   copy in this case */
			Py_INCREF(a);
			return (PyObject *)a;
		}
		if (Py_Size(a) == 0)
			return PyTuple_New(0);
	}
	size = Py_Size(a) * n;
	if (size/Py_Size(a) != n)
		return PyErr_NoMemory();
	np = (PyTupleObject *) PyTuple_New(size);
	if (np == NULL)
		return NULL;
	p = np->ob_item;
	items = a->ob_item;
	for (i = 0; i < n; i++) {
		for (j = 0; j < Py_Size(a); j++) {
			*p = items[j];
			Py_INCREF(*p);
			p++;
		}
	}
	return (PyObject *) np;
}

static int
tupletraverse(PyTupleObject *o, visitproc visit, void *arg)
{
	Py_ssize_t i;

	for (i = Py_Size(o); --i >= 0; )
		Py_VISIT(o->ob_item[i]);
	return 0;
}

static PyObject *
tuplerichcompare(PyObject *v, PyObject *w, int op)
{
	PyTupleObject *vt, *wt;
	Py_ssize_t i;
	Py_ssize_t vlen, wlen;

	if (!PyTuple_Check(v) || !PyTuple_Check(w)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	vt = (PyTupleObject *)v;
	wt = (PyTupleObject *)w;

	vlen = Py_Size(vt);
	wlen = Py_Size(wt);

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

static PyObject *
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
	PyObject *tmp, *newobj, *item;
	Py_ssize_t i, n;

	assert(PyType_IsSubtype(type, &PyTuple_Type));
	tmp = tuple_new(&PyTuple_Type, args, kwds);
	if (tmp == NULL)
		return NULL;
	assert(PyTuple_Check(tmp));
	newobj = type->tp_alloc(type, n = PyTuple_GET_SIZE(tmp));
	if (newobj == NULL)
		return NULL;
	for (i = 0; i < n; i++) {
		item = PyTuple_GET_ITEM(tmp, i);
		Py_INCREF(item);
		PyTuple_SET_ITEM(newobj, i, item);
	}
	Py_DECREF(tmp);
	return newobj;
}

PyDoc_STRVAR(tuple_doc,
"tuple() -> an empty tuple\n"
"tuple(sequence) -> tuple initialized from sequence's items\n"
"\n"
"If the argument is a tuple, the return value is the same object.");

static PySequenceMethods tuple_as_sequence = {
	(lenfunc)tuplelength,			/* sq_length */
	(binaryfunc)tupleconcat,		/* sq_concat */
	(ssizeargfunc)tuplerepeat,		/* sq_repeat */
	(ssizeargfunc)tupleitem,		/* sq_item */
	(ssizessizeargfunc)tupleslice,		/* sq_slice */
	0,					/* sq_ass_item */
	0,					/* sq_ass_slice */
	(objobjproc)tuplecontains,		/* sq_contains */
};

static PyObject*
tuplesubscript(PyTupleObject* self, PyObject* item)
{
	if (PyIndex_Check(item)) {
		Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
		if (i == -1 && PyErr_Occurred())
			return NULL;
		if (i < 0)
			i += PyTuple_GET_SIZE(self);
		return tupleitem(self, i);
	}
	else if (PySlice_Check(item)) {
		Py_ssize_t start, stop, step, slicelength, cur, i;
		PyObject* result;
		PyObject* it;
		PyObject **src, **dest;

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
			if (!result) return NULL;

			src = self->ob_item;
			dest = ((PyTupleObject *)result)->ob_item;
			for (cur = start, i = 0; i < slicelength; 
			     cur += step, i++) {
				it = src[cur];
				Py_INCREF(it);
				dest[i] = it;
			}
			
			return result;
		}
	}
	else {
		PyErr_Format(PyExc_TypeError, 
			     "tuple indices must be integers, not %.200s",
			     Py_Type(item)->tp_name);
		return NULL;
	}
}

static PyObject *
tuple_getnewargs(PyTupleObject *v)
{
	return Py_BuildValue("(N)", tupleslice(v, 0, Py_Size(v)));
	
}

static PyMethodDef tuple_methods[] = {
	{"__getnewargs__",	(PyCFunction)tuple_getnewargs,	METH_NOARGS},
	{NULL,		NULL}		/* sentinel */
};

static PyMappingMethods tuple_as_mapping = {
	(lenfunc)tuplelength,
	(binaryfunc)tuplesubscript,
	0
};

static PyObject *tuple_iter(PyObject *seq);

PyTypeObject PyTuple_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
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
		Py_TPFLAGS_BASETYPE | Py_TPFLAGS_TUPLE_SUBCLASS, /* tp_flags */
	tuple_doc,				/* tp_doc */
 	(traverseproc)tupletraverse,		/* tp_traverse */
	0,					/* tp_clear */
	tuplerichcompare,			/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	tuple_iter,	    			/* tp_iter */
	0,					/* tp_iternext */
	tuple_methods,				/* tp_methods */
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
_PyTuple_Resize(PyObject **pv, Py_ssize_t newsize)
{
	register PyTupleObject *v;
	register PyTupleObject *sv;
	Py_ssize_t i;
	Py_ssize_t oldsize;

	v = (PyTupleObject *) *pv;
	if (v == NULL || Py_Type(v) != &PyTuple_Type ||
	    (Py_Size(v) != 0 && Py_Refcnt(v) != 1)) {
		*pv = 0;
		Py_XDECREF(v);
		PyErr_BadInternalCall();
		return -1;
	}
	oldsize = Py_Size(v);
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
	_Py_DEC_REFTOTAL;
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

/*********************** Tuple Iterator **************************/

typedef struct {
	PyObject_HEAD
	long it_index;
	PyTupleObject *it_seq; /* Set to NULL when iterator is exhausted */
} tupleiterobject;

static void
tupleiter_dealloc(tupleiterobject *it)
{
	_PyObject_GC_UNTRACK(it);
	Py_XDECREF(it->it_seq);
	PyObject_GC_Del(it);
}

static int
tupleiter_traverse(tupleiterobject *it, visitproc visit, void *arg)
{
	Py_VISIT(it->it_seq);
	return 0;
}

static PyObject *
tupleiter_next(tupleiterobject *it)
{
	PyTupleObject *seq;
	PyObject *item;

	assert(it != NULL);
	seq = it->it_seq;
	if (seq == NULL)
		return NULL;
	assert(PyTuple_Check(seq));

	if (it->it_index < PyTuple_GET_SIZE(seq)) {
		item = PyTuple_GET_ITEM(seq, it->it_index);
		++it->it_index;
		Py_INCREF(item);
		return item;
	}

	Py_DECREF(seq);
	it->it_seq = NULL;
	return NULL;
}

static PyObject *
tupleiter_len(tupleiterobject *it)
{
	Py_ssize_t len = 0;
	if (it->it_seq)
		len = PyTuple_GET_SIZE(it->it_seq) - it->it_index;
	return PyInt_FromSsize_t(len);
}

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

static PyMethodDef tupleiter_methods[] = {
	{"__length_hint__", (PyCFunction)tupleiter_len, METH_NOARGS, length_hint_doc},
 	{NULL,		NULL}		/* sentinel */
};

PyTypeObject PyTupleIter_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"tupleiterator",			/* tp_name */
	sizeof(tupleiterobject),		/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	(destructor)tupleiter_dealloc,		/* tp_dealloc */
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
	(traverseproc)tupleiter_traverse,	/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	PyObject_SelfIter,			/* tp_iter */
	(iternextfunc)tupleiter_next,		/* tp_iternext */
	tupleiter_methods,			/* tp_methods */
	0,
};

static PyObject *
tuple_iter(PyObject *seq)
{
	tupleiterobject *it;

	if (!PyTuple_Check(seq)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	it = PyObject_GC_New(tupleiterobject, &PyTupleIter_Type);
	if (it == NULL)
		return NULL;
	it->it_index = 0;
	Py_INCREF(seq);
	it->it_seq = (PyTupleObject *)seq;
	_PyObject_GC_TRACK(it);
	return (PyObject *)it;
}
