/*
Written by Jim Hugunin and Chris Chase.

This includes both the singular ellipsis object and slice objects.

Guido, feel free to do whatever you want in the way of copyrights
for this file.
*/

/*
Py_Ellipsis encodes the '...' rubber index token. It is similar to
the Py_NoneStruct in that there is no way to create other objects of
this type and there is exactly one in existence.
*/

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_freelist.h"      // _Py_FREELIST_FREE(), _Py_FREELIST_POP()
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_modsupport.h"    // _PyArg_NoKeywords()
#include "pycore_object.h"        // _PyObject_GC_TRACK()


#define _PySlice_CAST(op) _Py_CAST(PySliceObject*, (op))


static PyObject *
ellipsis_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args) || (kwargs && PyDict_GET_SIZE(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "EllipsisType takes no arguments");
        return NULL;
    }
    return Py_Ellipsis;
}

static void
ellipsis_dealloc(PyObject *ellipsis)
{
    /* This should never get called, but we also don't want to SEGV if
     * we accidentally decref Ellipsis out of existence. Instead,
     * since Ellipsis is an immortal object, re-set the reference count.
     */
    _Py_SetImmortal(ellipsis);
}

static PyObject *
ellipsis_repr(PyObject *op)
{
    return PyUnicode_FromString("Ellipsis");
}

static PyObject *
ellipsis_reduce(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    return PyUnicode_FromString("Ellipsis");
}

static PyMethodDef ellipsis_methods[] = {
    {"__reduce__", ellipsis_reduce, METH_NOARGS, NULL},
    {NULL, NULL}
};

PyDoc_STRVAR(ellipsis_doc,
"ellipsis()\n"
"--\n\n"
"The type of the Ellipsis singleton.");

PyTypeObject PyEllipsis_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "ellipsis",                         /* tp_name */
    0,                                  /* tp_basicsize */
    0,                                  /* tp_itemsize */
    ellipsis_dealloc,                   /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    ellipsis_repr,                      /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    ellipsis_doc,                       /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    ellipsis_methods,                   /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    ellipsis_new,                       /* tp_new */
};

PyObject _Py_EllipsisObject = _PyObject_HEAD_INIT(&PyEllipsis_Type);


/* Slice object implementation */

/* start, stop, and step are python objects with None indicating no
   index is present.
*/

static PySliceObject *
_PyBuildSlice_Consume2(PyObject *start, PyObject *stop, PyObject *step)
{
    assert(start != NULL && stop != NULL && step != NULL);
    PySliceObject *obj = _Py_FREELIST_POP(PySliceObject, slices);
    if (obj == NULL) {
        obj = PyObject_GC_New(PySliceObject, &PySlice_Type);
        if (obj == NULL) {
            goto error;
        }
    }

    obj->start = start;
    obj->stop = stop;
    obj->step = Py_NewRef(step);

    _PyObject_GC_TRACK(obj);
    return obj;
error:
    Py_DECREF(start);
    Py_DECREF(stop);
    return NULL;
}

PyObject *
PySlice_New(PyObject *start, PyObject *stop, PyObject *step)
{
    if (step == NULL) {
        step = Py_None;
    }
    if (start == NULL) {
        start = Py_None;
    }
    if (stop == NULL) {
        stop = Py_None;
    }
    return (PyObject *)_PyBuildSlice_Consume2(Py_NewRef(start),
                                              Py_NewRef(stop), step);
}

PyObject *
_PyBuildSlice_ConsumeRefs(PyObject *start, PyObject *stop)
{
    assert(start != NULL && stop != NULL);
    return (PyObject *)_PyBuildSlice_Consume2(start, stop, Py_None);
}

PyObject *
_PySlice_FromIndices(Py_ssize_t istart, Py_ssize_t istop)
{
    PyObject *start, *end, *slice;
    start = PyLong_FromSsize_t(istart);
    if (!start)
        return NULL;
    end = PyLong_FromSsize_t(istop);
    if (!end) {
        Py_DECREF(start);
        return NULL;
    }

    slice = PySlice_New(start, end, NULL);
    Py_DECREF(start);
    Py_DECREF(end);
    return slice;
}

int
PySlice_GetIndices(PyObject *_r, Py_ssize_t length,
                   Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t *step)
{
    PySliceObject *r = (PySliceObject*)_r;
    /* XXX support long ints */
    if (r->step == Py_None) {
        *step = 1;
    } else {
        if (!PyLong_Check(r->step)) return -1;
        *step = PyLong_AsSsize_t(r->step);
    }
    if (r->start == Py_None) {
        *start = *step < 0 ? length-1 : 0;
    } else {
        if (!PyLong_Check(r->start)) return -1;
        *start = PyLong_AsSsize_t(r->start);
        if (*start < 0) *start += length;
    }
    if (r->stop == Py_None) {
        *stop = *step < 0 ? -1 : length;
    } else {
        if (!PyLong_Check(r->stop)) return -1;
        *stop = PyLong_AsSsize_t(r->stop);
        if (*stop < 0) *stop += length;
    }
    if (*stop > length) return -1;
    if (*start >= length) return -1;
    if (*step == 0) return -1;
    return 0;
}

int
PySlice_Unpack(PyObject *_r,
               Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t *step)
{
    PySliceObject *r = (PySliceObject*)_r;
    /* this is harder to get right than you might think */

    static_assert(PY_SSIZE_T_MIN + 1 <= -PY_SSIZE_T_MAX,
                  "-PY_SSIZE_T_MAX < PY_SSIZE_T_MIN + 1");

    if (r->step == Py_None) {
        *step = 1;
    }
    else {
        if (!_PyEval_SliceIndex(r->step, step)) return -1;
        if (*step == 0) {
            PyErr_SetString(PyExc_ValueError,
                            "slice step cannot be zero");
            return -1;
        }
        /* Here *step might be -PY_SSIZE_T_MAX-1; in this case we replace it
         * with -PY_SSIZE_T_MAX.  This doesn't affect the semantics, and it
         * guards against later undefined behaviour resulting from code that
         * does "step = -step" as part of a slice reversal.
         */
        if (*step < -PY_SSIZE_T_MAX)
            *step = -PY_SSIZE_T_MAX;
    }

    if (r->start == Py_None) {
        *start = *step < 0 ? PY_SSIZE_T_MAX : 0;
    }
    else {
        if (!_PyEval_SliceIndex(r->start, start)) return -1;
    }

    if (r->stop == Py_None) {
        *stop = *step < 0 ? PY_SSIZE_T_MIN : PY_SSIZE_T_MAX;
    }
    else {
        if (!_PyEval_SliceIndex(r->stop, stop)) return -1;
    }

    return 0;
}

Py_ssize_t
PySlice_AdjustIndices(Py_ssize_t length,
                      Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t step)
{
    /* this is harder to get right than you might think */

    assert(step != 0);
    assert(step >= -PY_SSIZE_T_MAX);

    if (*start < 0) {
        *start += length;
        if (*start < 0) {
            *start = (step < 0) ? -1 : 0;
        }
    }
    else if (*start >= length) {
        *start = (step < 0) ? length - 1 : length;
    }

    if (*stop < 0) {
        *stop += length;
        if (*stop < 0) {
            *stop = (step < 0) ? -1 : 0;
        }
    }
    else if (*stop >= length) {
        *stop = (step < 0) ? length - 1 : length;
    }

    if (step < 0) {
        if (*stop < *start) {
            return (*start - *stop - 1) / (-step) + 1;
        }
    }
    else {
        if (*start < *stop) {
            return (*stop - *start - 1) / step + 1;
        }
    }
    return 0;
}

#undef PySlice_GetIndicesEx

int
PySlice_GetIndicesEx(PyObject *_r, Py_ssize_t length,
                     Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t *step,
                     Py_ssize_t *slicelength)
{
    if (PySlice_Unpack(_r, start, stop, step) < 0)
        return -1;
    *slicelength = PySlice_AdjustIndices(length, start, stop, *step);
    return 0;
}

static PyObject *
slice_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PyObject *start, *stop, *step;

    start = stop = step = NULL;

    if (!_PyArg_NoKeywords("slice", kw))
        return NULL;

    if (!PyArg_UnpackTuple(args, "slice", 1, 3, &start, &stop, &step))
        return NULL;

    /* This swapping of stop and start is to maintain similarity with
       range(). */
    if (stop == NULL) {
        stop = start;
        start = NULL;
    }
    return PySlice_New(start, stop, step);
}

PyDoc_STRVAR(slice_doc,
"slice(stop)\n\
slice(start, stop[, step])\n\
\n\
Create a slice object.  This is used for extended slicing (e.g. a[0:10:2]).");

static void
slice_dealloc(PyObject *op)
{
    PySliceObject *r = _PySlice_CAST(op);
    PyObject_GC_UnTrack(r);
    Py_DECREF(r->step);
    Py_DECREF(r->start);
    Py_DECREF(r->stop);
    _Py_FREELIST_FREE(slices, r, PyObject_GC_Del);
}

static PyObject *
slice_repr(PyObject *op)
{
    PySliceObject *r = _PySlice_CAST(op);
    return PyUnicode_FromFormat("slice(%R, %R, %R)",
                                r->start, r->stop, r->step);
}

static PyMemberDef slice_members[] = {
    {"start", _Py_T_OBJECT, offsetof(PySliceObject, start), Py_READONLY},
    {"stop", _Py_T_OBJECT, offsetof(PySliceObject, stop), Py_READONLY},
    {"step", _Py_T_OBJECT, offsetof(PySliceObject, step), Py_READONLY},
    {0}
};

/* Helper function to convert a slice argument to a PyLong, and raise TypeError
   with a suitable message on failure. */

static PyObject*
evaluate_slice_index(PyObject *v)
{
    if (_PyIndex_Check(v)) {
        return PyNumber_Index(v);
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "slice indices must be integers or "
                        "None or have an __index__ method");
        return NULL;
    }
}

/* Compute slice indices given a slice and length.  Return -1 on failure.  Used
   by slice.indices and rangeobject slicing.  Assumes that `len` is a
   nonnegative instance of PyLong. */

int
_PySlice_GetLongIndices(PySliceObject *self, PyObject *length,
                        PyObject **start_ptr, PyObject **stop_ptr,
                        PyObject **step_ptr)
{
    PyObject *start=NULL, *stop=NULL, *step=NULL;
    PyObject *upper=NULL, *lower=NULL;
    int step_is_negative, cmp_result;

    /* Convert step to an integer; raise for zero step. */
    if (self->step == Py_None) {
        step = _PyLong_GetOne();
        step_is_negative = 0;
    }
    else {
        step = evaluate_slice_index(self->step);
        if (step == NULL) {
            goto error;
        }
        assert(PyLong_Check(step));

        int step_sign;
        (void)PyLong_GetSign(step, &step_sign);
        if (step_sign == 0) {
            PyErr_SetString(PyExc_ValueError,
                            "slice step cannot be zero");
            goto error;
        }
        step_is_negative = step_sign < 0;
    }

    /* Find lower and upper bounds for start and stop. */
    if (step_is_negative) {
        lower = PyLong_FromLong(-1L);
        if (lower == NULL)
            goto error;

        upper = PyNumber_Add(length, lower);
        if (upper == NULL)
            goto error;
    }
    else {
        lower = _PyLong_GetZero();
        upper = Py_NewRef(length);
    }

    /* Compute start. */
    if (self->start == Py_None) {
        start = Py_NewRef(step_is_negative ? upper : lower);
    }
    else {
        start = evaluate_slice_index(self->start);
        if (start == NULL)
            goto error;

        if (_PyLong_IsNegative((PyLongObject *)start)) {
            /* start += length */
            PyObject *tmp = PyNumber_Add(start, length);
            Py_SETREF(start, tmp);
            if (start == NULL)
                goto error;

            cmp_result = PyObject_RichCompareBool(start, lower, Py_LT);
            if (cmp_result < 0)
                goto error;
            if (cmp_result) {
                Py_SETREF(start, Py_NewRef(lower));
            }
        }
        else {
            cmp_result = PyObject_RichCompareBool(start, upper, Py_GT);
            if (cmp_result < 0)
                goto error;
            if (cmp_result) {
                Py_SETREF(start, Py_NewRef(upper));
            }
        }
    }

    /* Compute stop. */
    if (self->stop == Py_None) {
        stop = Py_NewRef(step_is_negative ? lower : upper);
    }
    else {
        stop = evaluate_slice_index(self->stop);
        if (stop == NULL)
            goto error;

        if (_PyLong_IsNegative((PyLongObject *)stop)) {
            /* stop += length */
            PyObject *tmp = PyNumber_Add(stop, length);
            Py_SETREF(stop, tmp);
            if (stop == NULL)
                goto error;

            cmp_result = PyObject_RichCompareBool(stop, lower, Py_LT);
            if (cmp_result < 0)
                goto error;
            if (cmp_result) {
                Py_SETREF(stop, Py_NewRef(lower));
            }
        }
        else {
            cmp_result = PyObject_RichCompareBool(stop, upper, Py_GT);
            if (cmp_result < 0)
                goto error;
            if (cmp_result) {
                Py_SETREF(stop, Py_NewRef(upper));
            }
        }
    }

    *start_ptr = start;
    *stop_ptr = stop;
    *step_ptr = step;
    Py_DECREF(upper);
    Py_DECREF(lower);
    return 0;

  error:
    *start_ptr = *stop_ptr = *step_ptr = NULL;
    Py_XDECREF(start);
    Py_XDECREF(stop);
    Py_XDECREF(step);
    Py_XDECREF(upper);
    Py_XDECREF(lower);
    return -1;
}

/* Implementation of slice.indices. */

static PyObject*
slice_indices(PySliceObject* self, PyObject* len)
{
    PyObject *start, *stop, *step;
    PyObject *length;
    int error;

    /* Convert length to an integer if necessary; raise for negative length. */
    length = PyNumber_Index(len);
    if (length == NULL)
        return NULL;

    if (_PyLong_IsNegative((PyLongObject *)length)) {
        PyErr_SetString(PyExc_ValueError,
                        "length should not be negative");
        Py_DECREF(length);
        return NULL;
    }

    error = _PySlice_GetLongIndices(self, length, &start, &stop, &step);
    Py_DECREF(length);
    if (error == -1)
        return NULL;
    else
        return Py_BuildValue("(NNN)", start, stop, step);
}

PyDoc_STRVAR(slice_indices_doc,
"S.indices(len) -> (start, stop, stride)\n\
\n\
Assuming a sequence of length len, calculate the start and stop\n\
indices, and the stride length of the extended slice described by\n\
S. Out of bounds indices are clipped in a manner consistent with the\n\
handling of normal slices.");

static PyObject *
slice_reduce(PySliceObject* self, PyObject *Py_UNUSED(ignored))
{
    return Py_BuildValue("O(OOO)", Py_TYPE(self), self->start, self->stop, self->step);
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyMethodDef slice_methods[] = {
    {"indices",         (PyCFunction)slice_indices,
     METH_O,            slice_indices_doc},
    {"__reduce__",      (PyCFunction)slice_reduce,
     METH_NOARGS,       reduce_doc},
    {NULL, NULL}
};

static PyObject *
slice_richcompare(PyObject *v, PyObject *w, int op)
{
    if (!PySlice_Check(v) || !PySlice_Check(w))
        Py_RETURN_NOTIMPLEMENTED;

    if (v == w) {
        PyObject *res;
        /* XXX Do we really need this shortcut?
           There's a unit test for it, but is that fair? */
        switch (op) {
        case Py_EQ:
        case Py_LE:
        case Py_GE:
            res = Py_True;
            break;
        default:
            res = Py_False;
            break;
        }
        return Py_NewRef(res);
    }


    PyObject *t1 = PyTuple_Pack(3,
                                ((PySliceObject *)v)->start,
                                ((PySliceObject *)v)->stop,
                                ((PySliceObject *)v)->step);
    if (t1 == NULL) {
        return NULL;
    }

    PyObject *t2 = PyTuple_Pack(3,
                                ((PySliceObject *)w)->start,
                                ((PySliceObject *)w)->stop,
                                ((PySliceObject *)w)->step);
    if (t2 == NULL) {
        Py_DECREF(t1);
        return NULL;
    }

    PyObject *res = PyObject_RichCompare(t1, t2, op);
    Py_DECREF(t1);
    Py_DECREF(t2);
    return res;
}

static int
slice_traverse(PyObject *op, visitproc visit, void *arg)
{
    PySliceObject *v = _PySlice_CAST(op);
    Py_VISIT(v->start);
    Py_VISIT(v->stop);
    Py_VISIT(v->step);
    return 0;
}

/* code based on tuplehash() of Objects/tupleobject.c */
#if SIZEOF_PY_UHASH_T > 4
#define _PyHASH_XXPRIME_1 ((Py_uhash_t)11400714785074694791ULL)
#define _PyHASH_XXPRIME_2 ((Py_uhash_t)14029467366897019727ULL)
#define _PyHASH_XXPRIME_5 ((Py_uhash_t)2870177450012600261ULL)
#define _PyHASH_XXROTATE(x) ((x << 31) | (x >> 33))  /* Rotate left 31 bits */
#else
#define _PyHASH_XXPRIME_1 ((Py_uhash_t)2654435761UL)
#define _PyHASH_XXPRIME_2 ((Py_uhash_t)2246822519UL)
#define _PyHASH_XXPRIME_5 ((Py_uhash_t)374761393UL)
#define _PyHASH_XXROTATE(x) ((x << 13) | (x >> 19))  /* Rotate left 13 bits */
#endif

static Py_hash_t
slice_hash(PyObject *op)
{
    PySliceObject *v = _PySlice_CAST(op);
    Py_uhash_t acc = _PyHASH_XXPRIME_5;
#define _PyHASH_SLICE_PART(com) { \
    Py_uhash_t lane = PyObject_Hash(v->com); \
    if(lane == (Py_uhash_t)-1) { \
        return -1; \
    } \
    acc += lane * _PyHASH_XXPRIME_2; \
    acc = _PyHASH_XXROTATE(acc); \
    acc *= _PyHASH_XXPRIME_1; \
}
    _PyHASH_SLICE_PART(start);
    _PyHASH_SLICE_PART(stop);
    _PyHASH_SLICE_PART(step);
#undef _PyHASH_SLICE_PART
    if(acc == (Py_uhash_t)-1) {
        return 1546275796;
    }
    return acc;
}

PyTypeObject PySlice_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "slice",                    /* Name of this type */
    sizeof(PySliceObject),      /* Basic object size */
    0,                          /* Item size for varobject */
    slice_dealloc,                              /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    slice_repr,                                 /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    slice_hash,                                 /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
    slice_doc,                                  /* tp_doc */
    slice_traverse,                             /* tp_traverse */
    0,                                          /* tp_clear */
    slice_richcompare,                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    slice_methods,                              /* tp_methods */
    slice_members,                              /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    slice_new,                                  /* tp_new */
};
