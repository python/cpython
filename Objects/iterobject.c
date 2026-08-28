/* Iterator objects */

#include "Python.h"
#include "pycore_abstract.h"      // _PyObject_HasLen()
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_ceval.h"         // _PyEval_GetBuiltin()
#include "pycore_genobject.h"     // _PyCoro_GetAwaitableIter()
#include "pycore_iterobject.h"    // _PyCallIter_NewEx()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_pyerrors.h"      // _PyErr_FormatFromCause()
#include "pycore_pystate.h"       // _PyThreadState_GET()


typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;
    PyObject *it_seq; /* Set to NULL when iterator is exhausted */
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
    seq = it->it_seq;
    if (seq == NULL)
        return NULL;
    if (it->it_index == PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "iter index too large");
        return NULL;
    }

    result = PySequence_GetItem(seq, it->it_index);
    if (result != NULL) {
        it->it_index++;
        return result;
    }
    if (PyErr_ExceptionMatches(PyExc_IndexError) ||
        PyErr_ExceptionMatches(PyExc_StopIteration))
    {
        PyErr_Clear();
        it->it_seq = NULL;
        Py_DECREF(seq);
    }
    return NULL;
}

static PyObject *
iter_len(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    seqiterobject *it = (seqiterobject*)op;
    Py_ssize_t seqsize, len;

    if (it->it_seq) {
        if (_PyObject_HasLen(it->it_seq)) {
            seqsize = PySequence_Size(it->it_seq);
            if (seqsize == -1)
                return NULL;
        }
        else {
            Py_RETURN_NOTIMPLEMENTED;
        }
        len = seqsize - it->it_index;
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

    if (it->it_seq != NULL)
        return Py_BuildValue("N(O)n", iter, it->it_seq, it->it_index);
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
    if (it->it_seq != NULL) {
        if (index < 0)
            index = 0;
        it->it_index = index;
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


/* -------------------------------------- */

typedef struct {
    PyObject_HEAD
    PyObject *wrapped;
    PyObject *default_value;
} anextawaitableobject;

#define anextawaitableobject_CAST(op)   ((anextawaitableobject *)(op))

static void
anextawaitable_dealloc(PyObject *op)
{
    anextawaitableobject *obj = anextawaitableobject_CAST(op);
    _PyObject_GC_UNTRACK(obj);
    Py_XDECREF(obj->wrapped);
    Py_XDECREF(obj->default_value);
    PyObject_GC_Del(obj);
}

static int
anextawaitable_traverse(PyObject *op, visitproc visit, void *arg)
{
    anextawaitableobject *obj = anextawaitableobject_CAST(op);
    Py_VISIT(obj->wrapped);
    Py_VISIT(obj->default_value);
    return 0;
}

static PyObject *
awaitable_getiter(PyObject *owner, PyObject *wrapped)
{
    assert(wrapped != NULL);
    PyObject *awaitable = _PyCoro_GetAwaitableIter(wrapped);
    if (awaitable == NULL) {
        return NULL;
    }
    if (Py_TYPE(awaitable)->tp_iternext == NULL) {
        /* _PyCoro_GetAwaitableIter returns a Coroutine, a Generator,
         * or an iterator. Of these, only coroutines lack tp_iternext.
         */
        assert(PyCoro_CheckExact(awaitable));
        unaryfunc getter = Py_TYPE(awaitable)->tp_as_async->am_await;
        PyObject *new_awaitable = getter(awaitable);
        if (new_awaitable == NULL) {
            Py_DECREF(awaitable);
            return NULL;
        }
        Py_SETREF(awaitable, new_awaitable);
        if (!PyIter_Check(awaitable)) {
            PyErr_Format(PyExc_TypeError,
                         "%T.__await__() must return an iterable, not %T",
                         owner, awaitable);
            Py_DECREF(awaitable);
            return NULL;
        }
    }
    return awaitable;
}

static PyObject *
anextawaitable_iternext(PyObject *op)
{
    /* Consider the following class:
     *
     *     class A:
     *         async def __anext__(self):
     *             ...
     *     a = A()
     *
     * Then `await anext(a)` should call
     * a.__anext__().__await__().__next__()
     *
     * On the other hand, given
     *
     *     async def agen():
     *         yield 1
     *         yield 2
     *     gen = agen()
     *
     * Then `await anext(gen)` can just call
     * gen.__anext__().__next__()
     */
    anextawaitableobject *obj = anextawaitableobject_CAST(op);
    PyObject *awaitable = awaitable_getiter(op, obj->wrapped);
    if (awaitable == NULL) {
        return NULL;
    }
    PyObject *result = (*Py_TYPE(awaitable)->tp_iternext)(awaitable);
    Py_DECREF(awaitable);
    if (result != NULL) {
        return result;
    }
    if (PyErr_ExceptionMatches(PyExc_StopAsyncIteration)) {
        PyErr_Clear();
        _PyGen_SetStopIterationValue(obj->default_value);
    }
    return NULL;
}


static PyObject *
anextawaitable_proxy(anextawaitableobject *obj, char *meth, PyObject *arg)
{
    PyObject *awaitable = awaitable_getiter((PyObject *)obj, obj->wrapped);
    if (awaitable == NULL) {
        return NULL;
    }
    // When specified, 'arg' may be a tuple (if coming from a METH_VARARGS
    // method) or a single object (if coming from a METH_O method).
    PyObject *ret = arg == NULL
        ? PyObject_CallMethod(awaitable, meth, NULL)
        : PyObject_CallMethod(awaitable, meth, "O", arg);
    Py_DECREF(awaitable);
    if (ret != NULL) {
        return ret;
    }
    if (PyErr_ExceptionMatches(PyExc_StopAsyncIteration)) {
        /* `anextawaitableobject` is only used by `anext()` when
         * a default value is provided. So when we have a StopAsyncIteration
         * exception we replace it with a `StopIteration(default)`, as if
         * it was the return value of `__anext__()` coroutine.
         */
        PyErr_Clear();
        _PyGen_SetStopIterationValue(obj->default_value);
    }
    return NULL;
}


static PyObject *
anextawaitable_send(PyObject *op, PyObject *arg)
{
    anextawaitableobject *obj = anextawaitableobject_CAST(op);
    return anextawaitable_proxy(obj, "send", arg);
}


static PyObject *
anextawaitable_throw(PyObject *op, PyObject *args)
{
    anextawaitableobject *obj = anextawaitableobject_CAST(op);
    return anextawaitable_proxy(obj, "throw", args);
}


static PyObject *
anextawaitable_close(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    anextawaitableobject *obj = anextawaitableobject_CAST(op);
    return anextawaitable_proxy(obj, "close", NULL);
}


PyDoc_STRVAR(send_doc,
"send(arg) -> send 'arg' into the wrapped iterator,\n\
return next yielded value or raise StopIteration.");


PyDoc_STRVAR(throw_doc,
"throw(value)\n\
throw(typ[,val[,tb]])\n\
\n\
raise exception in the wrapped iterator, return next yielded value\n\
or raise StopIteration.\n\
the (type, val, tb) signature is deprecated, \n\
and may be removed in a future version of Python.");


PyDoc_STRVAR(close_doc,
"close() -> raise GeneratorExit inside generator.");


static PyMethodDef anextawaitable_methods[] = {
    {"send", anextawaitable_send, METH_O, send_doc},
    {"throw", anextawaitable_throw, METH_VARARGS, throw_doc},
    {"close", anextawaitable_close, METH_NOARGS, close_doc},
    {NULL, NULL}        /* Sentinel */
};


static PyAsyncMethods anextawaitable_as_async = {
    PyObject_SelfIter,                          /* am_await */
    0,                                          /* am_aiter */
    0,                                          /* am_anext */
    0,                                          /* am_send  */
};

PyTypeObject _PyAnextAwaitable_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "anext_awaitable",                          /* tp_name */
    sizeof(anextawaitableobject),               /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    anextawaitable_dealloc,                     /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    &anextawaitable_as_async,                   /* tp_as_async */
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
    anextawaitable_traverse,                    /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    anextawaitable_iternext,                    /* tp_iternext */
    anextawaitable_methods,                     /* tp_methods */
};

PyObject *
PyAnextAwaitable_New(PyObject *awaitable, PyObject *default_value)
{
    anextawaitableobject *anext = PyObject_GC_New(
            anextawaitableobject, &_PyAnextAwaitable_Type);
    if (anext == NULL) {
        return NULL;
    }
    anext->wrapped = Py_NewRef(awaitable);
    anext->default_value = Py_NewRef(default_value);
    _PyObject_GC_TRACK(anext);
    return (PyObject *)anext;
}


/* -------------------------------------- */

/* The asynchronous counterpart of calliterobject: the callable is called
   and its result is awaited for every __anext__(). */

typedef struct {
    PyObject_HEAD
    PyObject *it_callable;  /* set to NULL when the iterator is exhausted */
    PyObject *it_sentinel;  /* can be NULL, and is when exhausted */
    PyObject *it_stop_exc;  /* never NULL */
} acalliterobject;

#define acalliterobject_CAST(op)        ((acalliterobject *)(op))

/* The awaitable returned by acalliter_anext().  The callable is only
   called when this object is awaited. */
typedef struct {
    PyObject_HEAD
    PyObject *aw_iterator; /* the iterator which created this object */
    PyObject *aw_wrapped;  /* the awaitable returned by the callable */
    bool aw_closed;
} acallawaitableobject;

#define acallawaitableobject_CAST(op)   ((acallawaitableobject *)(op))

PyObject *
_PyACallIter_New(PyObject *callable, PyObject *sentinel, PyObject *stop_exc)
{
    if (stop_exc == NULL) {
        stop_exc = PyExc_StopAsyncIteration;
    }
    else if (_PyEval_CheckExceptTypeValid(_PyThreadState_GET(), stop_exc) < 0) {
        return NULL;
    }
    acalliterobject *it = PyObject_GC_New(acalliterobject, &_PyACallIter_Type);
    if (it == NULL) {
        return NULL;
    }
    it->it_callable = Py_NewRef(callable);
    it->it_sentinel = Py_XNewRef(sentinel);
    it->it_stop_exc = Py_NewRef(stop_exc);
    _PyObject_GC_TRACK(it);
    return (PyObject *)it;
}

static void
acalliter_exhaust(acalliterobject *it)
{
    Py_CLEAR(it->it_callable);
    Py_CLEAR(it->it_sentinel);
}

static void
acalliter_dealloc(PyObject *op)
{
    acalliterobject *it = acalliterobject_CAST(op);
    _PyObject_GC_UNTRACK(it);
    Py_XDECREF(it->it_callable);
    Py_XDECREF(it->it_sentinel);
    Py_XDECREF(it->it_stop_exc);
    PyObject_GC_Del(it);
}

static int
acalliter_traverse(PyObject *op, visitproc visit, void *arg)
{
    acalliterobject *it = acalliterobject_CAST(op);
    Py_VISIT(it->it_callable);
    Py_VISIT(it->it_sentinel);
    Py_VISIT(it->it_stop_exc);
    return 0;
}

static PyObject *acallawaitable_new(PyObject *iterator);

static PyObject *
acalliter_anext(PyObject *op)
{
    return acallawaitable_new(op);
}

static PyAsyncMethods acalliter_as_async = {
    0,                                          /* am_await */
    PyObject_SelfIter,                          /* am_aiter */
    acalliter_anext,                            /* am_anext */
    0,                                          /* am_send  */
};

PyTypeObject _PyACallIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "async_callable_iterator",
    .tp_basicsize = sizeof(acalliterobject),
    .tp_dealloc = acalliter_dealloc,
    .tp_as_async = &acalliter_as_async,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = acalliter_traverse,
};

/* -------------------------------------- */

static PyObject *
acallawaitable_new(PyObject *iterator)
{
    acallawaitableobject *aw = PyObject_GC_New(
            acallawaitableobject, &_PyACallIterAwaitable_Type);
    if (aw == NULL) {
        return NULL;
    }
    aw->aw_iterator = Py_NewRef(iterator);
    aw->aw_wrapped = NULL;
    aw->aw_closed = false;
    _PyObject_GC_TRACK(aw);
    return (PyObject *)aw;
}

static void
acallawaitable_dealloc(PyObject *op)
{
    acallawaitableobject *aw = acallawaitableobject_CAST(op);
    _PyObject_GC_UNTRACK(aw);
    Py_XDECREF(aw->aw_iterator);
    Py_XDECREF(aw->aw_wrapped);
    PyObject_GC_Del(aw);
}

static int
acallawaitable_traverse(PyObject *op, visitproc visit, void *arg)
{
    acallawaitableobject *aw = acallawaitableobject_CAST(op);
    Py_VISIT(aw->aw_iterator);
    Py_VISIT(aw->aw_wrapped);
    return 0;
}

/* Call the callable.  Return 0 on success, -1 on failure. */
static int
acallawaitable_start(acallawaitableobject *aw)
{
    acalliterobject *it = acalliterobject_CAST(aw->aw_iterator);

    if (aw->aw_closed) {
        PyErr_SetString(PyExc_RuntimeError,
                        "cannot reuse already awaited __anext__()");
        return -1;
    }
    if (it->it_callable == NULL) {
        PyErr_SetNone(PyExc_StopAsyncIteration);
        return -1;
    }
    PyObject *awaitable = _PyObject_CallNoArgs(it->it_callable);
    if (awaitable == NULL) {
        if (PyErr_ExceptionMatches(it->it_stop_exc)) {
            PyErr_Clear();
            acalliter_exhaust(it);
            PyErr_SetNone(PyExc_StopAsyncIteration);
        }
        else if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
            /* It would be mistaken for the result of the await (PEP 525). */
            _PyErr_FormatFromCause(PyExc_RuntimeError,
                                   "callable raised StopIteration");
        }
        else if (PyErr_ExceptionMatches(PyExc_StopAsyncIteration)) {
            /* It would be mistaken for the end of the iteration (PEP 525). */
            _PyErr_FormatFromCause(PyExc_RuntimeError,
                                   "callable raised StopAsyncIteration");
        }
        return -1;
    }
    aw->aw_wrapped = awaitable;
    return 0;
}

/* Turn the exception raised by the wrapped awaitable into the result of
   the await.  Always returns NULL. */
static PyObject *
acallawaitable_handle_error(acallawaitableobject *aw)
{
    acalliterobject *it = acalliterobject_CAST(aw->aw_iterator);

    if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
        PyObject *value;
        if (_PyGen_FetchStopIterationValue(&value) < 0) {
            return NULL;
        }
        int ok = 0;
        if (it->it_sentinel != NULL) {
            ok = PyObject_RichCompareBool(it->it_sentinel, value, Py_EQ);
        }
        if (ok == 0) {
            (void)_PyGen_SetStopIterationValue(value);
        }
        else if (ok > 0) {
            acalliter_exhaust(it);
            PyErr_SetNone(PyExc_StopAsyncIteration);
        }
        Py_DECREF(value);
        return NULL;
    }
    if (PyErr_ExceptionMatches(it->it_stop_exc)) {
        PyErr_Clear();
        acalliter_exhaust(it);
        PyErr_SetNone(PyExc_StopAsyncIteration);
    }
    else if (PyErr_ExceptionMatches(PyExc_StopAsyncIteration)) {
        /* It would be mistaken for the end of the iteration (see PEP 525). */
        _PyErr_FormatFromCause(PyExc_RuntimeError,
                               "callable raised StopAsyncIteration");
    }
    return NULL;
}

static PyObject *
acallawaitable_iternext(PyObject *op)
{
    acallawaitableobject *aw = acallawaitableobject_CAST(op);

    if (aw->aw_wrapped == NULL && acallawaitable_start(aw) < 0) {
        return NULL;
    }
    PyObject *awaitable = awaitable_getiter(op, aw->aw_wrapped);
    if (awaitable == NULL) {
        return NULL;
    }
    PyObject *result = (*Py_TYPE(awaitable)->tp_iternext)(awaitable);
    Py_DECREF(awaitable);
    if (result != NULL) {
        return result;
    }
    return acallawaitable_handle_error(aw);
}

static PyObject *
acallawaitable_proxy(acallawaitableobject *aw, char *meth, PyObject *arg)
{
    PyObject *awaitable = awaitable_getiter((PyObject *)aw, aw->aw_wrapped);
    if (awaitable == NULL) {
        return NULL;
    }
    // When specified, 'arg' may be a tuple (if coming from a METH_VARARGS
    // method) or a single object (if coming from a METH_O method).
    PyObject *ret = arg == NULL
        ? PyObject_CallMethod(awaitable, meth, NULL)
        : PyObject_CallMethod(awaitable, meth, "O", arg);
    Py_DECREF(awaitable);
    if (ret != NULL) {
        return ret;
    }
    return acallawaitable_handle_error(aw);
}

static PyObject *
acallawaitable_send(PyObject *op, PyObject *arg)
{
    acallawaitableobject *aw = acallawaitableobject_CAST(op);

    if (aw->aw_wrapped == NULL && acallawaitable_start(aw) < 0) {
        return NULL;
    }
    return acallawaitable_proxy(aw, "send", arg);
}

static PyObject *
acallawaitable_throw(PyObject *op, PyObject *args)
{
    acallawaitableobject *aw = acallawaitableobject_CAST(op);

    if (aw->aw_wrapped == NULL) {
        /* Not started, so the exception is raised at the point of the
           await, as for a not started coroutine. */
        PyObject *typ, *val = NULL, *tb = NULL;
        if (!PyArg_UnpackTuple(args, "throw", 1, 3, &typ, &val, &tb)) {
            return NULL;
        }
        aw->aw_closed = true;
        (void)_PyGen_SetException(typ, val, tb);
        return NULL;
    }
    return acallawaitable_proxy(aw, "throw", args);
}

static PyObject *
acallawaitable_close(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    acallawaitableobject *aw = acallawaitableobject_CAST(op);

    if (aw->aw_wrapped == NULL) {
        /* Not started, so there is nothing to close. */
        aw->aw_closed = true;
        Py_RETURN_NONE;
    }
    PyObject *result = acallawaitable_proxy(aw, "close", NULL);
    aw->aw_closed = true;
    return result;
}

static PyMethodDef acallawaitable_methods[] = {
    {"send", acallawaitable_send, METH_O, send_doc},
    {"throw", acallawaitable_throw, METH_VARARGS, throw_doc},
    {"close", acallawaitable_close, METH_NOARGS, close_doc},
    {NULL, NULL}        /* Sentinel */
};

static PyAsyncMethods acallawaitable_as_async = {
    PyObject_SelfIter,                          /* am_await */
    0,                                          /* am_aiter */
    0,                                          /* am_anext */
    0,                                          /* am_send  */
};

PyTypeObject _PyACallIterAwaitable_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "async_callable_iterator_awaitable",
    .tp_basicsize = sizeof(acallawaitableobject),
    .tp_dealloc = acallawaitable_dealloc,
    .tp_as_async = &acallawaitable_as_async,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = acallawaitable_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = acallawaitable_iternext,
    .tp_methods = acallawaitable_methods,
};
