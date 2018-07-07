
/* Tuple object implementation */

#include "Python.h"
#include "internal/pystate.h"
#include "accu.h"

/*[clinic input]
class tuple "PyTupleObject *" "&PyTuple_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=f051ba3cfdf9a189]*/

#include "clinic/tupleobject.c.h"

/* Speed optimization to avoid frequent malloc/free of small tuples */
#ifndef PyTuple_MAXSAVESIZE
#define PyTuple_MAXSAVESIZE     20  /* Largest tuple to save on free list */
#endif
#ifndef PyTuple_MAXFREELIST
#define PyTuple_MAXFREELIST  2000  /* Maximum number of tuples of each size to save */
#endif

#if PyTuple_MAXSAVESIZE > 0
/* Entries 1 up to PyTuple_MAXSAVESIZE are free lists, entry 0 is the empty
   tuple () of which at most one instance will be allocated.
*/
static PyTupleObject *free_list[PyTuple_MAXSAVESIZE];
static int numfree[PyTuple_MAXSAVESIZE];
#endif
#ifdef COUNT_ALLOCS
Py_ssize_t fast_tuple_allocs;
Py_ssize_t tuple_zero_allocs;
#endif

/* Debug statistic to count GC tracking of tuples.
   Please note that tuples are only untracked when considered by the GC, and
   many of them will be dead before. Therefore, a tracking rate close to 100%
   does not necessarily prove that the heuristic is inefficient.
*/
#ifdef SHOW_TRACK_COUNT
static Py_ssize_t count_untracked = 0;
static Py_ssize_t count_tracked = 0;

static void
show_track(void)
{
    PyInterpreterState *interp = PyThreadState_GET()->interp;
    if (!interp->core_config.show_alloc_count) {
        return;
    }

    fprintf(stderr, "Tuples created: %" PY_FORMAT_SIZE_T "d\n",
        count_tracked + count_untracked);
    fprintf(stderr, "Tuples tracked by the GC: %" PY_FORMAT_SIZE_T
        "d\n", count_tracked);
    fprintf(stderr, "%.2f%% tuple tracking rate\n\n",
        (100.0*count_tracked/(count_untracked+count_tracked)));
}
#endif

/* Print summary info about the state of the optimized allocator */
void
_PyTuple_DebugMallocStats(FILE *out)
{
#if PyTuple_MAXSAVESIZE > 0
    int i;
    char buf[128];
    for (i = 1; i < PyTuple_MAXSAVESIZE; i++) {
        PyOS_snprintf(buf, sizeof(buf),
                      "free %d-sized PyTupleObject", i);
        _PyDebugAllocatorStats(out,
                               buf,
                               numfree[i], _PyObject_VAR_SIZE(&PyTuple_Type, i));
    }
#endif
}

PyObject *
PyTuple_New(Py_ssize_t size)
{
    PyTupleObject *op;
    Py_ssize_t i;
    if (size < 0) {
        PyErr_BadInternalCall();
        return NULL;
    }
#if PyTuple_MAXSAVESIZE > 0
    if (size == 0 && free_list[0]) {
        op = free_list[0];
        Py_INCREF(op);
#ifdef COUNT_ALLOCS
        tuple_zero_allocs++;
#endif
        return (PyObject *) op;
    }
    if (size < PyTuple_MAXSAVESIZE && (op = free_list[size]) != NULL) {
        free_list[size] = (PyTupleObject *) op->ob_item[0];
        numfree[size]--;
#ifdef COUNT_ALLOCS
        fast_tuple_allocs++;
#endif
        /* Inline PyObject_InitVar */
#ifdef Py_TRACE_REFS
        Py_SIZE(op) = size;
        Py_TYPE(op) = &PyTuple_Type;
#endif
        _Py_NewReference((PyObject *)op);
    }
    else
#endif
    {
        /* Check for overflow */
        if ((size_t)size > ((size_t)PY_SSIZE_T_MAX - sizeof(PyTupleObject) -
                    sizeof(PyObject *)) / sizeof(PyObject *)) {
            return PyErr_NoMemory();
        }
        op = PyObject_GC_NewVar(PyTupleObject, &PyTuple_Type, size);
        if (op == NULL)
            return NULL;
    }
    for (i=0; i < size; i++)
        op->ob_item[i] = NULL;
#if PyTuple_MAXSAVESIZE > 0
    if (size == 0) {
        free_list[0] = op;
        ++numfree[0];
        Py_INCREF(op);          /* extra INCREF so that this is never freed */
    }
#endif
#ifdef SHOW_TRACK_COUNT
    count_tracked++;
#endif
    _PyObject_GC_TRACK(op);
    return (PyObject *) op;
}

Py_ssize_t
PyTuple_Size(PyObject *op)
{
    if (!PyTuple_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    else
        return Py_SIZE(op);
}

PyObject *
PyTuple_GetItem(PyObject *op, Py_ssize_t i)
{
    if (!PyTuple_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    if (i < 0 || i >= Py_SIZE(op)) {
        PyErr_SetString(PyExc_IndexError, "tuple index out of range");
        return NULL;
    }
    return ((PyTupleObject *)op) -> ob_item[i];
}

int
PyTuple_SetItem(PyObject *op, Py_ssize_t i, PyObject *newitem)
{
    PyObject **p;
    if (!PyTuple_Check(op) || op->ob_refcnt != 1) {
        Py_XDECREF(newitem);
        PyErr_BadInternalCall();
        return -1;
    }
    if (i < 0 || i >= Py_SIZE(op)) {
        Py_XDECREF(newitem);
        PyErr_SetString(PyExc_IndexError,
                        "tuple assignment index out of range");
        return -1;
    }
    p = ((PyTupleObject *)op) -> ob_item + i;
    Py_XSETREF(*p, newitem);
    return 0;
}

void
_PyTuple_MaybeUntrack(PyObject *op)
{
    PyTupleObject *t;
    Py_ssize_t i, n;

    if (!PyTuple_CheckExact(op) || !_PyObject_GC_IS_TRACKED(op))
        return;
    t = (PyTupleObject *) op;
    n = Py_SIZE(t);
    for (i = 0; i < n; i++) {
        PyObject *elt = PyTuple_GET_ITEM(t, i);
        /* Tuple with NULL elements aren't
           fully constructed, don't untrack
           them yet. */
        if (!elt ||
            _PyObject_GC_MAY_BE_TRACKED(elt))
            return;
    }
#ifdef SHOW_TRACK_COUNT
    count_tracked--;
    count_untracked++;
#endif
    _PyObject_GC_UNTRACK(op);
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
    if (result == NULL) {
        va_end(vargs);
        return NULL;
    }
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
tupledealloc(PyTupleObject *op)
{
    Py_ssize_t i;
    Py_ssize_t len =  Py_SIZE(op);
    PyObject_GC_UnTrack(op);
    Py_TRASHCAN_SAFE_BEGIN(op)
    if (len > 0) {
        i = len;
        while (--i >= 0)
            Py_XDECREF(op->ob_item[i]);
#if PyTuple_MAXSAVESIZE > 0
        if (len < PyTuple_MAXSAVESIZE &&
            numfree[len] < PyTuple_MAXFREELIST &&
            Py_TYPE(op) == &PyTuple_Type)
        {
            op->ob_item[0] = (PyObject *) free_list[len];
            numfree[len]++;
            free_list[len] = op;
            goto done; /* return */
        }
#endif
    }
    Py_TYPE(op)->tp_free((PyObject *)op);
done:
    Py_TRASHCAN_SAFE_END(op)
}

static PyObject *
tuplerepr(PyTupleObject *v)
{
    Py_ssize_t i, n;
    _PyUnicodeWriter writer;

    n = Py_SIZE(v);
    if (n == 0)
        return PyUnicode_FromString("()");

    /* While not mutable, it is still possible to end up with a cycle in a
       tuple through an object that stores itself within a tuple (and thus
       infinitely asks for the repr of itself). This should only be
       possible within a type. */
    i = Py_ReprEnter((PyObject *)v);
    if (i != 0) {
        return i > 0 ? PyUnicode_FromString("(...)") : NULL;
    }

    _PyUnicodeWriter_Init(&writer);
    writer.overallocate = 1;
    if (Py_SIZE(v) > 1) {
        /* "(" + "1" + ", 2" * (len - 1) + ")" */
        writer.min_length = 1 + 1 + (2 + 1) * (Py_SIZE(v) - 1) + 1;
    }
    else {
        /* "(1,)" */
        writer.min_length = 4;
    }

    if (_PyUnicodeWriter_WriteChar(&writer, '(') < 0)
        goto error;

    /* Do repr() on each element. */
    for (i = 0; i < n; ++i) {
        PyObject *s;

        if (i > 0) {
            if (_PyUnicodeWriter_WriteASCIIString(&writer, ", ", 2) < 0)
                goto error;
        }

        s = PyObject_Repr(v->ob_item[i]);
        if (s == NULL)
            goto error;

        if (_PyUnicodeWriter_WriteStr(&writer, s) < 0) {
            Py_DECREF(s);
            goto error;
        }
        Py_DECREF(s);
    }

    writer.overallocate = 0;
    if (n > 1) {
        if (_PyUnicodeWriter_WriteChar(&writer, ')') < 0)
            goto error;
    }
    else {
        if (_PyUnicodeWriter_WriteASCIIString(&writer, ",)", 2) < 0)
            goto error;
    }

    Py_ReprLeave((PyObject *)v);
    return _PyUnicodeWriter_Finish(&writer);

error:
    _PyUnicodeWriter_Dealloc(&writer);
    Py_ReprLeave((PyObject *)v);
    return NULL;
}

/* The addend 82520, was selected from the range(0, 1000000) for
   generating the greatest number of prime multipliers for tuples
   up to length eight:

     1082527, 1165049, 1082531, 1165057, 1247581, 1330103, 1082533,
     1330111, 1412633, 1165069, 1247599, 1495177, 1577699

   Tests have shown that it's not worth to cache the hash value, see
   issue #9685.
*/

static Py_hash_t
tuplehash(PyTupleObject *v)
{
    Py_uhash_t x;  /* Unsigned for defined overflow behavior. */
    Py_hash_t y;
    Py_ssize_t len = Py_SIZE(v);
    PyObject **p;
    Py_uhash_t mult = _PyHASH_MULTIPLIER;
    x = 0x345678UL;
    p = v->ob_item;
    while (--len >= 0) {
        y = PyObject_Hash(*p++);
        if (y == -1)
            return -1;
        x = (x ^ y) * mult;
        /* the cast might truncate len; that doesn't change hash stability */
        mult += (Py_hash_t)(82520UL + len + len);
    }
    x += 97531UL;
    if (x == (Py_uhash_t)-1)
        x = -2;
    return x;
}

static Py_ssize_t
tuplelength(PyTupleObject *a)
{
    return Py_SIZE(a);
}

static int
tuplecontains(PyTupleObject *a, PyObject *el)
{
    Py_ssize_t i;
    int cmp;

    for (i = 0, cmp = 0 ; cmp == 0 && i < Py_SIZE(a); ++i)
        cmp = PyObject_RichCompareBool(el, PyTuple_GET_ITEM(a, i),
                                           Py_EQ);
    return cmp;
}

static PyObject *
tupleitem(PyTupleObject *a, Py_ssize_t i)
{
    if (i < 0 || i >= Py_SIZE(a)) {
        PyErr_SetString(PyExc_IndexError, "tuple index out of range");
        return NULL;
    }
    Py_INCREF(a->ob_item[i]);
    return a->ob_item[i];
}

static PyObject *
tupleslice(PyTupleObject *a, Py_ssize_t ilow,
           Py_ssize_t ihigh)
{
    PyTupleObject *np;
    PyObject **src, **dest;
    Py_ssize_t i;
    Py_ssize_t len;
    if (ilow < 0)
        ilow = 0;
    if (ihigh > Py_SIZE(a))
        ihigh = Py_SIZE(a);
    if (ihigh < ilow)
        ihigh = ilow;
    if (ilow == 0 && ihigh == Py_SIZE(a) && PyTuple_CheckExact(a)) {
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
tupleconcat(PyTupleObject *a, PyObject *bb)
{
    Py_ssize_t size;
    Py_ssize_t i;
    PyObject **src, **dest;
    PyTupleObject *np;
    if (Py_SIZE(a) == 0 && PyTuple_CheckExact(bb)) {
        Py_INCREF(bb);
        return bb;
    }
    if (!PyTuple_Check(bb)) {
        PyErr_Format(PyExc_TypeError,
             "can only concatenate tuple (not \"%.200s\") to tuple",
                 Py_TYPE(bb)->tp_name);
        return NULL;
    }
#define b ((PyTupleObject *)bb)
    if (Py_SIZE(b) == 0 && PyTuple_CheckExact(a)) {
        Py_INCREF(a);
        return (PyObject *)a;
    }
    if (Py_SIZE(a) > PY_SSIZE_T_MAX - Py_SIZE(b))
        return PyErr_NoMemory();
    size = Py_SIZE(a) + Py_SIZE(b);
    np = (PyTupleObject *) PyTuple_New(size);
    if (np == NULL) {
        return NULL;
    }
    src = a->ob_item;
    dest = np->ob_item;
    for (i = 0; i < Py_SIZE(a); i++) {
        PyObject *v = src[i];
        Py_INCREF(v);
        dest[i] = v;
    }
    src = b->ob_item;
    dest = np->ob_item + Py_SIZE(a);
    for (i = 0; i < Py_SIZE(b); i++) {
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
    if (Py_SIZE(a) == 0 || n == 1) {
        if (PyTuple_CheckExact(a)) {
            /* Since tuples are immutable, we can return a shared
               copy in this case */
            Py_INCREF(a);
            return (PyObject *)a;
        }
        if (Py_SIZE(a) == 0)
            return PyTuple_New(0);
    }
    if (n > PY_SSIZE_T_MAX / Py_SIZE(a))
        return PyErr_NoMemory();
    size = Py_SIZE(a) * n;
    np = (PyTupleObject *) PyTuple_New(size);
    if (np == NULL)
        return NULL;
    p = np->ob_item;
    items = a->ob_item;
    for (i = 0; i < n; i++) {
        for (j = 0; j < Py_SIZE(a); j++) {
            *p = items[j];
            Py_INCREF(*p);
            p++;
        }
    }
    return (PyObject *) np;
}

/*[clinic input]
tuple.index

    value: object
    start: slice_index(accept={int}) = 0
    stop: slice_index(accept={int}, c_default="PY_SSIZE_T_MAX") = sys.maxsize
    /

Return first index of value.

Raises ValueError if the value is not present.
[clinic start generated code]*/

static PyObject *
tuple_index_impl(PyTupleObject *self, PyObject *value, Py_ssize_t start,
                 Py_ssize_t stop)
/*[clinic end generated code: output=07b6f9f3cb5c33eb input=fb39e9874a21fe3f]*/
{
    Py_ssize_t i;

    if (start < 0) {
        start += Py_SIZE(self);
        if (start < 0)
            start = 0;
    }
    if (stop < 0) {
        stop += Py_SIZE(self);
    }
    else if (stop > Py_SIZE(self)) {
        stop = Py_SIZE(self);
    }
    for (i = start; i < stop; i++) {
        int cmp = PyObject_RichCompareBool(self->ob_item[i], value, Py_EQ);
        if (cmp > 0)
            return PyLong_FromSsize_t(i);
        else if (cmp < 0)
            return NULL;
    }
    PyErr_SetString(PyExc_ValueError, "tuple.index(x): x not in tuple");
    return NULL;
}

/*[clinic input]
tuple.count

     value: object
     /

Return number of occurrences of value.
[clinic start generated code]*/

static PyObject *
tuple_count(PyTupleObject *self, PyObject *value)
/*[clinic end generated code: output=aa927affc5a97605 input=531721aff65bd772]*/
{
    Py_ssize_t count = 0;
    Py_ssize_t i;

    for (i = 0; i < Py_SIZE(self); i++) {
        int cmp = PyObject_RichCompareBool(self->ob_item[i], value, Py_EQ);
        if (cmp > 0)
            count++;
        else if (cmp < 0)
            return NULL;
    }
    return PyLong_FromSsize_t(count);
}

static int
tupletraverse(PyTupleObject *o, visitproc visit, void *arg)
{
    Py_ssize_t i;

    for (i = Py_SIZE(o); --i >= 0; )
        Py_VISIT(o->ob_item[i]);
    return 0;
}

static PyObject *
tuplerichcompare(PyObject *v, PyObject *w, int op)
{
    PyTupleObject *vt, *wt;
    Py_ssize_t i;
    Py_ssize_t vlen, wlen;

    if (!PyTuple_Check(v) || !PyTuple_Check(w))
        Py_RETURN_NOTIMPLEMENTED;

    vt = (PyTupleObject *)v;
    wt = (PyTupleObject *)w;

    vlen = Py_SIZE(vt);
    wlen = Py_SIZE(wt);

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
        Py_RETURN_RICHCOMPARE(vlen, wlen, op);
    }

    /* We have an item that differs -- shortcuts for EQ/NE */
    if (op == Py_EQ) {
        Py_RETURN_FALSE;
    }
    if (op == Py_NE) {
        Py_RETURN_TRUE;
    }

    /* Compare the final item again using the proper operator */
    return PyObject_RichCompare(vt->ob_item[i], wt->ob_item[i], op);
}

static PyObject *
tuple_subtype_new(PyTypeObject *type, PyObject *iterable);

/*[clinic input]
@classmethod
tuple.__new__ as tuple_new
    iterable: object(c_default="NULL") = ()
    /

Built-in immutable sequence.

If no argument is given, the constructor returns an empty tuple.
If iterable is specified the tuple is initialized from iterable's items.

If the argument is a tuple, the return value is the same object.
[clinic start generated code]*/

static PyObject *
tuple_new_impl(PyTypeObject *type, PyObject *iterable)
/*[clinic end generated code: output=4546d9f0d469bce7 input=86963bcde633b5a2]*/
{
    if (type != &PyTuple_Type)
        return tuple_subtype_new(type, iterable);

    if (iterable == NULL)
        return PyTuple_New(0);
    else
        return PySequence_Tuple(iterable);
}

static PyObject *
tuple_subtype_new(PyTypeObject *type, PyObject *iterable)
{
    PyObject *tmp, *newobj, *item;
    Py_ssize_t i, n;

    assert(PyType_IsSubtype(type, &PyTuple_Type));
    tmp = tuple_new_impl(&PyTuple_Type, iterable);
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

static PySequenceMethods tuple_as_sequence = {
    (lenfunc)tuplelength,                       /* sq_length */
    (binaryfunc)tupleconcat,                    /* sq_concat */
    (ssizeargfunc)tuplerepeat,                  /* sq_repeat */
    (ssizeargfunc)tupleitem,                    /* sq_item */
    0,                                          /* sq_slice */
    0,                                          /* sq_ass_item */
    0,                                          /* sq_ass_slice */
    (objobjproc)tuplecontains,                  /* sq_contains */
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

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return NULL;
        }
        slicelength = PySlice_AdjustIndices(PyTuple_GET_SIZE(self), &start,
                                            &stop, step);

        if (slicelength <= 0) {
            return PyTuple_New(0);
        }
        else if (start == 0 && step == 1 &&
                 slicelength == PyTuple_GET_SIZE(self) &&
                 PyTuple_CheckExact(self)) {
            Py_INCREF(self);
            return (PyObject *)self;
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
                     "tuple indices must be integers or slices, not %.200s",
                     Py_TYPE(item)->tp_name);
        return NULL;
    }
}

/*[clinic input]
tuple.__getnewargs__
[clinic start generated code]*/

static PyObject *
tuple___getnewargs___impl(PyTupleObject *self)
/*[clinic end generated code: output=25e06e3ee56027e2 input=1aeb4b286a21639a]*/
{
    return Py_BuildValue("(N)", tupleslice(self, 0, Py_SIZE(self)));
}

static PyMethodDef tuple_methods[] = {
    TUPLE___GETNEWARGS___METHODDEF
    TUPLE_INDEX_METHODDEF
    TUPLE_COUNT_METHODDEF
    {NULL,              NULL}           /* sentinel */
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
    (destructor)tupledealloc,                   /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)tuplerepr,                        /* tp_repr */
    0,                                          /* tp_as_number */
    &tuple_as_sequence,                         /* tp_as_sequence */
    &tuple_as_mapping,                          /* tp_as_mapping */
    (hashfunc)tuplehash,                        /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE | Py_TPFLAGS_TUPLE_SUBCLASS, /* tp_flags */
    tuple_new__doc__,                           /* tp_doc */
    (traverseproc)tupletraverse,                /* tp_traverse */
    0,                                          /* tp_clear */
    tuplerichcompare,                           /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    tuple_iter,                                 /* tp_iter */
    0,                                          /* tp_iternext */
    tuple_methods,                              /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    tuple_new,                                  /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
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
    PyTupleObject *v;
    PyTupleObject *sv;
    Py_ssize_t i;
    Py_ssize_t oldsize;

    v = (PyTupleObject *) *pv;
    if (v == NULL || Py_TYPE(v) != &PyTuple_Type ||
        (Py_SIZE(v) != 0 && Py_REFCNT(v) != 1)) {
        *pv = 0;
        Py_XDECREF(v);
        PyErr_BadInternalCall();
        return -1;
    }
    oldsize = Py_SIZE(v);
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
    if (_PyObject_GC_IS_TRACKED(v))
        _PyObject_GC_UNTRACK(v);
    _Py_ForgetReference((PyObject *) v);
    /* DECREF items deleted by shrinkage */
    for (i = newsize; i < oldsize; i++) {
        Py_CLEAR(v->ob_item[i]);
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

int
PyTuple_ClearFreeList(void)
{
    int freelist_size = 0;
#if PyTuple_MAXSAVESIZE > 0
    int i;
    for (i = 1; i < PyTuple_MAXSAVESIZE; i++) {
        PyTupleObject *p, *q;
        p = free_list[i];
        freelist_size += numfree[i];
        free_list[i] = NULL;
        numfree[i] = 0;
        while (p) {
            q = p;
            p = (PyTupleObject *)(p->ob_item[0]);
            PyObject_GC_Del(q);
        }
    }
#endif
    return freelist_size;
}

void
PyTuple_Fini(void)
{
#if PyTuple_MAXSAVESIZE > 0
    /* empty tuples are used all over the place and applications may
     * rely on the fact that an empty tuple is a singleton. */
    Py_CLEAR(free_list[0]);

    (void)PyTuple_ClearFreeList();
#endif
#ifdef SHOW_TRACK_COUNT
    show_track();
#endif
}

/*********************** Tuple Iterator **************************/

typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;
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

    it->it_seq = NULL;
    Py_DECREF(seq);
    return NULL;
}

static PyObject *
tupleiter_len(tupleiterobject *it, PyObject *Py_UNUSED(ignored))
{
    Py_ssize_t len = 0;
    if (it->it_seq)
        len = PyTuple_GET_SIZE(it->it_seq) - it->it_index;
    return PyLong_FromSsize_t(len);
}

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

static PyObject *
tupleiter_reduce(tupleiterobject *it, PyObject *Py_UNUSED(ignored))
{
    if (it->it_seq)
        return Py_BuildValue("N(O)n", _PyObject_GetBuiltin("iter"),
                             it->it_seq, it->it_index);
    else
        return Py_BuildValue("N(())", _PyObject_GetBuiltin("iter"));
}

static PyObject *
tupleiter_setstate(tupleiterobject *it, PyObject *state)
{
    Py_ssize_t index = PyLong_AsSsize_t(state);
    if (index == -1 && PyErr_Occurred())
        return NULL;
    if (it->it_seq != NULL) {
        if (index < 0)
            index = 0;
        else if (index > PyTuple_GET_SIZE(it->it_seq))
            index = PyTuple_GET_SIZE(it->it_seq); /* exhausted iterator */
        it->it_index = index;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");
PyDoc_STRVAR(setstate_doc, "Set state information for unpickling.");

static PyMethodDef tupleiter_methods[] = {
    {"__length_hint__", (PyCFunction)tupleiter_len, METH_NOARGS, length_hint_doc},
    {"__reduce__", (PyCFunction)tupleiter_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", (PyCFunction)tupleiter_setstate, METH_O, setstate_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyTupleIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "tuple_iterator",                           /* tp_name */
    sizeof(tupleiterobject),                    /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)tupleiter_dealloc,              /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)tupleiter_traverse,           /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)tupleiter_next,               /* tp_iternext */
    tupleiter_methods,                          /* tp_methods */
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
