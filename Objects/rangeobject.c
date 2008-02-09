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
            goto Fail;
        stop = PyNumber_Index(stop);
        if (!stop)
            goto Fail;
        start = PyLong_FromLong(0);
        step = PyLong_FromLong(1);
        if (!start || !step)
            goto Fail;
    }
    else {
        if (!PyArg_UnpackTuple(args, "range", 2, 3,
                               &start, &stop, &step))
            goto Fail;

        /* Convert borrowed refs to owned refs */
        start = PyNumber_Index(start);
        stop = PyNumber_Index(stop);
        step = validate_step(step);
        if (!start || !stop || !step)
            goto Fail;
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

/* Return number of items in range (lo, hi, step), when arguments are
 * PyInt or PyLong objects.  step > 0 required.  Return a value < 0 if
 * & only if the true value is too large to fit in a signed long.
 * Arguments MUST return 1 with either PyLong_Check() or
 * PyLong_Check().  Return -1 when there is an error.
 */
static PyObject*
range_length_obj(rangeobject *r)
{
    /* -------------------------------------------------------------
    Algorithm is equal to that of get_len_of_range(), but it operates
    on PyObjects (which are assumed to be PyLong or PyInt objects).
    ---------------------------------------------------------------*/
    int cmp_result, cmp_call;
    PyObject *lo, *hi;
    PyObject *step = NULL;
    PyObject *diff = NULL;
    PyObject *one = NULL;
    PyObject *tmp1 = NULL, *tmp2 = NULL, *result;
                /* holds sub-expression evaluations */

    PyObject *zero = PyLong_FromLong(0);
    if (zero == NULL)
        return NULL;
    cmp_call = PyObject_Cmp(r->step, zero, &cmp_result);
    Py_DECREF(zero);
    if (cmp_call == -1)
        return NULL;

    assert(cmp_result != 0);
    if (cmp_result > 0) {
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
    if (PyObject_Compare(lo, hi) >= 0) {
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

static PySequenceMethods range_as_sequence = {
    (lenfunc)range_length,	/* sq_length */
    0,			/* sq_concat */
    0,			/* sq_repeat */
    (ssizeargfunc)range_item, /* sq_item */
    0,			/* sq_slice */
};

static PyObject * range_iter(PyObject *seq);
static PyObject * range_reverse(PyObject *seq);

PyDoc_STRVAR(reverse_doc,
"Returns a reverse iterator.");

static PyMethodDef range_methods[] = {
    {"__reversed__",	(PyCFunction)range_reverse, METH_NOARGS,
	reverse_doc},
    {NULL,		NULL}		/* sentinel */
};

PyTypeObject PyRange_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"range",		/* Name of this type */
	sizeof(rangeobject),	/* Basic object size */
	0,			/* Item size for varobject */
	(destructor)range_dealloc, /* tp_dealloc */
	0,			/* tp_print */
	0,			/* tp_getattr */
	0,			/* tp_setattr */
	0,			/* tp_compare */
	(reprfunc)range_repr,	/* tp_repr */
	0,			/* tp_as_number */
	&range_as_sequence,	/* tp_as_sequence */
	0,			/* tp_as_mapping */
	0,			/* tp_hash */
	0,			/* tp_call */
	0,			/* tp_str */
	PyObject_GenericGetAttr,  /* tp_getattro */
	0,			/* tp_setattro */
	0,			/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,	/* tp_flags */
	range_doc,		/* tp_doc */
	0,			/* tp_traverse */
	0,			/* tp_clear */
	0,			/* tp_richcompare */
	0,			/* tp_weaklistoffset */
	range_iter,		/* tp_iter */
	0,			/* tp_iternext */
	range_methods,		/* tp_methods */
	0,			/* tp_members */
	0,			/* tp_getset */
	0,			/* tp_base */
	0,			/* tp_dict */
	0,			/* tp_descr_get */
	0,			/* tp_descr_set */
	0,			/* tp_dictoffset */
	0,			/* tp_init */
	0,			/* tp_alloc */
	range_new,		/* tp_new */
};

/*********************** range Iterator **************************/

/* There are 2 types of iterators, one for C longs, the other for
   Python longs (ie, PyObjects).  This should make iteration fast
   in the normal case, but possible for any numeric value.
*/

typedef struct {
	PyObject_HEAD
	long	index;
	long	start;
	long	step;
	long	len;
} rangeiterobject;

static PyObject *
rangeiter_next(rangeiterobject *r)
{
    if (r->index < r->len)
        return PyLong_FromLong(r->start + (r->index++) * r->step);
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
    {NULL,		NULL}		/* sentinel */
};

PyTypeObject PyRangeIter_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"range_iterator",                        /* tp_name */
	sizeof(rangeiterobject),                /* tp_basicsize */
	0,                                      /* tp_itemsize */
	/* methods */
	(destructor)PyObject_Del,		/* tp_dealloc */
	0,                                      /* tp_print */
	0,                                      /* tp_getattr */
	0,                                      /* tp_setattr */
	0,                                      /* tp_compare */
	0,                                      /* tp_repr */
	0,                                      /* tp_as_number */
	0,					/* tp_as_sequence */
	0,                                      /* tp_as_mapping */
	0,                                      /* tp_hash */
	0,                                      /* tp_call */
	0,                                      /* tp_str */
	PyObject_GenericGetAttr,                /* tp_getattro */
	0,                                      /* tp_setattro */
	0,                                      /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
	0,                                      /* tp_doc */
	0,					/* tp_traverse */
	0,                                      /* tp_clear */
	0,                                      /* tp_richcompare */
	0,                                      /* tp_weaklistoffset */
	PyObject_SelfIter,			/* tp_iter */
	(iternextfunc)rangeiter_next,		/* tp_iternext */
	rangeiter_methods,			/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	rangeiter_new,				/* tp_new */
};

/* Return number of items in range/xrange (lo, hi, step).  step > 0
 * required.  Return a value < 0 if & only if the true value is too
 * large to fit in a signed long.
 */
static long
get_len_of_range(long lo, long hi, long step)
{
    /* -------------------------------------------------------------
    If lo >= hi, the range is empty.
    Else if n values are in the range, the last one is
    lo + (n-1)*step, which must be <= hi-1.  Rearranging,
    n <= (hi - lo - 1)/step + 1, so taking the floor of the RHS gives
    the proper value.  Since lo < hi in this case, hi-lo-1 >= 0, so
    the RHS is non-negative and so truncation is the same as the
    floor.  Letting M be the largest positive long, the worst case
    for the RHS numerator is hi=M, lo=-M-1, and then
    hi-lo-1 = M-(-M-1)-1 = 2*M.  Therefore unsigned long has enough
    precision to compute the RHS exactly.
    ---------------------------------------------------------------*/
    long n = 0;
    if (lo < hi) {
        unsigned long uhi = (unsigned long)hi;
        unsigned long ulo = (unsigned long)lo;
        unsigned long diff = uhi - ulo - 1;
        n = (long)(diff / (unsigned long)step + 1);
    }
    return n;
}

static PyObject *
int_range_iter(long start, long stop, long step)
{
    rangeiterobject *it = PyObject_New(rangeiterobject, &PyRangeIter_Type);
    if (it == NULL)
        return NULL;
    it->start = start;
    it->step = step;
    if (step > 0)
        it->len = get_len_of_range(start, stop, step);
    else
        it->len = get_len_of_range(stop, start, -step);
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
    {NULL,		NULL}		/* sentinel */
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

    product = PyNumber_Multiply(r->index, r->step);
    if (!product) {
        Py_DECREF(one);
        return NULL;
    }

    new_index = PyNumber_Add(r->index, one);
    Py_DECREF(one);
    if (!new_index) {
        Py_DECREF(product);
        return NULL;
    }

    result = PyNumber_Add(r->start, product);
    Py_DECREF(product);
    if (result) {
        Py_DECREF(r->index);
        r->index = new_index;
    }

    return result;
}

PyTypeObject PyLongRangeIter_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"longrange_iterator",                   /* tp_name */
	sizeof(longrangeiterobject),            /* tp_basicsize */
	0,                                      /* tp_itemsize */
	/* methods */
	(destructor)longrangeiter_dealloc,	/* tp_dealloc */
	0,                                      /* tp_print */
	0,                                      /* tp_getattr */
	0,                                      /* tp_setattr */
	0,                                      /* tp_compare */
	0,                                      /* tp_repr */
	0,                                      /* tp_as_number */
	0,					/* tp_as_sequence */
	0,                                      /* tp_as_mapping */
	0,                                      /* tp_hash */
	0,                                      /* tp_call */
	0,                                      /* tp_str */
	PyObject_GenericGetAttr,                /* tp_getattro */
	0,                                      /* tp_setattro */
	0,                                      /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
	0,                                      /* tp_doc */
	0,					/* tp_traverse */
	0,                                      /* tp_clear */
	0,                                      /* tp_richcompare */
	0,                                      /* tp_weaklistoffset */
	PyObject_SelfIter,			/* tp_iter */
	(iternextfunc)longrangeiter_next,	/* tp_iternext */
	longrangeiter_methods,			/* tp_methods */
	0,
};

static PyObject *
range_iter(PyObject *seq)
{
    rangeobject *r = (rangeobject *)seq;
    longrangeiterobject *it;
    PyObject *tmp, *len;
    long lstart, lstop, lstep;

    assert(PyRange_Check(seq));

    /* If all three fields convert to long, use the int version */
    lstart = PyLong_AsLong(r->start);
    if (lstart != -1 || !PyErr_Occurred()) {
	lstop = PyLong_AsLong(r->stop);
	if (lstop != -1 || !PyErr_Occurred()) {
	    lstep = PyLong_AsLong(r->step);
	    if (lstep != -1 || !PyErr_Occurred())
		return int_range_iter(lstart, lstop, lstep);
	}
    }
    /* Some conversion failed, so there is an error set. Clear it,
       and try again with a long range. */
    PyErr_Clear();

    it = PyObject_New(longrangeiterobject, &PyLongRangeIter_Type);
    if (it == NULL)
        return NULL;

    /* Do all initialization here, so we can DECREF on failure. */
    it->start = r->start;
    it->step = r->step;
    Py_INCREF(it->start);
    Py_INCREF(it->step);

    it->len = it->index = NULL;

    /* Calculate length: (r->stop - r->start) / r->step */
    tmp = PyNumber_Subtract(r->stop, r->start);
    if (!tmp)
        goto create_failure;
    len = PyNumber_FloorDivide(tmp, r->step);
    Py_DECREF(tmp);
    if (!len)
        goto create_failure;
    it->len = len;
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
    long lstart, lstop, lstep;

    /* XXX(nnorwitz): do the calc for the new start/stop first,
        then if they fit, call the proper iter()?
    */
    assert(PyRange_Check(seq));

    /* If all three fields convert to long, use the int version */
    lstart = PyLong_AsLong(range->start);
    if (lstart != -1 || !PyErr_Occurred()) {
	lstop = PyLong_AsLong(range->stop);
	if (lstop != -1 || !PyErr_Occurred()) {
	    lstep = PyLong_AsLong(range->step);
	    if (lstep != -1 || !PyErr_Occurred()) {
		/* XXX(nnorwitz): need to check for overflow and simplify. */
		long len = get_len_of_range(lstart, lstop, lstep);
		long new_start = lstart + (len - 1) * lstep;
		long new_stop = lstart;
		if (lstep > 0)
		    new_stop -= 1;
		else
		    new_stop += 1;
		return int_range_iter(new_start, new_stop, -lstep);
	    }
	}
    }
    PyErr_Clear();

    it = PyObject_New(longrangeiterobject, &PyLongRangeIter_Type);
    if (it == NULL)
        return NULL;

    /* start + (len - 1) * step */
    len = range_length_obj(range);
    if (!len)
        goto create_failure;

    one = PyLong_FromLong(1);
    if (!one)
        goto create_failure;

    diff = PyNumber_Subtract(len, one);
    Py_DECREF(one);
    if (!diff)
        goto create_failure;

    product = PyNumber_Multiply(len, range->step);
    if (!product)
        goto create_failure;

    sum = PyNumber_Add(range->start, product);
    Py_DECREF(product);
    it->start = sum;
    if (!it->start)
        goto create_failure;
    it->step = PyNumber_Negative(range->step);
    if (!it->step) {
        Py_DECREF(it->start);
        PyObject_Del(it);
        return NULL;
    }

    /* Steal reference to len. */
    it->len = len;

    it->index = PyLong_FromLong(0);
    if (!it->index) {
        Py_DECREF(it);
        return NULL;
    }

    return (PyObject *)it;

create_failure:
    Py_XDECREF(len);
    PyObject_Del(it);
    return NULL;
}
