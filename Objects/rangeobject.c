/* Range object implementation */

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_ceval.h"         // _PyEval_GetBuiltin()
#include "pycore_freelist.h"
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_modsupport.h"    // _PyArg_NoKwnames()
#include "pycore_range.h"
#include "pycore_tuple.h"         // _PyTuple_ITEMS()


/* Support objects whose length is > PY_SSIZE_T_MAX.

   This could be sped up for small PyLongs if they fit in a Py_ssize_t.
   This only matters on Win64.  Though we could use long long which
   would presumably help perf.
*/

typedef struct {
    PyObject_HEAD
    PyObject *start;
    PyObject *stop;
    PyObject *step;
    PyObject *length;
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
    if (step && _PyLong_IsZero((PyLongObject *)step)) {
        PyErr_SetString(PyExc_ValueError,
                        "range() arg 3 must not be zero");
        Py_CLEAR(step);
    }

    return step;
}

static PyObject *
compute_range_length(PyObject *start, PyObject *stop, PyObject *step);

static rangeobject *
make_range_object(PyTypeObject *type, PyObject *start,
                  PyObject *stop, PyObject *step)
{
    PyObject *length;
    length = compute_range_length(start, stop, step);
    if (length == NULL) {
        return NULL;
    }
    rangeobject *obj = _Py_FREELIST_POP(rangeobject, ranges);
    if (obj == NULL) {
        obj = PyObject_New(rangeobject, type);
        if (obj == NULL) {
            Py_DECREF(length);
            return NULL;
        }
    }
    obj->start = start;
    obj->stop = stop;
    obj->step = step;
    obj->length = length;
    return obj;
}

/* XXX(nnorwitz): should we error check if the user passes any empty ranges?
   range(-10)
   range(0, -5)
   range(0, 5, -1)
*/
static PyObject *
range_from_array(PyTypeObject *type, PyObject *const *args, Py_ssize_t num_args)
{
    rangeobject *obj;
    PyObject *start = NULL, *stop = NULL, *step = NULL;

    switch (num_args) {
        case 3:
            step = args[2];
            _Py_FALLTHROUGH;
        case 2:
            /* Convert borrowed refs to owned refs */
            start = PyNumber_Index(args[0]);
            if (!start) {
                return NULL;
            }
            stop = PyNumber_Index(args[1]);
            if (!stop) {
                Py_DECREF(start);
                return NULL;
            }
            step = validate_step(step);  /* Caution, this can clear exceptions */
            if (!step) {
                Py_DECREF(start);
                Py_DECREF(stop);
                return NULL;
            }
            break;
        case 1:
            stop = PyNumber_Index(args[0]);
            if (!stop) {
                return NULL;
            }
            start = _PyLong_GetZero();
            step = _PyLong_GetOne();
            break;
        case 0:
            PyErr_SetString(PyExc_TypeError,
                            "range expected at least 1 argument, got 0");
            return NULL;
        default:
            PyErr_Format(PyExc_TypeError,
                         "range expected at most 3 arguments, got %zd",
                         num_args);
            return NULL;
    }
    obj = make_range_object(type, start, stop, step);
    if (obj != NULL) {
        return (PyObject *) obj;
    }

    /* Failed to create object, release attributes */
    Py_DECREF(start);
    Py_DECREF(stop);
    Py_DECREF(step);
    return NULL;
}

static PyObject *
range_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    if (!_PyArg_NoKeywords("range", kw))
        return NULL;

    return range_from_array(type, _PyTuple_ITEMS(args), PyTuple_GET_SIZE(args));
}


static PyObject *
range_vectorcall(PyObject *rangetype, PyObject *const *args,
                 size_t nargsf, PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (!_PyArg_NoKwnames("range", kwnames)) {
        return NULL;
    }
    return range_from_array((PyTypeObject *)rangetype, args, nargs);
}

PyDoc_STRVAR(range_doc,
"range(stop) -> range object\n\
range(start, stop[, step]) -> range object\n\
\n\
Return an object that produces a sequence of integers from start (inclusive)\n\
to stop (exclusive) by step.  range(i, j) produces i, i+1, i+2, ..., j-1.\n\
start defaults to 0, and stop is omitted!  range(4) produces 0, 1, 2, 3.\n\
These are exactly the valid indices for a list of 4 elements.\n\
When step is given, it specifies the increment (or decrement).");

static void
range_dealloc(PyObject *op)
{
    rangeobject *r = (rangeobject*)op;
    Py_DECREF(r->start);
    Py_DECREF(r->stop);
    Py_DECREF(r->step);
    Py_DECREF(r->length);
    _Py_FREELIST_FREE(ranges, r, PyObject_Free);
}

static unsigned long
get_len_of_range(long lo, long hi, long step);

/* Return the length as a long, -2 for an overflow and -1 for any other type of error
 *
 * In case of an overflow no error is set
 */
static long compute_range_length_long(PyObject *start,
                PyObject *stop, PyObject *step) {
    int overflow = 0;

    long long_start = PyLong_AsLongAndOverflow(start, &overflow);
    if (overflow) {
        return -2;
    }
    if (long_start == -1 && PyErr_Occurred()) {
        return -1;
    }
    long long_stop = PyLong_AsLongAndOverflow(stop, &overflow);
    if (overflow) {
        return -2;
    }
    if (long_stop == -1 && PyErr_Occurred()) {
        return -1;
    }
    long long_step = PyLong_AsLongAndOverflow(step, &overflow);
    if (overflow) {
        return -2;
    }
    if (long_step == -1 && PyErr_Occurred()) {
        return -1;
    }

    unsigned long ulen = get_len_of_range(long_start, long_stop, long_step);
    if (ulen > (unsigned long)LONG_MAX) {
        /* length too large for a long */
        return -2;
    }
    else {
        return (long)ulen;
    }
}

/* Return number of items in range (lo, hi, step) as a PyLong object,
 * when arguments are PyLong objects.  Arguments MUST return 1 with
 * PyLong_Check().  Return NULL when there is an error.
 */
static PyObject*
compute_range_length(PyObject *start, PyObject *stop, PyObject *step)
{
    /* -------------------------------------------------------------
    Algorithm is equal to that of get_len_of_range(), but it operates
    on PyObjects (which are assumed to be PyLong objects).
    ---------------------------------------------------------------*/
    int cmp_result;
    PyObject *lo, *hi;
    PyObject *diff = NULL;
    PyObject *tmp1 = NULL, *tmp2 = NULL, *result;
                /* holds sub-expression evaluations */

    PyObject *zero = _PyLong_GetZero();  // borrowed reference
    PyObject *one = _PyLong_GetOne();  // borrowed reference

    assert(PyLong_Check(start));
    assert(PyLong_Check(stop));
    assert(PyLong_Check(step));

    /* fast path when all arguments fit into a long integer */
    long len = compute_range_length_long(start, stop, step);
    if (len >= 0) {
        return PyLong_FromLong(len);
    }
    else if (len == -1) {
        /* unexpected error from compute_range_length_long, we propagate to the caller */
        return NULL;
    }
    assert(len == -2);

    cmp_result = PyObject_RichCompareBool(step, zero, Py_GT);
    if (cmp_result == -1)
        return NULL;

    if (cmp_result == 1) {
        lo = start;
        hi = stop;
        Py_INCREF(step);
    } else {
        lo = stop;
        hi = start;
        step = PyNumber_Negative(step);
        if (!step)
            return NULL;
    }

    /* if (lo >= hi), return length of 0. */
    cmp_result = PyObject_RichCompareBool(lo, hi, Py_GE);
    if (cmp_result != 0) {
        Py_DECREF(step);
        if (cmp_result < 0)
            return NULL;
        result = zero;
        return Py_NewRef(result);
    }

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
    return result;

  Fail:
    Py_DECREF(step);
    Py_XDECREF(tmp2);
    Py_XDECREF(diff);
    Py_XDECREF(tmp1);
    return NULL;
}

static Py_ssize_t
range_length(PyObject *op)
{
    rangeobject *r = (rangeobject*)op;
    return PyLong_AsSsize_t(r->length);
}

static PyObject *
compute_item(rangeobject *r, PyObject *i)
{
    PyObject *incr, *result;
    /* PyLong equivalent to:
     *    return r->start + (i * r->step)
     */
    if (r->step == _PyLong_GetOne()) {
        result = PyNumber_Add(r->start, i);
    }
    else {
        incr = PyNumber_Multiply(i, r->step);
        if (!incr) {
            return NULL;
        }
        result = PyNumber_Add(r->start, incr);
        Py_DECREF(incr);
    }
    return result;
}

static PyObject *
compute_range_item(rangeobject *r, PyObject *arg)
{
    PyObject *zero = _PyLong_GetZero();  // borrowed reference
    int cmp_result;
    PyObject *i, *result;

    /* PyLong equivalent to:
     *   if (arg < 0) {
     *     i = r->length + arg
     *   } else {
     *     i = arg
     *   }
     */
    cmp_result = PyObject_RichCompareBool(arg, zero, Py_LT);
    if (cmp_result == -1) {
        return NULL;
    }
    if (cmp_result == 1) {
        i = PyNumber_Add(r->length, arg);
        if (!i) {
          return NULL;
        }
    } else {
        i = Py_NewRef(arg);
    }

    /* PyLong equivalent to:
     *   if (i < 0 || i >= r->length) {
     *     <report index out of bounds>
     *   }
     */
    cmp_result = PyObject_RichCompareBool(i, zero, Py_LT);
    if (cmp_result == 0) {
        cmp_result = PyObject_RichCompareBool(i, r->length, Py_GE);
    }
    if (cmp_result == -1) {
       Py_DECREF(i);
       return NULL;
    }
    if (cmp_result == 1) {
        Py_DECREF(i);
        PyErr_SetString(PyExc_IndexError,
                        "range object index out of range");
        return NULL;
    }

    result = compute_item(r, i);
    Py_DECREF(i);
    return result;
}

static PyObject *
range_item(PyObject *op, Py_ssize_t i)
{
    rangeobject *r = (rangeobject*)op;
    PyObject *res, *arg = PyLong_FromSsize_t(i);
    if (!arg) {
        return NULL;
    }
    res = compute_range_item(r, arg);
    Py_DECREF(arg);
    return res;
}

static PyObject *
compute_slice(rangeobject *r, PyObject *_slice)
{
    PySliceObject *slice = (PySliceObject *) _slice;
    rangeobject *result;
    PyObject *start = NULL, *stop = NULL, *step = NULL;
    PyObject *substart = NULL, *substop = NULL, *substep = NULL;
    int error;

    error = _PySlice_GetLongIndices(slice, r->length, &start, &stop, &step);
    if (error == -1)
        return NULL;

    substep = PyNumber_Multiply(r->step, step);
    if (substep == NULL) goto fail;
    Py_CLEAR(step);

    substart = compute_item(r, start);
    if (substart == NULL) goto fail;
    Py_CLEAR(start);

    substop = compute_item(r, stop);
    if (substop == NULL) goto fail;
    Py_CLEAR(stop);

    result = make_range_object(Py_TYPE(r), substart, substop, substep);
    if (result != NULL) {
        return (PyObject *) result;
    }
fail:
    Py_XDECREF(start);
    Py_XDECREF(stop);
    Py_XDECREF(step);
    Py_XDECREF(substart);
    Py_XDECREF(substop);
    Py_XDECREF(substep);
    return NULL;
}

/* Assumes (PyLong_CheckExact(ob) || PyBool_Check(ob)) */
static int
range_contains_long(rangeobject *r, PyObject *ob)
{
    PyObject *zero = _PyLong_GetZero();  // borrowed reference
    int cmp1, cmp2, cmp3;
    PyObject *tmp1 = NULL;
    PyObject *tmp2 = NULL;
    int result = -1;

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
    /* result = ((int(ob) - start) % step) == 0 */
    result = PyObject_RichCompareBool(tmp2, zero, Py_EQ);
  end:
    Py_XDECREF(tmp1);
    Py_XDECREF(tmp2);
    return result;
}

static int
range_contains(PyObject *self, PyObject *ob)
{
    rangeobject *r = (rangeobject*)self;
    if (PyLong_CheckExact(ob) || PyBool_Check(ob))
        return range_contains_long(r, ob);

    return (int)_PySequence_IterSearch((PyObject*)r, ob,
                                       PY_ITERSEARCH_CONTAINS);
}

/* Compare two range objects.  Return 1 for equal, 0 for not equal
   and -1 on error.  The algorithm is roughly the C equivalent of

   if r0 is r1:
       return True
   if len(r0) != len(r1):
       return False
   if not len(r0):
       return True
   if r0.start != r1.start:
       return False
   if len(r0) == 1:
       return True
   return r0.step == r1.step
*/
static int
range_equals(rangeobject *r0, rangeobject *r1)
{
    int cmp_result;

    if (r0 == r1)
        return 1;
    cmp_result = PyObject_RichCompareBool(r0->length, r1->length, Py_EQ);
    /* Return False or error to the caller. */
    if (cmp_result != 1)
        return cmp_result;
    cmp_result = PyObject_Not(r0->length);
    /* Return True or error to the caller. */
    if (cmp_result != 0)
        return cmp_result;
    cmp_result = PyObject_RichCompareBool(r0->start, r1->start, Py_EQ);
    /* Return False or error to the caller. */
    if (cmp_result != 1)
        return cmp_result;
    cmp_result = PyObject_RichCompareBool(r0->length, _PyLong_GetOne(), Py_EQ);
    /* Return True or error to the caller. */
    if (cmp_result != 0)
        return cmp_result;
    return PyObject_RichCompareBool(r0->step, r1->step, Py_EQ);
}

static PyObject *
range_richcompare(PyObject *self, PyObject *other, int op)
{
    int result;

    if (!PyRange_Check(other))
        Py_RETURN_NOTIMPLEMENTED;
    switch (op) {
    case Py_NE:
    case Py_EQ:
        result = range_equals((rangeobject*)self, (rangeobject*)other);
        if (result == -1)
            return NULL;
        if (op == Py_NE)
            result = !result;
        if (result)
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    case Py_LE:
    case Py_GE:
    case Py_LT:
    case Py_GT:
        Py_RETURN_NOTIMPLEMENTED;
    default:
        PyErr_BadArgument();
        return NULL;
    }
}

/* Hash function for range objects.  Rough C equivalent of

   if not len(r):
       return hash((len(r), None, None))
   if len(r) == 1:
       return hash((len(r), r.start, None))
   return hash((len(r), r.start, r.step))
*/
static Py_hash_t
range_hash(PyObject *op)
{
    rangeobject *r = (rangeobject*)op;
    PyObject *t;
    Py_hash_t result = -1;
    int cmp_result;

    t = PyTuple_New(3);
    if (!t)
        return -1;
    PyTuple_SET_ITEM(t, 0, Py_NewRef(r->length));
    cmp_result = PyObject_Not(r->length);
    if (cmp_result == -1)
        goto end;
    if (cmp_result == 1) {
        PyTuple_SET_ITEM(t, 1, Py_NewRef(Py_None));
        PyTuple_SET_ITEM(t, 2, Py_NewRef(Py_None));
    }
    else {
        PyTuple_SET_ITEM(t, 1, Py_NewRef(r->start));
        cmp_result = PyObject_RichCompareBool(r->length, _PyLong_GetOne(), Py_EQ);
        if (cmp_result == -1)
            goto end;
        if (cmp_result == 1) {
            PyTuple_SET_ITEM(t, 2, Py_NewRef(Py_None));
        }
        else {
            PyTuple_SET_ITEM(t, 2, Py_NewRef(r->step));
        }
    }
    result = PyObject_Hash(t);
  end:
    Py_DECREF(t);
    return result;
}

static PyObject *
range_count(PyObject *self, PyObject *ob)
{
    rangeobject *r = (rangeobject*)self;
    if (PyLong_CheckExact(ob) || PyBool_Check(ob)) {
        int result = range_contains_long(r, ob);
        if (result == -1)
            return NULL;
        return PyLong_FromLong(result);
    } else {
        Py_ssize_t count;
        count = _PySequence_IterSearch((PyObject*)r, ob, PY_ITERSEARCH_COUNT);
        if (count == -1)
            return NULL;
        return PyLong_FromSsize_t(count);
    }
}

static PyObject *
range_index(PyObject *self, PyObject *ob)
{
    rangeobject *r = (rangeobject*)self;
    int contains;

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

    if (contains) {
        PyObject *idx = PyNumber_Subtract(ob, r->start);
        if (idx == NULL) {
            return NULL;
        }

        if (r->step == _PyLong_GetOne()) {
            return idx;
        }

        /* idx = (ob - r.start) // r.step */
        PyObject *sidx = PyNumber_FloorDivide(idx, r->step);
        Py_DECREF(idx);
        return sidx;
    }

    /* object is not in the range */
    PyErr_SetString(PyExc_ValueError, "range.index(x): x not in range");
    return NULL;
}

static PySequenceMethods range_as_sequence = {
    range_length,               /* sq_length */
    0,                          /* sq_concat */
    0,                          /* sq_repeat */
    range_item,                 /* sq_item */
    0,                          /* sq_slice */
    0,                          /* sq_ass_item */
    0,                          /* sq_ass_slice */
    range_contains,             /* sq_contains */
};

static PyObject *
range_repr(PyObject *op)
{
    rangeobject *r = (rangeobject*)op;
    Py_ssize_t istep;

    /* Check for special case values for printing.  We don't always
       need the step value.  We don't care about overflow. */
    istep = PyNumber_AsSsize_t(r->step, NULL);
    if (istep == -1 && PyErr_Occurred()) {
        assert(!PyErr_ExceptionMatches(PyExc_OverflowError));
        return NULL;
    }

    if (istep == 1)
        return PyUnicode_FromFormat("range(%R, %R)", r->start, r->stop);
    else
        return PyUnicode_FromFormat("range(%R, %R, %R)",
                                    r->start, r->stop, r->step);
}

/* Pickling support */
static PyObject *
range_reduce(PyObject *op, PyObject *args)
{
    rangeobject *r = (rangeobject*)op;
    return Py_BuildValue("(O(OOO))", Py_TYPE(r),
                         r->start, r->stop, r->step);
}

static PyObject *
range_subscript(PyObject *op, PyObject *item)
{
    rangeobject *self = (rangeobject*)op;
    if (_PyIndex_Check(item)) {
        PyObject *i, *result;
        i = PyNumber_Index(item);
        if (!i)
            return NULL;
        result = compute_range_item(self, i);
        Py_DECREF(i);
        return result;
    }
    if (PySlice_Check(item)) {
        return compute_slice(self, item);
    }
    PyErr_Format(PyExc_TypeError,
                 "range indices must be integers or slices, not %.200s",
                 Py_TYPE(item)->tp_name);
    return NULL;
}


static PyMappingMethods range_as_mapping = {
        range_length,                /* mp_length */
        range_subscript,             /* mp_subscript */
        0,                           /* mp_ass_subscript */
};

static int
range_bool(PyObject *op)
{
    rangeobject *self = (rangeobject*)op;
    return PyObject_IsTrue(self->length);
}

static PyNumberMethods range_as_number = {
    .nb_bool = range_bool,
};

static PyObject * range_iter(PyObject *seq);
static PyObject * range_reverse(PyObject *seq, PyObject *Py_UNUSED(ignored));

PyDoc_STRVAR(reverse_doc,
"Return a reverse iterator.");

PyDoc_STRVAR(count_doc,
"rangeobject.count(value) -> integer -- return number of occurrences of value");

PyDoc_STRVAR(index_doc,
"rangeobject.index(value) -> integer -- return index of value.\n"
"Raise ValueError if the value is not present.");

static PyMethodDef range_methods[] = {
    {"__reversed__",    range_reverse,              METH_NOARGS, reverse_doc},
    {"__reduce__",      range_reduce,               METH_NOARGS},
    {"count",           range_count,                METH_O,      count_doc},
    {"index",           range_index,                METH_O,      index_doc},
    {NULL,              NULL}           /* sentinel */
};

static PyMemberDef range_members[] = {
    {"start",   Py_T_OBJECT_EX,    offsetof(rangeobject, start),   Py_READONLY},
    {"stop",    Py_T_OBJECT_EX,    offsetof(rangeobject, stop),    Py_READONLY},
    {"step",    Py_T_OBJECT_EX,    offsetof(rangeobject, step),    Py_READONLY},
    {0}
};

PyTypeObject PyRange_Type = {
        PyVarObject_HEAD_INIT(&PyType_Type, 0)
        "range",                /* Name of this type */
        sizeof(rangeobject),    /* Basic object size */
        0,                      /* Item size for varobject */
        range_dealloc,          /* tp_dealloc */
        0,                      /* tp_vectorcall_offset */
        0,                      /* tp_getattr */
        0,                      /* tp_setattr */
        0,                      /* tp_as_async */
        range_repr,             /* tp_repr */
        &range_as_number,       /* tp_as_number */
        &range_as_sequence,     /* tp_as_sequence */
        &range_as_mapping,      /* tp_as_mapping */
        range_hash,             /* tp_hash */
        0,                      /* tp_call */
        0,                      /* tp_str */
        PyObject_GenericGetAttr,  /* tp_getattro */
        0,                      /* tp_setattro */
        0,                      /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_SEQUENCE,  /* tp_flags */
        range_doc,              /* tp_doc */
        0,                      /* tp_traverse */
        0,                      /* tp_clear */
        range_richcompare,      /* tp_richcompare */
        0,                      /* tp_weaklistoffset */
        range_iter,             /* tp_iter */
        0,                      /* tp_iternext */
        range_methods,          /* tp_methods */
        range_members,          /* tp_members */
        0,                      /* tp_getset */
        0,                      /* tp_base */
        0,                      /* tp_dict */
        0,                      /* tp_descr_get */
        0,                      /* tp_descr_set */
        0,                      /* tp_dictoffset */
        0,                      /* tp_init */
        0,                      /* tp_alloc */
        range_new,              /* tp_new */
        .tp_vectorcall = range_vectorcall
};

/*********************** range Iterator **************************/

/* There are 2 types of iterators, one for C longs, the other for
   Python ints (ie, PyObjects).  This should make iteration fast
   in the normal case, but possible for any numeric value.
*/

static PyObject *
rangeiter_next(PyObject *op)
{
    _PyRangeIterObject *r = (_PyRangeIterObject*)op;
    if (r->len > 0) {
        long result = r->start;
        r->start = result + r->step;
        r->len--;
        return PyLong_FromLong(result);
    }
    return NULL;
}

static PyObject *
rangeiter_len(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    _PyRangeIterObject *r = (_PyRangeIterObject*)op;
    return PyLong_FromLong(r->len);
}

PyDoc_STRVAR(length_hint_doc,
             "Private method returning an estimate of len(list(it)).");

static PyObject *
rangeiter_reduce(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    _PyRangeIterObject *r = (_PyRangeIterObject*)op;
    PyObject *start=NULL, *stop=NULL, *step=NULL;
    PyObject *range;

    /* create a range object for pickling */
    start = PyLong_FromLong(r->start);
    if (start == NULL)
        goto err;
    stop = PyLong_FromLong(r->start + r->len * r->step);
    if (stop == NULL)
        goto err;
    step = PyLong_FromLong(r->step);
    if (step == NULL)
        goto err;
    range = (PyObject*)make_range_object(&PyRange_Type,
                               start, stop, step);
    if (range == NULL)
        goto err;
    /* return the result */
    return Py_BuildValue("N(N)O", _PyEval_GetBuiltin(&_Py_ID(iter)),
                         range, Py_None);
err:
    Py_XDECREF(start);
    Py_XDECREF(stop);
    Py_XDECREF(step);
    return NULL;
}

static PyObject *
rangeiter_setstate(PyObject *op, PyObject *state)
{
    _PyRangeIterObject *r = (_PyRangeIterObject*)op;
    long index = PyLong_AsLong(state);
    if (index == -1 && PyErr_Occurred())
        return NULL;
    /* silently clip the index value */
    if (index < 0)
        index = 0;
    else if (index > r->len)
        index = r->len; /* exhausted iterator */
    r->start += index * r->step;
    r->len -= index;
    Py_RETURN_NONE;
}

static void
rangeiter_dealloc(PyObject *self)
{
    _Py_FREELIST_FREE(range_iters, (_PyRangeIterObject *)self, PyObject_Free);
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");
PyDoc_STRVAR(setstate_doc, "Set state information for unpickling.");

static PyMethodDef rangeiter_methods[] = {
    {"__length_hint__", rangeiter_len, METH_NOARGS, length_hint_doc},
    {"__reduce__", rangeiter_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", rangeiter_setstate, METH_O, setstate_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyRangeIter_Type = {
        PyVarObject_HEAD_INIT(&PyType_Type, 0)
        "range_iterator",                       /* tp_name */
        sizeof(_PyRangeIterObject),             /* tp_basicsize */
        0,                                      /* tp_itemsize */
        /* methods */
        rangeiter_dealloc,                      /* tp_dealloc */
        0,                                      /* tp_vectorcall_offset */
        0,                                      /* tp_getattr */
        0,                                      /* tp_setattr */
        0,                                      /* tp_as_async */
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
        rangeiter_next,                         /* tp_iternext */
        rangeiter_methods,                      /* tp_methods */
        0,                                      /* tp_members */
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
fast_range_iter(long start, long stop, long step, long len)
{
    _PyRangeIterObject *it = _Py_FREELIST_POP(_PyRangeIterObject, range_iters);
    if (it == NULL) {
        it = PyObject_New(_PyRangeIterObject, &PyRangeIter_Type);
        if (it == NULL) {
            return NULL;
        }
    }
    assert(Py_IS_TYPE(it, &PyRangeIter_Type));
    it->start = start;
    it->step = step;
    it->len = len;
    return (PyObject *)it;
}

typedef struct {
    PyObject_HEAD
    PyObject *start;
    PyObject *step;
    PyObject *len;
} longrangeiterobject;

static PyObject *
longrangeiter_len(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    longrangeiterobject *r = (longrangeiterobject*)op;
    Py_INCREF(r->len);
    return r->len;
}

static PyObject *
longrangeiter_reduce(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    longrangeiterobject *r = (longrangeiterobject*)op;
    PyObject *product, *stop=NULL;
    PyObject *range;

    /* create a range object for pickling.  Must calculate the "stop" value */
    product = PyNumber_Multiply(r->len, r->step);
    if (product == NULL)
        return NULL;
    stop = PyNumber_Add(r->start, product);
    Py_DECREF(product);
    if (stop ==  NULL)
        return NULL;
    range =  (PyObject*)make_range_object(&PyRange_Type,
                               Py_NewRef(r->start), stop, Py_NewRef(r->step));
    if (range == NULL) {
        Py_DECREF(r->start);
        Py_DECREF(stop);
        Py_DECREF(r->step);
        return NULL;
    }

    /* return the result */
    return Py_BuildValue("N(N)O", _PyEval_GetBuiltin(&_Py_ID(iter)),
                         range, Py_None);
}

static PyObject *
longrangeiter_setstate(PyObject *op, PyObject *state)
{
    if (!PyLong_CheckExact(state)) {
        PyErr_Format(PyExc_TypeError, "state must be an int, not %T", state);
        return NULL;
    }

    longrangeiterobject *r = (longrangeiterobject*)op;
    PyObject *zero = _PyLong_GetZero();  // borrowed reference
    int cmp;

    /* clip the value */
    cmp = PyObject_RichCompareBool(state, zero, Py_LT);
    if (cmp < 0)
        return NULL;
    if (cmp > 0) {
        state = zero;
    }
    else {
        cmp = PyObject_RichCompareBool(r->len, state, Py_LT);
        if (cmp < 0)
            return NULL;
        if (cmp > 0)
            state = r->len;
    }
    PyObject *product = PyNumber_Multiply(state, r->step);
    if (product == NULL)
        return NULL;
    PyObject *new_start = PyNumber_Add(r->start, product);
    Py_DECREF(product);
    if (new_start == NULL)
        return NULL;
    PyObject *new_len = PyNumber_Subtract(r->len, state);
    if (new_len == NULL) {
        Py_DECREF(new_start);
        return NULL;
    }
    PyObject *tmp = r->start;
    r->start = new_start;
    Py_SETREF(r->len, new_len);
    Py_DECREF(tmp);
    Py_RETURN_NONE;
}

static PyMethodDef longrangeiter_methods[] = {
    {"__length_hint__", longrangeiter_len, METH_NOARGS, length_hint_doc},
    {"__reduce__", longrangeiter_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", longrangeiter_setstate, METH_O, setstate_doc},
    {NULL,              NULL}           /* sentinel */
};

static void
longrangeiter_dealloc(PyObject *op)
{
    longrangeiterobject *r = (longrangeiterobject*)op;
    Py_XDECREF(r->start);
    Py_XDECREF(r->step);
    Py_XDECREF(r->len);
    PyObject_Free(r);
}

static PyObject *
longrangeiter_next(PyObject *op)
{
    longrangeiterobject *r = (longrangeiterobject*)op;
    if (PyObject_RichCompareBool(r->len, _PyLong_GetZero(), Py_GT) != 1)
        return NULL;

    PyObject *new_start = PyNumber_Add(r->start, r->step);
    if (new_start == NULL) {
        return NULL;
    }
    PyObject *new_len = PyNumber_Subtract(r->len, _PyLong_GetOne());
    if (new_len == NULL) {
        Py_DECREF(new_start);
        return NULL;
    }
    PyObject *result = r->start;
    r->start = new_start;
    Py_SETREF(r->len, new_len);
    return result;
}

PyTypeObject PyLongRangeIter_Type = {
        PyVarObject_HEAD_INIT(&PyType_Type, 0)
        "longrange_iterator",                   /* tp_name */
        sizeof(longrangeiterobject),            /* tp_basicsize */
        0,                                      /* tp_itemsize */
        /* methods */
        longrangeiter_dealloc,                  /* tp_dealloc */
        0,                                      /* tp_vectorcall_offset */
        0,                                      /* tp_getattr */
        0,                                      /* tp_setattr */
        0,                                      /* tp_as_async */
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
        longrangeiter_next,                     /* tp_iternext */
        longrangeiter_methods,                  /* tp_methods */
        0,
};

static PyObject *
range_iter(PyObject *seq)
{
    rangeobject *r = (rangeobject *)seq;
    longrangeiterobject *it;
    long lstart, lstop, lstep;
    unsigned long ulen;

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
    ulen = get_len_of_range(lstart, lstop, lstep);
    if (ulen > (unsigned long)LONG_MAX) {
        goto long_range;
    }
    /* check for potential overflow of lstart + ulen * lstep */
    if (ulen) {
        if (lstep > 0) {
            if (lstop > LONG_MAX - (lstep - 1))
                goto long_range;
        }
        else {
            if (lstop < LONG_MIN + (-1 - lstep))
                goto long_range;
        }
    }
    return fast_range_iter(lstart, lstop, lstep, (long)ulen);

  long_range:
    it = PyObject_New(longrangeiterobject, &PyLongRangeIter_Type);
    if (it == NULL)
        return NULL;

    it->start = Py_NewRef(r->start);
    it->step = Py_NewRef(r->step);
    it->len = Py_NewRef(r->length);
    return (PyObject *)it;
}

static PyObject *
range_reverse(PyObject *seq, PyObject *Py_UNUSED(ignored))
{
    rangeobject *range = (rangeobject*) seq;
    longrangeiterobject *it;
    PyObject *sum, *diff, *product;
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
    return fast_range_iter(new_start, new_stop, -lstep, (long)ulen);

long_range:
    it = PyObject_New(longrangeiterobject, &PyLongRangeIter_Type);
    if (it == NULL)
        return NULL;
    it->start = it->step = NULL;

    /* start + (len - 1) * step */
    it->len = Py_NewRef(range->length);

    diff = PyNumber_Subtract(it->len, _PyLong_GetOne());
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

    return (PyObject *)it;

create_failure:
    Py_DECREF(it);
    return NULL;
}
