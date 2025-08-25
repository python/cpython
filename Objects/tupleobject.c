/* Tuple object implementation */

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_ceval.h"         // _PyEval_GetBuiltin()
#include "pycore_freelist.h"      // _Py_FREELIST_PUSH()
#include "pycore_gc.h"            // _PyObject_GC_IS_TRACKED()
#include "pycore_list.h"          // _Py_memory_repeat()
#include "pycore_modsupport.h"    // _PyArg_NoKwnames()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_stackref.h"      // PyStackRef_AsPyObjectSteal()
#include "pycore_tuple.h"         // _PyTupleIterObject


/*[clinic input]
class tuple "PyTupleObject *" "&PyTuple_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=f051ba3cfdf9a189]*/

#include "clinic/tupleobject.c.h"


static inline int maybe_freelist_push(PyTupleObject *);


/* Allocate an uninitialized tuple object. Before making it public, following
   steps must be done:

   - Initialize its items.
   - Call _PyObject_GC_TRACK() on it.

   Because the empty tuple is always reused and it's already tracked by GC,
   this function must not be called with size == 0 (unless from PyTuple_New()
   which wraps this function).
*/
static PyTupleObject *
tuple_alloc(Py_ssize_t size)
{
    if (size < 0) {
        PyErr_BadInternalCall();
        return NULL;
    }
    assert(size != 0);    // The empty tuple is statically allocated.
    Py_ssize_t index = size - 1;
    if (index < PyTuple_MAXSAVESIZE) {
        PyTupleObject *op = _Py_FREELIST_POP(PyTupleObject, tuples[index]);
        if (op != NULL) {
            _PyTuple_RESET_HASH_CACHE(op);
            return op;
        }
    }
    /* Check for overflow */
    if ((size_t)size > ((size_t)PY_SSIZE_T_MAX - (sizeof(PyTupleObject) -
                sizeof(PyObject *))) / sizeof(PyObject *)) {
        return (PyTupleObject *)PyErr_NoMemory();
    }
    PyTupleObject *result = PyObject_GC_NewVar(PyTupleObject, &PyTuple_Type, size);
    if (result != NULL) {
        _PyTuple_RESET_HASH_CACHE(result);
    }
    return result;
}

// The empty tuple singleton is not tracked by the GC.
// It does not contain any Python object.
// Note that tuple subclasses have their own empty instances.

static inline PyObject *
tuple_get_empty(void)
{
    return (PyObject *)&_Py_SINGLETON(tuple_empty);
}

PyObject *
PyTuple_New(Py_ssize_t size)
{
    PyTupleObject *op;
    if (size == 0) {
        return tuple_get_empty();
    }
    op = tuple_alloc(size);
    if (op == NULL) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < size; i++) {
        op->ob_item[i] = NULL;
    }
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
    if (!PyTuple_Check(op) || Py_REFCNT(op) != 1) {
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
    _PyObject_GC_UNTRACK(op);
}

PyObject *
PyTuple_Pack(Py_ssize_t n, ...)
{
    Py_ssize_t i;
    PyObject *o;
    PyObject **items;
    va_list vargs;

    if (n == 0) {
        return tuple_get_empty();
    }

    va_start(vargs, n);
    PyTupleObject *result = tuple_alloc(n);
    if (result == NULL) {
        va_end(vargs);
        return NULL;
    }
    items = result->ob_item;
    for (i = 0; i < n; i++) {
        o = va_arg(vargs, PyObject *);
        items[i] = Py_NewRef(o);
    }
    va_end(vargs);
    _PyObject_GC_TRACK(result);
    return (PyObject *)result;
}


/* Methods */

static void
tuple_dealloc(PyObject *self)
{
    PyTupleObject *op = _PyTuple_CAST(self);
    if (Py_SIZE(op) == 0) {
        /* The empty tuple is statically allocated. */
        if (op == &_Py_SINGLETON(tuple_empty)) {
#ifdef Py_DEBUG
            _Py_FatalRefcountError("deallocating the empty tuple singleton");
#else
            return;
#endif
        }
#ifdef Py_DEBUG
        /* tuple subclasses have their own empty instances. */
        assert(!PyTuple_CheckExact(op));
#endif
    }

    PyObject_GC_UnTrack(op);

    Py_ssize_t i = Py_SIZE(op);
    while (--i >= 0) {
        Py_XDECREF(op->ob_item[i]);
    }
    // This will abort on the empty singleton (if there is one).
    if (!maybe_freelist_push(op)) {
        Py_TYPE(op)->tp_free((PyObject *)op);
    }
}

static PyObject *
tuple_repr(PyObject *self)
{
    PyTupleObject *v = _PyTuple_CAST(self);
    Py_ssize_t n = PyTuple_GET_SIZE(v);
    if (n == 0) {
        return PyUnicode_FromString("()");
    }

    /* While not mutable, it is still possible to end up with a cycle in a
       tuple through an object that stores itself within a tuple (and thus
       infinitely asks for the repr of itself). This should only be
       possible within a type. */
    int res = Py_ReprEnter((PyObject *)v);
    if (res != 0) {
        return res > 0 ? PyUnicode_FromString("(...)") : NULL;
    }

    Py_ssize_t prealloc;
    if (n > 1) {
        // "(" + "1" + ", 2" * (len - 1) + ")"
        prealloc = 1 + 1 + (2 + 1) * (n - 1) + 1;
    }
    else {
        // "(1,)"
        prealloc = 4;
    }
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(prealloc);
    if (writer == NULL) {
        goto error;
    }

    if (PyUnicodeWriter_WriteChar(writer, '(') < 0) {
        goto error;
    }

    /* Do repr() on each element. */
    for (Py_ssize_t i = 0; i < n; ++i) {
        if (i > 0) {
            if (PyUnicodeWriter_WriteChar(writer, ',') < 0) {
                goto error;
            }
            if (PyUnicodeWriter_WriteChar(writer, ' ') < 0) {
                goto error;
            }
        }

        if (PyUnicodeWriter_WriteRepr(writer, v->ob_item[i]) < 0) {
            goto error;
        }
    }

    if (n == 1) {
        if (PyUnicodeWriter_WriteChar(writer, ',') < 0) {
            goto error;
        }
    }
    if (PyUnicodeWriter_WriteChar(writer, ')') < 0) {
        goto error;
    }

    Py_ReprLeave((PyObject *)v);
    return PyUnicodeWriter_Finish(writer);

error:
    PyUnicodeWriter_Discard(writer);
    Py_ReprLeave((PyObject *)v);
    return NULL;
}


/* Hash for tuples. This is a slightly simplified version of the xxHash
   non-cryptographic hash:
   - we do not use any parallelism, there is only 1 accumulator.
   - we drop the final mixing since this is just a permutation of the
     output space: it does not help against collisions.
   - at the end, we mangle the length with a single constant.
   For the xxHash specification, see
   https://github.com/Cyan4973/xxHash/blob/master/doc/xxhash_spec.md

   The constants for the hash function are defined in pycore_tuple.h.
*/

static Py_hash_t
tuple_hash(PyObject *op)
{
    PyTupleObject *v = _PyTuple_CAST(op);

    Py_uhash_t acc = FT_ATOMIC_LOAD_SSIZE_RELAXED(v->ob_hash);
    if (acc != (Py_uhash_t)-1) {
        return acc;
    }

    Py_ssize_t len = Py_SIZE(v);
    PyObject **item = v->ob_item;
    acc = _PyTuple_HASH_XXPRIME_5;
    for (Py_ssize_t i = 0; i < len; i++) {
        Py_uhash_t lane = PyObject_Hash(item[i]);
        if (lane == (Py_uhash_t)-1) {
            return -1;
        }
        acc += lane * _PyTuple_HASH_XXPRIME_2;
        acc = _PyTuple_HASH_XXROTATE(acc);
        acc *= _PyTuple_HASH_XXPRIME_1;
    }

    /* Add input length, mangled to keep the historical value of hash(()). */
    acc += len ^ (_PyTuple_HASH_XXPRIME_5 ^ 3527539UL);

    if (acc == (Py_uhash_t)-1) {
        acc = 1546275796;
    }

    FT_ATOMIC_STORE_SSIZE_RELAXED(v->ob_hash, acc);

    return acc;
}

static Py_ssize_t
tuple_length(PyObject *self)
{
    PyTupleObject *a = _PyTuple_CAST(self);
    return Py_SIZE(a);
}

static int
tuple_contains(PyObject *self, PyObject *el)
{
    PyTupleObject *a = _PyTuple_CAST(self);
    int cmp = 0;
    for (Py_ssize_t i = 0; cmp == 0 && i < Py_SIZE(a); ++i) {
        cmp = PyObject_RichCompareBool(PyTuple_GET_ITEM(a, i), el, Py_EQ);
    }
    return cmp;
}

static PyObject *
tuple_item(PyObject *op, Py_ssize_t i)
{
    PyTupleObject *a = _PyTuple_CAST(op);
    if (i < 0 || i >= Py_SIZE(a)) {
        PyErr_SetString(PyExc_IndexError, "tuple index out of range");
        return NULL;
    }
    return Py_NewRef(a->ob_item[i]);
}

PyObject *
_PyTuple_FromArray(PyObject *const *src, Py_ssize_t n)
{
    if (n == 0) {
        return tuple_get_empty();
    }

    PyTupleObject *tuple = tuple_alloc(n);
    if (tuple == NULL) {
        return NULL;
    }
    PyObject **dst = tuple->ob_item;
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *item = src[i];
        dst[i] = Py_NewRef(item);
    }
    _PyObject_GC_TRACK(tuple);
    return (PyObject *)tuple;
}

PyObject *
_PyTuple_FromStackRefStealOnSuccess(const _PyStackRef *src, Py_ssize_t n)
{
    if (n == 0) {
        return tuple_get_empty();
    }
    PyTupleObject *tuple = tuple_alloc(n);
    if (tuple == NULL) {
        return NULL;
    }
    PyObject **dst = tuple->ob_item;
    for (Py_ssize_t i = 0; i < n; i++) {
        dst[i] = PyStackRef_AsPyObjectSteal(src[i]);
    }
    _PyObject_GC_TRACK(tuple);
    return (PyObject *)tuple;
}

PyObject *
_PyTuple_FromArraySteal(PyObject *const *src, Py_ssize_t n)
{
    if (n == 0) {
        return tuple_get_empty();
    }
    PyTupleObject *tuple = tuple_alloc(n);
    if (tuple == NULL) {
        for (Py_ssize_t i = 0; i < n; i++) {
            Py_DECREF(src[i]);
        }
        return NULL;
    }
    PyObject **dst = tuple->ob_item;
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *item = src[i];
        dst[i] = item;
    }
    _PyObject_GC_TRACK(tuple);
    return (PyObject *)tuple;
}

static PyObject *
tuple_slice(PyTupleObject *a, Py_ssize_t ilow,
           Py_ssize_t ihigh)
{
    if (ilow < 0)
        ilow = 0;
    if (ihigh > Py_SIZE(a))
        ihigh = Py_SIZE(a);
    if (ihigh < ilow)
        ihigh = ilow;
    if (ilow == 0 && ihigh == Py_SIZE(a) && PyTuple_CheckExact(a)) {
        return Py_NewRef(a);
    }
    return _PyTuple_FromArray(a->ob_item + ilow, ihigh - ilow);
}

PyObject *
PyTuple_GetSlice(PyObject *op, Py_ssize_t i, Py_ssize_t j)
{
    if (op == NULL || !PyTuple_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return tuple_slice((PyTupleObject *)op, i, j);
}

static PyObject *
tuple_concat(PyObject *aa, PyObject *bb)
{
    PyTupleObject *a = _PyTuple_CAST(aa);
    if (Py_SIZE(a) == 0 && PyTuple_CheckExact(bb)) {
        return Py_NewRef(bb);
    }
    if (!PyTuple_Check(bb)) {
        PyErr_Format(PyExc_TypeError,
             "can only concatenate tuple (not \"%.200s\") to tuple",
                 Py_TYPE(bb)->tp_name);
        return NULL;
    }
    PyTupleObject *b = (PyTupleObject *)bb;

    if (Py_SIZE(b) == 0 && PyTuple_CheckExact(a)) {
        return Py_NewRef(a);
    }
    assert((size_t)Py_SIZE(a) + (size_t)Py_SIZE(b) < PY_SSIZE_T_MAX);
    Py_ssize_t size = Py_SIZE(a) + Py_SIZE(b);
    if (size == 0) {
        return tuple_get_empty();
    }

    PyTupleObject *np = tuple_alloc(size);
    if (np == NULL) {
        return NULL;
    }

    PyObject **src = a->ob_item;
    PyObject **dest = np->ob_item;
    for (Py_ssize_t i = 0; i < Py_SIZE(a); i++) {
        PyObject *v = src[i];
        dest[i] = Py_NewRef(v);
    }

    src = b->ob_item;
    dest = np->ob_item + Py_SIZE(a);
    for (Py_ssize_t i = 0; i < Py_SIZE(b); i++) {
        PyObject *v = src[i];
        dest[i] = Py_NewRef(v);
    }

    _PyObject_GC_TRACK(np);
    return (PyObject *)np;
}

static PyObject *
tuple_repeat(PyObject *self, Py_ssize_t n)
{
    PyTupleObject *a = _PyTuple_CAST(self);
    const Py_ssize_t input_size = Py_SIZE(a);
    if (input_size == 0 || n == 1) {
        if (PyTuple_CheckExact(a)) {
            /* Since tuples are immutable, we can return a shared
               copy in this case */
            return Py_NewRef(a);
        }
    }
    if (input_size == 0 || n <= 0) {
        return tuple_get_empty();
    }
    assert(n>0);

    if (input_size > PY_SSIZE_T_MAX / n)
        return PyErr_NoMemory();
    Py_ssize_t output_size = input_size * n;

    PyTupleObject *np = tuple_alloc(output_size);
    if (np == NULL)
        return NULL;

    PyObject **dest = np->ob_item;
    if (input_size == 1) {
        PyObject *elem = a->ob_item[0];
        _Py_RefcntAdd(elem, n);
        PyObject **dest_end = dest + output_size;
        while (dest < dest_end) {
            *dest++ = elem;
        }
    }
    else {
        PyObject **src = a->ob_item;
        PyObject **src_end = src + input_size;
        while (src < src_end) {
            _Py_RefcntAdd(*src, n);
            *dest++ = *src++;
        }

        _Py_memory_repeat((char *)np->ob_item, sizeof(PyObject *)*output_size,
                          sizeof(PyObject *)*input_size);
    }
    _PyObject_GC_TRACK(np);
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
tuple_count_impl(PyTupleObject *self, PyObject *value)
/*[clinic end generated code: output=cf02888d4bc15d7a input=531721aff65bd772]*/
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
tuple_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyTupleObject *o = _PyTuple_CAST(self);
    for (Py_ssize_t i = Py_SIZE(o); --i >= 0; ) {
        Py_VISIT(o->ob_item[i]);
    }
    return 0;
}

static PyObject *
tuple_richcompare(PyObject *v, PyObject *w, int op)
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

    if (iterable == NULL) {
        return tuple_get_empty();
    }
    else {
        return PySequence_Tuple(iterable);
    }
}

static PyObject *
tuple_vectorcall(PyObject *type, PyObject * const*args,
                 size_t nargsf, PyObject *kwnames)
{
    if (!_PyArg_NoKwnames("tuple", kwnames)) {
        return NULL;
    }

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (!_PyArg_CheckPositional("tuple", nargs, 0, 1)) {
        return NULL;
    }

    if (nargs) {
        return tuple_new_impl(_PyType_CAST(type), args[0]);
    }
    else {
        return tuple_get_empty();
    }
}

static PyObject *
tuple_subtype_new(PyTypeObject *type, PyObject *iterable)
{
    PyObject *tmp, *newobj, *item;
    Py_ssize_t i, n;

    assert(PyType_IsSubtype(type, &PyTuple_Type));
    // tuple subclasses must implement the GC protocol
    assert(_PyType_IS_GC(type));

    tmp = tuple_new_impl(&PyTuple_Type, iterable);
    if (tmp == NULL)
        return NULL;
    assert(PyTuple_Check(tmp));
    /* This may allocate an empty tuple that is not the global one. */
    newobj = type->tp_alloc(type, n = PyTuple_GET_SIZE(tmp));
    if (newobj == NULL) {
        Py_DECREF(tmp);
        return NULL;
    }
    for (i = 0; i < n; i++) {
        item = PyTuple_GET_ITEM(tmp, i);
        PyTuple_SET_ITEM(newobj, i, Py_NewRef(item));
    }
    Py_DECREF(tmp);

    _PyTuple_RESET_HASH_CACHE(newobj);

    // Don't track if a subclass tp_alloc is PyType_GenericAlloc()
    if (!_PyObject_GC_IS_TRACKED(newobj)) {
        _PyObject_GC_TRACK(newobj);
    }
    return newobj;
}

static PySequenceMethods tuple_as_sequence = {
    tuple_length,                               /* sq_length */
    tuple_concat,                               /* sq_concat */
    tuple_repeat,                               /* sq_repeat */
    tuple_item,                                 /* sq_item */
    0,                                          /* sq_slice */
    0,                                          /* sq_ass_item */
    0,                                          /* sq_ass_slice */
    tuple_contains,                             /* sq_contains */
};

static PyObject*
tuple_subscript(PyObject *op, PyObject* item)
{
    PyTupleObject *self = _PyTuple_CAST(op);
    if (_PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return NULL;
        if (i < 0)
            i += PyTuple_GET_SIZE(self);
        return tuple_item(op, i);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelength, i;
        size_t cur;
        PyObject* it;
        PyObject **src, **dest;

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return NULL;
        }
        slicelength = PySlice_AdjustIndices(PyTuple_GET_SIZE(self), &start,
                                            &stop, step);

        if (slicelength <= 0) {
            return tuple_get_empty();
        }
        else if (start == 0 && step == 1 &&
                 slicelength == PyTuple_GET_SIZE(self) &&
                 PyTuple_CheckExact(self)) {
            return Py_NewRef(self);
        }
        else {
            PyTupleObject* result = tuple_alloc(slicelength);
            if (!result) return NULL;

            src = self->ob_item;
            dest = result->ob_item;
            for (cur = start, i = 0; i < slicelength;
                 cur += step, i++) {
                it = Py_NewRef(src[cur]);
                dest[i] = it;
            }

            _PyObject_GC_TRACK(result);
            return (PyObject *)result;
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
    return Py_BuildValue("(N)", tuple_slice(self, 0, Py_SIZE(self)));
}

static PyMethodDef tuple_methods[] = {
    TUPLE___GETNEWARGS___METHODDEF
    TUPLE_INDEX_METHODDEF
    TUPLE_COUNT_METHODDEF
    {"__class_getitem__", Py_GenericAlias, METH_O|METH_CLASS, PyDoc_STR("See PEP 585")},
    {NULL,              NULL}           /* sentinel */
};

static PyMappingMethods tuple_as_mapping = {
    tuple_length,
    tuple_subscript,
    0
};

static PyObject *tuple_iter(PyObject *seq);

PyTypeObject PyTuple_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "tuple",
    sizeof(PyTupleObject) - sizeof(PyObject *),
    sizeof(PyObject *),
    tuple_dealloc,                              /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    tuple_repr,                                 /* tp_repr */
    0,                                          /* tp_as_number */
    &tuple_as_sequence,                         /* tp_as_sequence */
    &tuple_as_mapping,                          /* tp_as_mapping */
    tuple_hash,                                 /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE | Py_TPFLAGS_TUPLE_SUBCLASS |
        _Py_TPFLAGS_MATCH_SELF | Py_TPFLAGS_SEQUENCE,  /* tp_flags */
    tuple_new__doc__,                           /* tp_doc */
    tuple_traverse,                             /* tp_traverse */
    0,                                          /* tp_clear */
    tuple_richcompare,                          /* tp_richcompare */
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
    .tp_vectorcall = tuple_vectorcall,
    .tp_version_tag = _Py_TYPE_VERSION_TUPLE,
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
    if (v == NULL || !Py_IS_TYPE(v, &PyTuple_Type) ||
        (Py_SIZE(v) != 0 && Py_REFCNT(v) != 1)) {
        *pv = 0;
        Py_XDECREF(v);
        PyErr_BadInternalCall();
        return -1;
    }

    oldsize = Py_SIZE(v);
    if (oldsize == newsize) {
        return 0;
    }
    if (newsize == 0) {
        Py_DECREF(v);
        *pv = tuple_get_empty();
        return 0;
    }
    if (oldsize == 0) {
#ifdef Py_DEBUG
        assert(v == &_Py_SINGLETON(tuple_empty));
#endif
        /* The empty tuple is statically allocated so we never
           resize it in-place. */
        Py_DECREF(v);
        *pv = PyTuple_New(newsize);
        return *pv == NULL ? -1 : 0;
    }

    if (_PyObject_GC_IS_TRACKED(v)) {
        _PyObject_GC_UNTRACK(v);
    }
#ifdef Py_TRACE_REFS
    _Py_ForgetReference((PyObject *) v);
#endif
    /* DECREF items deleted by shrinkage */
    for (i = newsize; i < oldsize; i++) {
        Py_CLEAR(v->ob_item[i]);
    }
    _PyReftracerTrack((PyObject *)v, PyRefTracer_DESTROY);
    sv = PyObject_GC_Resize(PyTupleObject, v, newsize);
    if (sv == NULL) {
        *pv = NULL;
#ifdef Py_REF_DEBUG
        _Py_DecRefTotal(_PyThreadState_GET());
#endif
        PyObject_GC_Del(v);
        return -1;
    }
    _Py_NewReferenceNoTotal((PyObject *) sv);
    /* Zero out items added by growing */
    if (newsize > oldsize)
        memset(&sv->ob_item[oldsize], 0,
               sizeof(*sv->ob_item) * (newsize - oldsize));
    *pv = (PyObject *) sv;
    _PyObject_GC_TRACK(sv);
    return 0;
}

/*********************** Tuple Iterator **************************/

#define _PyTupleIterObject_CAST(op) ((_PyTupleIterObject *)(op))

static void
tupleiter_dealloc(PyObject *self)
{
    _PyTupleIterObject *it = _PyTupleIterObject_CAST(self);
    _PyObject_GC_UNTRACK(it);
    Py_XDECREF(it->it_seq);
    assert(Py_IS_TYPE(self, &PyTupleIter_Type));
    _Py_FREELIST_FREE(tuple_iters, it, PyObject_GC_Del);
}

static int
tupleiter_traverse(PyObject *self, visitproc visit, void *arg)
{
    _PyTupleIterObject *it = _PyTupleIterObject_CAST(self);
    Py_VISIT(it->it_seq);
    return 0;
}

static PyObject *
tupleiter_next(PyObject *self)
{
    _PyTupleIterObject *it = _PyTupleIterObject_CAST(self);
    PyTupleObject *seq;
    PyObject *item;

    assert(it != NULL);
    seq = it->it_seq;
#ifndef Py_GIL_DISABLED
    if (seq == NULL)
        return NULL;
#endif
    assert(PyTuple_Check(seq));

    Py_ssize_t index = FT_ATOMIC_LOAD_SSIZE_RELAXED(it->it_index);
    if (index < PyTuple_GET_SIZE(seq)) {
        FT_ATOMIC_STORE_SSIZE_RELAXED(it->it_index, index + 1);
        item = PyTuple_GET_ITEM(seq, index);
        return Py_NewRef(item);
    }

#ifndef Py_GIL_DISABLED
    it->it_seq = NULL;
    Py_DECREF(seq);
#endif
    return NULL;
}

static PyObject *
tupleiter_len(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    _PyTupleIterObject *it = _PyTupleIterObject_CAST(self);
    Py_ssize_t len = 0;
#ifdef Py_GIL_DISABLED
    Py_ssize_t idx = FT_ATOMIC_LOAD_SSIZE_RELAXED(it->it_index);
    Py_ssize_t seq_len = PyTuple_GET_SIZE(it->it_seq);
    if (idx < seq_len)
        len = seq_len - idx;
#else
    if (it->it_seq)
        len = PyTuple_GET_SIZE(it->it_seq) - it->it_index;
#endif
    return PyLong_FromSsize_t(len);
}

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

static PyObject *
tupleiter_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *iter = _PyEval_GetBuiltin(&_Py_ID(iter));

    /* _PyEval_GetBuiltin can invoke arbitrary code,
     * call must be before access of iterator pointers.
     * see issue #101765 */
    _PyTupleIterObject *it = _PyTupleIterObject_CAST(self);

#ifdef Py_GIL_DISABLED
    Py_ssize_t idx = FT_ATOMIC_LOAD_SSIZE_RELAXED(it->it_index);
    if (idx < PyTuple_GET_SIZE(it->it_seq))
        return Py_BuildValue("N(O)n", iter, it->it_seq, idx);
#else
    if (it->it_seq)
        return Py_BuildValue("N(O)n", iter, it->it_seq, it->it_index);
#endif
    return Py_BuildValue("N(())", iter);
}

static PyObject *
tupleiter_setstate(PyObject *self, PyObject *state)
{
    _PyTupleIterObject *it = _PyTupleIterObject_CAST(self);
    Py_ssize_t index = PyLong_AsSsize_t(state);
    if (index == -1 && PyErr_Occurred())
        return NULL;
    if (it->it_seq != NULL) {
        if (index < 0)
            index = 0;
        else if (index > PyTuple_GET_SIZE(it->it_seq))
            index = PyTuple_GET_SIZE(it->it_seq); /* exhausted iterator */
        FT_ATOMIC_STORE_SSIZE_RELAXED(it->it_index, index);
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");
PyDoc_STRVAR(setstate_doc, "Set state information for unpickling.");

static PyMethodDef tupleiter_methods[] = {
    {"__length_hint__", tupleiter_len, METH_NOARGS, length_hint_doc},
    {"__reduce__", tupleiter_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", tupleiter_setstate, METH_O, setstate_doc},
    {NULL, NULL, 0, NULL} /* sentinel */
};

PyTypeObject PyTupleIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "tuple_iterator",                           /* tp_name */
    sizeof(_PyTupleIterObject),                 /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    tupleiter_dealloc,                          /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
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
    tupleiter_traverse,                         /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    tupleiter_next,                             /* tp_iternext */
    tupleiter_methods,                          /* tp_methods */
    0,
};

static PyObject *
tuple_iter(PyObject *seq)
{
    if (!PyTuple_Check(seq)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    _PyTupleIterObject *it = _Py_FREELIST_POP(_PyTupleIterObject, tuple_iters);
    if (it == NULL) {
        it = PyObject_GC_New(_PyTupleIterObject, &PyTupleIter_Type);
        if (it == NULL)
            return NULL;
    }
    it->it_index = 0;
    it->it_seq = (PyTupleObject *)Py_NewRef(seq);
    _PyObject_GC_TRACK(it);
    return (PyObject *)it;
}


/*************
 * freelists *
 *************/

static inline int
maybe_freelist_push(PyTupleObject *op)
{
    if (!Py_IS_TYPE(op, &PyTuple_Type)) {
        return 0;
    }
    Py_ssize_t index = Py_SIZE(op) - 1;
    if (index < PyTuple_MAXSAVESIZE) {
        return _Py_FREELIST_PUSH(tuples[index], op, Py_tuple_MAXFREELIST);
    }
    return 0;
}

/* Print summary info about the state of the optimized allocator */
void
_PyTuple_DebugMallocStats(FILE *out)
{
    for (int i = 0; i < PyTuple_MAXSAVESIZE; i++) {
        int len = i + 1;
        char buf[128];
        PyOS_snprintf(buf, sizeof(buf),
                      "free %d-sized PyTupleObject", len);
        _PyDebugAllocatorStats(out, buf, _Py_FREELIST_SIZE(tuples[i]),
                               _PyObject_VAR_SIZE(&PyTuple_Type, len));
    }
}
