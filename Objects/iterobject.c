/* Iterator objects */

#include "Python.h"
#include "pycore_abstract.h"      // _PyObject_HasLen()
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_ceval.h"         // _PyEval_GetBuiltin()
#include "pycore_genobject.h"     // _PyCoro_GetAwaitableIter()
#include "pycore_object.h"        // _PyObject_GC_TRACK()


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
    PyObject *it_callable; /* Set to NULL when iterator is exhausted */
    PyObject *it_sentinel; /* Set to NULL when iterator is exhausted */
} calliterobject;

PyObject *
PyCallIter_New(PyObject *callable, PyObject *sentinel)
{
    calliterobject *it;
    it = PyObject_GC_New(calliterobject, &PyCallIter_Type);
    if (it == NULL)
        return NULL;
    it->it_callable = Py_NewRef(callable);
    it->it_sentinel = Py_NewRef(sentinel);
    _PyObject_GC_TRACK(it);
    return (PyObject *)it;
}
static void
calliter_dealloc(PyObject *op)
{
    calliterobject *it = (calliterobject*)op;
    _PyObject_GC_UNTRACK(it);
    Py_XDECREF(it->it_callable);
    Py_XDECREF(it->it_sentinel);
    PyObject_GC_Del(it);
}

static int
calliter_traverse(PyObject *op, visitproc visit, void *arg)
{
    calliterobject *it = (calliterobject*)op;
    Py_VISIT(it->it_callable);
    Py_VISIT(it->it_sentinel);
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
    if (result != NULL && it->it_sentinel != NULL){
        int ok;

        ok = PyObject_RichCompareBool(it->it_sentinel, result, Py_EQ);
        if (ok == 0) {
            return result; /* Common case, fast path */
        }

        if (ok > 0) {
            Py_CLEAR(it->it_callable);
            Py_CLEAR(it->it_sentinel);
        }
    }
    else if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
        PyErr_Clear();
        Py_CLEAR(it->it_callable);
        Py_CLEAR(it->it_sentinel);
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

    if (it->it_callable != NULL && it->it_sentinel != NULL)
        return Py_BuildValue("N(OO)", iter, it->it_callable, it->it_sentinel);
    else
        return Py_BuildValue("N(())", iter);
}

static PyMethodDef calliter_methods[] = {
    {"__reduce__", calliter_reduce, METH_NOARGS, reduce_doc},
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
anextawaitable_getiter(anextawaitableobject *obj)
{
    assert(obj->wrapped != NULL);
    PyObject *awaitable = _PyCoro_GetAwaitableIter(obj->wrapped);
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
                         obj, awaitable);
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
    PyObject *awaitable = anextawaitable_getiter(obj);
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
    PyObject *awaitable = anextawaitable_getiter(obj);
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
