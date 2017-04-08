/* List object implementation */

#include "Python.h"
#include "accu.h"

#ifdef STDC_HEADERS
#include <stddef.h>
#else
#include <sys/types.h>          /* For size_t */
#endif

/* Ensure ob_item has room for at least newsize elements, and set
 * ob_size to newsize.  If newsize > ob_size on entry, the content
 * of the new slots at exit is undefined heap trash; it's the caller's
 * responsibility to overwrite them with sane values.
 * The number of allocated elements may grow, shrink, or stay the same.
 * Failure is impossible if newsize <= self.allocated on entry, although
 * that partly relies on an assumption that the system realloc() never
 * fails when passed a number of bytes <= the number of bytes last
 * allocated (the C standard doesn't guarantee this, but it's hard to
 * imagine a realloc implementation where it wouldn't be true).
 * Note that self->ob_item may change, and even if newsize is less
 * than ob_size on entry.
 */
static int
list_resize(PyListObject *self, Py_ssize_t newsize)
{
    PyObject **items;
    size_t new_allocated;
    Py_ssize_t allocated = self->allocated;

    /* Bypass realloc() when a previous overallocation is large enough
       to accommodate the newsize.  If the newsize falls lower than half
       the allocated size, then proceed with the realloc() to shrink the list.
    */
    if (allocated >= newsize && newsize >= (allocated >> 1)) {
        assert(self->ob_item != NULL || newsize == 0);
        Py_SIZE(self) = newsize;
        return 0;
    }

    /* This over-allocates proportional to the list size, making room
     * for additional growth.  The over-allocation is mild, but is
     * enough to give linear-time amortized behavior over a long
     * sequence of appends() in the presence of a poorly-performing
     * system realloc().
     * The growth pattern is:  0, 4, 8, 16, 25, 35, 46, 58, 72, 88, ...
     */
    new_allocated = (newsize >> 3) + (newsize < 9 ? 3 : 6);

    /* check for integer overflow */
    if (new_allocated > SIZE_MAX - newsize) {
        PyErr_NoMemory();
        return -1;
    } else {
        new_allocated += newsize;
    }

    if (newsize == 0)
        new_allocated = 0;
    items = self->ob_item;
    if (new_allocated <= (SIZE_MAX / sizeof(PyObject *)))
        PyMem_RESIZE(items, PyObject *, new_allocated);
    else
        items = NULL;
    if (items == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    self->ob_item = items;
    Py_SIZE(self) = newsize;
    self->allocated = new_allocated;
    return 0;
}

/* Debug statistic to compare allocations with reuse through the free list */
#undef SHOW_ALLOC_COUNT
#ifdef SHOW_ALLOC_COUNT
static size_t count_alloc = 0;
static size_t count_reuse = 0;

static void
show_alloc(void)
{
    PyObject *xoptions, *value;
    _Py_IDENTIFIER(showalloccount);

    xoptions = PySys_GetXOptions();
    if (xoptions == NULL)
        return;
    value = _PyDict_GetItemId(xoptions, &PyId_showalloccount);
    if (value != Py_True)
        return;

    fprintf(stderr, "List allocations: %" PY_FORMAT_SIZE_T "d\n",
        count_alloc);
    fprintf(stderr, "List reuse through freelist: %" PY_FORMAT_SIZE_T
        "d\n", count_reuse);
    fprintf(stderr, "%.2f%% reuse rate\n\n",
        (100.0*count_reuse/(count_alloc+count_reuse)));
}
#endif

/* Empty list reuse scheme to save calls to malloc and free */
#ifndef PyList_MAXFREELIST
#define PyList_MAXFREELIST 80
#endif
static PyListObject *free_list[PyList_MAXFREELIST];
static int numfree = 0;

int
PyList_ClearFreeList(void)
{
    PyListObject *op;
    int ret = numfree;
    while (numfree) {
        op = free_list[--numfree];
        assert(PyList_CheckExact(op));
        PyObject_GC_Del(op);
    }
    return ret;
}

void
PyList_Fini(void)
{
    PyList_ClearFreeList();
}

/* Print summary info about the state of the optimized allocator */
void
_PyList_DebugMallocStats(FILE *out)
{
    _PyDebugAllocatorStats(out,
                           "free PyListObject",
                           numfree, sizeof(PyListObject));
}

PyObject *
PyList_New(Py_ssize_t size)
{
    PyListObject *op;
#ifdef SHOW_ALLOC_COUNT
    static int initialized = 0;
    if (!initialized) {
        Py_AtExit(show_alloc);
        initialized = 1;
    }
#endif

    if (size < 0) {
        PyErr_BadInternalCall();
        return NULL;
    }
    if (numfree) {
        numfree--;
        op = free_list[numfree];
        _Py_NewReference((PyObject *)op);
#ifdef SHOW_ALLOC_COUNT
        count_reuse++;
#endif
    } else {
        op = PyObject_GC_New(PyListObject, &PyList_Type);
        if (op == NULL)
            return NULL;
#ifdef SHOW_ALLOC_COUNT
        count_alloc++;
#endif
    }
    if (size <= 0)
        op->ob_item = NULL;
    else {
        op->ob_item = (PyObject **) PyMem_Calloc(size, sizeof(PyObject *));
        if (op->ob_item == NULL) {
            Py_DECREF(op);
            return PyErr_NoMemory();
        }
    }
    Py_SIZE(op) = size;
    op->allocated = size;
    _PyObject_GC_TRACK(op);
    return (PyObject *) op;
}

Py_ssize_t
PyList_Size(PyObject *op)
{
    if (!PyList_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    else
        return Py_SIZE(op);
}

static PyObject *indexerr = NULL;

PyObject *
PyList_GetItem(PyObject *op, Py_ssize_t i)
{
    if (!PyList_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    if (i < 0 || i >= Py_SIZE(op)) {
        if (indexerr == NULL) {
            indexerr = PyUnicode_FromString(
                "list index out of range");
            if (indexerr == NULL)
                return NULL;
        }
        PyErr_SetObject(PyExc_IndexError, indexerr);
        return NULL;
    }
    return ((PyListObject *)op) -> ob_item[i];
}

int
PyList_SetItem(PyObject *op, Py_ssize_t i,
               PyObject *newitem)
{
    PyObject **p;
    if (!PyList_Check(op)) {
        Py_XDECREF(newitem);
        PyErr_BadInternalCall();
        return -1;
    }
    if (i < 0 || i >= Py_SIZE(op)) {
        Py_XDECREF(newitem);
        PyErr_SetString(PyExc_IndexError,
                        "list assignment index out of range");
        return -1;
    }
    p = ((PyListObject *)op) -> ob_item + i;
    Py_XSETREF(*p, newitem);
    return 0;
}

static int
ins1(PyListObject *self, Py_ssize_t where, PyObject *v)
{
    Py_ssize_t i, n = Py_SIZE(self);
    PyObject **items;
    if (v == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (n == PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError,
            "cannot add more objects to list");
        return -1;
    }

    if (list_resize(self, n+1) < 0)
        return -1;

    if (where < 0) {
        where += n;
        if (where < 0)
            where = 0;
    }
    if (where > n)
        where = n;
    items = self->ob_item;
    for (i = n; --i >= where; )
        items[i+1] = items[i];
    Py_INCREF(v);
    items[where] = v;
    return 0;
}

int
PyList_Insert(PyObject *op, Py_ssize_t where, PyObject *newitem)
{
    if (!PyList_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    return ins1((PyListObject *)op, where, newitem);
}

static int
app1(PyListObject *self, PyObject *v)
{
    Py_ssize_t n = PyList_GET_SIZE(self);

    assert (v != NULL);
    if (n == PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError,
            "cannot add more objects to list");
        return -1;
    }

    if (list_resize(self, n+1) < 0)
        return -1;

    Py_INCREF(v);
    PyList_SET_ITEM(self, n, v);
    return 0;
}

int
PyList_Append(PyObject *op, PyObject *newitem)
{
    if (PyList_Check(op) && (newitem != NULL))
        return app1((PyListObject *)op, newitem);
    PyErr_BadInternalCall();
    return -1;
}

/* Methods */

static void
list_dealloc(PyListObject *op)
{
    Py_ssize_t i;
    PyObject_GC_UnTrack(op);
    Py_TRASHCAN_SAFE_BEGIN(op)
    if (op->ob_item != NULL) {
        /* Do it backwards, for Christian Tismer.
           There's a simple test case where somehow this reduces
           thrashing when a *very* large list is created and
           immediately deleted. */
        i = Py_SIZE(op);
        while (--i >= 0) {
            Py_XDECREF(op->ob_item[i]);
        }
        PyMem_FREE(op->ob_item);
    }
    if (numfree < PyList_MAXFREELIST && PyList_CheckExact(op))
        free_list[numfree++] = op;
    else
        Py_TYPE(op)->tp_free((PyObject *)op);
    Py_TRASHCAN_SAFE_END(op)
}

static PyObject *
list_repr(PyListObject *v)
{
    Py_ssize_t i;
    PyObject *s;
    _PyUnicodeWriter writer;

    if (Py_SIZE(v) == 0) {
        return PyUnicode_FromString("[]");
    }

    i = Py_ReprEnter((PyObject*)v);
    if (i != 0) {
        return i > 0 ? PyUnicode_FromString("[...]") : NULL;
    }

    _PyUnicodeWriter_Init(&writer);
    writer.overallocate = 1;
    /* "[" + "1" + ", 2" * (len - 1) + "]" */
    writer.min_length = 1 + 1 + (2 + 1) * (Py_SIZE(v) - 1) + 1;

    if (_PyUnicodeWriter_WriteChar(&writer, '[') < 0)
        goto error;

    /* Do repr() on each element.  Note that this may mutate the list,
       so must refetch the list size on each iteration. */
    for (i = 0; i < Py_SIZE(v); ++i) {
        if (i > 0) {
            if (_PyUnicodeWriter_WriteASCIIString(&writer, ", ", 2) < 0)
                goto error;
        }

        if (Py_EnterRecursiveCall(" while getting the repr of a list"))
            goto error;
        s = PyObject_Repr(v->ob_item[i]);
        Py_LeaveRecursiveCall();
        if (s == NULL)
            goto error;

        if (_PyUnicodeWriter_WriteStr(&writer, s) < 0) {
            Py_DECREF(s);
            goto error;
        }
        Py_DECREF(s);
    }

    writer.overallocate = 0;
    if (_PyUnicodeWriter_WriteChar(&writer, ']') < 0)
        goto error;

    Py_ReprLeave((PyObject *)v);
    return _PyUnicodeWriter_Finish(&writer);

error:
    _PyUnicodeWriter_Dealloc(&writer);
    Py_ReprLeave((PyObject *)v);
    return NULL;
}

static Py_ssize_t
list_length(PyListObject *a)
{
    return Py_SIZE(a);
}

static int
list_contains(PyListObject *a, PyObject *el)
{
    Py_ssize_t i;
    int cmp;

    for (i = 0, cmp = 0 ; cmp == 0 && i < Py_SIZE(a); ++i)
        cmp = PyObject_RichCompareBool(el, PyList_GET_ITEM(a, i),
                                           Py_EQ);
    return cmp;
}

static PyObject *
list_item(PyListObject *a, Py_ssize_t i)
{
    if (i < 0 || i >= Py_SIZE(a)) {
        if (indexerr == NULL) {
            indexerr = PyUnicode_FromString(
                "list index out of range");
            if (indexerr == NULL)
                return NULL;
        }
        PyErr_SetObject(PyExc_IndexError, indexerr);
        return NULL;
    }
    Py_INCREF(a->ob_item[i]);
    return a->ob_item[i];
}

static PyObject *
list_slice(PyListObject *a, Py_ssize_t ilow, Py_ssize_t ihigh)
{
    PyListObject *np;
    PyObject **src, **dest;
    Py_ssize_t i, len;
    if (ilow < 0)
        ilow = 0;
    else if (ilow > Py_SIZE(a))
        ilow = Py_SIZE(a);
    if (ihigh < ilow)
        ihigh = ilow;
    else if (ihigh > Py_SIZE(a))
        ihigh = Py_SIZE(a);
    len = ihigh - ilow;
    np = (PyListObject *) PyList_New(len);
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
PyList_GetSlice(PyObject *a, Py_ssize_t ilow, Py_ssize_t ihigh)
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
    Py_ssize_t size;
    Py_ssize_t i;
    PyObject **src, **dest;
    PyListObject *np;
    if (!PyList_Check(bb)) {
        PyErr_Format(PyExc_TypeError,
                  "can only concatenate list (not \"%.200s\") to list",
                  bb->ob_type->tp_name);
        return NULL;
    }
#define b ((PyListObject *)bb)
    if (Py_SIZE(a) > PY_SSIZE_T_MAX - Py_SIZE(b))
        return PyErr_NoMemory();
    size = Py_SIZE(a) + Py_SIZE(b);
    np = (PyListObject *) PyList_New(size);
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
list_repeat(PyListObject *a, Py_ssize_t n)
{
    Py_ssize_t i, j;
    Py_ssize_t size;
    PyListObject *np;
    PyObject **p, **items;
    PyObject *elem;
    if (n < 0)
        n = 0;
    if (n > 0 && Py_SIZE(a) > PY_SSIZE_T_MAX / n)
        return PyErr_NoMemory();
    size = Py_SIZE(a) * n;
    if (size == 0)
        return PyList_New(0);
    np = (PyListObject *) PyList_New(size);
    if (np == NULL)
        return NULL;

    items = np->ob_item;
    if (Py_SIZE(a) == 1) {
        elem = a->ob_item[0];
        for (i = 0; i < n; i++) {
            items[i] = elem;
            Py_INCREF(elem);
        }
        return (PyObject *) np;
    }
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

static int
list_clear(PyListObject *a)
{
    Py_ssize_t i;
    PyObject **item = a->ob_item;
    if (item != NULL) {
        /* Because XDECREF can recursively invoke operations on
           this list, we make it empty first. */
        i = Py_SIZE(a);
        Py_SIZE(a) = 0;
        a->ob_item = NULL;
        a->allocated = 0;
        while (--i >= 0) {
            Py_XDECREF(item[i]);
        }
        PyMem_FREE(item);
    }
    /* Never fails; the return value can be ignored.
       Note that there is no guarantee that the list is actually empty
       at this point, because XDECREF may have populated it again! */
    return 0;
}

/* a[ilow:ihigh] = v if v != NULL.
 * del a[ilow:ihigh] if v == NULL.
 *
 * Special speed gimmick:  when v is NULL and ihigh - ilow <= 8, it's
 * guaranteed the call cannot fail.
 */
static int
list_ass_slice(PyListObject *a, Py_ssize_t ilow, Py_ssize_t ihigh, PyObject *v)
{
    /* Because [X]DECREF can recursively invoke list operations on
       this list, we must postpone all [X]DECREF activity until
       after the list is back in its canonical shape.  Therefore
       we must allocate an additional array, 'recycle', into which
       we temporarily copy the items that are deleted from the
       list. :-( */
    PyObject *recycle_on_stack[8];
    PyObject **recycle = recycle_on_stack; /* will allocate more if needed */
    PyObject **item;
    PyObject **vitem = NULL;
    PyObject *v_as_SF = NULL; /* PySequence_Fast(v) */
    Py_ssize_t n; /* # of elements in replacement list */
    Py_ssize_t norig; /* # of elements in list getting replaced */
    Py_ssize_t d; /* Change in size */
    Py_ssize_t k;
    size_t s;
    int result = -1;            /* guilty until proved innocent */
#define b ((PyListObject *)v)
    if (v == NULL)
        n = 0;
    else {
        if (a == b) {
            /* Special case "a[i:j] = a" -- copy b first */
            v = list_slice(b, 0, Py_SIZE(b));
            if (v == NULL)
                return result;
            result = list_ass_slice(a, ilow, ihigh, v);
            Py_DECREF(v);
            return result;
        }
        v_as_SF = PySequence_Fast(v, "can only assign an iterable");
        if(v_as_SF == NULL)
            goto Error;
        n = PySequence_Fast_GET_SIZE(v_as_SF);
        vitem = PySequence_Fast_ITEMS(v_as_SF);
    }
    if (ilow < 0)
        ilow = 0;
    else if (ilow > Py_SIZE(a))
        ilow = Py_SIZE(a);

    if (ihigh < ilow)
        ihigh = ilow;
    else if (ihigh > Py_SIZE(a))
        ihigh = Py_SIZE(a);

    norig = ihigh - ilow;
    assert(norig >= 0);
    d = n - norig;
    if (Py_SIZE(a) + d == 0) {
        Py_XDECREF(v_as_SF);
        return list_clear(a);
    }
    item = a->ob_item;
    /* recycle the items that we are about to remove */
    s = norig * sizeof(PyObject *);
    /* If norig == 0, item might be NULL, in which case we may not memcpy from it. */
    if (s) {
        if (s > sizeof(recycle_on_stack)) {
            recycle = (PyObject **)PyMem_MALLOC(s);
            if (recycle == NULL) {
                PyErr_NoMemory();
                goto Error;
            }
        }
        memcpy(recycle, &item[ilow], s);
    }

    if (d < 0) { /* Delete -d items */
        Py_ssize_t tail;
        tail = (Py_SIZE(a) - ihigh) * sizeof(PyObject *);
        memmove(&item[ihigh+d], &item[ihigh], tail);
        if (list_resize(a, Py_SIZE(a) + d) < 0) {
            memmove(&item[ihigh], &item[ihigh+d], tail);
            memcpy(&item[ilow], recycle, s);
            goto Error;
        }
        item = a->ob_item;
    }
    else if (d > 0) { /* Insert d items */
        k = Py_SIZE(a);
        if (list_resize(a, k+d) < 0)
            goto Error;
        item = a->ob_item;
        memmove(&item[ihigh+d], &item[ihigh],
            (k - ihigh)*sizeof(PyObject *));
    }
    for (k = 0; k < n; k++, ilow++) {
        PyObject *w = vitem[k];
        Py_XINCREF(w);
        item[ilow] = w;
    }
    for (k = norig - 1; k >= 0; --k)
        Py_XDECREF(recycle[k]);
    result = 0;
 Error:
    if (recycle != recycle_on_stack)
        PyMem_FREE(recycle);
    Py_XDECREF(v_as_SF);
    return result;
#undef b
}

int
PyList_SetSlice(PyObject *a, Py_ssize_t ilow, Py_ssize_t ihigh, PyObject *v)
{
    if (!PyList_Check(a)) {
        PyErr_BadInternalCall();
        return -1;
    }
    return list_ass_slice((PyListObject *)a, ilow, ihigh, v);
}

static PyObject *
list_inplace_repeat(PyListObject *self, Py_ssize_t n)
{
    PyObject **items;
    Py_ssize_t size, i, j, p;


    size = PyList_GET_SIZE(self);
    if (size == 0 || n == 1) {
        Py_INCREF(self);
        return (PyObject *)self;
    }

    if (n < 1) {
        (void)list_clear(self);
        Py_INCREF(self);
        return (PyObject *)self;
    }

    if (size > PY_SSIZE_T_MAX / n) {
        return PyErr_NoMemory();
    }

    if (list_resize(self, size*n) < 0)
        return NULL;

    p = size;
    items = self->ob_item;
    for (i = 1; i < n; i++) { /* Start counting at 1, not 0 */
        for (j = 0; j < size; j++) {
            PyObject *o = items[j];
            Py_INCREF(o);
            items[p++] = o;
        }
    }
    Py_INCREF(self);
    return (PyObject *)self;
}

static int
list_ass_item(PyListObject *a, Py_ssize_t i, PyObject *v)
{
    if (i < 0 || i >= Py_SIZE(a)) {
        PyErr_SetString(PyExc_IndexError,
                        "list assignment index out of range");
        return -1;
    }
    if (v == NULL)
        return list_ass_slice(a, i, i+1, v);
    Py_INCREF(v);
    Py_SETREF(a->ob_item[i], v);
    return 0;
}

static PyObject *
listinsert(PyListObject *self, PyObject *args)
{
    Py_ssize_t i;
    PyObject *v;
    if (!PyArg_ParseTuple(args, "nO:insert", &i, &v))
        return NULL;
    if (ins1(self, i, v) == 0)
        Py_RETURN_NONE;
    return NULL;
}

static PyObject *
listclear(PyListObject *self)
{
    list_clear(self);
    Py_RETURN_NONE;
}

static PyObject *
listcopy(PyListObject *self)
{
    return list_slice(self, 0, Py_SIZE(self));
}

static PyObject *
listappend(PyListObject *self, PyObject *v)
{
    if (app1(self, v) == 0)
        Py_RETURN_NONE;
    return NULL;
}

static PyObject *
listextend(PyListObject *self, PyObject *b)
{
    PyObject *it;      /* iter(v) */
    Py_ssize_t m;                  /* size of self */
    Py_ssize_t n;                  /* guess for size of b */
    Py_ssize_t mn;                 /* m + n */
    Py_ssize_t i;
    PyObject *(*iternext)(PyObject *);

    /* Special cases:
       1) lists and tuples which can use PySequence_Fast ops
       2) extending self to self requires making a copy first
    */
    if (PyList_CheckExact(b) || PyTuple_CheckExact(b) || (PyObject *)self == b) {
        PyObject **src, **dest;
        b = PySequence_Fast(b, "argument must be iterable");
        if (!b)
            return NULL;
        n = PySequence_Fast_GET_SIZE(b);
        if (n == 0) {
            /* short circuit when b is empty */
            Py_DECREF(b);
            Py_RETURN_NONE;
        }
        m = Py_SIZE(self);
        if (list_resize(self, m + n) < 0) {
            Py_DECREF(b);
            return NULL;
        }
        /* note that we may still have self == b here for the
         * situation a.extend(a), but the following code works
         * in that case too.  Just make sure to resize self
         * before calling PySequence_Fast_ITEMS.
         */
        /* populate the end of self with b's items */
        src = PySequence_Fast_ITEMS(b);
        dest = self->ob_item + m;
        for (i = 0; i < n; i++) {
            PyObject *o = src[i];
            Py_INCREF(o);
            dest[i] = o;
        }
        Py_DECREF(b);
        Py_RETURN_NONE;
    }

    it = PyObject_GetIter(b);
    if (it == NULL)
        return NULL;
    iternext = *it->ob_type->tp_iternext;

    /* Guess a result list size. */
    n = PyObject_LengthHint(b, 8);
    if (n < 0) {
        Py_DECREF(it);
        return NULL;
    }
    m = Py_SIZE(self);
    if (m > PY_SSIZE_T_MAX - n) {
        /* m + n overflowed; on the chance that n lied, and there really
         * is enough room, ignore it.  If n was telling the truth, we'll
         * eventually run out of memory during the loop.
         */
    }
    else {
        mn = m + n;
        /* Make room. */
        if (list_resize(self, mn) < 0)
            goto error;
        /* Make the list sane again. */
        Py_SIZE(self) = m;
    }

    /* Run iterator to exhaustion. */
    for (;;) {
        PyObject *item = iternext(it);
        if (item == NULL) {
            if (PyErr_Occurred()) {
                if (PyErr_ExceptionMatches(PyExc_StopIteration))
                    PyErr_Clear();
                else
                    goto error;
            }
            break;
        }
        if (Py_SIZE(self) < self->allocated) {
            /* steals ref */
            PyList_SET_ITEM(self, Py_SIZE(self), item);
            ++Py_SIZE(self);
        }
        else {
            int status = app1(self, item);
            Py_DECREF(item);  /* append creates a new ref */
            if (status < 0)
                goto error;
        }
    }

    /* Cut back result list if initial guess was too large. */
    if (Py_SIZE(self) < self->allocated) {
        if (list_resize(self, Py_SIZE(self)) < 0)
            goto error;
    }

    Py_DECREF(it);
    Py_RETURN_NONE;

  error:
    Py_DECREF(it);
    return NULL;
}

PyObject *
_PyList_Extend(PyListObject *self, PyObject *b)
{
    return listextend(self, b);
}

static PyObject *
list_inplace_concat(PyListObject *self, PyObject *other)
{
    PyObject *result;

    result = listextend(self, other);
    if (result == NULL)
        return result;
    Py_DECREF(result);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
listpop(PyListObject *self, PyObject *args)
{
    Py_ssize_t i = -1;
    PyObject *v;
    int status;

    if (!PyArg_ParseTuple(args, "|n:pop", &i))
        return NULL;

    if (Py_SIZE(self) == 0) {
        /* Special-case most common failure cause */
        PyErr_SetString(PyExc_IndexError, "pop from empty list");
        return NULL;
    }
    if (i < 0)
        i += Py_SIZE(self);
    if (i < 0 || i >= Py_SIZE(self)) {
        PyErr_SetString(PyExc_IndexError, "pop index out of range");
        return NULL;
    }
    v = self->ob_item[i];
    if (i == Py_SIZE(self) - 1) {
        status = list_resize(self, Py_SIZE(self) - 1);
        if (status >= 0)
            return v; /* and v now owns the reference the list had */
        else
            return NULL;
    }
    Py_INCREF(v);
    status = list_ass_slice(self, i, i+1, (PyObject *)NULL);
    if (status < 0) {
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

/* A sortslice contains a pointer to an array of keys and a pointer to
 * an array of corresponding values.  In other words, keys[i]
 * corresponds with values[i].  If values == NULL, then the keys are
 * also the values.
 *
 * Several convenience routines are provided here, so that keys and
 * values are always moved in sync.
 */

typedef struct {
    PyObject **keys;
    PyObject **values;
} sortslice;

Py_LOCAL_INLINE(void)
sortslice_copy(sortslice *s1, Py_ssize_t i, sortslice *s2, Py_ssize_t j)
{
    s1->keys[i] = s2->keys[j];
    if (s1->values != NULL)
        s1->values[i] = s2->values[j];
}

Py_LOCAL_INLINE(void)
sortslice_copy_incr(sortslice *dst, sortslice *src)
{
    *dst->keys++ = *src->keys++;
    if (dst->values != NULL)
        *dst->values++ = *src->values++;
}

Py_LOCAL_INLINE(void)
sortslice_copy_decr(sortslice *dst, sortslice *src)
{
    *dst->keys-- = *src->keys--;
    if (dst->values != NULL)
        *dst->values-- = *src->values--;
}


Py_LOCAL_INLINE(void)
sortslice_memcpy(sortslice *s1, Py_ssize_t i, sortslice *s2, Py_ssize_t j,
                 Py_ssize_t n)
{
    memcpy(&s1->keys[i], &s2->keys[j], sizeof(PyObject *) * n);
    if (s1->values != NULL)
        memcpy(&s1->values[i], &s2->values[j], sizeof(PyObject *) * n);
}

Py_LOCAL_INLINE(void)
sortslice_memmove(sortslice *s1, Py_ssize_t i, sortslice *s2, Py_ssize_t j,
                  Py_ssize_t n)
{
    memmove(&s1->keys[i], &s2->keys[j], sizeof(PyObject *) * n);
    if (s1->values != NULL)
        memmove(&s1->values[i], &s2->values[j], sizeof(PyObject *) * n);
}

Py_LOCAL_INLINE(void)
sortslice_advance(sortslice *slice, Py_ssize_t n)
{
    slice->keys += n;
    if (slice->values != NULL)
        slice->values += n;
}

/* Comparison function: PyObject_RichCompareBool with Py_LT.
 * Returns -1 on error, 1 if x < y, 0 if x >= y.
 */

#define ISLT(X, Y) (PyObject_RichCompareBool(X, Y, Py_LT))

/* Compare X to Y via "<".  Goto "fail" if the comparison raises an
   error.  Else "k" is set to true iff X<Y, and an "if (k)" block is
   started.  It makes more sense in context <wink>.  X and Y are PyObject*s.
*/
#define IFLT(X, Y) if ((k = ISLT(X, Y)) < 0) goto fail;  \
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
binarysort(sortslice lo, PyObject **hi, PyObject **start)
{
    Py_ssize_t k;
    PyObject **l, **p, **r;
    PyObject *pivot;

    assert(lo.keys <= start && start <= hi);
    /* assert [lo, start) is sorted */
    if (lo.keys == start)
        ++start;
    for (; start < hi; ++start) {
        /* set l to where *start belongs */
        l = lo.keys;
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
        if (lo.values != NULL) {
            Py_ssize_t offset = lo.values - lo.keys;
            p = start + offset;
            pivot = *p;
            l += offset;
            for (p = start + offset; p > l; --p)
                *p = *(p-1);
            *l = pivot;
        }
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
static Py_ssize_t
count_run(PyObject **lo, PyObject **hi, int *descending)
{
    Py_ssize_t k;
    Py_ssize_t n;

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
static Py_ssize_t
gallop_left(PyObject *key, PyObject **a, Py_ssize_t n, Py_ssize_t hint)
{
    Py_ssize_t ofs;
    Py_ssize_t lastofs;
    Py_ssize_t k;

    assert(key && a && n > 0 && hint >= 0 && hint < n);

    a += hint;
    lastofs = 0;
    ofs = 1;
    IFLT(*a, key) {
        /* a[hint] < key -- gallop right, until
         * a[hint + lastofs] < key <= a[hint + ofs]
         */
        const Py_ssize_t maxofs = n - hint;             /* &a[n-1] is highest */
        while (ofs < maxofs) {
            IFLT(a[ofs], key) {
                lastofs = ofs;
                ofs = (ofs << 1) + 1;
                if (ofs <= 0)                   /* int overflow */
                    ofs = maxofs;
            }
            else                /* key <= a[hint + ofs] */
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
        const Py_ssize_t maxofs = hint + 1;             /* &a[0] is lowest */
        while (ofs < maxofs) {
            IFLT(*(a-ofs), key)
                break;
            /* key <= a[hint - ofs] */
            lastofs = ofs;
            ofs = (ofs << 1) + 1;
            if (ofs <= 0)               /* int overflow */
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
        Py_ssize_t m = lastofs + ((ofs - lastofs) >> 1);

        IFLT(a[m], key)
            lastofs = m+1;              /* a[m] < key */
        else
            ofs = m;                    /* key <= a[m] */
    }
    assert(lastofs == ofs);             /* so a[ofs-1] < key <= a[ofs] */
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
static Py_ssize_t
gallop_right(PyObject *key, PyObject **a, Py_ssize_t n, Py_ssize_t hint)
{
    Py_ssize_t ofs;
    Py_ssize_t lastofs;
    Py_ssize_t k;

    assert(key && a && n > 0 && hint >= 0 && hint < n);

    a += hint;
    lastofs = 0;
    ofs = 1;
    IFLT(key, *a) {
        /* key < a[hint] -- gallop left, until
         * a[hint - ofs] <= key < a[hint - lastofs]
         */
        const Py_ssize_t maxofs = hint + 1;             /* &a[0] is lowest */
        while (ofs < maxofs) {
            IFLT(key, *(a-ofs)) {
                lastofs = ofs;
                ofs = (ofs << 1) + 1;
                if (ofs <= 0)                   /* int overflow */
                    ofs = maxofs;
            }
            else                /* a[hint - ofs] <= key */
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
        const Py_ssize_t maxofs = n - hint;             /* &a[n-1] is highest */
        while (ofs < maxofs) {
            IFLT(key, a[ofs])
                break;
            /* a[hint + ofs] <= key */
            lastofs = ofs;
            ofs = (ofs << 1) + 1;
            if (ofs <= 0)               /* int overflow */
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
        Py_ssize_t m = lastofs + ((ofs - lastofs) >> 1);

        IFLT(key, a[m])
            ofs = m;                    /* key < a[m] */
        else
            lastofs = m+1;              /* a[m] <= key */
    }
    assert(lastofs == ofs);             /* so a[ofs-1] <= key < a[ofs] */
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
    sortslice base;
    Py_ssize_t len;
};

typedef struct s_MergeState {
    /* This controls when we get *into* galloping mode.  It's initialized
     * to MIN_GALLOP.  merge_lo and merge_hi tend to nudge it higher for
     * random data, and lower for highly structured data.
     */
    Py_ssize_t min_gallop;

    /* 'a' is temp storage to help with merges.  It contains room for
     * alloced entries.
     */
    sortslice a;        /* may point to temparray below */
    Py_ssize_t alloced;

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
merge_init(MergeState *ms, Py_ssize_t list_size, int has_keyfunc)
{
    assert(ms != NULL);
    if (has_keyfunc) {
        /* The temporary space for merging will need at most half the list
         * size rounded up.  Use the minimum possible space so we can use the
         * rest of temparray for other things.  In particular, if there is
         * enough extra space, listsort() will use it to store the keys.
         */
        ms->alloced = (list_size + 1) / 2;

        /* ms->alloced describes how many keys will be stored at
           ms->temparray, but we also need to store the values.  Hence,
           ms->alloced is capped at half of MERGESTATE_TEMP_SIZE. */
        if (MERGESTATE_TEMP_SIZE / 2 < ms->alloced)
            ms->alloced = MERGESTATE_TEMP_SIZE / 2;
        ms->a.values = &ms->temparray[ms->alloced];
    }
    else {
        ms->alloced = MERGESTATE_TEMP_SIZE;
        ms->a.values = NULL;
    }
    ms->a.keys = ms->temparray;
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
    if (ms->a.keys != ms->temparray)
        PyMem_Free(ms->a.keys);
}

/* Ensure enough temp memory for 'need' array slots is available.
 * Returns 0 on success and -1 if the memory can't be gotten.
 */
static int
merge_getmem(MergeState *ms, Py_ssize_t need)
{
    int multiplier;

    assert(ms != NULL);
    if (need <= ms->alloced)
        return 0;

    multiplier = ms->a.values != NULL ? 2 : 1;

    /* Don't realloc!  That can cost cycles to copy the old data, but
     * we don't care what's in the block.
     */
    merge_freemem(ms);
    if ((size_t)need > PY_SSIZE_T_MAX / sizeof(PyObject*) / multiplier) {
        PyErr_NoMemory();
        return -1;
    }
    ms->a.keys = (PyObject**)PyMem_Malloc(multiplier * need
                                          * sizeof(PyObject *));
    if (ms->a.keys != NULL) {
        ms->alloced = need;
        if (ms->a.values != NULL)
            ms->a.values = &ms->a.keys[need];
        return 0;
    }
    PyErr_NoMemory();
    return -1;
}
#define MERGE_GETMEM(MS, NEED) ((NEED) <= (MS)->alloced ? 0 :   \
                                merge_getmem(MS, NEED))

/* Merge the na elements starting at ssa with the nb elements starting at
 * ssb.keys = ssa.keys + na in a stable way, in-place.  na and nb must be > 0.
 * Must also have that ssa.keys[na-1] belongs at the end of the merge, and
 * should have na <= nb.  See listsort.txt for more info.  Return 0 if
 * successful, -1 if error.
 */
static Py_ssize_t
merge_lo(MergeState *ms, sortslice ssa, Py_ssize_t na,
         sortslice ssb, Py_ssize_t nb)
{
    Py_ssize_t k;
    sortslice dest;
    int result = -1;            /* guilty until proved innocent */
    Py_ssize_t min_gallop;

    assert(ms && ssa.keys && ssb.keys && na > 0 && nb > 0);
    assert(ssa.keys + na == ssb.keys);
    if (MERGE_GETMEM(ms, na) < 0)
        return -1;
    sortslice_memcpy(&ms->a, 0, &ssa, 0, na);
    dest = ssa;
    ssa = ms->a;

    sortslice_copy_incr(&dest, &ssb);
    --nb;
    if (nb == 0)
        goto Succeed;
    if (na == 1)
        goto CopyB;

    min_gallop = ms->min_gallop;
    for (;;) {
        Py_ssize_t acount = 0;          /* # of times A won in a row */
        Py_ssize_t bcount = 0;          /* # of times B won in a row */

        /* Do the straightforward thing until (if ever) one run
         * appears to win consistently.
         */
        for (;;) {
            assert(na > 1 && nb > 0);
            k = ISLT(ssb.keys[0], ssa.keys[0]);
            if (k) {
                if (k < 0)
                    goto Fail;
                sortslice_copy_incr(&dest, &ssb);
                ++bcount;
                acount = 0;
                --nb;
                if (nb == 0)
                    goto Succeed;
                if (bcount >= min_gallop)
                    break;
            }
            else {
                sortslice_copy_incr(&dest, &ssa);
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
            k = gallop_right(ssb.keys[0], ssa.keys, na, 0);
            acount = k;
            if (k) {
                if (k < 0)
                    goto Fail;
                sortslice_memcpy(&dest, 0, &ssa, 0, k);
                sortslice_advance(&dest, k);
                sortslice_advance(&ssa, k);
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
            sortslice_copy_incr(&dest, &ssb);
            --nb;
            if (nb == 0)
                goto Succeed;

            k = gallop_left(ssa.keys[0], ssb.keys, nb, 0);
            bcount = k;
            if (k) {
                if (k < 0)
                    goto Fail;
                sortslice_memmove(&dest, 0, &ssb, 0, k);
                sortslice_advance(&dest, k);
                sortslice_advance(&ssb, k);
                nb -= k;
                if (nb == 0)
                    goto Succeed;
            }
            sortslice_copy_incr(&dest, &ssa);
            --na;
            if (na == 1)
                goto CopyB;
        } while (acount >= MIN_GALLOP || bcount >= MIN_GALLOP);
        ++min_gallop;           /* penalize it for leaving galloping mode */
        ms->min_gallop = min_gallop;
    }
Succeed:
    result = 0;
Fail:
    if (na)
        sortslice_memcpy(&dest, 0, &ssa, 0, na);
    return result;
CopyB:
    assert(na == 1 && nb > 0);
    /* The last element of ssa belongs at the end of the merge. */
    sortslice_memmove(&dest, 0, &ssb, 0, nb);
    sortslice_copy(&dest, nb, &ssa, 0);
    return 0;
}

/* Merge the na elements starting at pa with the nb elements starting at
 * ssb.keys = ssa.keys + na in a stable way, in-place.  na and nb must be > 0.
 * Must also have that ssa.keys[na-1] belongs at the end of the merge, and
 * should have na >= nb.  See listsort.txt for more info.  Return 0 if
 * successful, -1 if error.
 */
static Py_ssize_t
merge_hi(MergeState *ms, sortslice ssa, Py_ssize_t na,
         sortslice ssb, Py_ssize_t nb)
{
    Py_ssize_t k;
    sortslice dest, basea, baseb;
    int result = -1;            /* guilty until proved innocent */
    Py_ssize_t min_gallop;

    assert(ms && ssa.keys && ssb.keys && na > 0 && nb > 0);
    assert(ssa.keys + na == ssb.keys);
    if (MERGE_GETMEM(ms, nb) < 0)
        return -1;
    dest = ssb;
    sortslice_advance(&dest, nb-1);
    sortslice_memcpy(&ms->a, 0, &ssb, 0, nb);
    basea = ssa;
    baseb = ms->a;
    ssb.keys = ms->a.keys + nb - 1;
    if (ssb.values != NULL)
        ssb.values = ms->a.values + nb - 1;
    sortslice_advance(&ssa, na - 1);

    sortslice_copy_decr(&dest, &ssa);
    --na;
    if (na == 0)
        goto Succeed;
    if (nb == 1)
        goto CopyA;

    min_gallop = ms->min_gallop;
    for (;;) {
        Py_ssize_t acount = 0;          /* # of times A won in a row */
        Py_ssize_t bcount = 0;          /* # of times B won in a row */

        /* Do the straightforward thing until (if ever) one run
         * appears to win consistently.
         */
        for (;;) {
            assert(na > 0 && nb > 1);
            k = ISLT(ssb.keys[0], ssa.keys[0]);
            if (k) {
                if (k < 0)
                    goto Fail;
                sortslice_copy_decr(&dest, &ssa);
                ++acount;
                bcount = 0;
                --na;
                if (na == 0)
                    goto Succeed;
                if (acount >= min_gallop)
                    break;
            }
            else {
                sortslice_copy_decr(&dest, &ssb);
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
            k = gallop_right(ssb.keys[0], basea.keys, na, na-1);
            if (k < 0)
                goto Fail;
            k = na - k;
            acount = k;
            if (k) {
                sortslice_advance(&dest, -k);
                sortslice_advance(&ssa, -k);
                sortslice_memmove(&dest, 1, &ssa, 1, k);
                na -= k;
                if (na == 0)
                    goto Succeed;
            }
            sortslice_copy_decr(&dest, &ssb);
            --nb;
            if (nb == 1)
                goto CopyA;

            k = gallop_left(ssa.keys[0], baseb.keys, nb, nb-1);
            if (k < 0)
                goto Fail;
            k = nb - k;
            bcount = k;
            if (k) {
                sortslice_advance(&dest, -k);
                sortslice_advance(&ssb, -k);
                sortslice_memcpy(&dest, 1, &ssb, 1, k);
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
            sortslice_copy_decr(&dest, &ssa);
            --na;
            if (na == 0)
                goto Succeed;
        } while (acount >= MIN_GALLOP || bcount >= MIN_GALLOP);
        ++min_gallop;           /* penalize it for leaving galloping mode */
        ms->min_gallop = min_gallop;
    }
Succeed:
    result = 0;
Fail:
    if (nb)
        sortslice_memcpy(&dest, -(nb-1), &baseb, 0, nb);
    return result;
CopyA:
    assert(nb == 1 && na > 0);
    /* The first element of ssb belongs at the front of the merge. */
    sortslice_memmove(&dest, 1-na, &ssa, 1-na, na);
    sortslice_advance(&dest, -na);
    sortslice_advance(&ssa, -na);
    sortslice_copy(&dest, 0, &ssb, 0);
    return 0;
}

/* Merge the two runs at stack indices i and i+1.
 * Returns 0 on success, -1 on error.
 */
static Py_ssize_t
merge_at(MergeState *ms, Py_ssize_t i)
{
    sortslice ssa, ssb;
    Py_ssize_t na, nb;
    Py_ssize_t k;

    assert(ms != NULL);
    assert(ms->n >= 2);
    assert(i >= 0);
    assert(i == ms->n - 2 || i == ms->n - 3);

    ssa = ms->pending[i].base;
    na = ms->pending[i].len;
    ssb = ms->pending[i+1].base;
    nb = ms->pending[i+1].len;
    assert(na > 0 && nb > 0);
    assert(ssa.keys + na == ssb.keys);

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
    k = gallop_right(*ssb.keys, ssa.keys, na, 0);
    if (k < 0)
        return -1;
    sortslice_advance(&ssa, k);
    na -= k;
    if (na == 0)
        return 0;

    /* Where does a end in b?  Elements in b after that can be
     * ignored (already in place).
     */
    nb = gallop_left(ssa.keys[na-1], ssb.keys, nb, nb-1);
    if (nb <= 0)
        return nb;

    /* Merge what remains of the runs, using a temp array with
     * min(na, nb) elements.
     */
    if (na <= nb)
        return merge_lo(ms, ssa, na, ssb, nb);
    else
        return merge_hi(ms, ssa, na, ssb, nb);
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
        Py_ssize_t n = ms->n - 2;
        if ((n > 0 && p[n-1].len <= p[n].len + p[n+1].len) ||
            (n > 1 && p[n-2].len <= p[n-1].len + p[n].len)) {
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
        Py_ssize_t n = ms->n - 2;
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
static Py_ssize_t
merge_compute_minrun(Py_ssize_t n)
{
    Py_ssize_t r = 0;           /* becomes 1 if any 1 bits are shifted off */

    assert(n >= 0);
    while (n >= 64) {
        r |= n & 1;
        n >>= 1;
    }
    return n + r;
}

static void
reverse_sortslice(sortslice *s, Py_ssize_t n)
{
    reverse_slice(s->keys, &s->keys[n]);
    if (s->values != NULL)
        reverse_slice(s->values, &s->values[n]);
}

/* An adaptive, stable, natural mergesort.  See listsort.txt.
 * Returns Py_None on success, NULL on error.  Even in case of error, the
 * list will be some permutation of its input state (nothing is lost or
 * duplicated).
 */
static PyObject *
listsort(PyListObject *self, PyObject *args, PyObject *kwds)
{
    MergeState ms;
    Py_ssize_t nremaining;
    Py_ssize_t minrun;
    sortslice lo;
    Py_ssize_t saved_ob_size, saved_allocated;
    PyObject **saved_ob_item;
    PyObject **final_ob_item;
    PyObject *result = NULL;            /* guilty until proved innocent */
    int reverse = 0;
    PyObject *keyfunc = NULL;
    Py_ssize_t i;
    static char *kwlist[] = {"key", "reverse", 0};
    PyObject **keys;

    assert(self != NULL);
    assert (PyList_Check(self));
    if (args != NULL) {
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Oi:sort",
            kwlist, &keyfunc, &reverse))
            return NULL;
        if (Py_SIZE(args) > 0) {
            PyErr_SetString(PyExc_TypeError,
                "must use keyword argument for key function");
            return NULL;
        }
    }
    if (keyfunc == Py_None)
        keyfunc = NULL;

    /* The list is temporarily made empty, so that mutations performed
     * by comparison functions can't affect the slice of memory we're
     * sorting (allowing mutations during sorting is a core-dump
     * factory, since ob_item may change).
     */
    saved_ob_size = Py_SIZE(self);
    saved_ob_item = self->ob_item;
    saved_allocated = self->allocated;
    Py_SIZE(self) = 0;
    self->ob_item = NULL;
    self->allocated = -1; /* any operation will reset it to >= 0 */

    if (keyfunc == NULL) {
        keys = NULL;
        lo.keys = saved_ob_item;
        lo.values = NULL;
    }
    else {
        if (saved_ob_size < MERGESTATE_TEMP_SIZE/2)
            /* Leverage stack space we allocated but won't otherwise use */
            keys = &ms.temparray[saved_ob_size+1];
        else {
            keys = PyMem_MALLOC(sizeof(PyObject *) * saved_ob_size);
            if (keys == NULL) {
                PyErr_NoMemory();
                goto keyfunc_fail;
            }
        }

        for (i = 0; i < saved_ob_size ; i++) {
            keys[i] = PyObject_CallFunctionObjArgs(keyfunc, saved_ob_item[i],
                                                   NULL);
            if (keys[i] == NULL) {
                for (i=i-1 ; i>=0 ; i--)
                    Py_DECREF(keys[i]);
                if (saved_ob_size >= MERGESTATE_TEMP_SIZE/2)
                    PyMem_FREE(keys);
                goto keyfunc_fail;
            }
        }

        lo.keys = keys;
        lo.values = saved_ob_item;
    }

    merge_init(&ms, saved_ob_size, keys != NULL);

    nremaining = saved_ob_size;
    if (nremaining < 2)
        goto succeed;

    /* Reverse sort stability achieved by initially reversing the list,
    applying a stable forward sort, then reversing the final result. */
    if (reverse) {
        if (keys != NULL)
            reverse_slice(&keys[0], &keys[saved_ob_size]);
        reverse_slice(&saved_ob_item[0], &saved_ob_item[saved_ob_size]);
    }

    /* March over the array once, left to right, finding natural runs,
     * and extending short natural runs to minrun elements.
     */
    minrun = merge_compute_minrun(nremaining);
    do {
        int descending;
        Py_ssize_t n;

        /* Identify next run. */
        n = count_run(lo.keys, lo.keys + nremaining, &descending);
        if (n < 0)
            goto fail;
        if (descending)
            reverse_sortslice(&lo, n);
        /* If short, extend to min(minrun, nremaining). */
        if (n < minrun) {
            const Py_ssize_t force = nremaining <= minrun ?
                              nremaining : minrun;
            if (binarysort(lo, lo.keys + force, lo.keys + n) < 0)
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
        sortslice_advance(&lo, n);
        nremaining -= n;
    } while (nremaining);

    if (merge_force_collapse(&ms) < 0)
        goto fail;
    assert(ms.n == 1);
    assert(keys == NULL
           ? ms.pending[0].base.keys == saved_ob_item
           : ms.pending[0].base.keys == &keys[0]);
    assert(ms.pending[0].len == saved_ob_size);
    lo = ms.pending[0].base;

succeed:
    result = Py_None;
fail:
    if (keys != NULL) {
        for (i = 0; i < saved_ob_size; i++)
            Py_DECREF(keys[i]);
        if (saved_ob_size >= MERGESTATE_TEMP_SIZE/2)
            PyMem_FREE(keys);
    }

    if (self->allocated != -1 && result != NULL) {
        /* The user mucked with the list during the sort,
         * and we don't already have another error to report.
         */
        PyErr_SetString(PyExc_ValueError, "list modified during sort");
        result = NULL;
    }

    if (reverse && saved_ob_size > 1)
        reverse_slice(saved_ob_item, saved_ob_item + saved_ob_size);

    merge_freemem(&ms);

keyfunc_fail:
    final_ob_item = self->ob_item;
    i = Py_SIZE(self);
    Py_SIZE(self) = saved_ob_size;
    self->ob_item = saved_ob_item;
    self->allocated = saved_allocated;
    if (final_ob_item != NULL) {
        /* we cannot use list_clear() for this because it does not
           guarantee that the list is really empty when it returns */
        while (--i >= 0) {
            Py_XDECREF(final_ob_item[i]);
        }
        PyMem_FREE(final_ob_item);
    }
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
    v = listsort((PyListObject *)v, (PyObject *)NULL, (PyObject *)NULL);
    if (v == NULL)
        return -1;
    Py_DECREF(v);
    return 0;
}

static PyObject *
listreverse(PyListObject *self)
{
    if (Py_SIZE(self) > 1)
        reverse_slice(self->ob_item, self->ob_item + Py_SIZE(self));
    Py_RETURN_NONE;
}

int
PyList_Reverse(PyObject *v)
{
    PyListObject *self = (PyListObject *)v;

    if (v == NULL || !PyList_Check(v)) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (Py_SIZE(self) > 1)
        reverse_slice(self->ob_item, self->ob_item + Py_SIZE(self));
    return 0;
}

PyObject *
PyList_AsTuple(PyObject *v)
{
    PyObject *w;
    PyObject **p, **q;
    Py_ssize_t n;
    if (v == NULL || !PyList_Check(v)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    n = Py_SIZE(v);
    w = PyTuple_New(n);
    if (w == NULL)
        return NULL;
    p = ((PyTupleObject *)w)->ob_item;
    q = ((PyListObject *)v)->ob_item;
    while (--n >= 0) {
        Py_INCREF(*q);
        *p = *q;
        p++;
        q++;
    }
    return w;
}

static PyObject *
listindex(PyListObject *self, PyObject *args)
{
    Py_ssize_t i, start=0, stop=Py_SIZE(self);
    PyObject *v;

    if (!PyArg_ParseTuple(args, "O|O&O&:index", &v,
                                _PyEval_SliceIndexNotNone, &start,
                                _PyEval_SliceIndexNotNone, &stop))
        return NULL;
    if (start < 0) {
        start += Py_SIZE(self);
        if (start < 0)
            start = 0;
    }
    if (stop < 0) {
        stop += Py_SIZE(self);
        if (stop < 0)
            stop = 0;
    }
    for (i = start; i < stop && i < Py_SIZE(self); i++) {
        int cmp = PyObject_RichCompareBool(self->ob_item[i], v, Py_EQ);
        if (cmp > 0)
            return PyLong_FromSsize_t(i);
        else if (cmp < 0)
            return NULL;
    }
    PyErr_Format(PyExc_ValueError, "%R is not in list", v);
    return NULL;
}

static PyObject *
listcount(PyListObject *self, PyObject *v)
{
    Py_ssize_t count = 0;
    Py_ssize_t i;

    for (i = 0; i < Py_SIZE(self); i++) {
        int cmp = PyObject_RichCompareBool(self->ob_item[i], v, Py_EQ);
        if (cmp > 0)
            count++;
        else if (cmp < 0)
            return NULL;
    }
    return PyLong_FromSsize_t(count);
}

static PyObject *
listremove(PyListObject *self, PyObject *v)
{
    Py_ssize_t i;

    for (i = 0; i < Py_SIZE(self); i++) {
        int cmp = PyObject_RichCompareBool(self->ob_item[i], v, Py_EQ);
        if (cmp > 0) {
            if (list_ass_slice(self, i, i+1,
                               (PyObject *)NULL) == 0)
                Py_RETURN_NONE;
            return NULL;
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
    Py_ssize_t i;

    for (i = Py_SIZE(o); --i >= 0; )
        Py_VISIT(o->ob_item[i]);
    return 0;
}

static PyObject *
list_richcompare(PyObject *v, PyObject *w, int op)
{
    PyListObject *vl, *wl;
    Py_ssize_t i;

    if (!PyList_Check(v) || !PyList_Check(w))
        Py_RETURN_NOTIMPLEMENTED;

    vl = (PyListObject *)v;
    wl = (PyListObject *)w;

    if (Py_SIZE(vl) != Py_SIZE(wl) && (op == Py_EQ || op == Py_NE)) {
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
    for (i = 0; i < Py_SIZE(vl) && i < Py_SIZE(wl); i++) {
        int k = PyObject_RichCompareBool(vl->ob_item[i],
                                         wl->ob_item[i], Py_EQ);
        if (k < 0)
            return NULL;
        if (!k)
            break;
    }

    if (i >= Py_SIZE(vl) || i >= Py_SIZE(wl)) {
        /* No more items to compare -- compare sizes */
        Py_ssize_t vs = Py_SIZE(vl);
        Py_ssize_t ws = Py_SIZE(wl);
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

static int
list_init(PyListObject *self, PyObject *args, PyObject *kw)
{
    PyObject *arg = NULL;
    static char *kwlist[] = {"sequence", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O:list", kwlist, &arg))
        return -1;

    /* Verify list invariants established by PyType_GenericAlloc() */
    assert(0 <= Py_SIZE(self));
    assert(Py_SIZE(self) <= self->allocated || self->allocated == -1);
    assert(self->ob_item != NULL ||
           self->allocated == 0 || self->allocated == -1);

    /* Empty previous contents */
    if (self->ob_item != NULL) {
        (void)list_clear(self);
    }
    if (arg != NULL) {
        PyObject *rv = listextend(self, arg);
        if (rv == NULL)
            return -1;
        Py_DECREF(rv);
    }
    return 0;
}

static PyObject *
list_sizeof(PyListObject *self)
{
    Py_ssize_t res;

    res = _PyObject_SIZE(Py_TYPE(self)) + self->allocated * sizeof(void*);
    return PyLong_FromSsize_t(res);
}

static PyObject *list_iter(PyObject *seq);
static PyObject *list_reversed(PyListObject* seq, PyObject* unused);

PyDoc_STRVAR(getitem_doc,
"x.__getitem__(y) <==> x[y]");
PyDoc_STRVAR(reversed_doc,
"L.__reversed__() -- return a reverse iterator over the list");
PyDoc_STRVAR(sizeof_doc,
"L.__sizeof__() -- size of L in memory, in bytes");
PyDoc_STRVAR(clear_doc,
"L.clear() -> None -- remove all items from L");
PyDoc_STRVAR(copy_doc,
"L.copy() -> list -- a shallow copy of L");
PyDoc_STRVAR(append_doc,
"L.append(object) -> None -- append object to end");
PyDoc_STRVAR(extend_doc,
"L.extend(iterable) -> None -- extend list by appending elements from the iterable");
PyDoc_STRVAR(insert_doc,
"L.insert(index, object) -- insert object before index");
PyDoc_STRVAR(pop_doc,
"L.pop([index]) -> item -- remove and return item at index (default last).\n"
"Raises IndexError if list is empty or index is out of range.");
PyDoc_STRVAR(remove_doc,
"L.remove(value) -> None -- remove first occurrence of value.\n"
"Raises ValueError if the value is not present.");
PyDoc_STRVAR(index_doc,
"L.index(value, [start, [stop]]) -> integer -- return first index of value.\n"
"Raises ValueError if the value is not present.");
PyDoc_STRVAR(count_doc,
"L.count(value) -> integer -- return number of occurrences of value");
PyDoc_STRVAR(reverse_doc,
"L.reverse() -- reverse *IN PLACE*");
PyDoc_STRVAR(sort_doc,
"L.sort(key=None, reverse=False) -> None -- stable sort *IN PLACE*");

static PyObject *list_subscript(PyListObject*, PyObject*);

static PyMethodDef list_methods[] = {
    {"__getitem__", (PyCFunction)list_subscript, METH_O|METH_COEXIST, getitem_doc},
    {"__reversed__",(PyCFunction)list_reversed, METH_NOARGS, reversed_doc},
    {"__sizeof__",  (PyCFunction)list_sizeof, METH_NOARGS, sizeof_doc},
    {"clear",           (PyCFunction)listclear,   METH_NOARGS, clear_doc},
    {"copy",            (PyCFunction)listcopy,   METH_NOARGS, copy_doc},
    {"append",          (PyCFunction)listappend,  METH_O, append_doc},
    {"insert",          (PyCFunction)listinsert,  METH_VARARGS, insert_doc},
    {"extend",          (PyCFunction)listextend,  METH_O, extend_doc},
    {"pop",             (PyCFunction)listpop,     METH_VARARGS, pop_doc},
    {"remove",          (PyCFunction)listremove,  METH_O, remove_doc},
    {"index",           (PyCFunction)listindex,   METH_VARARGS, index_doc},
    {"count",           (PyCFunction)listcount,   METH_O, count_doc},
    {"reverse",         (PyCFunction)listreverse, METH_NOARGS, reverse_doc},
    {"sort",            (PyCFunction)listsort,    METH_VARARGS | METH_KEYWORDS, sort_doc},
    {NULL,              NULL}           /* sentinel */
};

static PySequenceMethods list_as_sequence = {
    (lenfunc)list_length,                       /* sq_length */
    (binaryfunc)list_concat,                    /* sq_concat */
    (ssizeargfunc)list_repeat,                  /* sq_repeat */
    (ssizeargfunc)list_item,                    /* sq_item */
    0,                                          /* sq_slice */
    (ssizeobjargproc)list_ass_item,             /* sq_ass_item */
    0,                                          /* sq_ass_slice */
    (objobjproc)list_contains,                  /* sq_contains */
    (binaryfunc)list_inplace_concat,            /* sq_inplace_concat */
    (ssizeargfunc)list_inplace_repeat,          /* sq_inplace_repeat */
};

PyDoc_STRVAR(list_doc,
"list() -> new empty list\n"
"list(iterable) -> new list initialized from iterable's items");

static PyObject *
list_subscript(PyListObject* self, PyObject* item)
{
    if (PyIndex_Check(item)) {
        Py_ssize_t i;
        i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return NULL;
        if (i < 0)
            i += PyList_GET_SIZE(self);
        return list_item(self, i);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelength, cur, i;
        PyObject* result;
        PyObject* it;
        PyObject **src, **dest;

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return NULL;
        }
        slicelength = PySlice_AdjustIndices(Py_SIZE(self), &start, &stop,
                                            step);

        if (slicelength <= 0) {
            return PyList_New(0);
        }
        else if (step == 1) {
            return list_slice(self, start, stop);
        }
        else {
            result = PyList_New(slicelength);
            if (!result) return NULL;

            src = self->ob_item;
            dest = ((PyListObject *)result)->ob_item;
            for (cur = start, i = 0; i < slicelength;
                 cur += (size_t)step, i++) {
                it = src[cur];
                Py_INCREF(it);
                dest[i] = it;
            }

            return result;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "list indices must be integers or slices, not %.200s",
                     item->ob_type->tp_name);
        return NULL;
    }
}

static int
list_ass_subscript(PyListObject* self, PyObject* item, PyObject* value)
{
    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return -1;
        if (i < 0)
            i += PyList_GET_SIZE(self);
        return list_ass_item(self, i, value);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelength;

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return -1;
        }
        slicelength = PySlice_AdjustIndices(Py_SIZE(self), &start, &stop,
                                            step);

        if (step == 1)
            return list_ass_slice(self, start, stop, value);

        /* Make sure s[5:2] = [..] inserts at the right place:
           before 5, not before 2. */
        if ((step < 0 && start < stop) ||
            (step > 0 && start > stop))
            stop = start;

        if (value == NULL) {
            /* delete slice */
            PyObject **garbage;
            size_t cur;
            Py_ssize_t i;
            int res;

            if (slicelength <= 0)
                return 0;

            if (step < 0) {
                stop = start + 1;
                start = stop + step*(slicelength - 1) - 1;
                step = -step;
            }

            garbage = (PyObject**)
                PyMem_MALLOC(slicelength*sizeof(PyObject*));
            if (!garbage) {
                PyErr_NoMemory();
                return -1;
            }

            /* drawing pictures might help understand these for
               loops. Basically, we memmove the parts of the
               list that are *not* part of the slice: step-1
               items for each item that is part of the slice,
               and then tail end of the list that was not
               covered by the slice */
            for (cur = start, i = 0;
                 cur < (size_t)stop;
                 cur += step, i++) {
                Py_ssize_t lim = step - 1;

                garbage[i] = PyList_GET_ITEM(self, cur);

                if (cur + step >= (size_t)Py_SIZE(self)) {
                    lim = Py_SIZE(self) - cur - 1;
                }

                memmove(self->ob_item + cur - i,
                    self->ob_item + cur + 1,
                    lim * sizeof(PyObject *));
            }
            cur = start + (size_t)slicelength * step;
            if (cur < (size_t)Py_SIZE(self)) {
                memmove(self->ob_item + cur - slicelength,
                    self->ob_item + cur,
                    (Py_SIZE(self) - cur) *
                     sizeof(PyObject *));
            }

            Py_SIZE(self) -= slicelength;
            res = list_resize(self, Py_SIZE(self));

            for (i = 0; i < slicelength; i++) {
                Py_DECREF(garbage[i]);
            }
            PyMem_FREE(garbage);

            return res;
        }
        else {
            /* assign slice */
            PyObject *ins, *seq;
            PyObject **garbage, **seqitems, **selfitems;
            Py_ssize_t cur, i;

            /* protect against a[::-1] = a */
            if (self == (PyListObject*)value) {
                seq = list_slice((PyListObject*)value, 0,
                                   PyList_GET_SIZE(value));
            }
            else {
                seq = PySequence_Fast(value,
                                      "must assign iterable "
                                      "to extended slice");
            }
            if (!seq)
                return -1;

            if (PySequence_Fast_GET_SIZE(seq) != slicelength) {
                PyErr_Format(PyExc_ValueError,
                    "attempt to assign sequence of "
                    "size %zd to extended slice of "
                    "size %zd",
                         PySequence_Fast_GET_SIZE(seq),
                         slicelength);
                Py_DECREF(seq);
                return -1;
            }

            if (!slicelength) {
                Py_DECREF(seq);
                return 0;
            }

            garbage = (PyObject**)
                PyMem_MALLOC(slicelength*sizeof(PyObject*));
            if (!garbage) {
                Py_DECREF(seq);
                PyErr_NoMemory();
                return -1;
            }

            selfitems = self->ob_item;
            seqitems = PySequence_Fast_ITEMS(seq);
            for (cur = start, i = 0; i < slicelength;
                 cur += (size_t)step, i++) {
                garbage[i] = selfitems[cur];
                ins = seqitems[i];
                Py_INCREF(ins);
                selfitems[cur] = ins;
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
        PyErr_Format(PyExc_TypeError,
                     "list indices must be integers or slices, not %.200s",
                     item->ob_type->tp_name);
        return -1;
    }
}

static PyMappingMethods list_as_mapping = {
    (lenfunc)list_length,
    (binaryfunc)list_subscript,
    (objobjargproc)list_ass_subscript
};

PyTypeObject PyList_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "list",
    sizeof(PyListObject),
    0,
    (destructor)list_dealloc,                   /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)list_repr,                        /* tp_repr */
    0,                                          /* tp_as_number */
    &list_as_sequence,                          /* tp_as_sequence */
    &list_as_mapping,                           /* tp_as_mapping */
    PyObject_HashNotImplemented,                /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE | Py_TPFLAGS_LIST_SUBCLASS,         /* tp_flags */
    list_doc,                                   /* tp_doc */
    (traverseproc)list_traverse,                /* tp_traverse */
    (inquiry)list_clear,                        /* tp_clear */
    list_richcompare,                           /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    list_iter,                                  /* tp_iter */
    0,                                          /* tp_iternext */
    list_methods,                               /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc)list_init,                        /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    PyType_GenericNew,                          /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
};


/*********************** List Iterator **************************/

typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;
    PyListObject *it_seq; /* Set to NULL when iterator is exhausted */
} listiterobject;

static PyObject *list_iter(PyObject *);
static void listiter_dealloc(listiterobject *);
static int listiter_traverse(listiterobject *, visitproc, void *);
static PyObject *listiter_next(listiterobject *);
static PyObject *listiter_len(listiterobject *);
static PyObject *listiter_reduce_general(void *_it, int forward);
static PyObject *listiter_reduce(listiterobject *);
static PyObject *listiter_setstate(listiterobject *, PyObject *state);

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");
PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");
PyDoc_STRVAR(setstate_doc, "Set state information for unpickling.");

static PyMethodDef listiter_methods[] = {
    {"__length_hint__", (PyCFunction)listiter_len, METH_NOARGS, length_hint_doc},
    {"__reduce__", (PyCFunction)listiter_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", (PyCFunction)listiter_setstate, METH_O, setstate_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyListIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "list_iterator",                            /* tp_name */
    sizeof(listiterobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)listiter_dealloc,               /* tp_dealloc */
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
    (traverseproc)listiter_traverse,            /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)listiter_next,                /* tp_iternext */
    listiter_methods,                           /* tp_methods */
    0,                                          /* tp_members */
};


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
    Py_VISIT(it->it_seq);
    return 0;
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

    it->it_seq = NULL;
    Py_DECREF(seq);
    return NULL;
}

static PyObject *
listiter_len(listiterobject *it)
{
    Py_ssize_t len;
    if (it->it_seq) {
        len = PyList_GET_SIZE(it->it_seq) - it->it_index;
        if (len >= 0)
            return PyLong_FromSsize_t(len);
    }
    return PyLong_FromLong(0);
}

static PyObject *
listiter_reduce(listiterobject *it)
{
    return listiter_reduce_general(it, 1);
}

static PyObject *
listiter_setstate(listiterobject *it, PyObject *state)
{
    Py_ssize_t index = PyLong_AsSsize_t(state);
    if (index == -1 && PyErr_Occurred())
        return NULL;
    if (it->it_seq != NULL) {
        if (index < 0)
            index = 0;
        else if (index > PyList_GET_SIZE(it->it_seq))
            index = PyList_GET_SIZE(it->it_seq); /* iterator exhausted */
        it->it_index = index;
    }
    Py_RETURN_NONE;
}

/*********************** List Reverse Iterator **************************/

typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;
    PyListObject *it_seq; /* Set to NULL when iterator is exhausted */
} listreviterobject;

static PyObject *list_reversed(PyListObject *, PyObject *);
static void listreviter_dealloc(listreviterobject *);
static int listreviter_traverse(listreviterobject *, visitproc, void *);
static PyObject *listreviter_next(listreviterobject *);
static PyObject *listreviter_len(listreviterobject *);
static PyObject *listreviter_reduce(listreviterobject *);
static PyObject *listreviter_setstate(listreviterobject *, PyObject *);

static PyMethodDef listreviter_methods[] = {
    {"__length_hint__", (PyCFunction)listreviter_len, METH_NOARGS, length_hint_doc},
    {"__reduce__", (PyCFunction)listreviter_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", (PyCFunction)listreviter_setstate, METH_O, setstate_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyListRevIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "list_reverseiterator",                     /* tp_name */
    sizeof(listreviterobject),                  /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)listreviter_dealloc,            /* tp_dealloc */
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
    (traverseproc)listreviter_traverse,         /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)listreviter_next,             /* tp_iternext */
    listreviter_methods,                /* tp_methods */
    0,
};

static PyObject *
list_reversed(PyListObject *seq, PyObject *unused)
{
    listreviterobject *it;

    it = PyObject_GC_New(listreviterobject, &PyListRevIter_Type);
    if (it == NULL)
        return NULL;
    assert(PyList_Check(seq));
    it->it_index = PyList_GET_SIZE(seq) - 1;
    Py_INCREF(seq);
    it->it_seq = seq;
    PyObject_GC_Track(it);
    return (PyObject *)it;
}

static void
listreviter_dealloc(listreviterobject *it)
{
    PyObject_GC_UnTrack(it);
    Py_XDECREF(it->it_seq);
    PyObject_GC_Del(it);
}

static int
listreviter_traverse(listreviterobject *it, visitproc visit, void *arg)
{
    Py_VISIT(it->it_seq);
    return 0;
}

static PyObject *
listreviter_next(listreviterobject *it)
{
    PyObject *item;
    Py_ssize_t index;
    PyListObject *seq;

    assert(it != NULL);
    seq = it->it_seq;
    if (seq == NULL) {
        return NULL;
    }
    assert(PyList_Check(seq));

    index = it->it_index;
    if (index>=0 && index < PyList_GET_SIZE(seq)) {
        item = PyList_GET_ITEM(seq, index);
        it->it_index--;
        Py_INCREF(item);
        return item;
    }
    it->it_index = -1;
    it->it_seq = NULL;
    Py_DECREF(seq);
    return NULL;
}

static PyObject *
listreviter_len(listreviterobject *it)
{
    Py_ssize_t len = it->it_index + 1;
    if (it->it_seq == NULL || PyList_GET_SIZE(it->it_seq) < len)
        len = 0;
    return PyLong_FromSsize_t(len);
}

static PyObject *
listreviter_reduce(listreviterobject *it)
{
    return listiter_reduce_general(it, 0);
}

static PyObject *
listreviter_setstate(listreviterobject *it, PyObject *state)
{
    Py_ssize_t index = PyLong_AsSsize_t(state);
    if (index == -1 && PyErr_Occurred())
        return NULL;
    if (it->it_seq != NULL) {
        if (index < -1)
            index = -1;
        else if (index > PyList_GET_SIZE(it->it_seq) - 1)
            index = PyList_GET_SIZE(it->it_seq) - 1;
        it->it_index = index;
    }
    Py_RETURN_NONE;
}

/* common pickling support */

static PyObject *
listiter_reduce_general(void *_it, int forward)
{
    PyObject *list;

    /* the objects are not the same, index is of different types! */
    if (forward) {
        listiterobject *it = (listiterobject *)_it;
        if (it->it_seq)
            return Py_BuildValue("N(O)n", _PyObject_GetBuiltin("iter"),
                                 it->it_seq, it->it_index);
    } else {
        listreviterobject *it = (listreviterobject *)_it;
        if (it->it_seq)
            return Py_BuildValue("N(O)n", _PyObject_GetBuiltin("reversed"),
                                 it->it_seq, it->it_index);
    }
    /* empty iterator, create an empty list */
    list = PyList_New(0);
    if (list == NULL)
        return NULL;
    return Py_BuildValue("N(N)", _PyObject_GetBuiltin("iter"), list);
}
