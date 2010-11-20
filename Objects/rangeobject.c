/* Range object implementation */

#include "Python.h"

/* Support objects whose length is > PY_SSIZE_T_MAX.

   This could be sped up for small PyLongs if they fit in an Py_ssize_t.
   This only matters on Win64.  Though we could use PY_LONG_LONG which
   would presumably help perf.
*/

typedef struct {
    PyObject_HEAD
    PyObject *start;
    PyObject *stop;
    PyObject *step;
} rangeobject;

/* Helper function for validating step.  Always returns a new reference or
   NULL on error.
*/
static PyObject *
validate_step(PyObject *step)
{
    /* No step specified, use a step of 1. */
    if (!step)
        return PyLong_FromLong(1);

    step = PyNumber_Index(step);
    if (step) {
        Py_ssize_t istep = PyNumber_AsSsize_t(step, NULL);
        if (istep == -1 && PyErr_Occurred()) {
            /* Ignore OverflowError, we know the value isn't 0. */
            PyErr_Clear();
        }
        else if (istep == 0) {
            PyErr_SetString(PyExc_ValueError,
                            "range() arg 3 must not be zero");
            Py_CLEAR(step);
        }
    }

    return step;
}

/* XXX(nnorwitz): should we error check if the user passes any empty ranges?
   range(-10)
   range(0, -5)
   range(0, 5, -1)
*/
static PyObject *
range_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    rangeobject *obj = NULL;
    PyObject *start = NULL, *stop = NULL, *step = NULL;

    if (!_PyArg_NoKeywords("range()", kw))
        return NULL;

    if (PyTuple_Size(args) <= 1) {
        if (!PyArg_UnpackTuple(args, "range", 1, 1, &stop))
            return NULL;
        stop = PyNumber_Index(stop);
        if (!stop)
            return NULL;
        start = PyLong_FromLong(0);
        if (!start) {
            Py_DECREF(stop);
            return NULL;
        }
        step = PyLong_FromLong(1);
        if (!step) {
            Py_DECREF(stop);
            Py_DECREF(start);
            return NULL;
        }
    }
    else {
        if (!PyArg_UnpackTuple(args, "range", 2, 3,
                               &start, &stop, &step))
            return NULL;

        /* Convert borrowed refs to owned refs */
        start = PyNumber_Index(start);
        if (!start)
            return NULL;
        stop = PyNumber_Index(stop);
        if (!stop) {
            Py_DECREF(start);
            return NULL;
        }
        step = validate_step(step);    /* Caution, this can clear exceptions */
        if (!step) {
            Py_DECREF(start);
            Py_DECREF(stop);
            return NULL;
        }
    }

    obj = PyObject_New(rangeobject, &PyRange_Type);
    if (obj == NULL)
        goto Fail;
    obj->start = start;
    obj->stop = stop;
    obj->step = step;
    return (PyObject *) obj;

Fail:
    Py_XDECREF(start);
    Py_XDECREF(stop);
    Py_XDECREF(step);
    return NULL;
}

PyDoc_STRVAR(range_doc,
"range([start,] stop[, step]) -> range object\n\
\n\
Returns an iterator that generates the numbers in the range on demand.");

static void
range_dealloc(rangeobject *r)
{
    Py_DECREF(r->start);
    Py_DECREF(r->stop);
    Py_DECREF(r->step);
    PyObject_Del(r);
}

/* Return number of items in range (lo, hi, step) as a PyLong object,
 * when arguments are PyLong objects.  Arguments MUST return 1 with
 * PyLong_Check().  Return NULL when there is an error.
 */
static PyObject*
range_length_obj(rangeobject *r)
{
    /* -------------------------------------------------------------
    Algorithm is equal to that of get_len_of_range(), but it operates
    on PyObjects (which are assumed to be PyLong objects).
    ---------------------------------------------------------------*/
    int cmp_result;
    PyObject *lo, *hi;
    PyObject *step = NULL;
    PyObject *diff = NULL;
    PyObject *one = NULL;
    PyObject *tmp1 = NULL, *tmp2 = NULL, *result;
                /* holds sub-expression evaluations */

    PyObject *zero = PyLong_FromLong(0);
    if (zero == NULL)
        return NULL;
    cmp_result = PyObject_RichCompareBool(r->step, zero, Py_GT);
    Py_DECREF(zero);
    if (cmp_result == -1)
        return NULL;

    if (cmp_result == 1) {
        lo = r->start;
        hi = r->stop;
        step = r->step;
        Py_INCREF(step);
    } else {
        lo = r->stop;
        hi = r->start;
        step = PyNumber_Negative(r->step);
        if (!step)
            return NULL;
    }

    /* if (lo >= hi), return length of 0. */
    if (PyObject_RichCompareBool(lo, hi, Py_GE) == 1) {
        Py_XDECREF(step);
        return PyLong_FromLong(0);
    }

    if ((one = PyLong_FromLong(1L)) == NULL)
        goto Fail;

    if ((tmp1 = PyNumber_Subtract(hi, lo)) == NULL)
        goto Fail;

    if ((diff = PyNumber_Subtract(tmp1, one)) == NULL)
        goto Fail;

    if ((tmp2 = PyNumber_FloorDivide(diff, step)) == NULL)
        goto Fail;

    if ((result = PyNumber_Add(tmp2, one)) == NULL)
        goto Fail;

    Py_DECREF(tmp2);
    Py_DECREF(diff);
    Py_DECREF(step);
    Py_DECREF(tmp1);
    Py_DECREF(one);
    return result;

  Fail:
    Py_XDECREF(tmp2);
    Py_XDECREF(diff);
    Py_XDECREF(step);
    Py_XDECREF(tmp1);
    Py_XDECREF(one);
    return NULL;
}

static Py_ssize_t
range_length(rangeobject *r)
{
    PyObject *len = range_length_obj(r);
    Py_ssize_t result = -1;
    if (len) {
        result = PyLong_AsSsize_t(len);
        Py_DECREF(len);
    }
    return result;
}

/* range(...)[x] is necessary for:  seq[:] = range(...) */

static PyObject *
range_item(rangeobject *r, Py_ssize_t i)
{
    Py_ssize_t len = range_length(r);
    PyObject *rem, *incr, *result;

    /* XXX(nnorwitz): should negative indices be supported? */
    /* XXX(nnorwitz): should support range[x] where x > PY_SSIZE_T_MAX? */
    if (i < 0 || i >= len) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_IndexError,
                            "range object index out of range");
        return NULL;
    }

    /* XXX(nnorwitz): optimize for short ints. */
    rem = PyLong_FromSsize_t(i);
    if (!rem)
        return NULL;
    incr = PyNumber_Multiply(rem, r->step);
    Py_DECREF(rem);
    if (!incr)
        return NULL;
    result = PyNumber_Add(r->start, incr);
    Py_DECREF(incr);
    return result;
}

static PyObject *
range_repr(rangeobject *r)
{
    Py_ssize_t istep;

    /* Check for special case values for printing.  We don't always
       need the step value.  We don't care about errors
       (it means overflow), so clear the errors. */
    istep = PyNumber_AsSsize_t(r->step, NULL);
    if (istep != 1 || (istep == -1 && PyErr_Occurred())) {
        PyErr_Clear();
    }

    if (istep == 1)
        return PyUnicode_FromFormat("range(%R, %R)", r->start, r->stop);
    else
        return PyUnicode_FromFormat("range(%R, %R, %R)",
                                    r->start, r->stop, r->step);
}

/* Pickling support */
static PyObject *
range_reduce(rangeobject *r, PyObject *args)
{
        return Py_BuildValue("(O(OOO))", Py_TYPE(r),
                         r->start, r->stop, r->step);
}

/* Assumes (PyLong_CheckExact(ob) || PyBool_Check(ob)) */
static int
range_contains_long(rangeobject *r, PyObject *ob)
{
    int cmp1, cmp2, cmp3;
    PyObject *tmp1 = NULL;
    PyObject *tmp2 = NULL;
    PyObject *zero = NULL;
    int result = -1;

    zero = PyLong_FromLong(0);
    if (zero == NULL) /* MemoryError in int(0) */
        goto end;

    /* Check if the value can possibly be in the range. */

    cmp1 = PyObject_RichCompareBool(r->step, zero, Py_GT);
    if (cmp1 == -1)
        goto end;
    if (cmp1 == 1) { /* positive steps: start <= ob < stop */
        cmp2 = PyObject_RichCompareBool(r->start, ob, Py_LE);
        cmp3 = PyObject_RichCompareBool(ob, r->stop, Py_LT);
    }
    else { /* negative steps: stop < ob <= start */
        cmp2 = PyObject_RichCompareBool(ob, r->start, Py_LE);
        cmp3 = PyObject_RichCompareBool(r->stop, ob, Py_LT);
    }

    if (cmp2 == -1 || cmp3 == -1) /* TypeError */
        goto end;
    if (cmp2 == 0 || cmp3 == 0) { /* ob outside of range */
        result = 0;
        goto end;
    }

    /* Check that the stride does not invalidate ob's membership. */
    tmp1 = PyNumber_Subtract(ob, r->start);
    if (tmp1 == NULL)
        goto end;
    tmp2 = PyNumber_Remainder(tmp1, r->step);
    if (tmp2 == NULL)
        goto end;
    /* result = (int(ob) - start % step) == 0 */
    result = PyObject_RichCompareBool(tmp2, zero, Py_EQ);
  end:
    Py_XDECREF(tmp1);
    Py_XDECREF(tmp2);
    Py_XDECREF(zero);
    return result;
}

static int
range_contains(rangeobject *r, PyObject *ob) {
    if (PyLong_CheckExact(ob) || PyBool_Check(ob))
        return range_contains_long(r, ob);

    return (int)_PySequence_IterSearch((PyObject*)r, ob,
                                       PY_ITERSEARCH_CONTAINS);
}

static PyObject *
range_count(rangeobject *r, PyObject *ob)
{
    if (PyLong_CheckExact(ob) || PyBool_Check(ob)) {
        int result = range_contains_long(r, ob);
        if (result == -1)
            return NULL;
        else if (result)
            return PyLong_FromLong(1);
        else
            return PyLong_FromLong(0);
    } else {
        Py_ssize_t count;
        count = _PySequence_IterSearch((PyObject*)r, ob, PY_ITERSEARCH_COUNT);
        if (count == -1)
            return NULL;
        return PyLong_FromSsize_t(count);
    }
}

static PyObject *
range_index(rangeobject *r, PyObject *ob)
{
    PyObject *idx, *tmp;
    int contains;
    PyObject *format_tuple, *err_string;
    static PyObject *err_format = NULL;

    if (!PyLong_CheckExact(ob) && !PyBool_Check(ob)) {
        Py_ssize_t index;
        index = _PySequence_IterSearch((PyObject*)r, ob, PY_ITERSEARCH_INDEX);
        if (index == -1)
            return NULL;
        return PyLong_FromSsize_t(index);
    }

    contains = range_contains_long(r, ob);
    if (contains == -1)
        return NULL;

    if (!contains)
        goto value_error;

    tmp = PyNumber_Subtract(ob, r->start);
    if (tmp == NULL)
        return NULL;

    /* idx = (ob - r.start) // r.step */
    idx = PyNumber_FloorDivide(tmp, r->step);
    Py_DECREF(tmp);
    return idx;

value_error:

    /* object is not in the range */
    if (err_format == NULL) {
        err_format = PyUnicode_FromString("%r is not in range");
        if (err_format == NULL)
            return NULL;
    }
    format_tuple = PyTuple_Pack(1, ob);
    if (format_tuple == NULL)
        return NULL;
    err_string = PyUnicode_Format(err_format, format_tuple);
    Py_DECREF(format_tuple);
    if (err_string == NULL)
        return NULL;
    PyErr_SetObject(PyExc_ValueError, err_string);
    Py_DECREF(err_string);
    return NULL;
}

static PySequenceMethods range_as_sequence = {
    (lenfunc)range_length,      /* sq_length */
    0,                  /* sq_concat */
    0,                  /* sq_repeat */
    (ssizeargfunc)range_item, /* sq_item */
    0,                  /* sq_slice */
    0, /* sq_ass_item */
    0, /* sq_ass_slice */
    (objobjproc)range_contains, /* sq_contains */
};

static PyObject * range_iter(PyObject *seq);
static PyObject * range_reverse(PyObject *seq);

PyDoc_STRVAR(reverse_doc,
"Returns a reverse iterator.");

PyDoc_STRVAR(count_doc,
"rangeobject.count(value) -> integer -- return number of occurrences of value");

PyDoc_STRVAR(index_doc,
"rangeobject.index(value, [start, [stop]]) -> integer -- return index of value.\n"
"Raises ValueError if the value is not present.");

static PyMethodDef range_methods[] = {
    {"__reversed__",    (PyCFunction)range_reverse, METH_NOARGS, reverse_doc},
    {"__reduce__",      (PyCFunction)range_reduce,  METH_VARARGS},
    {"count",           (PyCFunction)range_count,   METH_O,      count_doc},
    {"index",           (PyCFunction)range_index,   METH_O,      index_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyRange_Type = {
        PyVarObject_HEAD_INIT(&PyType_Type, 0)
        "range",                /* Name of this type */
        sizeof(rangeobject),    /* Basic object size */
        0,                      /* Item size for varobject */
        (destructor)range_dealloc, /* tp_dealloc */
        0,                      /* tp_print */
        0,                      /* tp_getattr */
        0,                      /* tp_setattr */
        0,                      /* tp_reserved */
        (reprfunc)range_repr,   /* tp_repr */
        0,                      /* tp_as_number */
        &range_as_sequence,     /* tp_as_sequence */
        0,                      /* tp_as_mapping */
        0,                      /* tp_hash */
        0,                      /* tp_call */
        0,                      /* tp_str */
        PyObject_GenericGetAttr,  /* tp_getattro */
        0,                      /* tp_setattro */
        0,                      /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT,     /* tp_flags */
        range_doc,              /* tp_doc */
        0,                      /* tp_traverse */
        0,                      /* tp_clear */
        0,                      /* tp_richcompare */
        0,                      /* tp_weaklistoffset */
        range_iter,             /* tp_iter */
        0,                      /* tp_iternext */
        range_methods,          /* tp_methods */
        0,                      /* tp_members */
        0,                      /* tp_getset */
        0,                      /* tp_base */
        0,                      /* tp_dict */
        0,                      /* tp_descr_get */
        0,                      /* tp_descr_set */
        0,                      /* tp_dictoffset */
        0,                      /* tp_init */
        0,                      /* tp_alloc */
        range_new,              /* tp_new */
};

/*********************** range Iterator **************************/

/* There are 2 types of iterators, one for C longs, the other for
   Python longs (ie, PyObjects).  This should make iteration fast
   in the normal case, but possible for any numeric value.
*/

typedef struct {
        PyObject_HEAD
        long    index;
        long    start;
        long    step;
        long    len;
} rangeiterobject;

static PyObject *
rangeiter_next(rangeiterobject *r)
{
    if (r->index < r->len)
        /* cast to unsigned to avoid possible signed overflow
           in intermediate calculations. */
        return PyLong_FromLong((long)(r->start +
                                      (unsigned long)(r->index++) * r->step));
    return NULL;
}

static PyObject *
rangeiter_len(rangeiterobject *r)
{
    return PyLong_FromLong(r->len - r->index);
}

typedef struct {
    PyObject_HEAD
    PyObject *index;
    PyObject *start;
    PyObject *step;
    PyObject *len;
} longrangeiterobject;

static PyObject *
longrangeiter_len(longrangeiterobject *r, PyObject *no_args)
{
    return PyNumber_Subtract(r->len, r->index);
}

static PyObject *rangeiter_new(PyTypeObject *, PyObject *args, PyObject *kw);

PyDoc_STRVAR(length_hint_doc,
             "Private method returning an estimate of len(list(it)).");

static PyMethodDef rangeiter_methods[] = {
    {"__length_hint__", (PyCFunction)rangeiter_len, METH_NOARGS,
        length_hint_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyRangeIter_Type = {
        PyVarObject_HEAD_INIT(&PyType_Type, 0)
        "range_iterator",                        /* tp_name */
        sizeof(rangeiterobject),                /* tp_basicsize */
        0,                                      /* tp_itemsize */
        /* methods */
        (destructor)PyObject_Del,               /* tp_dealloc */
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
        Py_TPFLAGS_DEFAULT,                     /* tp_flags */
        0,                                      /* tp_doc */
        0,                                      /* tp_traverse */
        0,                                      /* tp_clear */
        0,                                      /* tp_richcompare */
        0,                                      /* tp_weaklistoffset */
        PyObject_SelfIter,                      /* tp_iter */
        (iternextfunc)rangeiter_next,           /* tp_iternext */
        rangeiter_methods,                      /* tp_methods */
        0,                                      /* tp_members */
        0,                                      /* tp_getset */
        0,                                      /* tp_base */
        0,                                      /* tp_dict */
        0,                                      /* tp_descr_get */
        0,                                      /* tp_descr_set */
        0,                                      /* tp_dictoffset */
        0,                                      /* tp_init */
        0,                                      /* tp_alloc */
        rangeiter_new,                          /* tp_new */
};

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

/* Initialize a rangeiter object.  If the length of the rangeiter object
   is not representable as a C long, OverflowError is raised. */

static PyObject *
int_range_iter(long start, long stop, long step)
{
    rangeiterobject *it = PyObject_New(rangeiterobject, &PyRangeIter_Type);
    unsigned long ulen;
    if (it == NULL)
        return NULL;
    it->start = start;
    it->step = step;
    ulen = get_len_of_range(start, stop, step);
    if (ulen > (unsigned long)LONG_MAX) {
        Py_DECREF(it);
        PyErr_SetString(PyExc_OverflowError,
                        "range too large to represent as a range_iterator");
        return NULL;
    }
    it->len = (long)ulen;
    it->index = 0;
    return (PyObject *)it;
}

static PyObject *
rangeiter_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    long start, stop, step;

    if (!_PyArg_NoKeywords("rangeiter()", kw))
        return NULL;

    if (!PyArg_ParseTuple(args, "lll;rangeiter() requires 3 int arguments",
                          &start, &stop, &step))
        return NULL;

    return int_range_iter(start, stop, step);
}

static PyMethodDef longrangeiter_methods[] = {
    {"__length_hint__", (PyCFunction)longrangeiter_len, METH_NOARGS,
        length_hint_doc},
    {NULL,              NULL}           /* sentinel */
};

static void
longrangeiter_dealloc(longrangeiterobject *r)
{
    Py_XDECREF(r->index);
    Py_XDECREF(r->start);
    Py_XDECREF(r->step);
    Py_XDECREF(r->len);
    PyObject_Del(r);
}

static PyObject *
longrangeiter_next(longrangeiterobject *r)
{
    PyObject *one, *product, *new_index, *result;
    if (PyObject_RichCompareBool(r->index, r->len, Py_LT) != 1)
        return NULL;

    one = PyLong_FromLong(1);
    if (!one)
        return NULL;

    new_index = PyNumber_Add(r->index, one);
    Py_DECREF(one);
    if (!new_index)
        return NULL;

    product = PyNumber_Multiply(r->index, r->step);
    if (!product) {
        Py_DECREF(new_index);
        return NULL;
    }

    result = PyNumber_Add(r->start, product);
    Py_DECREF(product);
    if (result) {
        Py_DECREF(r->index);
        r->index = new_index;
    }
    else {
        Py_DECREF(new_index);
    }

    return result;
}

PyTypeObject PyLongRangeIter_Type = {
        PyVarObject_HEAD_INIT(&PyType_Type, 0)
        "longrange_iterator",                   /* tp_name */
        sizeof(longrangeiterobject),            /* tp_basicsize */
        0,                                      /* tp_itemsize */
        /* methods */
        (destructor)longrangeiter_dealloc,      /* tp_dealloc */
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
        Py_TPFLAGS_DEFAULT,                     /* tp_flags */
        0,                                      /* tp_doc */
        0,                                      /* tp_traverse */
        0,                                      /* tp_clear */
        0,                                      /* tp_richcompare */
        0,                                      /* tp_weaklistoffset */
        PyObject_SelfIter,                      /* tp_iter */
        (iternextfunc)longrangeiter_next,       /* tp_iternext */
        longrangeiter_methods,                  /* tp_methods */
        0,
};

static PyObject *
range_iter(PyObject *seq)
{
    rangeobject *r = (rangeobject *)seq;
    longrangeiterobject *it;
    long lstart, lstop, lstep;
    PyObject *int_it;

    assert(PyRange_Check(seq));

    /* If all three fields and the length convert to long, use the int
     * version */
    lstart = PyLong_AsLong(r->start);
    if (lstart == -1 && PyErr_Occurred()) {
        PyErr_Clear();
        goto long_range;
    }
    lstop = PyLong_AsLong(r->stop);
    if (lstop == -1 && PyErr_Occurred()) {
        PyErr_Clear();
        goto long_range;
    }
    lstep = PyLong_AsLong(r->step);
    if (lstep == -1 && PyErr_Occurred()) {
        PyErr_Clear();
        goto long_range;
    }
    int_it = int_range_iter(lstart, lstop, lstep);
    if (int_it == NULL && PyErr_ExceptionMatches(PyExc_OverflowError)) {
        PyErr_Clear();
        goto long_range;
    }
    return (PyObject *)int_it;

  long_range:
    it = PyObject_New(longrangeiterobject, &PyLongRangeIter_Type);
    if (it == NULL)
        return NULL;

    /* Do all initialization here, so we can DECREF on failure. */
    it->start = r->start;
    it->step = r->step;
    Py_INCREF(it->start);
    Py_INCREF(it->step);

    it->len = it->index = NULL;

    it->len = range_length_obj(r);
    if (!it->len)
        goto create_failure;
    it->index = PyLong_FromLong(0);
    if (!it->index)
        goto create_failure;

    return (PyObject *)it;

create_failure:
    Py_DECREF(it);
    return NULL;
}

static PyObject *
range_reverse(PyObject *seq)
{
    rangeobject *range = (rangeobject*) seq;
    longrangeiterobject *it;
    PyObject *one, *sum, *diff, *len = NULL, *product;
    long lstart, lstop, lstep, new_start, new_stop;
    unsigned long ulen;

    assert(PyRange_Check(seq));

    /* reversed(range(start, stop, step)) can be expressed as
       range(start+(n-1)*step, start-step, -step), where n is the number of
       integers in the range.

       If each of start, stop, step, -step, start-step, and the length
       of the iterator is representable as a C long, use the int
       version.  This excludes some cases where the reversed range is
       representable as a range_iterator, but it's good enough for
       common cases and it makes the checks simple. */

    lstart = PyLong_AsLong(range->start);
    if (lstart == -1 && PyErr_Occurred()) {
        PyErr_Clear();
        goto long_range;
    }
    lstop = PyLong_AsLong(range->stop);
    if (lstop == -1 && PyErr_Occurred()) {
        PyErr_Clear();
        goto long_range;
    }
    lstep = PyLong_AsLong(range->step);
    if (lstep == -1 && PyErr_Occurred()) {
        PyErr_Clear();
        goto long_range;
    }
    /* check for possible overflow of -lstep */
    if (lstep == LONG_MIN)
        goto long_range;

    /* check for overflow of lstart - lstep:

       for lstep > 0, need only check whether lstart - lstep < LONG_MIN.
       for lstep < 0, need only check whether lstart - lstep > LONG_MAX

       Rearrange these inequalities as:

           lstart - LONG_MIN < lstep  (lstep > 0)
           LONG_MAX - lstart < -lstep  (lstep < 0)

       and compute both sides as unsigned longs, to avoid the
       possibility of undefined behaviour due to signed overflow. */

    if (lstep > 0) {
         if ((unsigned long)lstart - LONG_MIN < (unsigned long)lstep)
            goto long_range;
    }
    else {
        if (LONG_MAX - (unsigned long)lstart < 0UL - lstep)
            goto long_range;
    }

    ulen = get_len_of_range(lstart, lstop, lstep);
    if (ulen > (unsigned long)LONG_MAX)
        goto long_range;

    new_stop = lstart - lstep;
    new_start = (long)(new_stop + ulen * lstep);
    return int_range_iter(new_start, new_stop, -lstep);

long_range:
    it = PyObject_New(longrangeiterobject, &PyLongRangeIter_Type);
    if (it == NULL)
        return NULL;

    /* start + (len - 1) * step */
    len = range_length_obj(range);
    if (!len)
        goto create_failure;

    /* Steal reference to len. */
    it->len = len;

    one = PyLong_FromLong(1);
    if (!one)
        goto create_failure;

    diff = PyNumber_Subtract(len, one);
    Py_DECREF(one);
    if (!diff)
        goto create_failure;

    product = PyNumber_Multiply(diff, range->step);
    Py_DECREF(diff);
    if (!product)
        goto create_failure;

    sum = PyNumber_Add(range->start, product);
    Py_DECREF(product);
    it->start = sum;
    if (!it->start)
        goto create_failure;

    it->step = PyNumber_Negative(range->step);
    if (!it->step)
        goto create_failure;

    it->index = PyLong_FromLong(0);
    if (!it->index)
        goto create_failure;

    return (PyObject *)it;

create_failure:
    Py_DECREF(it);
    return NULL;
}
