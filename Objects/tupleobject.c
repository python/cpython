
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
	register int i;
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
		    (nbytes += sizeof(PyTupleObject) - sizeof(PyObject *)
		     		+ PyGC_HEAD_SIZE)
		    <= 0)
		{
			return PyErr_NoMemory();
		}
		/* PyObject_NewVar is inlined */
		op = (PyTupleObject *) PyObject_MALLOC(nbytes);
		if (op == NULL)
			return PyErr_NoMemory();
		op = (PyTupleObject *) PyObject_FROM_GC(op);
		PyObject_INIT_VAR(op, &PyTuple_Type, size);
	}
	for (i = 0; i < size; i++)
		op->ob_item[i] = NULL;
#if MAXSAVESIZE > 0
	if (size == 0) {
		free_tuples[0] = op;
		++num_free_tuples[0];
		Py_INCREF(op);	/* extra INCREF so that this is never freed */
	}
#endif
	PyObject_GC_Init(op);
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
	Py_TRASHCAN_SAFE_BEGIN(op)
	PyObject_GC_Fini(op);
	if (len > 0) {
		i = len;
		while (--i >= 0)
			Py_XDECREF(op->ob_item[i]);
#if MAXSAVESIZE > 0
		if (len < MAXSAVESIZE && num_free_tuples[len] < MAXSAVEDTUPLES) {
			op->ob_item[0] = (PyObject *) free_tuples[len];
			num_free_tuples[len]++;
			free_tuples[len] = op;
			goto done; /* return */
		}
#endif
	}
	op = (PyTupleObject *) PyObject_AS_GC(op);
	PyObject_DEL(op);
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
	PyObject *s, *comma;
	int i;
	s = PyString_FromString("(");
	comma = PyString_FromString(", ");
	for (i = 0; i < v->ob_size && s != NULL; i++) {
		if (i > 0)
			PyString_Concat(&s, comma);
		PyString_ConcatAndDel(&s, PyObject_Repr(v->ob_item[i]));
	}
	Py_DECREF(comma);
	if (v->ob_size == 1)
		PyString_ConcatAndDel(&s, PyString_FromString(","));
	PyString_ConcatAndDel(&s, PyString_FromString(")"));
	return s;
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
	if (ilow == 0 && ihigh == a->ob_size) {
		/* XXX can only do this if tuples are immutable! */
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
		/* Since tuples are immutable, we can return a shared
		   copy in this case */
		Py_INCREF(a);
		return (PyObject *)a;
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

	if (!PyTuple_Check(v) || !PyTuple_Check(w)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	vt = (PyTupleObject *)v;
	wt = (PyTupleObject *)w;

	if (vt->ob_size != wt->ob_size && (op == Py_EQ || op == Py_NE)) {
		/* Shortcut: if the lengths differ, the tuples differ */
		PyObject *res;
		if (op == Py_EQ)
			res = Py_False;
		else
			res = Py_True;
		Py_INCREF(res);
		return res;
	}

	/* Search for the first index where items are different */
	for (i = 0; i < vt->ob_size && i < wt->ob_size; i++) {
		int k = PyObject_RichCompareBool(vt->ob_item[i],
						 wt->ob_item[i], Py_EQ);
		if (k < 0)
			return NULL;
		if (!k)
			break;
	}

	if (i >= vt->ob_size || i >= wt->ob_size) {
		/* No more items to compare -- compare sizes */
		int vs = vt->ob_size;
		int ws = wt->ob_size;
		int cmp;
		PyObject *res;
		switch (op) {
		case Py_LT: cmp = vs <  ws; break;
		case Py_LE: cmp = ws <= ws; break;
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
	return PyObject_RichCompare(vt->ob_item[i], wt->ob_item[i], op);
}

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

PyTypeObject PyTuple_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"tuple",
	sizeof(PyTupleObject) - sizeof(PyObject *) + PyGC_HEAD_SIZE,
	sizeof(PyObject *),
	(destructor)tupledealloc,		/* tp_dealloc */
	(printfunc)tupleprint,			/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)tuplerepr,			/* tp_repr */
	0,					/* tp_as_number */
	&tuple_as_sequence,			/* tp_as_sequence */
	0,					/* tp_as_mapping */
	(hashfunc)tuplehash,			/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_GC,	/* tp_flags */
	0,             				/* tp_doc */
 	(traverseproc)tupletraverse,		/* tp_traverse */
	0,					/* tp_clear */
	tuplerichcompare,			/* tp_richcompare */
};

/* The following function breaks the notion that tuples are immutable:
   it changes the size of a tuple.  We get away with this only if there
   is only one module referencing the object.  You can also think of it
   as creating a new tuple object and destroying the old one, only more
   efficiently.  In any case, don't use this if the tuple may already be
   known to some other part of the code.  The last_is_sticky is not used
   and must always be false. */

int
_PyTuple_Resize(PyObject **pv, int newsize, int last_is_sticky)
{
	register PyTupleObject *v;
	register PyTupleObject *sv;
	int i;
	int sizediff;

	v = (PyTupleObject *) *pv;
	if (v == NULL || !PyTuple_Check(v) || last_is_sticky ||
	    (v->ob_size != 0 && v->ob_refcnt != 1)) {
		*pv = 0;
		Py_XDECREF(v);
		PyErr_BadInternalCall();
		return -1;
	}
	sizediff = newsize - v->ob_size;
	if (sizediff == 0)
		return 0;

	if (v->ob_size == 0) {
		/* Empty tuples are often shared, so we should never 
		   resize them in-place even if we do own the only
		   (current) reference */
		Py_DECREF(v);
		*pv = PyTuple_New(newsize);
		return 0;
	}

	/* XXX UNREF/NEWREF interface should be more symmetrical */
#ifdef Py_REF_DEBUG
	--_Py_RefTotal;
#endif
	_Py_ForgetReference((PyObject *) v);
	for (i = newsize; i < v->ob_size; i++) {
		Py_XDECREF(v->ob_item[i]);
		v->ob_item[i] = NULL;
	}
	PyObject_GC_Fini(v);
	v = (PyTupleObject *) PyObject_AS_GC(v);
	sv = (PyTupleObject *) PyObject_REALLOC((char *)v,
						sizeof(PyTupleObject)
						+ PyGC_HEAD_SIZE
						+ newsize * sizeof(PyObject *));
	if (sv == NULL) {
		*pv = NULL;
		PyObject_DEL(v);
		PyErr_NoMemory();
		return -1;
	}
	sv = (PyTupleObject *) PyObject_FROM_GC(sv);
	_Py_NewReference((PyObject *) sv);
	for (i = sv->ob_size; i < newsize; i++)
		sv->ob_item[i] = NULL;
	sv->ob_size = newsize;
	*pv = (PyObject *) sv;
	PyObject_GC_Init(sv);
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
			q = (PyTupleObject *) PyObject_AS_GC(q);
			PyObject_DEL(q);
		}
	}
#endif
}
