/* Iterator objects */

#include "Python.h"
#include "pycore_abstract.h"      // _PyObject_HasLen()
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_ceval.h"         // _PyEval_GetBuiltin()
#include "pycore_genobject.h"     // _PyCoro_GetAwaitableIter()
#include "pycore_iterobject.h"    // _PyCallIter_NewEx()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_pyatomic_ft_wrappers.h"  // FT_ATOMIC_LOAD_SSIZE_RELAXED()
#include "pycore_pyerrors.h"      // _PyErr_FormatFromCause()
#include "pycore_pystate.h"       // _PyThreadState_GET()


typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;  /* -1 when iterator is exhausted */
    PyObject *it_seq; /* Set to NULL when iterator is exhausted
                         (in the default build) */
} seqiterobject;

PyObject *
PySeqIter_New(PyObject *seq)
{
    seqiterobject *it;

    if (!PySequence_Check(seq)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    it = PyObject_GC_New(seqiterobject, &PySeqIter_Type);
    if (it == NULL)
        return NULL;
    it->it_index = 0;
    it->it_seq = Py_NewRef(seq);
    _PyObject_GC_TRACK(it);
    return (PyObject *)it;
}

static void
iter_dealloc(PyObject *op)
{
    seqiterobject *it = (seqiterobject*)op;
    _PyObject_GC_UNTRACK(it);
    Py_XDECREF(it->it_seq);
    PyObject_GC_Del(it);
}

static int
iter_traverse(PyObject *op, visitproc visit, void *arg)
{
    seqiterobject *it = (seqiterobject*)op;
    Py_VISIT(it->it_seq);
    return 0;
}

static PyObject *
iter_iternext(PyObject *iterator)
{
    seqiterobject *it;
    PyObject *seq;
    PyObject *result;

    assert(PySeqIter_Check(iterator));
    it = (seqiterobject *)iterator;
    Py_ssize_t index = FT_ATOMIC_LOAD_SSIZE_RELAXED(it->it_index);
    if (index < 0)
        return NULL;
    seq = it->it_seq;
#ifndef Py_GIL_DISABLED
    if (seq == NULL)
        return NULL;
#endif
    if (index == PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "iter index too large");
        return NULL;
    }

    result = PySequence_GetItem(seq, index);
    if (result != NULL) {
        /* PySequence_GetItem() can exhaust the iterator re-entrantly.
         * Preserve the exhaustion sentinel if it is observed.  Concurrent
         * exhaustion can still race with the store, but remains memory-safe
         * because the sequence stays alive. */
        if (FT_ATOMIC_LOAD_SSIZE_RELAXED(it->it_index) >= 0) {
            FT_ATOMIC_STORE_SSIZE_RELAXED(it->it_index, index + 1);
        }
        return result;
    }
    if (PyErr_ExceptionMatches(PyExc_IndexError) ||
        PyErr_ExceptionMatches(PyExc_StopIteration))
    {
        /* Mark the iterator exhausted before anything that can run
         * arbitrary code. */
        FT_ATOMIC_STORE_SSIZE_RELAXED(it->it_index, -1);
#ifndef Py_GIL_DISABLED
        Py_CLEAR(it->it_seq);
#endif
        PyErr_Clear();
    }
    return NULL;
}

static PyObject *
iter_len(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    seqiterobject *it = (seqiterobject*)op;
    Py_ssize_t seqsize, len;

    Py_ssize_t index = FT_ATOMIC_LOAD_SSIZE_RELAXED(it->it_index);
    if (index >= 0 && it->it_seq != NULL) {
        if (_PyObject_HasLen(it->it_seq)) {
            seqsize = PySequence_Size(it->it_seq);
            if (seqsize == -1)
                return NULL;
        }
        else {
            Py_RETURN_NOTIMPLEMENTED;
        }
        len = seqsize - index;
        if (len >= 0)
            return PyLong_FromSsize_t(len);
    }
    return PyLong_FromLong(0);
}

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

static PyObject *
iter_reduce(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    seqiterobject *it = (seqiterobject*)op;
    PyObject *iter = _PyEval_GetBuiltin(&_Py_ID(iter));

    /* _PyEval_GetBuiltin can invoke arbitrary code,
     * call must be before access of iterator pointers.
     * see issue #101765 */

    Py_ssize_t index = FT_ATOMIC_LOAD_SSIZE_RELAXED(it->it_index);
    if (index >= 0 && it->it_seq != NULL)
        return Py_BuildValue("N(O)n", iter, it->it_seq, index);
    else
        return Py_BuildValue("N(())", iter);
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyObject *
iter_setstate(PyObject *op, PyObject *state)
{
    seqiterobject *it = (seqiterobject*)op;
    Py_ssize_t index = PyLong_AsSsize_t(state);
    if (index == -1 && PyErr_Occurred())
        return NULL;
    if (index < 0)
        index = 0;
    if (it->it_seq && FT_ATOMIC_LOAD_SSIZE_RELAXED(it->it_index) >= 0) {
        FT_ATOMIC_STORE_SSIZE_RELAXED(it->it_index, index);
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(setstate_doc, "Set state information for unpickling.");

static PyMethodDef seqiter_methods[] = {
    {"__length_hint__", iter_len, METH_NOARGS, length_hint_doc},
    {"__reduce__", iter_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", iter_setstate, METH_O, setstate_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PySeqIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "iterator",                                 /* tp_name */
    sizeof(seqiterobject),                      /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    iter_dealloc,                               /* tp_dealloc */
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
    0,                                          /* tp_doc */
    iter_traverse,                              /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    iter_iternext,                              /* tp_iternext */
    seqiter_methods,                            /* tp_methods */
    0,                                          /* tp_members */
};

/* -------------------------------------- */

typedef struct {
    PyObject_HEAD
    PyObject *it_callable;  /* set to NULL when the iterator is exhausted */
    PyObject *it_sentinel;  /* can be NULL, and is when exhausted */
    PyObject *it_stop_exc;  /* never NULL */
} calliterobject;

PyObject *
_PyCallIter_NewEx(PyObject *callable, PyObject *sentinel, PyObject *stop_exc)
{
    calliterobject *it;
    if (stop_exc == NULL) {
        stop_exc = PyExc_StopIteration;
    }
    else if (_PyEval_CheckExceptTypeValid(_PyThreadState_GET(), stop_exc) < 0) {
        return NULL;
    }
    it = PyObject_GC_New(calliterobject, &PyCallIter_Type);
    if (it == NULL)
        return NULL;
    it->it_callable = Py_NewRef(callable);
    it->it_sentinel = Py_XNewRef(sentinel);
    it->it_stop_exc = Py_NewRef(stop_exc);
    _PyObject_GC_TRACK(it);
    return (PyObject *)it;
}

PyObject *
PyCallIter_New(PyObject *callable, PyObject *sentinel)
{
    return _PyCallIter_NewEx(callable, sentinel, NULL);
}

static void
calliter_exhaust(calliterobject *it)
{
    Py_CLEAR(it->it_callable);
    Py_CLEAR(it->it_sentinel);
}

static void
calliter_dealloc(PyObject *op)
{
    calliterobject *it = (calliterobject*)op;
    _PyObject_GC_UNTRACK(it);
    Py_XDECREF(it->it_callable);
    Py_XDECREF(it->it_sentinel);
    Py_XDECREF(it->it_stop_exc);
    PyObject_GC_Del(it);
}

static int
calliter_traverse(PyObject *op, visitproc visit, void *arg)
{
    calliterobject *it = (calliterobject*)op;
    Py_VISIT(it->it_callable);
    Py_VISIT(it->it_sentinel);
    Py_VISIT(it->it_stop_exc);
    return 0;
}

static PyObject *
calliter_iternext(PyObject *op)
{
    calliterobject *it = (calliterobject*)op;
    PyObject *result;

    if (it->it_callable == NULL) {
        return NULL;
    }

    result = _PyObject_CallNoArgs(it->it_callable);
    /* The call can exhaust the iterator re-entrantly. */
    if (result != NULL && it->it_callable != NULL) {
        if (it->it_sentinel == NULL) {
            return result; /* Common case, fast path */
        }
        int ok = PyObject_RichCompareBool(it->it_sentinel, result, Py_EQ);
        if (ok == 0) {
            return result; /* Common case, fast path */
        }

        if (ok > 0) {
            calliter_exhaust(it);
        }
    }
    else if (PyErr_ExceptionMatches(it->it_stop_exc)) {
        PyErr_Clear();
        calliter_exhaust(it);
    }
    else if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
        /* It would be mistaken for the end of the iteration (see PEP 479). */
        _PyErr_FormatFromCause(PyExc_RuntimeError,
                               "callable raised StopIteration");
    }
    Py_XDECREF(result);
    return NULL;
}

static PyObject *
calliter_reduce(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    calliterobject *it = (calliterobject*)op;
    PyObject *iter = _PyEval_GetBuiltin(&_Py_ID(iter));

    /* _PyEval_GetBuiltin can invoke arbitrary code,
     * call must be before access of iterator pointers.
     * see issue #101765 */

    if (it->it_callable == NULL) {
        return Py_BuildValue("N(())", iter);
    }
    /* Only the sentinel can be passed as an argument of iter(), so other
       attributes are restored from the state (see calliter_setstate()). */
    if (it->it_sentinel == NULL) {
        return Py_BuildValue("N(OO)(()O)", iter, it->it_callable, Py_None,
                             it->it_stop_exc);
    }
    else if (it->it_stop_exc == PyExc_StopIteration) {
        return Py_BuildValue("N(OO)", iter, it->it_callable, it->it_sentinel);
    }
    else {
        return Py_BuildValue("N(OO)((O)O)", iter, it->it_callable, Py_None,
                             it->it_sentinel, it->it_stop_exc);
    }
}

static PyObject *
calliter_setstate(PyObject *op, PyObject *state)
{
    calliterobject *it = (calliterobject*)op;
    PyObject *sentinel, *stop_exc;

    if (!PyTuple_Check(state) || PyTuple_GET_SIZE(state) != 2) {
        goto error;
    }
    sentinel = PyTuple_GET_ITEM(state, 0);
    stop_exc = PyTuple_GET_ITEM(state, 1);
    if (!PyTuple_Check(sentinel) || PyTuple_GET_SIZE(sentinel) > 1) {
        goto error;
    }
    if (_PyEval_CheckExceptTypeValid(_PyThreadState_GET(), stop_exc) < 0) {
        return NULL;
    }
    if (it->it_callable != NULL) {
        Py_XSETREF(it->it_sentinel,
                   PyTuple_GET_SIZE(sentinel) ?
                   Py_NewRef(PyTuple_GET_ITEM(sentinel, 0)) : NULL);
        Py_SETREF(it->it_stop_exc, Py_NewRef(stop_exc));
    }
    Py_RETURN_NONE;

error:
    PyErr_SetString(PyExc_TypeError, "invalid state for callable_iterator");
    return NULL;
}

static PyMethodDef calliter_methods[] = {
    {"__reduce__", calliter_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", calliter_setstate, METH_O, setstate_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyCallIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "callable_iterator",                        /* tp_name */
    sizeof(calliterobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    calliter_dealloc,                           /* tp_dealloc */
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
    0,                                          /* tp_doc */
    calliter_traverse,                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    calliter_iternext,                          /* tp_iternext */
    calliter_methods,                           /* tp_methods */
};
