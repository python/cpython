/* Range object implementation */

#include "Python.h"

typedef struct {
    PyObject_HEAD
    long        start;
    long        step;
    long        len;
} rangeobject;

/* Return number of items in range (lo, hi, step).  step != 0
 * required.  The result always fits in an unsigned long.
 */
static unsigned long
get_len_of_range(long lo, long hi, long step)
{
    /* -------------------------------------------------------------
    If step > 0 and lo >= hi, or step < 0 and lo <= hi, the range is empty.
    Else for step > 0, if n values are in the range, the last one is
    lo + (n-1)*step, which must be <= hi-1.  Rearranging,
    n <= (hi - lo - 1)/step + 1, so taking the floor of the RHS gives
    the proper value.  Since lo < hi in this case, hi-lo-1 >= 0, so
    the RHS is non-negative and so truncation is the same as the
    floor.  Letting M be the largest positive long, the worst case
    for the RHS numerator is hi=M, lo=-M-1, and then
    hi-lo-1 = M-(-M-1)-1 = 2*M.  Therefore unsigned long has enough
    precision to compute the RHS exactly.  The analysis for step < 0
    is similar.
    ---------------------------------------------------------------*/
    assert(step != 0);
    if (step > 0 && lo < hi)
    return 1UL + (hi - 1UL - lo) / step;
    else if (step < 0 && lo > hi)
    return 1UL + (lo - 1UL - hi) / (0UL - step);
    else
    return 0UL;
}

static PyObject *
range_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    rangeobject *obj;
    long ilow = 0, ihigh = 0, istep = 1;
    unsigned long n;

    if (!_PyArg_NoKeywords("xrange()", kw))
        return NULL;

    if (PyTuple_Size(args) <= 1) {
        if (!PyArg_ParseTuple(args,
                        "l;xrange() requires 1-3 int arguments",
                        &ihigh))
            return NULL;
    }
    else {
        if (!PyArg_ParseTuple(args,
                        "ll|l;xrange() requires 1-3 int arguments",
                        &ilow, &ihigh, &istep))
            return NULL;
    }
    if (istep == 0) {
        PyErr_SetString(PyExc_ValueError, "xrange() arg 3 must not be zero");
        return NULL;
    }
    n = get_len_of_range(ilow, ihigh, istep);
    if (n > (unsigned long)LONG_MAX || (long)n > PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "xrange() result has too many items");
        return NULL;
    }

    obj = PyObject_New(rangeobject, &PyRange_Type);
    if (obj == NULL)
        return NULL;
    obj->start = ilow;
    obj->len   = (long)n;
    obj->step  = istep;
    return (PyObject *) obj;
}

PyDoc_STRVAR(range_doc,
"xrange([start,] stop[, step]) -> xrange object\n\
\n\
Like range(), but instead of returning a list, returns an object that\n\
generates the numbers in the range on demand.  For looping, this is \n\
slightly faster than range() and more memory efficient.");

static PyObject *
range_item(rangeobject *r, Py_ssize_t i)
{
    if (i < 0 || i >= r->len) {
        PyErr_SetString(PyExc_IndexError,
                        "xrange object index out of range");
        return NULL;
    }
    /* do calculation entirely using unsigned longs, to avoid
       undefined behaviour due to signed overflow. */
    return PyInt_FromLong((long)(r->start + (unsigned long)i * r->step));
}

static Py_ssize_t
range_length(rangeobject *r)
{
    return (Py_ssize_t)(r->len);
}

static PyObject *
range_repr(rangeobject *r)
{
    PyObject *rtn;

    if (r->start == 0 && r->step == 1)
        rtn = PyString_FromFormat("xrange(%ld)",
                                  r->start + r->len * r->step);

    else if (r->step == 1)
        rtn = PyString_FromFormat("xrange(%ld, %ld)",
                                  r->start,
                                  r->start + r->len * r->step);

    else
        rtn = PyString_FromFormat("xrange(%ld, %ld, %ld)",
                                  r->start,
                                  r->start + r->len * r->step,
                                  r->step);
    return rtn;
}

/* Pickling support */
static PyObject *
range_reduce(rangeobject *r, PyObject *args)
{
    return Py_BuildValue("(O(iii))", Py_TYPE(r),
                         r->start,
                         r->start + r->len * r->step,
                         r->step);
}

static PySequenceMethods range_as_sequence = {
    (lenfunc)range_length,      /* sq_length */
    0,                          /* sq_concat */
    0,                          /* sq_repeat */
    (ssizeargfunc)range_item, /* sq_item */
    0,                          /* sq_slice */
};

static PyObject * range_iter(PyObject *seq);
static PyObject * range_reverse(PyObject *seq);

PyDoc_STRVAR(reverse_doc,
"Returns a reverse iterator.");

static PyMethodDef range_methods[] = {
    {"__reversed__",            (PyCFunction)range_reverse, METH_NOARGS, reverse_doc},
    {"__reduce__",              (PyCFunction)range_reduce, METH_VARARGS},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyRange_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                          /* Number of items for varobject */
    "xrange",                   /* Name of this type */
    sizeof(rangeobject),        /* Basic object size */
    0,                          /* Item size for varobject */
    (destructor)PyObject_Del, /* tp_dealloc */
    0,                          /* tp_print */
    0,                          /* tp_getattr */
    0,                          /* tp_setattr */
    0,                          /* tp_compare */
    (reprfunc)range_repr,       /* tp_repr */
    0,                          /* tp_as_number */
    &range_as_sequence,         /* tp_as_sequence */
    0,                          /* tp_as_mapping */
    0,                          /* tp_hash */
    0,                          /* tp_call */
    0,                          /* tp_str */
    PyObject_GenericGetAttr,  /* tp_getattro */
    0,                          /* tp_setattro */
    0,                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,         /* tp_flags */
    range_doc,                  /* tp_doc */
    0,                          /* tp_traverse */
    0,                          /* tp_clear */
    0,                          /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
    range_iter,                 /* tp_iter */
    0,                          /* tp_iternext */
    range_methods,              /* tp_methods */
    0,                          /* tp_members */
    0,                          /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    0,                          /* tp_dictoffset */
    0,                          /* tp_init */
    0,                          /* tp_alloc */
    range_new,                  /* tp_new */
};

/*********************** Xrange Iterator **************************/

typedef struct {
    PyObject_HEAD
    long        index;
    long        start;
    long        step;
    long        len;
} rangeiterobject;

static PyObject *
rangeiter_next(rangeiterobject *r)
{
    if (r->index < r->len)
        return PyInt_FromLong(r->start + (r->index++) * r->step);
    return NULL;
}

static PyObject *
rangeiter_len(rangeiterobject *r)
{
    return PyInt_FromLong(r->len - r->index);
}

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

static PyMethodDef rangeiter_methods[] = {
    {"__length_hint__", (PyCFunction)rangeiter_len, METH_NOARGS, length_hint_doc},
    {NULL,              NULL}           /* sentinel */
};

static PyTypeObject Pyrangeiter_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                                      /* ob_size */
    "rangeiterator",                        /* tp_name */
    sizeof(rangeiterobject),                /* tp_basicsize */
    0,                                      /* tp_itemsize */
    /* methods */
    (destructor)PyObject_Del,                   /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    0,                                      /* tp_repr */
    0,                                      /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                      /* tp_as_mapping */
    0,                                      /* tp_hash */
    0,                                      /* tp_call */
    0,                                      /* tp_str */
    PyObject_GenericGetAttr,                /* tp_getattro */
    0,                                      /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                         /* tp_flags */
    0,                                      /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)rangeiter_next,               /* tp_iternext */
    rangeiter_methods,                          /* tp_methods */
    0,
};

static PyObject *
range_iter(PyObject *seq)
{
    rangeiterobject *it;

    if (!PyRange_Check(seq)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    it = PyObject_New(rangeiterobject, &Pyrangeiter_Type);
    if (it == NULL)
        return NULL;
    it->index = 0;
    it->start = ((rangeobject *)seq)->start;
    it->step = ((rangeobject *)seq)->step;
    it->len = ((rangeobject *)seq)->len;
    return (PyObject *)it;
}

static PyObject *
range_reverse(PyObject *seq)
{
    rangeiterobject *it;
    long start, step, len;

    if (!PyRange_Check(seq)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    it = PyObject_New(rangeiterobject, &Pyrangeiter_Type);
    if (it == NULL)
        return NULL;

    start = ((rangeobject *)seq)->start;
    step = ((rangeobject *)seq)->step;
    len = ((rangeobject *)seq)->len;

    it->index = 0;
    it->len = len;
    /* the casts below guard against signed overflow by turning it
       into unsigned overflow instead.  The correctness of this
       code still depends on conversion from unsigned long to long
       wrapping modulo ULONG_MAX+1, which isn't guaranteed (see
       C99 6.3.1.3p3) but seems to hold in practice for all
       platforms we're likely to meet.

       If step == LONG_MIN then we still end up with LONG_MIN
       after negation; but this works out, since we've still got
       the correct value modulo ULONG_MAX+1, and the range_item
       calculation is also done modulo ULONG_MAX+1.
    */
    it->start = (long)(start + (unsigned long)(len-1) * step);
    it->step = (long)(0UL-step);

    return (PyObject *)it;
}
