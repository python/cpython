/* Array object implementation */

/* An array is a uniform list -- all items have the same type.
   The item type is restricted to simple C types like int or float */

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"

#ifdef STDC_HEADERS
#include <stddef.h>
#else /* !STDC_HEADERS */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>          /* For size_t */
#endif /* HAVE_SYS_TYPES_H */
#endif /* !STDC_HEADERS */

struct arrayobject; /* Forward */

/* All possible arraydescr values are defined in the vector "descriptors"
 * below.  That's defined later because the appropriate get and set
 * functions aren't visible yet.
 */
struct arraydescr {
    Py_UNICODE typecode;
    int itemsize;
    PyObject * (*getitem)(struct arrayobject *, Py_ssize_t);
    int (*setitem)(struct arrayobject *, Py_ssize_t, PyObject *);
    char *formats;
    int is_integer_type;
    int is_signed;
};

typedef struct arrayobject {
    PyObject_VAR_HEAD
    char *ob_item;
    Py_ssize_t allocated;
    struct arraydescr *ob_descr;
    PyObject *weakreflist; /* List of weak references */
    int ob_exports;  /* Number of exported buffers */
} arrayobject;

static PyTypeObject Arraytype;

#define array_Check(op) PyObject_TypeCheck(op, &Arraytype)
#define array_CheckExact(op) (Py_TYPE(op) == &Arraytype)

static int
array_resize(arrayobject *self, Py_ssize_t newsize)
{
    char *items;
    size_t _new_size;

    if (self->ob_exports > 0 && newsize != Py_SIZE(self)) {
        PyErr_SetString(PyExc_BufferError,
            "cannot resize an array that is exporting buffers");
        return -1;
    }

    /* Bypass realloc() when a previous overallocation is large enough
       to accommodate the newsize.  If the newsize is 16 smaller than the
       current size, then proceed with the realloc() to shrink the array.
    */

    if (self->allocated >= newsize &&
        Py_SIZE(self) < newsize + 16 &&
        self->ob_item != NULL) {
        Py_SIZE(self) = newsize;
        return 0;
    }

    if (newsize == 0) {
        PyMem_FREE(self->ob_item);
        self->ob_item = NULL;
        Py_SIZE(self) = 0;
        self->allocated = 0;
        return 0;
    }

    /* This over-allocates proportional to the array size, making room
     * for additional growth.  The over-allocation is mild, but is
     * enough to give linear-time amortized behavior over a long
     * sequence of appends() in the presence of a poorly-performing
     * system realloc().
     * The growth pattern is:  0, 4, 8, 16, 25, 34, 46, 56, 67, 79, ...
     * Note, the pattern starts out the same as for lists but then
     * grows at a smaller rate so that larger arrays only overallocate
     * by about 1/16th -- this is done because arrays are presumed to be more
     * memory critical.
     */

    _new_size = (newsize >> 4) + (Py_SIZE(self) < 8 ? 3 : 7) + newsize;
    items = self->ob_item;
    /* XXX The following multiplication and division does not optimize away
       like it does for lists since the size is not known at compile time */
    if (_new_size <= ((~(size_t)0) / self->ob_descr->itemsize))
        PyMem_RESIZE(items, char, (_new_size * self->ob_descr->itemsize));
    else
        items = NULL;
    if (items == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    self->ob_item = items;
    Py_SIZE(self) = newsize;
    self->allocated = _new_size;
    return 0;
}

/****************************************************************************
Get and Set functions for each type.
A Get function takes an arrayobject* and an integer index, returning the
array value at that index wrapped in an appropriate PyObject*.
A Set function takes an arrayobject, integer index, and PyObject*; sets
the array value at that index to the raw C data extracted from the PyObject*,
and returns 0 if successful, else nonzero on failure (PyObject* not of an
appropriate type or value).
Note that the basic Get and Set functions do NOT check that the index is
in bounds; that's the responsibility of the caller.
****************************************************************************/

static PyObject *
b_getitem(arrayobject *ap, Py_ssize_t i)
{
    long x = ((char *)ap->ob_item)[i];
    if (x >= 128)
        x -= 256;
    return PyLong_FromLong(x);
}

static int
b_setitem(arrayobject *ap, Py_ssize_t i, PyObject *v)
{
    short x;
    /* PyArg_Parse's 'b' formatter is for an unsigned char, therefore
       must use the next size up that is signed ('h') and manually do
       the overflow checking */
    if (!PyArg_Parse(v, "h;array item must be integer", &x))
        return -1;
    else if (x < -128) {
        PyErr_SetString(PyExc_OverflowError,
            "signed char is less than minimum");
        return -1;
    }
    else if (x > 127) {
        PyErr_SetString(PyExc_OverflowError,
            "signed char is greater than maximum");
        return -1;
    }
    if (i >= 0)
        ((char *)ap->ob_item)[i] = (char)x;
    return 0;
}

static PyObject *
BB_getitem(arrayobject *ap, Py_ssize_t i)
{
    long x = ((unsigned char *)ap->ob_item)[i];
    return PyLong_FromLong(x);
}

static int
BB_setitem(arrayobject *ap, Py_ssize_t i, PyObject *v)
{
    unsigned char x;
    /* 'B' == unsigned char, maps to PyArg_Parse's 'b' formatter */
    if (!PyArg_Parse(v, "b;array item must be integer", &x))
        return -1;
    if (i >= 0)
        ((char *)ap->ob_item)[i] = x;
    return 0;
}

static PyObject *
u_getitem(arrayobject *ap, Py_ssize_t i)
{
    return PyUnicode_FromUnicode(&((Py_UNICODE *) ap->ob_item)[i], 1);
}

static int
u_setitem(arrayobject *ap, Py_ssize_t i, PyObject *v)
{
    Py_UNICODE *p;
    Py_ssize_t len;

    if (!PyArg_Parse(v, "u#;array item must be unicode character", &p, &len))
        return -1;
    if (len != 1) {
        PyErr_SetString(PyExc_TypeError,
                        "array item must be unicode character");
        return -1;
    }
    if (i >= 0)
        ((Py_UNICODE *)ap->ob_item)[i] = p[0];
    return 0;
}


static PyObject *
h_getitem(arrayobject *ap, Py_ssize_t i)
{
    return PyLong_FromLong((long) ((short *)ap->ob_item)[i]);
}


static int
h_setitem(arrayobject *ap, Py_ssize_t i, PyObject *v)
{
    short x;
    /* 'h' == signed short, maps to PyArg_Parse's 'h' formatter */
    if (!PyArg_Parse(v, "h;array item must be integer", &x))
        return -1;
    if (i >= 0)
                 ((short *)ap->ob_item)[i] = x;
    return 0;
}

static PyObject *
HH_getitem(arrayobject *ap, Py_ssize_t i)
{
    return PyLong_FromLong((long) ((unsigned short *)ap->ob_item)[i]);
}

static int
HH_setitem(arrayobject *ap, Py_ssize_t i, PyObject *v)
{
    int x;
    /* PyArg_Parse's 'h' formatter is for a signed short, therefore
       must use the next size up and manually do the overflow checking */
    if (!PyArg_Parse(v, "i;array item must be integer", &x))
        return -1;
    else if (x < 0) {
        PyErr_SetString(PyExc_OverflowError,
            "unsigned short is less than minimum");
        return -1;
    }
    else if (x > USHRT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
            "unsigned short is greater than maximum");
        return -1;
    }
    if (i >= 0)
        ((short *)ap->ob_item)[i] = (short)x;
    return 0;
}

static PyObject *
i_getitem(arrayobject *ap, Py_ssize_t i)
{
    return PyLong_FromLong((long) ((int *)ap->ob_item)[i]);
}

static int
i_setitem(arrayobject *ap, Py_ssize_t i, PyObject *v)
{
    int x;
    /* 'i' == signed int, maps to PyArg_Parse's 'i' formatter */
    if (!PyArg_Parse(v, "i;array item must be integer", &x))
        return -1;
    if (i >= 0)
                 ((int *)ap->ob_item)[i] = x;
    return 0;
}

static PyObject *
II_getitem(arrayobject *ap, Py_ssize_t i)
{
    return PyLong_FromUnsignedLong(
        (unsigned long) ((unsigned int *)ap->ob_item)[i]);
}

static int
II_setitem(arrayobject *ap, Py_ssize_t i, PyObject *v)
{
    unsigned long x;
    if (PyLong_Check(v)) {
        x = PyLong_AsUnsignedLong(v);
        if (x == (unsigned long) -1 && PyErr_Occurred())
            return -1;
    }
    else {
        long y;
        if (!PyArg_Parse(v, "l;array item must be integer", &y))
            return -1;
        if (y < 0) {
            PyErr_SetString(PyExc_OverflowError,
                "unsigned int is less than minimum");
            return -1;
        }
        x = (unsigned long)y;

    }
    if (x > UINT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
            "unsigned int is greater than maximum");
        return -1;
    }

    if (i >= 0)
        ((unsigned int *)ap->ob_item)[i] = (unsigned int)x;
    return 0;
}

static PyObject *
l_getitem(arrayobject *ap, Py_ssize_t i)
{
    return PyLong_FromLong(((long *)ap->ob_item)[i]);
}

static int
l_setitem(arrayobject *ap, Py_ssize_t i, PyObject *v)
{
    long x;
    if (!PyArg_Parse(v, "l;array item must be integer", &x))
        return -1;
    if (i >= 0)
                 ((long *)ap->ob_item)[i] = x;
    return 0;
}

static PyObject *
LL_getitem(arrayobject *ap, Py_ssize_t i)
{
    return PyLong_FromUnsignedLong(((unsigned long *)ap->ob_item)[i]);
}

static int
LL_setitem(arrayobject *ap, Py_ssize_t i, PyObject *v)
{
    unsigned long x;
    if (PyLong_Check(v)) {
        x = PyLong_AsUnsignedLong(v);
        if (x == (unsigned long) -1 && PyErr_Occurred())
            return -1;
    }
    else {
        long y;
        if (!PyArg_Parse(v, "l;array item must be integer", &y))
            return -1;
        if (y < 0) {
            PyErr_SetString(PyExc_OverflowError,
                "unsigned long is less than minimum");
            return -1;
        }
        x = (unsigned long)y;

    }
    if (x > ULONG_MAX) {
        PyErr_SetString(PyExc_OverflowError,
            "unsigned long is greater than maximum");
        return -1;
    }

    if (i >= 0)
        ((unsigned long *)ap->ob_item)[i] = x;
    return 0;
}

static PyObject *
f_getitem(arrayobject *ap, Py_ssize_t i)
{
    return PyFloat_FromDouble((double) ((float *)ap->ob_item)[i]);
}

static int
f_setitem(arrayobject *ap, Py_ssize_t i, PyObject *v)
{
    float x;
    if (!PyArg_Parse(v, "f;array item must be float", &x))
        return -1;
    if (i >= 0)
                 ((float *)ap->ob_item)[i] = x;
    return 0;
}

static PyObject *
d_getitem(arrayobject *ap, Py_ssize_t i)
{
    return PyFloat_FromDouble(((double *)ap->ob_item)[i]);
}

static int
d_setitem(arrayobject *ap, Py_ssize_t i, PyObject *v)
{
    double x;
    if (!PyArg_Parse(v, "d;array item must be float", &x))
        return -1;
    if (i >= 0)
                 ((double *)ap->ob_item)[i] = x;
    return 0;
}


/* Description of types.
 *
 * Don't forget to update typecode_to_mformat_code() if you add a new
 * typecode.
 */
static struct arraydescr descriptors[] = {
    {'b', 1, b_getitem, b_setitem, "b", 1, 1},
    {'B', 1, BB_getitem, BB_setitem, "B", 1, 0},
    {'u', sizeof(Py_UNICODE), u_getitem, u_setitem, "u", 0, 0},
    {'h', sizeof(short), h_getitem, h_setitem, "h", 1, 1},
    {'H', sizeof(short), HH_getitem, HH_setitem, "H", 1, 0},
    {'i', sizeof(int), i_getitem, i_setitem, "i", 1, 1},
    {'I', sizeof(int), II_getitem, II_setitem, "I", 1, 0},
    {'l', sizeof(long), l_getitem, l_setitem, "l", 1, 1},
    {'L', sizeof(long), LL_getitem, LL_setitem, "L", 1, 0},
    {'f', sizeof(float), f_getitem, f_setitem, "f", 0, 0},
    {'d', sizeof(double), d_getitem, d_setitem, "d", 0, 0},
    {'\0', 0, 0, 0, 0, 0, 0} /* Sentinel */
};

/****************************************************************************
Implementations of array object methods.
****************************************************************************/

static PyObject *
newarrayobject(PyTypeObject *type, Py_ssize_t size, struct arraydescr *descr)
{
    arrayobject *op;
    size_t nbytes;

    if (size < 0) {
        PyErr_BadInternalCall();
        return NULL;
    }

    nbytes = size * descr->itemsize;
    /* Check for overflow */
    if (nbytes / descr->itemsize != (size_t)size) {
        return PyErr_NoMemory();
    }
    op = (arrayobject *) type->tp_alloc(type, 0);
    if (op == NULL) {
        return NULL;
    }
    op->ob_descr = descr;
    op->allocated = size;
    op->weakreflist = NULL;
    Py_SIZE(op) = size;
    if (size <= 0) {
        op->ob_item = NULL;
    }
    else {
        op->ob_item = PyMem_NEW(char, nbytes);
        if (op->ob_item == NULL) {
            Py_DECREF(op);
            return PyErr_NoMemory();
        }
    }
    op->ob_exports = 0;
    return (PyObject *) op;
}

static PyObject *
getarrayitem(PyObject *op, Py_ssize_t i)
{
    register arrayobject *ap;
    assert(array_Check(op));
    ap = (arrayobject *)op;
    assert(i>=0 && i<Py_SIZE(ap));
    return (*ap->ob_descr->getitem)(ap, i);
}

static int
ins1(arrayobject *self, Py_ssize_t where, PyObject *v)
{
    char *items;
    Py_ssize_t n = Py_SIZE(self);
    if (v == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }
    if ((*self->ob_descr->setitem)(self, -1, v) < 0)
        return -1;

    if (array_resize(self, n+1) == -1)
        return -1;
    items = self->ob_item;
    if (where < 0) {
        where += n;
        if (where < 0)
            where = 0;
    }
    if (where > n)
        where = n;
    /* appends don't need to call memmove() */
    if (where != n)
        memmove(items + (where+1)*self->ob_descr->itemsize,
            items + where*self->ob_descr->itemsize,
            (n-where)*self->ob_descr->itemsize);
    return (*self->ob_descr->setitem)(self, where, v);
}

/* Methods */

static void
array_dealloc(arrayobject *op)
{
    if (op->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) op);
    if (op->ob_item != NULL)
        PyMem_DEL(op->ob_item);
    Py_TYPE(op)->tp_free((PyObject *)op);
}

static PyObject *
array_richcompare(PyObject *v, PyObject *w, int op)
{
    arrayobject *va, *wa;
    PyObject *vi = NULL;
    PyObject *wi = NULL;
    Py_ssize_t i, k;
    PyObject *res;

    if (!array_Check(v) || !array_Check(w)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    va = (arrayobject *)v;
    wa = (arrayobject *)w;

    if (Py_SIZE(va) != Py_SIZE(wa) && (op == Py_EQ || op == Py_NE)) {
        /* Shortcut: if the lengths differ, the arrays differ */
        if (op == Py_EQ)
            res = Py_False;
        else
            res = Py_True;
        Py_INCREF(res);
        return res;
    }

    /* Search for the first index where items are different */
    k = 1;
    for (i = 0; i < Py_SIZE(va) && i < Py_SIZE(wa); i++) {
        vi = getarrayitem(v, i);
        wi = getarrayitem(w, i);
        if (vi == NULL || wi == NULL) {
            Py_XDECREF(vi);
            Py_XDECREF(wi);
            return NULL;
        }
        k = PyObject_RichCompareBool(vi, wi, Py_EQ);
        if (k == 0)
            break; /* Keeping vi and wi alive! */
        Py_DECREF(vi);
        Py_DECREF(wi);
        if (k < 0)
            return NULL;
    }

    if (k) {
        /* No more items to compare -- compare sizes */
        Py_ssize_t vs = Py_SIZE(va);
        Py_ssize_t ws = Py_SIZE(wa);
        int cmp;
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

    /* We have an item that differs.  First, shortcuts for EQ/NE */
    if (op == Py_EQ) {
        Py_INCREF(Py_False);
        res = Py_False;
    }
    else if (op == Py_NE) {
        Py_INCREF(Py_True);
        res = Py_True;
    }
    else {
        /* Compare the final item again using the proper operator */
        res = PyObject_RichCompare(vi, wi, op);
    }
    Py_DECREF(vi);
    Py_DECREF(wi);
    return res;
}

static Py_ssize_t
array_length(arrayobject *a)
{
    return Py_SIZE(a);
}

static PyObject *
array_item(arrayobject *a, Py_ssize_t i)
{
    if (i < 0 || i >= Py_SIZE(a)) {
        PyErr_SetString(PyExc_IndexError, "array index out of range");
        return NULL;
    }
    return getarrayitem((PyObject *)a, i);
}

static PyObject *
array_slice(arrayobject *a, Py_ssize_t ilow, Py_ssize_t ihigh)
{
    arrayobject *np;
    if (ilow < 0)
        ilow = 0;
    else if (ilow > Py_SIZE(a))
        ilow = Py_SIZE(a);
    if (ihigh < 0)
        ihigh = 0;
    if (ihigh < ilow)
        ihigh = ilow;
    else if (ihigh > Py_SIZE(a))
        ihigh = Py_SIZE(a);
    np = (arrayobject *) newarrayobject(&Arraytype, ihigh - ilow, a->ob_descr);
    if (np == NULL)
        return NULL;
    memcpy(np->ob_item, a->ob_item + ilow * a->ob_descr->itemsize,
           (ihigh-ilow) * a->ob_descr->itemsize);
    return (PyObject *)np;
}

static PyObject *
array_copy(arrayobject *a, PyObject *unused)
{
    return array_slice(a, 0, Py_SIZE(a));
}

PyDoc_STRVAR(copy_doc,
"copy(array)\n\
\n\
 Return a copy of the array.");

static PyObject *
array_concat(arrayobject *a, PyObject *bb)
{
    Py_ssize_t size;
    arrayobject *np;
    if (!array_Check(bb)) {
        PyErr_Format(PyExc_TypeError,
             "can only append array (not \"%.200s\") to array",
                 Py_TYPE(bb)->tp_name);
        return NULL;
    }
#define b ((arrayobject *)bb)
    if (a->ob_descr != b->ob_descr) {
        PyErr_BadArgument();
        return NULL;
    }
    if (Py_SIZE(a) > PY_SSIZE_T_MAX - Py_SIZE(b)) {
        return PyErr_NoMemory();
    }
    size = Py_SIZE(a) + Py_SIZE(b);
    np = (arrayobject *) newarrayobject(&Arraytype, size, a->ob_descr);
    if (np == NULL) {
        return NULL;
    }
    memcpy(np->ob_item, a->ob_item, Py_SIZE(a)*a->ob_descr->itemsize);
    memcpy(np->ob_item + Py_SIZE(a)*a->ob_descr->itemsize,
           b->ob_item, Py_SIZE(b)*b->ob_descr->itemsize);
    return (PyObject *)np;
#undef b
}

static PyObject *
array_repeat(arrayobject *a, Py_ssize_t n)
{
    Py_ssize_t i;
    Py_ssize_t size;
    arrayobject *np;
    char *p;
    Py_ssize_t nbytes;
    if (n < 0)
        n = 0;
    if ((Py_SIZE(a) != 0) && (n > PY_SSIZE_T_MAX / Py_SIZE(a))) {
        return PyErr_NoMemory();
    }
    size = Py_SIZE(a) * n;
    np = (arrayobject *) newarrayobject(&Arraytype, size, a->ob_descr);
    if (np == NULL)
        return NULL;
    p = np->ob_item;
    nbytes = Py_SIZE(a) * a->ob_descr->itemsize;
    for (i = 0; i < n; i++) {
        memcpy(p, a->ob_item, nbytes);
        p += nbytes;
    }
    return (PyObject *) np;
}

static int
array_ass_slice(arrayobject *a, Py_ssize_t ilow, Py_ssize_t ihigh, PyObject *v)
{
    char *item;
    Py_ssize_t n; /* Size of replacement array */
    Py_ssize_t d; /* Change in size */
#define b ((arrayobject *)v)
    if (v == NULL)
        n = 0;
    else if (array_Check(v)) {
        n = Py_SIZE(b);
        if (a == b) {
            /* Special case "a[i:j] = a" -- copy b first */
            int ret;
            v = array_slice(b, 0, n);
            if (!v)
                return -1;
            ret = array_ass_slice(a, ilow, ihigh, v);
            Py_DECREF(v);
            return ret;
        }
        if (b->ob_descr != a->ob_descr) {
            PyErr_BadArgument();
            return -1;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
         "can only assign array (not \"%.200s\") to array slice",
                         Py_TYPE(v)->tp_name);
        return -1;
    }
    if (ilow < 0)
        ilow = 0;
    else if (ilow > Py_SIZE(a))
        ilow = Py_SIZE(a);
    if (ihigh < 0)
        ihigh = 0;
    if (ihigh < ilow)
        ihigh = ilow;
    else if (ihigh > Py_SIZE(a))
        ihigh = Py_SIZE(a);
    item = a->ob_item;
    d = n - (ihigh-ilow);
    /* Issue #4509: If the array has exported buffers and the slice
       assignment would change the size of the array, fail early to make
       sure we don't modify it. */
    if (d != 0 && a->ob_exports > 0) {
        PyErr_SetString(PyExc_BufferError,
            "cannot resize an array that is exporting buffers");
        return -1;
    }
    if (d < 0) { /* Delete -d items */
        memmove(item + (ihigh+d)*a->ob_descr->itemsize,
            item + ihigh*a->ob_descr->itemsize,
            (Py_SIZE(a)-ihigh)*a->ob_descr->itemsize);
        if (array_resize(a, Py_SIZE(a) + d) == -1)
            return -1;
    }
    else if (d > 0) { /* Insert d items */
        if (array_resize(a, Py_SIZE(a) + d))
            return -1;
        memmove(item + (ihigh+d)*a->ob_descr->itemsize,
            item + ihigh*a->ob_descr->itemsize,
            (Py_SIZE(a)-ihigh)*a->ob_descr->itemsize);
    }
    if (n > 0)
        memcpy(item + ilow*a->ob_descr->itemsize, b->ob_item,
               n*b->ob_descr->itemsize);
    return 0;
#undef b
}

static int
array_ass_item(arrayobject *a, Py_ssize_t i, PyObject *v)
{
    if (i < 0 || i >= Py_SIZE(a)) {
        PyErr_SetString(PyExc_IndexError,
                         "array assignment index out of range");
        return -1;
    }
    if (v == NULL)
        return array_ass_slice(a, i, i+1, v);
    return (*a->ob_descr->setitem)(a, i, v);
}

static int
setarrayitem(PyObject *a, Py_ssize_t i, PyObject *v)
{
    assert(array_Check(a));
    return array_ass_item((arrayobject *)a, i, v);
}

static int
array_iter_extend(arrayobject *self, PyObject *bb)
{
    PyObject *it, *v;

    it = PyObject_GetIter(bb);
    if (it == NULL)
        return -1;

    while ((v = PyIter_Next(it)) != NULL) {
        if (ins1(self, Py_SIZE(self), v) != 0) {
            Py_DECREF(v);
            Py_DECREF(it);
            return -1;
        }
        Py_DECREF(v);
    }
    Py_DECREF(it);
    if (PyErr_Occurred())
        return -1;
    return 0;
}

static int
array_do_extend(arrayobject *self, PyObject *bb)
{
    Py_ssize_t size, oldsize, bbsize;

    if (!array_Check(bb))
        return array_iter_extend(self, bb);
#define b ((arrayobject *)bb)
    if (self->ob_descr != b->ob_descr) {
        PyErr_SetString(PyExc_TypeError,
                     "can only extend with array of same kind");
        return -1;
    }
    if ((Py_SIZE(self) > PY_SSIZE_T_MAX - Py_SIZE(b)) ||
        ((Py_SIZE(self) + Py_SIZE(b)) > PY_SSIZE_T_MAX / self->ob_descr->itemsize)) {
        PyErr_NoMemory();
        return -1;
    }
    oldsize = Py_SIZE(self);
    /* Get the size of bb before resizing the array since bb could be self. */
    bbsize = Py_SIZE(bb);
    size = oldsize + Py_SIZE(b);
    if (array_resize(self, size) == -1)
        return -1;
    memcpy(self->ob_item + oldsize * self->ob_descr->itemsize,
        b->ob_item, bbsize * b->ob_descr->itemsize);

    return 0;
#undef b
}

static PyObject *
array_inplace_concat(arrayobject *self, PyObject *bb)
{
    if (!array_Check(bb)) {
        PyErr_Format(PyExc_TypeError,
            "can only extend array with array (not \"%.200s\")",
            Py_TYPE(bb)->tp_name);
        return NULL;
    }
    if (array_do_extend(self, bb) == -1)
        return NULL;
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
array_inplace_repeat(arrayobject *self, Py_ssize_t n)
{
    char *items, *p;
    Py_ssize_t size, i;

    if (Py_SIZE(self) > 0) {
        if (n < 0)
            n = 0;
        items = self->ob_item;
        if ((self->ob_descr->itemsize != 0) &&
            (Py_SIZE(self) > PY_SSIZE_T_MAX / self->ob_descr->itemsize)) {
            return PyErr_NoMemory();
        }
        size = Py_SIZE(self) * self->ob_descr->itemsize;
        if (n > 0 && size > PY_SSIZE_T_MAX / n) {
            return PyErr_NoMemory();
        }
        if (array_resize(self, n * Py_SIZE(self)) == -1)
            return NULL;
        items = p = self->ob_item;
        for (i = 1; i < n; i++) {
            p += size;
            memcpy(p, items, size);
        }
    }
    Py_INCREF(self);
    return (PyObject *)self;
}


static PyObject *
ins(arrayobject *self, Py_ssize_t where, PyObject *v)
{
    if (ins1(self, where, v) != 0)
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
array_count(arrayobject *self, PyObject *v)
{
    Py_ssize_t count = 0;
    Py_ssize_t i;

    for (i = 0; i < Py_SIZE(self); i++) {
        PyObject *selfi = getarrayitem((PyObject *)self, i);
        int cmp = PyObject_RichCompareBool(selfi, v, Py_EQ);
        Py_DECREF(selfi);
        if (cmp > 0)
            count++;
        else if (cmp < 0)
            return NULL;
    }
    return PyLong_FromSsize_t(count);
}

PyDoc_STRVAR(count_doc,
"count(x)\n\
\n\
Return number of occurrences of x in the array.");

static PyObject *
array_index(arrayobject *self, PyObject *v)
{
    Py_ssize_t i;

    for (i = 0; i < Py_SIZE(self); i++) {
        PyObject *selfi = getarrayitem((PyObject *)self, i);
        int cmp = PyObject_RichCompareBool(selfi, v, Py_EQ);
        Py_DECREF(selfi);
        if (cmp > 0) {
            return PyLong_FromLong((long)i);
        }
        else if (cmp < 0)
            return NULL;
    }
    PyErr_SetString(PyExc_ValueError, "array.index(x): x not in list");
    return NULL;
}

PyDoc_STRVAR(index_doc,
"index(x)\n\
\n\
Return index of first occurrence of x in the array.");

static int
array_contains(arrayobject *self, PyObject *v)
{
    Py_ssize_t i;
    int cmp;

    for (i = 0, cmp = 0 ; cmp == 0 && i < Py_SIZE(self); i++) {
        PyObject *selfi = getarrayitem((PyObject *)self, i);
        cmp = PyObject_RichCompareBool(selfi, v, Py_EQ);
        Py_DECREF(selfi);
    }
    return cmp;
}

static PyObject *
array_remove(arrayobject *self, PyObject *v)
{
    int i;

    for (i = 0; i < Py_SIZE(self); i++) {
        PyObject *selfi = getarrayitem((PyObject *)self,i);
        int cmp = PyObject_RichCompareBool(selfi, v, Py_EQ);
        Py_DECREF(selfi);
        if (cmp > 0) {
            if (array_ass_slice(self, i, i+1,
                               (PyObject *)NULL) != 0)
                return NULL;
            Py_INCREF(Py_None);
            return Py_None;
        }
        else if (cmp < 0)
            return NULL;
    }
    PyErr_SetString(PyExc_ValueError, "array.remove(x): x not in list");
    return NULL;
}

PyDoc_STRVAR(remove_doc,
"remove(x)\n\
\n\
Remove the first occurrence of x in the array.");

static PyObject *
array_pop(arrayobject *self, PyObject *args)
{
    Py_ssize_t i = -1;
    PyObject *v;
    if (!PyArg_ParseTuple(args, "|n:pop", &i))
        return NULL;
    if (Py_SIZE(self) == 0) {
        /* Special-case most common failure cause */
        PyErr_SetString(PyExc_IndexError, "pop from empty array");
        return NULL;
    }
    if (i < 0)
        i += Py_SIZE(self);
    if (i < 0 || i >= Py_SIZE(self)) {
        PyErr_SetString(PyExc_IndexError, "pop index out of range");
        return NULL;
    }
    v = getarrayitem((PyObject *)self,i);
    if (array_ass_slice(self, i, i+1, (PyObject *)NULL) != 0) {
        Py_DECREF(v);
        return NULL;
    }
    return v;
}

PyDoc_STRVAR(pop_doc,
"pop([i])\n\
\n\
Return the i-th element and delete it from the array. i defaults to -1.");

static PyObject *
array_extend(arrayobject *self, PyObject *bb)
{
    if (array_do_extend(self, bb) == -1)
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(extend_doc,
"extend(array or iterable)\n\
\n\
 Append items to the end of the array.");

static PyObject *
array_insert(arrayobject *self, PyObject *args)
{
    Py_ssize_t i;
    PyObject *v;
    if (!PyArg_ParseTuple(args, "nO:insert", &i, &v))
        return NULL;
    return ins(self, i, v);
}

PyDoc_STRVAR(insert_doc,
"insert(i,x)\n\
\n\
Insert a new item x into the array before position i.");


static PyObject *
array_buffer_info(arrayobject *self, PyObject *unused)
{
    PyObject* retval = NULL;
    retval = PyTuple_New(2);
    if (!retval)
        return NULL;

    PyTuple_SET_ITEM(retval, 0, PyLong_FromVoidPtr(self->ob_item));
    PyTuple_SET_ITEM(retval, 1, PyLong_FromLong((long)(Py_SIZE(self))));

    return retval;
}

PyDoc_STRVAR(buffer_info_doc,
"buffer_info() -> (address, length)\n\
\n\
Return a tuple (address, length) giving the current memory address and\n\
the length in items of the buffer used to hold array's contents\n\
The length should be multiplied by the itemsize attribute to calculate\n\
the buffer length in bytes.");


static PyObject *
array_append(arrayobject *self, PyObject *v)
{
    return ins(self, Py_SIZE(self), v);
}

PyDoc_STRVAR(append_doc,
"append(x)\n\
\n\
Append new value x to the end of the array.");


static PyObject *
array_byteswap(arrayobject *self, PyObject *unused)
{
    char *p;
    Py_ssize_t i;

    switch (self->ob_descr->itemsize) {
    case 1:
        break;
    case 2:
        for (p = self->ob_item, i = Py_SIZE(self); --i >= 0; p += 2) {
            char p0 = p[0];
            p[0] = p[1];
            p[1] = p0;
        }
        break;
    case 4:
        for (p = self->ob_item, i = Py_SIZE(self); --i >= 0; p += 4) {
            char p0 = p[0];
            char p1 = p[1];
            p[0] = p[3];
            p[1] = p[2];
            p[2] = p1;
            p[3] = p0;
        }
        break;
    case 8:
        for (p = self->ob_item, i = Py_SIZE(self); --i >= 0; p += 8) {
            char p0 = p[0];
            char p1 = p[1];
            char p2 = p[2];
            char p3 = p[3];
            p[0] = p[7];
            p[1] = p[6];
            p[2] = p[5];
            p[3] = p[4];
            p[4] = p3;
            p[5] = p2;
            p[6] = p1;
            p[7] = p0;
        }
        break;
    default:
        PyErr_SetString(PyExc_RuntimeError,
                   "don't know how to byteswap this array type");
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(byteswap_doc,
"byteswap()\n\
\n\
Byteswap all items of the array.  If the items in the array are not 1, 2,\n\
4, or 8 bytes in size, RuntimeError is raised.");

static PyObject *
array_reverse(arrayobject *self, PyObject *unused)
{
    register Py_ssize_t itemsize = self->ob_descr->itemsize;
    register char *p, *q;
    /* little buffer to hold items while swapping */
    char tmp[256];      /* 8 is probably enough -- but why skimp */
    assert((size_t)itemsize <= sizeof(tmp));

    if (Py_SIZE(self) > 1) {
        for (p = self->ob_item,
             q = self->ob_item + (Py_SIZE(self) - 1)*itemsize;
             p < q;
             p += itemsize, q -= itemsize) {
            /* memory areas guaranteed disjoint, so memcpy
             * is safe (& memmove may be slower).
             */
            memcpy(tmp, p, itemsize);
            memcpy(p, q, itemsize);
            memcpy(q, tmp, itemsize);
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(reverse_doc,
"reverse()\n\
\n\
Reverse the order of the items in the array.");


/* Forward */
static PyObject *array_frombytes(arrayobject *self, PyObject *args);

static PyObject *
array_fromfile(arrayobject *self, PyObject *args)
{
    PyObject *f, *b, *res;
    Py_ssize_t itemsize = self->ob_descr->itemsize;
    Py_ssize_t n, nbytes;
    int not_enough_bytes;

    if (!PyArg_ParseTuple(args, "On:fromfile", &f, &n))
        return NULL;

    nbytes = n * itemsize;
    if (nbytes < 0 || nbytes/itemsize != n) {
        PyErr_NoMemory();
        return NULL;
    }

    b = PyObject_CallMethod(f, "read", "n", nbytes);
    if (b == NULL)
        return NULL;

    if (!PyBytes_Check(b)) {
        PyErr_SetString(PyExc_TypeError,
                        "read() didn't return bytes");
        Py_DECREF(b);
        return NULL;
    }

    not_enough_bytes = (PyBytes_GET_SIZE(b) != nbytes);

    args = Py_BuildValue("(O)", b);
    Py_DECREF(b);
    if (args == NULL)
        return NULL;

    res = array_frombytes(self, args);
    Py_DECREF(args);
    if (res == NULL)
        return NULL;

    if (not_enough_bytes) {
        PyErr_SetString(PyExc_EOFError,
                        "read() didn't return enough bytes");
        Py_DECREF(res);
        return NULL;
    }

    return res;
}

PyDoc_STRVAR(fromfile_doc,
"fromfile(f, n)\n\
\n\
Read n objects from the file object f and append them to the end of the\n\
array.");


static PyObject *
array_tofile(arrayobject *self, PyObject *f)
{
    Py_ssize_t nbytes = Py_SIZE(self) * self->ob_descr->itemsize;
    /* Write 64K blocks at a time */
    /* XXX Make the block size settable */
    int BLOCKSIZE = 64*1024;
    Py_ssize_t nblocks = (nbytes + BLOCKSIZE - 1) / BLOCKSIZE;
    Py_ssize_t i;

    if (Py_SIZE(self) == 0)
        goto done;

    for (i = 0; i < nblocks; i++) {
        char* ptr = self->ob_item + i*BLOCKSIZE;
        Py_ssize_t size = BLOCKSIZE;
        PyObject *bytes, *res;
        if (i*BLOCKSIZE + size > nbytes)
            size = nbytes - i*BLOCKSIZE;
        bytes = PyBytes_FromStringAndSize(ptr, size);
        if (bytes == NULL)
            return NULL;
        res = PyObject_CallMethod(f, "write", "O", bytes);
        Py_DECREF(bytes);
        if (res == NULL)
            return NULL;
        Py_DECREF(res); /* drop write result */
    }

  done:
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(tofile_doc,
"tofile(f)\n\
\n\
Write all items (as machine values) to the file object f.");


static PyObject *
array_fromlist(arrayobject *self, PyObject *list)
{
    Py_ssize_t n;

    if (!PyList_Check(list)) {
        PyErr_SetString(PyExc_TypeError, "arg must be list");
        return NULL;
    }
    n = PyList_Size(list);
    if (n > 0) {
        Py_ssize_t i, old_size;
        old_size = Py_SIZE(self);
        if (array_resize(self, old_size + n) == -1)
            return NULL;
        for (i = 0; i < n; i++) {
            PyObject *v = PyList_GetItem(list, i);
            if ((*self->ob_descr->setitem)(self,
                            Py_SIZE(self) - n + i, v) != 0) {
                array_resize(self, old_size);
                return NULL;
            }
        }
    }
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(fromlist_doc,
"fromlist(list)\n\
\n\
Append items to array from list.");

static PyObject *
array_tolist(arrayobject *self, PyObject *unused)
{
    PyObject *list = PyList_New(Py_SIZE(self));
    Py_ssize_t i;

    if (list == NULL)
        return NULL;
    for (i = 0; i < Py_SIZE(self); i++) {
        PyObject *v = getarrayitem((PyObject *)self, i);
        if (v == NULL) {
            Py_DECREF(list);
            return NULL;
        }
        PyList_SetItem(list, i, v);
    }
    return list;
}

PyDoc_STRVAR(tolist_doc,
"tolist() -> list\n\
\n\
Convert array to an ordinary list with the same items.");

static PyObject *
frombytes(arrayobject *self, Py_buffer *buffer)
{
    int itemsize = self->ob_descr->itemsize;
    Py_ssize_t n;
    if (buffer->itemsize != 1) {
        PyBuffer_Release(buffer);
        PyErr_SetString(PyExc_TypeError, "string/buffer of bytes required.");
        return NULL;
    }
    n = buffer->len;
    if (n % itemsize != 0) {
        PyBuffer_Release(buffer);
        PyErr_SetString(PyExc_ValueError,
                   "string length not a multiple of item size");
        return NULL;
    }
    n = n / itemsize;
    if (n > 0) {
        Py_ssize_t old_size = Py_SIZE(self);
        if ((n > PY_SSIZE_T_MAX - old_size) ||
            ((old_size + n) > PY_SSIZE_T_MAX / itemsize)) {
                PyBuffer_Release(buffer);
                return PyErr_NoMemory();
        }
        if (array_resize(self, old_size + n) == -1) {
            PyBuffer_Release(buffer);
            return NULL;
        }
        memcpy(self->ob_item + old_size * itemsize,
            buffer->buf, n * itemsize);
    }
    PyBuffer_Release(buffer);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
array_fromstring(arrayobject *self, PyObject *args)
{
    Py_buffer buffer;
    if (PyErr_WarnEx(PyExc_DeprecationWarning,
            "fromstring() is deprecated. Use frombytes() instead.", 2) != 0)
        return NULL;
    if (!PyArg_ParseTuple(args, "s*:fromstring", &buffer))
        return NULL;
    else
        return frombytes(self, &buffer);
}

PyDoc_STRVAR(fromstring_doc,
"fromstring(string)\n\
\n\
Appends items from the string, interpreting it as an array of machine\n\
values, as if it had been read from a file using the fromfile() method).\n\
\n\
This method is deprecated. Use frombytes instead.");


static PyObject *
array_frombytes(arrayobject *self, PyObject *args)
{
    Py_buffer buffer;
    if (!PyArg_ParseTuple(args, "y*:frombytes", &buffer))
        return NULL;
    else
        return frombytes(self, &buffer);
}

PyDoc_STRVAR(frombytes_doc,
"frombytes(bytestring)\n\
\n\
Appends items from the string, interpreting it as an array of machine\n\
values, as if it had been read from a file using the fromfile() method).");


static PyObject *
array_tobytes(arrayobject *self, PyObject *unused)
{
    if (Py_SIZE(self) <= PY_SSIZE_T_MAX / self->ob_descr->itemsize) {
        return PyBytes_FromStringAndSize(self->ob_item,
                            Py_SIZE(self) * self->ob_descr->itemsize);
    } else {
        return PyErr_NoMemory();
    }
}

PyDoc_STRVAR(tobytes_doc,
"tobytes() -> bytes\n\
\n\
Convert the array to an array of machine values and return the bytes\n\
representation.");


static PyObject *
array_tostring(arrayobject *self, PyObject *unused)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning,
            "tostring() is deprecated. Use tobytes() instead.", 2) != 0)
        return NULL;
    return array_tobytes(self, unused);
}

PyDoc_STRVAR(tostring_doc,
"tostring() -> bytes\n\
\n\
Convert the array to an array of machine values and return the bytes\n\
representation.\n\
\n\
This method is deprecated. Use tobytes instead.");


static PyObject *
array_fromunicode(arrayobject *self, PyObject *args)
{
    Py_UNICODE *ustr;
    Py_ssize_t n;
    Py_UNICODE typecode;

    if (!PyArg_ParseTuple(args, "u#:fromunicode", &ustr, &n))
        return NULL;
    typecode = self->ob_descr->typecode;
    if ((typecode != 'u')) {
        PyErr_SetString(PyExc_ValueError,
            "fromunicode() may only be called on "
            "unicode type arrays");
        return NULL;
    }
    if (n > 0) {
        Py_ssize_t old_size = Py_SIZE(self);
        if (array_resize(self, old_size + n) == -1)
            return NULL;
        memcpy(self->ob_item + old_size * sizeof(Py_UNICODE),
               ustr, n * sizeof(Py_UNICODE));
    }

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(fromunicode_doc,
"fromunicode(ustr)\n\
\n\
Extends this array with data from the unicode string ustr.\n\
The array must be a unicode type array; otherwise a ValueError\n\
is raised.  Use array.frombytes(ustr.decode(...)) to\n\
append Unicode data to an array of some other type.");


static PyObject *
array_tounicode(arrayobject *self, PyObject *unused)
{
    Py_UNICODE typecode;
    typecode = self->ob_descr->typecode;
    if ((typecode != 'u')) {
        PyErr_SetString(PyExc_ValueError,
             "tounicode() may only be called on unicode type arrays");
        return NULL;
    }
    return PyUnicode_FromUnicode((Py_UNICODE *) self->ob_item, Py_SIZE(self));
}

PyDoc_STRVAR(tounicode_doc,
"tounicode() -> unicode\n\
\n\
Convert the array to a unicode string.  The array must be\n\
a unicode type array; otherwise a ValueError is raised.  Use\n\
array.tostring().decode() to obtain a unicode string from\n\
an array of some other type.");



/*********************** Pickling support ************************/

enum machine_format_code {
    UNKNOWN_FORMAT = -1,
    /* UNKNOWN_FORMAT is used to indicate that the machine format for an
     * array type code cannot be interpreted. When this occurs, a list of
     * Python objects is used to represent the content of the array
     * instead of using the memory content of the array directly. In that
     * case, the array_reconstructor mechanism is bypassed completely, and
     * the standard array constructor is used instead.
     *
     * This is will most likely occur when the machine doesn't use IEEE
     * floating-point numbers.
     */

    UNSIGNED_INT8 = 0,
    SIGNED_INT8 = 1,
    UNSIGNED_INT16_LE = 2,
    UNSIGNED_INT16_BE = 3,
    SIGNED_INT16_LE = 4,
    SIGNED_INT16_BE = 5,
    UNSIGNED_INT32_LE = 6,
    UNSIGNED_INT32_BE = 7,
    SIGNED_INT32_LE = 8,
    SIGNED_INT32_BE = 9,
    UNSIGNED_INT64_LE = 10,
    UNSIGNED_INT64_BE = 11,
    SIGNED_INT64_LE = 12,
    SIGNED_INT64_BE = 13,
    IEEE_754_FLOAT_LE = 14,
    IEEE_754_FLOAT_BE = 15,
    IEEE_754_DOUBLE_LE = 16,
    IEEE_754_DOUBLE_BE = 17,
    UTF16_LE = 18,
    UTF16_BE = 19,
    UTF32_LE = 20,
    UTF32_BE = 21
};
#define MACHINE_FORMAT_CODE_MIN 0
#define MACHINE_FORMAT_CODE_MAX 21

static const struct mformatdescr {
    size_t size;
    int is_signed;
    int is_big_endian;
} mformat_descriptors[] = {
    {1, 0, 0},                  /* 0: UNSIGNED_INT8 */
    {1, 1, 0},                  /* 1: SIGNED_INT8 */
    {2, 0, 0},                  /* 2: UNSIGNED_INT16_LE */
    {2, 0, 1},                  /* 3: UNSIGNED_INT16_BE */
    {2, 1, 0},                  /* 4: SIGNED_INT16_LE */
    {2, 1, 1},                  /* 5: SIGNED_INT16_BE */
    {4, 0, 0},                  /* 6: UNSIGNED_INT32_LE */
    {4, 0, 1},                  /* 7: UNSIGNED_INT32_BE */
    {4, 1, 0},                  /* 8: SIGNED_INT32_LE */
    {4, 1, 1},                  /* 9: SIGNED_INT32_BE */
    {8, 0, 0},                  /* 10: UNSIGNED_INT64_LE */
    {8, 0, 1},                  /* 11: UNSIGNED_INT64_BE */
    {8, 1, 0},                  /* 12: SIGNED_INT64_LE */
    {8, 1, 1},                  /* 13: SIGNED_INT64_BE */
    {4, 0, 0},                  /* 14: IEEE_754_FLOAT_LE */
    {4, 0, 1},                  /* 15: IEEE_754_FLOAT_BE */
    {8, 0, 0},                  /* 16: IEEE_754_DOUBLE_LE */
    {8, 0, 1},                  /* 17: IEEE_754_DOUBLE_BE */
    {4, 0, 0},                  /* 18: UTF16_LE */
    {4, 0, 1},                  /* 19: UTF16_BE */
    {8, 0, 0},                  /* 20: UTF32_LE */
    {8, 0, 1}                   /* 21: UTF32_BE */
};


/*
 * Internal: This function is used to find the machine format of a given
 * array type code. This returns UNKNOWN_FORMAT when the machine format cannot
 * be found.
 */
static enum machine_format_code
typecode_to_mformat_code(int typecode)
{
#ifdef WORDS_BIGENDIAN
    const int is_big_endian = 1;
#else
    const int is_big_endian = 0;
#endif
    size_t intsize;
    int is_signed;

    switch (typecode) {
    case 'b':
        return SIGNED_INT8;
    case 'B':
        return UNSIGNED_INT8;

    case 'u':
        if (sizeof(Py_UNICODE) == 2) {
            return UTF16_LE + is_big_endian;
        }
        if (sizeof(Py_UNICODE) == 4) {
            return UTF32_LE + is_big_endian;
        }
        return UNKNOWN_FORMAT;

    case 'f':
        if (sizeof(float) == 4) {
            const float y = 16711938.0;
            if (memcmp(&y, "\x4b\x7f\x01\x02", 4) == 0)
                return IEEE_754_FLOAT_BE;
            if (memcmp(&y, "\x02\x01\x7f\x4b", 4) == 0)
                return IEEE_754_FLOAT_LE;
        }
        return UNKNOWN_FORMAT;

    case 'd':
        if (sizeof(double) == 8) {
            const double x = 9006104071832581.0;
            if (memcmp(&x, "\x43\x3f\xff\x01\x02\x03\x04\x05", 8) == 0)
                return IEEE_754_DOUBLE_BE;
            if (memcmp(&x, "\x05\x04\x03\x02\x01\xff\x3f\x43", 8) == 0)
                return IEEE_754_DOUBLE_LE;
        }
        return UNKNOWN_FORMAT;

    /* Integers */
    case 'h':
        intsize = sizeof(short);
        is_signed = 1;
        break;
    case 'H':
        intsize = sizeof(short);
        is_signed = 0;
        break;
    case 'i':
        intsize = sizeof(int);
        is_signed = 1;
        break;
    case 'I':
        intsize = sizeof(int);
        is_signed = 0;
        break;
    case 'l':
        intsize = sizeof(long);
        is_signed = 1;
        break;
    case 'L':
        intsize = sizeof(long);
        is_signed = 0;
        break;
    default:
        return UNKNOWN_FORMAT;
    }
    switch (intsize) {
    case 2:
        return UNSIGNED_INT16_LE + is_big_endian + (2 * is_signed);
    case 4:
        return UNSIGNED_INT32_LE + is_big_endian + (2 * is_signed);
    case 8:
        return UNSIGNED_INT64_LE + is_big_endian + (2 * is_signed);
    default:
        return UNKNOWN_FORMAT;
    }
}

/* Forward declaration. */
static PyObject *array_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

/*
 * Internal: This function wraps the array constructor--i.e., array_new()--to
 * allow the creation of array objects from C code without having to deal
 * directly the tuple argument of array_new(). The typecode argument is a
 * Unicode character value, like 'i' or 'f' for example, representing an array
 * type code. The items argument is a bytes or a list object from which
 * contains the initial value of the array.
 *
 * On success, this functions returns the array object created. Otherwise,
 * NULL is returned to indicate a failure.
 */
static PyObject *
make_array(PyTypeObject *arraytype, Py_UNICODE typecode, PyObject *items)
{
    PyObject *new_args;
    PyObject *array_obj;
    PyObject *typecode_obj;

    assert(arraytype != NULL);
    assert(items != NULL);

    typecode_obj = PyUnicode_FromUnicode(&typecode, 1);
    if (typecode_obj == NULL)
        return NULL;

    new_args = PyTuple_New(2);
    if (new_args == NULL)
        return NULL;
    Py_INCREF(items);
    PyTuple_SET_ITEM(new_args, 0, typecode_obj);
    PyTuple_SET_ITEM(new_args, 1, items);

    array_obj = array_new(arraytype, new_args, NULL);
    Py_DECREF(new_args);
    if (array_obj == NULL)
        return NULL;

    return array_obj;
}

/*
 * This functions is a special constructor used when unpickling an array. It
 * provides a portable way to rebuild an array from its memory representation.
 */
static PyObject *
array_reconstructor(PyObject *self, PyObject *args)
{
    PyTypeObject *arraytype;
    PyObject *items;
    PyObject *converted_items;
    PyObject *result;
    int typecode_int;
    Py_UNICODE typecode;
    enum machine_format_code mformat_code;
    struct arraydescr *descr;

    if (!PyArg_ParseTuple(args, "OCiO:array._array_reconstructor",
                    &arraytype, &typecode_int, &mformat_code, &items))
        return NULL;

    typecode = (Py_UNICODE)typecode_int;

    if (!PyType_Check(arraytype)) {
        PyErr_Format(PyExc_TypeError,
            "first argument must a type object, not %.200s",
            Py_TYPE(arraytype)->tp_name);
        return NULL;
    }
    if (!PyType_IsSubtype(arraytype, &Arraytype)) {
        PyErr_Format(PyExc_TypeError,
            "%.200s is not a subtype of %.200s",
            arraytype->tp_name, Arraytype.tp_name);
        return NULL;
    }
    for (descr = descriptors; descr->typecode != '\0'; descr++) {
        if (descr->typecode == typecode)
            break;
    }
    if (descr->typecode == '\0') {
        PyErr_SetString(PyExc_ValueError,
                        "second argument must be a valid type code");
        return NULL;
    }
    if (mformat_code < MACHINE_FORMAT_CODE_MIN ||
        mformat_code > MACHINE_FORMAT_CODE_MAX) {
        PyErr_SetString(PyExc_ValueError,
            "third argument must be a valid machine format code.");
        return NULL;
    }
    if (!PyBytes_Check(items)) {
        PyErr_Format(PyExc_TypeError,
            "fourth argument should be bytes, not %.200s",
            Py_TYPE(items)->tp_name);
        return NULL;
    }

    /* Fast path: No decoding has to be done. */
    if (mformat_code == typecode_to_mformat_code(typecode) ||
        mformat_code == UNKNOWN_FORMAT) {
        return make_array(arraytype, typecode, items);
    }

    /* Slow path: Decode the byte string according to the given machine
     * format code. This occurs when the computer unpickling the array
     * object is architecturally different from the one that pickled the
     * array.
     */
    if (Py_SIZE(items) % mformat_descriptors[mformat_code].size != 0) {
        PyErr_SetString(PyExc_ValueError,
                        "string length not a multiple of item size");
        return NULL;
    }
    switch (mformat_code) {
    case IEEE_754_FLOAT_LE:
    case IEEE_754_FLOAT_BE: {
        int i;
        int le = (mformat_code == IEEE_754_FLOAT_LE) ? 1 : 0;
        Py_ssize_t itemcount = Py_SIZE(items) / 4;
        const unsigned char *memstr =
            (unsigned char *)PyBytes_AS_STRING(items);

        converted_items = PyList_New(itemcount);
        if (converted_items == NULL)
            return NULL;
        for (i = 0; i < itemcount; i++) {
            PyObject *pyfloat = PyFloat_FromDouble(
                _PyFloat_Unpack4(&memstr[i * 4], le));
            if (pyfloat == NULL) {
                Py_DECREF(converted_items);
                return NULL;
            }
            PyList_SET_ITEM(converted_items, i, pyfloat);
        }
        break;
    }
    case IEEE_754_DOUBLE_LE:
    case IEEE_754_DOUBLE_BE: {
        int i;
        int le = (mformat_code == IEEE_754_DOUBLE_LE) ? 1 : 0;
        Py_ssize_t itemcount = Py_SIZE(items) / 8;
        const unsigned char *memstr =
            (unsigned char *)PyBytes_AS_STRING(items);

        converted_items = PyList_New(itemcount);
        if (converted_items == NULL)
            return NULL;
        for (i = 0; i < itemcount; i++) {
            PyObject *pyfloat = PyFloat_FromDouble(
                _PyFloat_Unpack8(&memstr[i * 8], le));
            if (pyfloat == NULL) {
                Py_DECREF(converted_items);
                return NULL;
            }
            PyList_SET_ITEM(converted_items, i, pyfloat);
        }
        break;
    }
    case UTF16_LE:
    case UTF16_BE: {
        int byteorder = (mformat_code == UTF16_LE) ? -1 : 1;
        converted_items = PyUnicode_DecodeUTF16(
            PyBytes_AS_STRING(items), Py_SIZE(items),
            "strict", &byteorder);
        if (converted_items == NULL)
            return NULL;
        break;
    }
    case UTF32_LE:
    case UTF32_BE: {
        int byteorder = (mformat_code == UTF32_LE) ? -1 : 1;
        converted_items = PyUnicode_DecodeUTF32(
            PyBytes_AS_STRING(items), Py_SIZE(items),
            "strict", &byteorder);
        if (converted_items == NULL)
            return NULL;
        break;
    }

    case UNSIGNED_INT8:
    case SIGNED_INT8:
    case UNSIGNED_INT16_LE:
    case UNSIGNED_INT16_BE:
    case SIGNED_INT16_LE:
    case SIGNED_INT16_BE:
    case UNSIGNED_INT32_LE:
    case UNSIGNED_INT32_BE:
    case SIGNED_INT32_LE:
    case SIGNED_INT32_BE:
    case UNSIGNED_INT64_LE:
    case UNSIGNED_INT64_BE:
    case SIGNED_INT64_LE:
    case SIGNED_INT64_BE: {
        int i;
        const struct mformatdescr mf_descr =
            mformat_descriptors[mformat_code];
        Py_ssize_t itemcount = Py_SIZE(items) / mf_descr.size;
        const unsigned char *memstr =
            (unsigned char *)PyBytes_AS_STRING(items);
        struct arraydescr *descr;

        /* If possible, try to pack array's items using a data type
         * that fits better. This may result in an array with narrower
         * or wider elements.
         *
         * For example, if a 32-bit machine pickles a L-code array of
         * unsigned longs, then the array will be unpickled by 64-bit
         * machine as an I-code array of unsigned ints.
         *
         * XXX: Is it possible to write a unit test for this?
         */
        for (descr = descriptors; descr->typecode != '\0'; descr++) {
            if (descr->is_integer_type &&
                descr->itemsize == mf_descr.size &&
                descr->is_signed == mf_descr.is_signed)
                typecode = descr->typecode;
        }

        converted_items = PyList_New(itemcount);
        if (converted_items == NULL)
            return NULL;
        for (i = 0; i < itemcount; i++) {
            PyObject *pylong;

            pylong = _PyLong_FromByteArray(
                &memstr[i * mf_descr.size],
                mf_descr.size,
                !mf_descr.is_big_endian,
                mf_descr.is_signed);
            if (pylong == NULL) {
                Py_DECREF(converted_items);
                return NULL;
            }
            PyList_SET_ITEM(converted_items, i, pylong);
        }
        break;
    }
    case UNKNOWN_FORMAT:
        /* Impossible, but needed to shut up GCC about the unhandled
         * enumeration value.
         */
    default:
        PyErr_BadArgument();
        return NULL;
    }

    result = make_array(arraytype, typecode, converted_items);
    Py_DECREF(converted_items);
    return result;
}

static PyObject *
array_reduce_ex(arrayobject *array, PyObject *value)
{
    PyObject *dict;
    PyObject *result;
    PyObject *array_str;
    int typecode = array->ob_descr->typecode;
    int mformat_code;
    static PyObject *array_reconstructor = NULL;
    long protocol;

    if (array_reconstructor == NULL) {
        PyObject *array_module = PyImport_ImportModule("array");
        if (array_module == NULL)
            return NULL;
        array_reconstructor = PyObject_GetAttrString(
            array_module,
            "_array_reconstructor");
        Py_DECREF(array_module);
        if (array_reconstructor == NULL)
            return NULL;
    }

    if (!PyLong_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "__reduce_ex__ argument should an integer");
        return NULL;
    }
    protocol = PyLong_AsLong(value);
    if (protocol == -1 && PyErr_Occurred())
        return NULL;

    dict = PyObject_GetAttrString((PyObject *)array, "__dict__");
    if (dict == NULL) {
        if (!PyErr_ExceptionMatches(PyExc_AttributeError))
            return NULL;
        PyErr_Clear();
        dict = Py_None;
        Py_INCREF(dict);
    }

    mformat_code = typecode_to_mformat_code(typecode);
    if (mformat_code == UNKNOWN_FORMAT || protocol < 3) {
        /* Convert the array to a list if we got something weird
         * (e.g., non-IEEE floats), or we are pickling the array using
         * a Python 2.x compatible protocol.
         *
         * It is necessary to use a list representation for Python 2.x
         * compatible pickle protocol, since Python 2's str objects
         * are unpickled as unicode by Python 3. Thus it is impossible
         * to make arrays unpicklable by Python 3 by using their memory
         * representation, unless we resort to ugly hacks such as
         * coercing unicode objects to bytes in array_reconstructor.
         */
        PyObject *list;
        list = array_tolist(array, NULL);
        if (list == NULL) {
            Py_DECREF(dict);
            return NULL;
        }
        result = Py_BuildValue(
            "O(CO)O", Py_TYPE(array), typecode, list, dict);
        Py_DECREF(list);
        Py_DECREF(dict);
        return result;
    }

    array_str = array_tobytes(array, NULL);
    if (array_str == NULL) {
        Py_DECREF(dict);
        return NULL;
    }
    result = Py_BuildValue(
        "O(OCiN)O", array_reconstructor, Py_TYPE(array), typecode,
        mformat_code, array_str, dict);
    Py_DECREF(dict);
    return result;
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyObject *
array_get_typecode(arrayobject *a, void *closure)
{
    Py_UNICODE tc = a->ob_descr->typecode;
    return PyUnicode_FromUnicode(&tc, 1);
}

static PyObject *
array_get_itemsize(arrayobject *a, void *closure)
{
    return PyLong_FromLong((long)a->ob_descr->itemsize);
}

static PyGetSetDef array_getsets [] = {
    {"typecode", (getter) array_get_typecode, NULL,
     "the typecode character used to create the array"},
    {"itemsize", (getter) array_get_itemsize, NULL,
     "the size, in bytes, of one array item"},
    {NULL}
};

static PyMethodDef array_methods[] = {
    {"append",          (PyCFunction)array_append,      METH_O,
     append_doc},
    {"buffer_info", (PyCFunction)array_buffer_info, METH_NOARGS,
     buffer_info_doc},
    {"byteswap",        (PyCFunction)array_byteswap,    METH_NOARGS,
     byteswap_doc},
    {"__copy__",        (PyCFunction)array_copy,        METH_NOARGS,
     copy_doc},
    {"count",           (PyCFunction)array_count,       METH_O,
     count_doc},
    {"__deepcopy__",(PyCFunction)array_copy,            METH_O,
     copy_doc},
    {"extend",      (PyCFunction)array_extend,          METH_O,
     extend_doc},
    {"fromfile",        (PyCFunction)array_fromfile,    METH_VARARGS,
     fromfile_doc},
    {"fromlist",        (PyCFunction)array_fromlist,    METH_O,
     fromlist_doc},
    {"fromstring",      (PyCFunction)array_fromstring,  METH_VARARGS,
     fromstring_doc},
    {"frombytes",       (PyCFunction)array_frombytes,   METH_VARARGS,
     frombytes_doc},
    {"fromunicode",     (PyCFunction)array_fromunicode, METH_VARARGS,
     fromunicode_doc},
    {"index",           (PyCFunction)array_index,       METH_O,
     index_doc},
    {"insert",          (PyCFunction)array_insert,      METH_VARARGS,
     insert_doc},
    {"pop",             (PyCFunction)array_pop,         METH_VARARGS,
     pop_doc},
    {"__reduce_ex__", (PyCFunction)array_reduce_ex,     METH_O,
     reduce_doc},
    {"remove",          (PyCFunction)array_remove,      METH_O,
     remove_doc},
    {"reverse",         (PyCFunction)array_reverse,     METH_NOARGS,
     reverse_doc},
/*      {"sort",        (PyCFunction)array_sort,        METH_VARARGS,
    sort_doc},*/
    {"tofile",          (PyCFunction)array_tofile,      METH_O,
     tofile_doc},
    {"tolist",          (PyCFunction)array_tolist,      METH_NOARGS,
     tolist_doc},
    {"tostring",        (PyCFunction)array_tostring,    METH_NOARGS,
     tostring_doc},
    {"tobytes",         (PyCFunction)array_tobytes,     METH_NOARGS,
     tobytes_doc},
    {"tounicode",   (PyCFunction)array_tounicode,       METH_NOARGS,
     tounicode_doc},
    {NULL,              NULL}           /* sentinel */
};

static PyObject *
array_repr(arrayobject *a)
{
    Py_UNICODE typecode;
    PyObject *s, *v = NULL;
    Py_ssize_t len;

    len = Py_SIZE(a);
    typecode = a->ob_descr->typecode;
    if (len == 0) {
        return PyUnicode_FromFormat("array('%c')", (int)typecode);
    }
    if ((typecode == 'u'))
        v = array_tounicode(a, NULL);
    else
        v = array_tolist(a, NULL);

    s = PyUnicode_FromFormat("array('%c', %R)", (int)typecode, v);
    Py_DECREF(v);
    return s;
}

static PyObject*
array_subscr(arrayobject* self, PyObject* item)
{
    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i==-1 && PyErr_Occurred()) {
            return NULL;
        }
        if (i < 0)
            i += Py_SIZE(self);
        return array_item(self, i);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelength, cur, i;
        PyObject* result;
        arrayobject* ar;
        int itemsize = self->ob_descr->itemsize;

        if (PySlice_GetIndicesEx((PySliceObject*)item, Py_SIZE(self),
                         &start, &stop, &step, &slicelength) < 0) {
            return NULL;
        }

        if (slicelength <= 0) {
            return newarrayobject(&Arraytype, 0, self->ob_descr);
        }
        else if (step == 1) {
            PyObject *result = newarrayobject(&Arraytype,
                                    slicelength, self->ob_descr);
            if (result == NULL)
                return NULL;
            memcpy(((arrayobject *)result)->ob_item,
                   self->ob_item + start * itemsize,
                   slicelength * itemsize);
            return result;
        }
        else {
            result = newarrayobject(&Arraytype, slicelength, self->ob_descr);
            if (!result) return NULL;

            ar = (arrayobject*)result;

            for (cur = start, i = 0; i < slicelength;
                 cur += step, i++) {
                memcpy(ar->ob_item + i*itemsize,
                       self->ob_item + cur*itemsize,
                       itemsize);
            }

            return result;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "array indices must be integers");
        return NULL;
    }
}

static int
array_ass_subscr(arrayobject* self, PyObject* item, PyObject* value)
{
    Py_ssize_t start, stop, step, slicelength, needed;
    arrayobject* other;
    int itemsize;

    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);

        if (i == -1 && PyErr_Occurred())
            return -1;
        if (i < 0)
            i += Py_SIZE(self);
        if (i < 0 || i >= Py_SIZE(self)) {
            PyErr_SetString(PyExc_IndexError,
                "array assignment index out of range");
            return -1;
        }
        if (value == NULL) {
            /* Fall through to slice assignment */
            start = i;
            stop = i + 1;
            step = 1;
            slicelength = 1;
        }
        else
            return (*self->ob_descr->setitem)(self, i, value);
    }
    else if (PySlice_Check(item)) {
        if (PySlice_GetIndicesEx((PySliceObject *)item,
                                 Py_SIZE(self), &start, &stop,
                                 &step, &slicelength) < 0) {
            return -1;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "array indices must be integer");
        return -1;
    }
    if (value == NULL) {
        other = NULL;
        needed = 0;
    }
    else if (array_Check(value)) {
        other = (arrayobject *)value;
        needed = Py_SIZE(other);
        if (self == other) {
            /* Special case "self[i:j] = self" -- copy self first */
            int ret;
            value = array_slice(other, 0, needed);
            if (value == NULL)
                return -1;
            ret = array_ass_subscr(self, item, value);
            Py_DECREF(value);
            return ret;
        }
        if (other->ob_descr != self->ob_descr) {
            PyErr_BadArgument();
            return -1;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
         "can only assign array (not \"%.200s\") to array slice",
                         Py_TYPE(value)->tp_name);
        return -1;
    }
    itemsize = self->ob_descr->itemsize;
    /* for 'a[2:1] = ...', the insertion point is 'start', not 'stop' */
    if ((step > 0 && stop < start) ||
        (step < 0 && stop > start))
        stop = start;

    /* Issue #4509: If the array has exported buffers and the slice
       assignment would change the size of the array, fail early to make
       sure we don't modify it. */
    if ((needed == 0 || slicelength != needed) && self->ob_exports > 0) {
        PyErr_SetString(PyExc_BufferError,
            "cannot resize an array that is exporting buffers");
        return -1;
    }

    if (step == 1) {
        if (slicelength > needed) {
            memmove(self->ob_item + (start + needed) * itemsize,
                self->ob_item + stop * itemsize,
                (Py_SIZE(self) - stop) * itemsize);
            if (array_resize(self, Py_SIZE(self) +
                needed - slicelength) < 0)
                return -1;
        }
        else if (slicelength < needed) {
            if (array_resize(self, Py_SIZE(self) +
                needed - slicelength) < 0)
                return -1;
            memmove(self->ob_item + (start + needed) * itemsize,
                self->ob_item + stop * itemsize,
                (Py_SIZE(self) - start - needed) * itemsize);
        }
        if (needed > 0)
            memcpy(self->ob_item + start * itemsize,
                   other->ob_item, needed * itemsize);
        return 0;
    }
    else if (needed == 0) {
        /* Delete slice */
        size_t cur;
        Py_ssize_t i;

        if (step < 0) {
            stop = start + 1;
            start = stop + step * (slicelength - 1) - 1;
            step = -step;
        }
        for (cur = start, i = 0; i < slicelength;
             cur += step, i++) {
            Py_ssize_t lim = step - 1;

            if (cur + step >= (size_t)Py_SIZE(self))
                lim = Py_SIZE(self) - cur - 1;
            memmove(self->ob_item + (cur - i) * itemsize,
                self->ob_item + (cur + 1) * itemsize,
                lim * itemsize);
        }
        cur = start + slicelength * step;
        if (cur < (size_t)Py_SIZE(self)) {
            memmove(self->ob_item + (cur-slicelength) * itemsize,
                self->ob_item + cur * itemsize,
                (Py_SIZE(self) - cur) * itemsize);
        }
        if (array_resize(self, Py_SIZE(self) - slicelength) < 0)
            return -1;
        return 0;
    }
    else {
        Py_ssize_t cur, i;

        if (needed != slicelength) {
            PyErr_Format(PyExc_ValueError,
                "attempt to assign array of size %zd "
                "to extended slice of size %zd",
                needed, slicelength);
            return -1;
        }
        for (cur = start, i = 0; i < slicelength;
             cur += step, i++) {
            memcpy(self->ob_item + cur * itemsize,
                   other->ob_item + i * itemsize,
                   itemsize);
        }
        return 0;
    }
}

static PyMappingMethods array_as_mapping = {
    (lenfunc)array_length,
    (binaryfunc)array_subscr,
    (objobjargproc)array_ass_subscr
};

static const void *emptybuf = "";


static int
array_buffer_getbuf(arrayobject *self, Py_buffer *view, int flags)
{
    if (view==NULL) goto finish;

    view->buf = (void *)self->ob_item;
    view->obj = (PyObject*)self;
    Py_INCREF(self);
    if (view->buf == NULL)
        view->buf = (void *)emptybuf;
    view->len = (Py_SIZE(self)) * self->ob_descr->itemsize;
    view->readonly = 0;
    view->ndim = 1;
    view->itemsize = self->ob_descr->itemsize;
    view->suboffsets = NULL;
    view->shape = NULL;
    if ((flags & PyBUF_ND)==PyBUF_ND) {
        view->shape = &((Py_SIZE(self)));
    }
    view->strides = NULL;
    if ((flags & PyBUF_STRIDES)==PyBUF_STRIDES)
        view->strides = &(view->itemsize);
    view->format = NULL;
    view->internal = NULL;
    if ((flags & PyBUF_FORMAT) == PyBUF_FORMAT) {
        view->format = self->ob_descr->formats;
#ifdef Py_UNICODE_WIDE
        if (self->ob_descr->typecode == 'u') {
            view->format = "w";
        }
#endif
    }

 finish:
    self->ob_exports++;
    return 0;
}

static void
array_buffer_relbuf(arrayobject *self, Py_buffer *view)
{
    self->ob_exports--;
}

static PySequenceMethods array_as_sequence = {
    (lenfunc)array_length,                      /*sq_length*/
    (binaryfunc)array_concat,               /*sq_concat*/
    (ssizeargfunc)array_repeat,                 /*sq_repeat*/
    (ssizeargfunc)array_item,                           /*sq_item*/
    0,                                          /*sq_slice*/
    (ssizeobjargproc)array_ass_item,                    /*sq_ass_item*/
    0,                                          /*sq_ass_slice*/
    (objobjproc)array_contains,                 /*sq_contains*/
    (binaryfunc)array_inplace_concat,           /*sq_inplace_concat*/
    (ssizeargfunc)array_inplace_repeat          /*sq_inplace_repeat*/
};

static PyBufferProcs array_as_buffer = {
    (getbufferproc)array_buffer_getbuf,
    (releasebufferproc)array_buffer_relbuf
};

static PyObject *
array_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int c;
    PyObject *initial = NULL, *it = NULL;
    struct arraydescr *descr;

    if (type == &Arraytype && !_PyArg_NoKeywords("array.array()", kwds))
        return NULL;

    if (!PyArg_ParseTuple(args, "C|O:array", &c, &initial))
        return NULL;

    if (!(initial == NULL || PyList_Check(initial)
          || PyByteArray_Check(initial)
          || PyBytes_Check(initial)
          || PyTuple_Check(initial)
          || ((c=='u') && PyUnicode_Check(initial)))) {
        it = PyObject_GetIter(initial);
        if (it == NULL)
            return NULL;
        /* We set initial to NULL so that the subsequent code
           will create an empty array of the appropriate type
           and afterwards we can use array_iter_extend to populate
           the array.
        */
        initial = NULL;
    }
    for (descr = descriptors; descr->typecode != '\0'; descr++) {
        if (descr->typecode == c) {
            PyObject *a;
            Py_ssize_t len;

            if (initial == NULL || !(PyList_Check(initial)
                || PyTuple_Check(initial)))
                len = 0;
            else
                len = PySequence_Size(initial);

            a = newarrayobject(type, len, descr);
            if (a == NULL)
                return NULL;

            if (len > 0) {
                Py_ssize_t i;
                for (i = 0; i < len; i++) {
                    PyObject *v =
                        PySequence_GetItem(initial, i);
                    if (v == NULL) {
                        Py_DECREF(a);
                        return NULL;
                    }
                    if (setarrayitem(a, i, v) != 0) {
                        Py_DECREF(v);
                        Py_DECREF(a);
                        return NULL;
                    }
                    Py_DECREF(v);
                }
            }
            else if (initial != NULL && (PyByteArray_Check(initial) ||
                               PyBytes_Check(initial))) {
                PyObject *t_initial, *v;
                t_initial = PyTuple_Pack(1, initial);
                if (t_initial == NULL) {
                    Py_DECREF(a);
                    return NULL;
                }
                v = array_frombytes((arrayobject *)a,
                                         t_initial);
                Py_DECREF(t_initial);
                if (v == NULL) {
                    Py_DECREF(a);
                    return NULL;
                }
                Py_DECREF(v);
            }
            else if (initial != NULL && PyUnicode_Check(initial))  {
                Py_ssize_t n = PyUnicode_GET_DATA_SIZE(initial);
                if (n > 0) {
                    arrayobject *self = (arrayobject *)a;
                    char *item = self->ob_item;
                    item = (char *)PyMem_Realloc(item, n);
                    if (item == NULL) {
                        PyErr_NoMemory();
                        Py_DECREF(a);
                        return NULL;
                    }
                    self->ob_item = item;
                    Py_SIZE(self) = n / sizeof(Py_UNICODE);
                    memcpy(item, PyUnicode_AS_DATA(initial), n);
                    self->allocated = Py_SIZE(self);
                }
            }
            if (it != NULL) {
                if (array_iter_extend((arrayobject *)a, it) == -1) {
                    Py_DECREF(it);
                    Py_DECREF(a);
                    return NULL;
                }
                Py_DECREF(it);
            }
            return a;
        }
    }
    PyErr_SetString(PyExc_ValueError,
        "bad typecode (must be b, B, u, h, H, i, I, l, L, f or d)");
    return NULL;
}


PyDoc_STRVAR(module_doc,
"This module defines an object type which can efficiently represent\n\
an array of basic values: characters, integers, floating point\n\
numbers.  Arrays are sequence types and behave very much like lists,\n\
except that the type of objects stored in them is constrained.  The\n\
type is specified at object creation time by using a type code, which\n\
is a single character.  The following type codes are defined:\n\
\n\
    Type code   C Type             Minimum size in bytes \n\
    'b'         signed integer     1 \n\
    'B'         unsigned integer   1 \n\
    'u'         Unicode character  2 (see note) \n\
    'h'         signed integer     2 \n\
    'H'         unsigned integer   2 \n\
    'i'         signed integer     2 \n\
    'I'         unsigned integer   2 \n\
    'l'         signed integer     4 \n\
    'L'         unsigned integer   4 \n\
    'f'         floating point     4 \n\
    'd'         floating point     8 \n\
\n\
NOTE: The 'u' typecode corresponds to Python's unicode character. On \n\
narrow builds this is 2-bytes on wide builds this is 4-bytes.\n\
\n\
The constructor is:\n\
\n\
array(typecode [, initializer]) -- create a new array\n\
");

PyDoc_STRVAR(arraytype_doc,
"array(typecode [, initializer]) -> array\n\
\n\
Return a new array whose items are restricted by typecode, and\n\
initialized from the optional initializer value, which must be a list,\n\
string. or iterable over elements of the appropriate type.\n\
\n\
Arrays represent basic values and behave very much like lists, except\n\
the type of objects stored in them is constrained.\n\
\n\
Methods:\n\
\n\
append() -- append a new item to the end of the array\n\
buffer_info() -- return information giving the current memory info\n\
byteswap() -- byteswap all the items of the array\n\
count() -- return number of occurrences of an object\n\
extend() -- extend array by appending multiple elements from an iterable\n\
fromfile() -- read items from a file object\n\
fromlist() -- append items from the list\n\
fromstring() -- append items from the string\n\
index() -- return index of first occurrence of an object\n\
insert() -- insert a new item into the array at a provided position\n\
pop() -- remove and return item (default last)\n\
remove() -- remove first occurrence of an object\n\
reverse() -- reverse the order of the items in the array\n\
tofile() -- write all items to a file object\n\
tolist() -- return the array converted to an ordinary list\n\
tostring() -- return the array converted to a string\n\
\n\
Attributes:\n\
\n\
typecode -- the typecode character used to create the array\n\
itemsize -- the length in bytes of one array item\n\
");

static PyObject *array_iter(arrayobject *ao);

static PyTypeObject Arraytype = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "array.array",
    sizeof(arrayobject),
    0,
    (destructor)array_dealloc,                  /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)array_repr,                       /* tp_repr */
    0,                                          /* tp_as_number*/
    &array_as_sequence,                         /* tp_as_sequence*/
    &array_as_mapping,                          /* tp_as_mapping*/
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    &array_as_buffer,                           /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    arraytype_doc,                              /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    array_richcompare,                          /* tp_richcompare */
    offsetof(arrayobject, weakreflist),         /* tp_weaklistoffset */
    (getiterfunc)array_iter,                    /* tp_iter */
    0,                                          /* tp_iternext */
    array_methods,                              /* tp_methods */
    0,                                          /* tp_members */
    array_getsets,                              /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    array_new,                                  /* tp_new */
    PyObject_Del,                               /* tp_free */
};


/*********************** Array Iterator **************************/

typedef struct {
    PyObject_HEAD
    Py_ssize_t                          index;
    arrayobject                 *ao;
    PyObject                    * (*getitem)(struct arrayobject *, Py_ssize_t);
} arrayiterobject;

static PyTypeObject PyArrayIter_Type;

#define PyArrayIter_Check(op) PyObject_TypeCheck(op, &PyArrayIter_Type)

static PyObject *
array_iter(arrayobject *ao)
{
    arrayiterobject *it;

    if (!array_Check(ao)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    it = PyObject_GC_New(arrayiterobject, &PyArrayIter_Type);
    if (it == NULL)
        return NULL;

    Py_INCREF(ao);
    it->ao = ao;
    it->index = 0;
    it->getitem = ao->ob_descr->getitem;
    PyObject_GC_Track(it);
    return (PyObject *)it;
}

static PyObject *
arrayiter_next(arrayiterobject *it)
{
    assert(PyArrayIter_Check(it));
    if (it->index < Py_SIZE(it->ao))
        return (*it->getitem)(it->ao, it->index++);
    return NULL;
}

static void
arrayiter_dealloc(arrayiterobject *it)
{
    PyObject_GC_UnTrack(it);
    Py_XDECREF(it->ao);
    PyObject_GC_Del(it);
}

static int
arrayiter_traverse(arrayiterobject *it, visitproc visit, void *arg)
{
    Py_VISIT(it->ao);
    return 0;
}

static PyTypeObject PyArrayIter_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "arrayiterator",                        /* tp_name */
    sizeof(arrayiterobject),                /* tp_basicsize */
    0,                                      /* tp_itemsize */
    /* methods */
    (destructor)arrayiter_dealloc,              /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_reserved */
    0,                                      /* tp_repr */
    0,                                      /* tp_as_number */
    0,                                      /* tp_as_sequence */
    0,                                      /* tp_as_mapping */
    0,                                      /* tp_hash */
    0,                                      /* tp_call */
    0,                                      /* tp_str */
    PyObject_GenericGetAttr,                /* tp_getattro */
    0,                                      /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                      /* tp_doc */
    (traverseproc)arrayiter_traverse,           /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)arrayiter_next,               /* tp_iternext */
    0,                                          /* tp_methods */
};


/*********************** Install Module **************************/

/* No functions in array module. */
static PyMethodDef a_methods[] = {
    {"_array_reconstructor", array_reconstructor, METH_VARARGS,
     PyDoc_STR("Internal. Used for pickling support.")},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef arraymodule = {
    PyModuleDef_HEAD_INIT,
    "array",
    module_doc,
    -1,
    a_methods,
    NULL,
    NULL,
    NULL,
    NULL
};


PyMODINIT_FUNC
PyInit_array(void)
{
    PyObject *m;
    PyObject *typecodes;
    Py_ssize_t size = 0;
    register Py_UNICODE *p;
    struct arraydescr *descr;

    if (PyType_Ready(&Arraytype) < 0)
        return NULL;
    Py_TYPE(&PyArrayIter_Type) = &PyType_Type;
    m = PyModule_Create(&arraymodule);
    if (m == NULL)
        return NULL;

    Py_INCREF((PyObject *)&Arraytype);
    PyModule_AddObject(m, "ArrayType", (PyObject *)&Arraytype);
    Py_INCREF((PyObject *)&Arraytype);
    PyModule_AddObject(m, "array", (PyObject *)&Arraytype);

    for (descr=descriptors; descr->typecode != '\0'; descr++) {
        size++;
    }

    typecodes = PyUnicode_FromStringAndSize(NULL, size);
    p = PyUnicode_AS_UNICODE(typecodes);
    for (descr = descriptors; descr->typecode != '\0'; descr++) {
        *p++ = (char)descr->typecode;
    }

    PyModule_AddObject(m, "typecodes", (PyObject *)typecodes);

    if (PyErr_Occurred()) {
        Py_DECREF(m);
        m = NULL;
    }
    return m;
}
