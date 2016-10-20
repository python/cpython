#include "Python.h"
#include "structmember.h"


/* identifiers used from some functions */
_Py_IDENTIFIER(call_soon);


/* State of the _asyncio module */
static PyObject *traceback_extract_stack;
static PyObject *asyncio_get_event_loop;
static PyObject *asyncio_repr_info_func;
static PyObject *asyncio_InvalidStateError;
static PyObject *asyncio_CancelledError;


/* Get FutureIter from Future */
static PyObject* new_future_iter(PyObject *fut);


typedef enum {
    STATE_PENDING,
    STATE_CANCELLED,
    STATE_FINISHED
} fut_state;


typedef struct {
    PyObject_HEAD
    PyObject *fut_loop;
    PyObject *fut_callbacks;
    PyObject *fut_exception;
    PyObject *fut_result;
    PyObject *fut_source_tb;
    fut_state fut_state;
    int fut_log_tb;
    int fut_blocking;
    PyObject *dict;
    PyObject *fut_weakreflist;
} FutureObj;


static int
_schedule_callbacks(FutureObj *fut)
{
    Py_ssize_t len;
    PyObject* iters;
    int i;

    if (fut->fut_callbacks == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "NULL callbacks");
        return -1;
    }

    len = PyList_GET_SIZE(fut->fut_callbacks);
    if (len == 0) {
        return 0;
    }

    iters = PyList_GetSlice(fut->fut_callbacks, 0, len);
    if (iters == NULL) {
        return -1;
    }
    if (PyList_SetSlice(fut->fut_callbacks, 0, len, NULL) < 0) {
        Py_DECREF(iters);
        return -1;
    }

    for (i = 0; i < len; i++) {
        PyObject *handle = NULL;
        PyObject *cb = PyList_GET_ITEM(iters, i);

        handle = _PyObject_CallMethodId(
            fut->fut_loop, &PyId_call_soon, "OO", cb, fut, NULL);

        if (handle == NULL) {
            Py_DECREF(iters);
            return -1;
        }
        else {
            Py_DECREF(handle);
        }
    }

    Py_DECREF(iters);
    return 0;
}

static int
FutureObj_init(FutureObj *fut, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"loop", NULL};
    PyObject *loop = NULL;
    PyObject *res = NULL;
    _Py_IDENTIFIER(get_debug);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|$O", kwlist, &loop)) {
        return -1;
    }
    if (loop == NULL || loop == Py_None) {
        loop = PyObject_CallObject(asyncio_get_event_loop, NULL);
        if (loop == NULL) {
            return -1;
        }
    }
    else {
        Py_INCREF(loop);
    }
    Py_CLEAR(fut->fut_loop);
    fut->fut_loop = loop;

    res = _PyObject_CallMethodId(fut->fut_loop, &PyId_get_debug, "()", NULL);
    if (res == NULL) {
        return -1;
    }
    if (PyObject_IsTrue(res)) {
        Py_CLEAR(res);
        fut->fut_source_tb = PyObject_CallObject(traceback_extract_stack, NULL);
        if (fut->fut_source_tb == NULL) {
            return -1;
        }
    }
    else {
        Py_CLEAR(res);
    }

    fut->fut_callbacks = PyList_New(0);
    if (fut->fut_callbacks == NULL) {
        return -1;
    }
    return 0;
}

static int
FutureObj_clear(FutureObj *fut)
{
    Py_CLEAR(fut->fut_loop);
    Py_CLEAR(fut->fut_callbacks);
    Py_CLEAR(fut->fut_result);
    Py_CLEAR(fut->fut_exception);
    Py_CLEAR(fut->fut_source_tb);
    Py_CLEAR(fut->dict);
    return 0;
}

static int
FutureObj_traverse(FutureObj *fut, visitproc visit, void *arg)
{
    Py_VISIT(fut->fut_loop);
    Py_VISIT(fut->fut_callbacks);
    Py_VISIT(fut->fut_result);
    Py_VISIT(fut->fut_exception);
    Py_VISIT(fut->fut_source_tb);
    Py_VISIT(fut->dict);
    return 0;
}

PyDoc_STRVAR(pydoc_result,
    "Return the result this future represents.\n"
    "\n"
    "If the future has been cancelled, raises CancelledError.  If the\n"
    "future's result isn't yet available, raises InvalidStateError.  If\n"
    "the future is done and has an exception set, this exception is raised."
);

static PyObject *
FutureObj_result(FutureObj *fut, PyObject *arg)
{
    if (fut->fut_state == STATE_CANCELLED) {
        PyErr_SetString(asyncio_CancelledError, "");
        return NULL;
    }

    if (fut->fut_state != STATE_FINISHED) {
        PyErr_SetString(asyncio_InvalidStateError, "Result is not ready.");
        return NULL;
    }

    fut->fut_log_tb = 0;
    if (fut->fut_exception != NULL) {
        PyObject *type = NULL;
        type = PyExceptionInstance_Class(fut->fut_exception);
        PyErr_SetObject(type, fut->fut_exception);
        return NULL;
    }

    Py_INCREF(fut->fut_result);
    return fut->fut_result;
}

PyDoc_STRVAR(pydoc_exception,
    "Return the exception that was set on this future.\n"
    "\n"
    "The exception (or None if no exception was set) is returned only if\n"
    "the future is done.  If the future has been cancelled, raises\n"
    "CancelledError.  If the future isn't done yet, raises\n"
    "InvalidStateError."
);

static PyObject *
FutureObj_exception(FutureObj *fut, PyObject *arg)
{
    if (fut->fut_state == STATE_CANCELLED) {
        PyErr_SetString(asyncio_CancelledError, "");
        return NULL;
    }

    if (fut->fut_state != STATE_FINISHED) {
        PyErr_SetString(asyncio_InvalidStateError, "Result is not ready.");
        return NULL;
    }

    if (fut->fut_exception != NULL) {
        fut->fut_log_tb = 0;
        Py_INCREF(fut->fut_exception);
        return fut->fut_exception;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(pydoc_set_result,
    "Mark the future done and set its result.\n"
    "\n"
    "If the future is already done when this method is called, raises\n"
    "InvalidStateError."
);

static PyObject *
FutureObj_set_result(FutureObj *fut, PyObject *res)
{
    if (fut->fut_state != STATE_PENDING) {
        PyErr_SetString(asyncio_InvalidStateError, "invalid state");
        return NULL;
    }

    Py_INCREF(res);
    fut->fut_result = res;
    fut->fut_state = STATE_FINISHED;

    if (_schedule_callbacks(fut) == -1) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(pydoc_set_exception,
    "Mark the future done and set an exception.\n"
    "\n"
    "If the future is already done when this method is called, raises\n"
    "InvalidStateError."
);

static PyObject *
FutureObj_set_exception(FutureObj *fut, PyObject *exc)
{
    PyObject *exc_val = NULL;

    if (fut->fut_state != STATE_PENDING) {
        PyErr_SetString(asyncio_InvalidStateError, "invalid state");
        return NULL;
    }

    if (PyExceptionClass_Check(exc)) {
        exc_val = PyObject_CallObject(exc, NULL);
        if (exc_val == NULL) {
            return NULL;
        }
    }
    else {
        exc_val = exc;
        Py_INCREF(exc_val);
    }
    if (!PyExceptionInstance_Check(exc_val)) {
        Py_DECREF(exc_val);
        PyErr_SetString(PyExc_TypeError, "invalid exception object");
        return NULL;
    }
    if ((PyObject*)Py_TYPE(exc_val) == PyExc_StopIteration) {
        Py_DECREF(exc_val);
        PyErr_SetString(PyExc_TypeError,
                        "StopIteration interacts badly with generators "
                        "and cannot be raised into a Future");
        return NULL;
    }

    fut->fut_exception = exc_val;
    fut->fut_state = STATE_FINISHED;

    if (_schedule_callbacks(fut) == -1) {
        return NULL;
    }

    fut->fut_log_tb = 1;
    Py_RETURN_NONE;
}

PyDoc_STRVAR(pydoc_add_done_callback,
    "Add a callback to be run when the future becomes done.\n"
    "\n"
    "The callback is called with a single argument - the future object. If\n"
    "the future is already done when this is called, the callback is\n"
    "scheduled with call_soon.";
);

static PyObject *
FutureObj_add_done_callback(FutureObj *fut, PyObject *arg)
{
    if (fut->fut_state != STATE_PENDING) {
        PyObject *handle = _PyObject_CallMethodId(
            fut->fut_loop, &PyId_call_soon, "OO", arg, fut, NULL);

        if (handle == NULL) {
            return NULL;
        }
        else {
            Py_DECREF(handle);
        }
    }
    else {
        int err = PyList_Append(fut->fut_callbacks, arg);
        if (err != 0) {
            return NULL;
        }
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(pydoc_remove_done_callback,
    "Remove all instances of a callback from the \"call when done\" list.\n"
    "\n"
    "Returns the number of callbacks removed."
);

static PyObject *
FutureObj_remove_done_callback(FutureObj *fut, PyObject *arg)
{
    PyObject *newlist;
    Py_ssize_t len, i, j=0;

    len = PyList_GET_SIZE(fut->fut_callbacks);
    if (len == 0) {
        return PyLong_FromSsize_t(0);
    }

    newlist = PyList_New(len);
    if (newlist == NULL) {
        return NULL;
    }

    for (i = 0; i < len; i++) {
        int ret;
        PyObject *item = PyList_GET_ITEM(fut->fut_callbacks, i);

        if ((ret = PyObject_RichCompareBool(arg, item, Py_EQ)) < 0) {
            goto fail;
        }
        if (ret == 0) {
            Py_INCREF(item);
            PyList_SET_ITEM(newlist, j, item);
            j++;
        }
    }

    if (PyList_SetSlice(newlist, j, len, NULL) < 0) {
        goto fail;
    }
    if (PyList_SetSlice(fut->fut_callbacks, 0, len, newlist) < 0) {
        goto fail;
    }
    Py_DECREF(newlist);
    return PyLong_FromSsize_t(len - j);

fail:
    Py_DECREF(newlist);
    return NULL;
}

PyDoc_STRVAR(pydoc_cancel,
    "Cancel the future and schedule callbacks.\n"
    "\n"
    "If the future is already done or cancelled, return False.  Otherwise,\n"
    "change the future's state to cancelled, schedule the callbacks and\n"
    "return True."
);

static PyObject *
FutureObj_cancel(FutureObj *fut, PyObject *arg)
{
    if (fut->fut_state != STATE_PENDING) {
        Py_RETURN_FALSE;
    }
    fut->fut_state = STATE_CANCELLED;

    if (_schedule_callbacks(fut) == -1) {
        return NULL;
    }

    Py_RETURN_TRUE;
}

PyDoc_STRVAR(pydoc_cancelled, "Return True if the future was cancelled.");

static PyObject *
FutureObj_cancelled(FutureObj *fut, PyObject *arg)
{
    if (fut->fut_state == STATE_CANCELLED) {
        Py_RETURN_TRUE;
    }
    else {
        Py_RETURN_FALSE;
    }
}

PyDoc_STRVAR(pydoc_done,
    "Return True if the future is done.\n"
    "\n"
    "Done means either that a result / exception are available, or that the\n"
    "future was cancelled."
);

static PyObject *
FutureObj_done(FutureObj *fut, PyObject *arg)
{
    if (fut->fut_state == STATE_PENDING) {
        Py_RETURN_FALSE;
    }
    else {
        Py_RETURN_TRUE;
    }
}

static PyObject *
FutureObj_get_blocking(FutureObj *fut)
{
    if (fut->fut_blocking) {
        Py_RETURN_TRUE;
    }
    else {
        Py_RETURN_FALSE;
    }
}

static int
FutureObj_set_blocking(FutureObj *fut, PyObject *val)
{
    int is_true = PyObject_IsTrue(val);
    if (is_true < 0) {
        return -1;
    }
    fut->fut_blocking = is_true;
    return 0;
}

static PyObject *
FutureObj_get_log_traceback(FutureObj *fut)
{
    if (fut->fut_log_tb) {
        Py_RETURN_TRUE;
    }
    else {
        Py_RETURN_FALSE;
    }
}

static PyObject *
FutureObj_get_loop(FutureObj *fut)
{
    if (fut->fut_loop == NULL) {
        Py_RETURN_NONE;
    }
    Py_INCREF(fut->fut_loop);
    return fut->fut_loop;
}

static PyObject *
FutureObj_get_callbacks(FutureObj *fut)
{
    if (fut->fut_callbacks == NULL) {
        Py_RETURN_NONE;
    }
    Py_INCREF(fut->fut_callbacks);
    return fut->fut_callbacks;
}

static PyObject *
FutureObj_get_result(FutureObj *fut)
{
    if (fut->fut_result == NULL) {
        Py_RETURN_NONE;
    }
    Py_INCREF(fut->fut_result);
    return fut->fut_result;
}

static PyObject *
FutureObj_get_exception(FutureObj *fut)
{
    if (fut->fut_exception == NULL) {
        Py_RETURN_NONE;
    }
    Py_INCREF(fut->fut_exception);
    return fut->fut_exception;
}

static PyObject *
FutureObj_get_source_traceback(FutureObj *fut)
{
    if (fut->fut_source_tb == NULL) {
        Py_RETURN_NONE;
    }
    Py_INCREF(fut->fut_source_tb);
    return fut->fut_source_tb;
}

static PyObject *
FutureObj_get_state(FutureObj *fut)
{
    _Py_IDENTIFIER(PENDING);
    _Py_IDENTIFIER(CANCELLED);
    _Py_IDENTIFIER(FINISHED);
    PyObject *ret = NULL;

    switch (fut->fut_state) {
    case STATE_PENDING:
        ret = _PyUnicode_FromId(&PyId_PENDING);
        break;
    case STATE_CANCELLED:
        ret = _PyUnicode_FromId(&PyId_CANCELLED);
        break;
    case STATE_FINISHED:
        ret = _PyUnicode_FromId(&PyId_FINISHED);
        break;
    default:
        assert (0);
    }
    Py_INCREF(ret);
    return ret;
}

static PyObject*
FutureObj__repr_info(FutureObj *fut)
{
    if (asyncio_repr_info_func == NULL) {
        return PyList_New(0);
    }
    return PyObject_CallFunctionObjArgs(asyncio_repr_info_func, fut, NULL);
}

static PyObject *
FutureObj_repr(FutureObj *fut)
{
    _Py_IDENTIFIER(_repr_info);

    PyObject *_repr_info = _PyUnicode_FromId(&PyId__repr_info);  // borrowed
    if (_repr_info == NULL) {
        return NULL;
    }

    PyObject *rinfo = PyObject_CallMethodObjArgs((PyObject*)fut, _repr_info,
                                                 NULL);
    if (rinfo == NULL) {
        return NULL;
    }

    PyObject *sp = PyUnicode_FromString(" ");
    if (sp == NULL) {
        Py_DECREF(rinfo);
        return NULL;
    }

    PyObject *rinfo_s = PyUnicode_Join(sp, rinfo);
    Py_DECREF(sp);
    Py_DECREF(rinfo);
    if (rinfo_s == NULL) {
        return NULL;
    }

    PyObject *rstr = NULL;
    PyObject *type_name = PyObject_GetAttrString((PyObject*)Py_TYPE(fut),
                                                 "__name__");
    if (type_name != NULL) {
        rstr = PyUnicode_FromFormat("<%S %S>", type_name, rinfo_s);
        Py_DECREF(type_name);
    }
    Py_DECREF(rinfo_s);
    return rstr;
}

static void
FutureObj_finalize(FutureObj *fut)
{
    _Py_IDENTIFIER(call_exception_handler);
    _Py_IDENTIFIER(message);
    _Py_IDENTIFIER(exception);
    _Py_IDENTIFIER(future);
    _Py_IDENTIFIER(source_traceback);

    if (!fut->fut_log_tb) {
        return;
    }
    assert(fut->fut_exception != NULL);
    fut->fut_log_tb = 0;;

    PyObject *error_type, *error_value, *error_traceback;
    /* Save the current exception, if any. */
    PyErr_Fetch(&error_type, &error_value, &error_traceback);

    PyObject *context = NULL;
    PyObject *type_name = NULL;
    PyObject *message = NULL;
    PyObject *func = NULL;
    PyObject *res = NULL;

    context = PyDict_New();
    if (context == NULL) {
        goto finally;
    }

    type_name = PyObject_GetAttrString((PyObject*)Py_TYPE(fut), "__name__");
    if (type_name == NULL) {
        goto finally;
    }

    message = PyUnicode_FromFormat(
        "%S exception was never retrieved", type_name);
    if (message == NULL) {
        goto finally;
    }

    if (_PyDict_SetItemId(context, &PyId_message, message) < 0 ||
        _PyDict_SetItemId(context, &PyId_exception, fut->fut_exception) < 0 ||
        _PyDict_SetItemId(context, &PyId_future, (PyObject*)fut) < 0) {
        goto finally;
    }
    if (fut->fut_source_tb != NULL) {
        if (_PyDict_SetItemId(context, &PyId_source_traceback,
                              fut->fut_source_tb) < 0) {
            goto finally;
        }
    }

    func = _PyObject_GetAttrId(fut->fut_loop, &PyId_call_exception_handler);
    if (func != NULL) {
        res = _PyObject_CallArg1(func, context);
        if (res == NULL) {
            PyErr_WriteUnraisable(func);
        }
    }

finally:
    Py_CLEAR(context);
    Py_CLEAR(type_name);
    Py_CLEAR(message);
    Py_CLEAR(func);
    Py_CLEAR(res);

    /* Restore the saved exception. */
    PyErr_Restore(error_type, error_value, error_traceback);
}


static PyAsyncMethods FutureType_as_async = {
    (unaryfunc)new_future_iter,         /* am_await */
    0,                                  /* am_aiter */
    0                                   /* am_anext */
};

static PyMethodDef FutureType_methods[] = {
    {"_repr_info", (PyCFunction)FutureObj__repr_info, METH_NOARGS, NULL},
    {"add_done_callback",
        (PyCFunction)FutureObj_add_done_callback,
        METH_O, pydoc_add_done_callback},
    {"remove_done_callback",
        (PyCFunction)FutureObj_remove_done_callback,
        METH_O, pydoc_remove_done_callback},
    {"set_result",
        (PyCFunction)FutureObj_set_result, METH_O, pydoc_set_result},
    {"set_exception",
        (PyCFunction)FutureObj_set_exception, METH_O, pydoc_set_exception},
    {"cancel", (PyCFunction)FutureObj_cancel, METH_NOARGS, pydoc_cancel},
    {"cancelled",
        (PyCFunction)FutureObj_cancelled, METH_NOARGS, pydoc_cancelled},
    {"done", (PyCFunction)FutureObj_done, METH_NOARGS, pydoc_done},
    {"result", (PyCFunction)FutureObj_result, METH_NOARGS, pydoc_result},
    {"exception",
        (PyCFunction)FutureObj_exception, METH_NOARGS, pydoc_exception},
    {NULL, NULL}        /* Sentinel */
};

static PyGetSetDef FutureType_getsetlist[] = {
    {"_state", (getter)FutureObj_get_state, NULL, NULL},
    {"_asyncio_future_blocking", (getter)FutureObj_get_blocking,
                                 (setter)FutureObj_set_blocking, NULL},
    {"_loop", (getter)FutureObj_get_loop, NULL, NULL},
    {"_callbacks", (getter)FutureObj_get_callbacks, NULL, NULL},
    {"_result", (getter)FutureObj_get_result, NULL, NULL},
    {"_exception", (getter)FutureObj_get_exception, NULL, NULL},
    {"_log_traceback", (getter)FutureObj_get_log_traceback, NULL, NULL},
    {"_source_traceback", (getter)FutureObj_get_source_traceback, NULL, NULL},
    {NULL} /* Sentinel */
};

static void FutureObj_dealloc(PyObject *self);

static PyTypeObject FutureType = {
    PyVarObject_HEAD_INIT(0, 0)
    "_asyncio.Future",
    sizeof(FutureObj),                       /* tp_basicsize */
    .tp_dealloc = FutureObj_dealloc,
    .tp_as_async = &FutureType_as_async,
    .tp_repr = (reprfunc)FutureObj_repr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE
        | Py_TPFLAGS_HAVE_FINALIZE,
    .tp_doc = "Fast asyncio.Future implementation.",
    .tp_traverse = (traverseproc)FutureObj_traverse,
    .tp_clear = (inquiry)FutureObj_clear,
    .tp_weaklistoffset = offsetof(FutureObj, fut_weakreflist),
    .tp_iter = (getiterfunc)new_future_iter,
    .tp_methods = FutureType_methods,
    .tp_getset = FutureType_getsetlist,
    .tp_dictoffset = offsetof(FutureObj, dict),
    .tp_init = (initproc)FutureObj_init,
    .tp_new = PyType_GenericNew,
    .tp_finalize = (destructor)FutureObj_finalize,
};

static void
FutureObj_dealloc(PyObject *self)
{
    FutureObj *fut = (FutureObj *)self;

    if (Py_TYPE(fut) == &FutureType) {
        /* When fut is subclass of Future, finalizer is called from
         * subtype_dealloc.
         */
        if (PyObject_CallFinalizerFromDealloc(self) < 0) {
            // resurrected.
            return;
        }
    }

    if (fut->fut_weakreflist != NULL) {
        PyObject_ClearWeakRefs(self);
    }

    FutureObj_clear(fut);
    Py_TYPE(fut)->tp_free(fut);
}


/*********************** Future Iterator **************************/

typedef struct {
    PyObject_HEAD
    FutureObj *future;
} futureiterobject;

static void
FutureIter_dealloc(futureiterobject *it)
{
    _PyObject_GC_UNTRACK(it);
    Py_XDECREF(it->future);
    PyObject_GC_Del(it);
}

static PyObject *
FutureIter_iternext(futureiterobject *it)
{
    PyObject *res;
    FutureObj *fut = it->future;

    if (fut == NULL) {
        return NULL;
    }

    if (fut->fut_state == STATE_PENDING) {
        if (!fut->fut_blocking) {
            fut->fut_blocking = 1;
            Py_INCREF(fut);
            return (PyObject *)fut;
        }
        PyErr_Format(PyExc_AssertionError,
                     "yield from wasn't used with future");
        return NULL;
    }

    res = FutureObj_result(fut, NULL);
    if (res != NULL) {
        /* The result of the Future is not an exception.

           We cunstruct an exception instance manually with
           PyObject_CallFunctionObjArgs and pass it to PyErr_SetObject
           (similarly to what genobject.c does).

           This is to handle a situation when "res" is a tuple, in which
           case PyErr_SetObject would set the value of StopIteration to
           the first element of the tuple.

           (See PyErr_SetObject/_PyErr_CreateException code for details.)
        */
        PyObject *e = PyObject_CallFunctionObjArgs(
            PyExc_StopIteration, res, NULL);
        Py_DECREF(res);
        if (e == NULL) {
            return NULL;
        }
        PyErr_SetObject(PyExc_StopIteration, e);
        Py_DECREF(e);
    }

    it->future = NULL;
    Py_DECREF(fut);
    return NULL;
}

static PyObject *
FutureIter_send(futureiterobject *self, PyObject *arg)
{
    if (arg != Py_None) {
        PyErr_Format(PyExc_TypeError,
                     "can't send non-None value to a FutureIter");
        return NULL;
    }
    return FutureIter_iternext(self);
}

static PyObject *
FutureIter_throw(futureiterobject *self, PyObject *args)
{
    PyObject *type=NULL, *val=NULL, *tb=NULL;
    if (!PyArg_ParseTuple(args, "O|OO", &type, &val, &tb))
        return NULL;

    if (val == Py_None) {
        val = NULL;
    }
    if (tb == Py_None) {
        tb = NULL;
    }

    Py_CLEAR(self->future);

    if (tb != NULL) {
        PyErr_Restore(type, val, tb);
    }
    else if (val != NULL) {
        PyErr_SetObject(type, val);
    }
    else {
        if (PyExceptionClass_Check(type)) {
            val = PyObject_CallObject(type, NULL);
        }
        else {
            val = type;
            assert (PyExceptionInstance_Check(val));
            type = (PyObject*)Py_TYPE(val);
            assert (PyExceptionClass_Check(type));
        }
        PyErr_SetObject(type, val);
    }
    return FutureIter_iternext(self);
}

static PyObject *
FutureIter_close(futureiterobject *self, PyObject *arg)
{
    Py_CLEAR(self->future);
    Py_RETURN_NONE;
}

static int
FutureIter_traverse(futureiterobject *it, visitproc visit, void *arg)
{
    Py_VISIT(it->future);
    return 0;
}

static PyMethodDef FutureIter_methods[] = {
    {"send",  (PyCFunction)FutureIter_send, METH_O, NULL},
    {"throw", (PyCFunction)FutureIter_throw, METH_VARARGS, NULL},
    {"close", (PyCFunction)FutureIter_close, METH_NOARGS, NULL},
    {NULL, NULL}        /* Sentinel */
};

static PyTypeObject FutureIterType = {
    PyVarObject_HEAD_INIT(0, 0)
    "_asyncio.FutureIter",
    sizeof(futureiterobject),                /* tp_basicsize */
    0,                                       /* tp_itemsize */
    (destructor)FutureIter_dealloc,          /* tp_dealloc */
    0,                                       /* tp_print */
    0,                                       /* tp_getattr */
    0,                                       /* tp_setattr */
    0,                                       /* tp_as_async */
    0,                                       /* tp_repr */
    0,                                       /* tp_as_number */
    0,                                       /* tp_as_sequence */
    0,                                       /* tp_as_mapping */
    0,                                       /* tp_hash */
    0,                                       /* tp_call */
    0,                                       /* tp_str */
    PyObject_GenericGetAttr,                 /* tp_getattro */
    0,                                       /* tp_setattro */
    0,                                       /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    0,                                       /* tp_doc */
    (traverseproc)FutureIter_traverse,       /* tp_traverse */
    0,                                       /* tp_clear */
    0,                                       /* tp_richcompare */
    0,                                       /* tp_weaklistoffset */
    PyObject_SelfIter,                       /* tp_iter */
    (iternextfunc)FutureIter_iternext,       /* tp_iternext */
    FutureIter_methods,                      /* tp_methods */
    0,                                       /* tp_members */
};

static PyObject *
new_future_iter(PyObject *fut)
{
    futureiterobject *it;

    if (!PyObject_TypeCheck(fut, &FutureType)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    it = PyObject_GC_New(futureiterobject, &FutureIterType);
    if (it == NULL) {
        return NULL;
    }
    Py_INCREF(fut);
    it->future = (FutureObj*)fut;
    PyObject_GC_Track(it);
    return (PyObject*)it;
}

/*********************** Module **************************/

static int
init_module(void)
{
    PyObject *module = NULL;

    module = PyImport_ImportModule("traceback");
    if (module == NULL) {
        return -1;
    }
    // new reference
    traceback_extract_stack = PyObject_GetAttrString(module, "extract_stack");
    if (traceback_extract_stack == NULL) {
        goto fail;
    }
    Py_DECREF(module);

    module = PyImport_ImportModule("asyncio.events");
    if (module == NULL) {
        goto fail;
    }
    asyncio_get_event_loop = PyObject_GetAttrString(module, "get_event_loop");
    if (asyncio_get_event_loop == NULL) {
        goto fail;
    }
    Py_DECREF(module);

    module = PyImport_ImportModule("asyncio.futures");
    if (module == NULL) {
        goto fail;
    }
    asyncio_repr_info_func = PyObject_GetAttrString(module,
                                                    "_future_repr_info");
    if (asyncio_repr_info_func == NULL) {
        goto fail;
    }

    asyncio_InvalidStateError = PyObject_GetAttrString(module,
                                                       "InvalidStateError");
    if (asyncio_InvalidStateError == NULL) {
        goto fail;
    }

    asyncio_CancelledError = PyObject_GetAttrString(module, "CancelledError");
    if (asyncio_CancelledError == NULL) {
        goto fail;
    }

    Py_DECREF(module);
    return 0;

fail:
    Py_CLEAR(traceback_extract_stack);
    Py_CLEAR(asyncio_get_event_loop);
    Py_CLEAR(asyncio_repr_info_func);
    Py_CLEAR(asyncio_InvalidStateError);
    Py_CLEAR(asyncio_CancelledError);
    Py_CLEAR(module);
    return -1;
}


PyDoc_STRVAR(module_doc, "Accelerator module for asyncio");

static struct PyModuleDef _asynciomodule = {
    PyModuleDef_HEAD_INIT,      /* m_base */
    "_asyncio",                 /* m_name */
    module_doc,                 /* m_doc */
    -1,                         /* m_size */
    NULL,                       /* m_methods */
    NULL,                       /* m_slots */
    NULL,                       /* m_traverse */
    NULL,                       /* m_clear */
    NULL,                       /* m_free */
};


PyMODINIT_FUNC
PyInit__asyncio(void)
{
    if (init_module() < 0) {
        return NULL;
    }
    if (PyType_Ready(&FutureType) < 0) {
        return NULL;
    }
    if (PyType_Ready(&FutureIterType) < 0) {
        return NULL;
    }

    PyObject *m = PyModule_Create(&_asynciomodule);
    if (m == NULL) {
        return NULL;
    }

    Py_INCREF(&FutureType);
    if (PyModule_AddObject(m, "Future", (PyObject *)&FutureType) < 0) {
        Py_DECREF(&FutureType);
        return NULL;
    }

    return m;
}
