/* Generator object implementation */

#include "Python.h"
#include "frameobject.h"
#include "structmember.h"
#include "opcode.h"

static PyObject *gen_close(PyGenObject *gen, PyObject *args);

static int
gen_traverse(PyGenObject *gen, visitproc visit, void *arg)
{
    Py_VISIT((PyObject *)gen->gi_frame);
    Py_VISIT(gen->gi_code);
    Py_VISIT(gen->gi_name);
    Py_VISIT(gen->gi_qualname);
    return 0;
}

void
_PyGen_Finalize(PyObject *self)
{
    PyGenObject *gen = (PyGenObject *)self;
    PyObject *res;
    PyObject *error_type, *error_value, *error_traceback;

    /* If `gen` is a coroutine, and if it was never awaited on,
       issue a RuntimeWarning. */
    if (gen->gi_code != NULL
            && ((PyCodeObject *)gen->gi_code)->co_flags & CO_COROUTINE
            && gen->gi_frame != NULL
            && gen->gi_frame->f_lasti == -1
            && !PyErr_Occurred()
            && PyErr_WarnFormat(PyExc_RuntimeWarning, 1,
                                "coroutine '%.50S' was never awaited",
                                gen->gi_qualname))
        return;

    if (gen->gi_frame == NULL || gen->gi_frame->f_stacktop == NULL)
        /* Generator isn't paused, so no need to close */
        return;

    /* Save the current exception, if any. */
    PyErr_Fetch(&error_type, &error_value, &error_traceback);

    res = gen_close(gen, NULL);

    if (res == NULL)
        PyErr_WriteUnraisable(self);
    else
        Py_DECREF(res);

    /* Restore the saved exception. */
    PyErr_Restore(error_type, error_value, error_traceback);
}

static void
gen_dealloc(PyGenObject *gen)
{
    PyObject *self = (PyObject *) gen;

    _PyObject_GC_UNTRACK(gen);

    if (gen->gi_weakreflist != NULL)
        PyObject_ClearWeakRefs(self);

    _PyObject_GC_TRACK(self);

    if (PyObject_CallFinalizerFromDealloc(self))
        return;                     /* resurrected.  :( */

    _PyObject_GC_UNTRACK(self);
    Py_CLEAR(gen->gi_frame);
    Py_CLEAR(gen->gi_code);
    Py_CLEAR(gen->gi_name);
    Py_CLEAR(gen->gi_qualname);
    PyObject_GC_Del(gen);
}

static PyObject *
gen_send_ex(PyGenObject *gen, PyObject *arg, int exc)
{
    PyThreadState *tstate = PyThreadState_GET();
    PyFrameObject *f = gen->gi_frame;
    PyObject *result;

    if (gen->gi_running) {
        char *msg = "generator already executing";
        if (PyCoro_CheckExact(gen))
            msg = "coroutine already executing";
        PyErr_SetString(PyExc_ValueError, msg);
        return NULL;
    }
    if (f == NULL || f->f_stacktop == NULL) {
        /* Only set exception if called from send() */
        if (arg && !exc)
            PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }

    if (f->f_lasti == -1) {
        if (arg && arg != Py_None) {
            char *msg = "can't send non-None value to a "
                        "just-started generator";
            if (PyCoro_CheckExact(gen))
                msg = "can't send non-None value to a "
                      "just-started coroutine";
            PyErr_SetString(PyExc_TypeError, msg);
            return NULL;
        }
    } else {
        /* Push arg onto the frame's value stack */
        result = arg ? arg : Py_None;
        Py_INCREF(result);
        *(f->f_stacktop++) = result;
    }

    /* Generators always return to their most recent caller, not
     * necessarily their creator. */
    Py_XINCREF(tstate->frame);
    assert(f->f_back == NULL);
    f->f_back = tstate->frame;

    gen->gi_running = 1;
    result = PyEval_EvalFrameEx(f, exc);
    gen->gi_running = 0;

    /* Don't keep the reference to f_back any longer than necessary.  It
     * may keep a chain of frames alive or it could create a reference
     * cycle. */
    assert(f->f_back == tstate->frame);
    Py_CLEAR(f->f_back);

    /* If the generator just returned (as opposed to yielding), signal
     * that the generator is exhausted. */
    if (result && f->f_stacktop == NULL) {
        if (result == Py_None) {
            /* Delay exception instantiation if we can */
            PyErr_SetNone(PyExc_StopIteration);
        } else {
            PyObject *e = PyObject_CallFunctionObjArgs(
                               PyExc_StopIteration, result, NULL);
            if (e != NULL) {
                PyErr_SetObject(PyExc_StopIteration, e);
                Py_DECREF(e);
            }
        }
        Py_CLEAR(result);
    }
    else if (!result && PyErr_ExceptionMatches(PyExc_StopIteration)) {
        /* Check for __future__ generator_stop and conditionally turn
         * a leaking StopIteration into RuntimeError (with its cause
         * set appropriately). */
        if (((PyCodeObject *)gen->gi_code)->co_flags &
              (CO_FUTURE_GENERATOR_STOP | CO_COROUTINE | CO_ITERABLE_COROUTINE))
        {
            PyObject *exc, *val, *val2, *tb;
            char *msg = "generator raised StopIteration";
            if (PyCoro_CheckExact(gen))
                msg = "coroutine raised StopIteration";
            PyErr_Fetch(&exc, &val, &tb);
            PyErr_NormalizeException(&exc, &val, &tb);
            if (tb != NULL)
                PyException_SetTraceback(val, tb);
            Py_DECREF(exc);
            Py_XDECREF(tb);
            PyErr_SetString(PyExc_RuntimeError, msg);
            PyErr_Fetch(&exc, &val2, &tb);
            PyErr_NormalizeException(&exc, &val2, &tb);
            Py_INCREF(val);
            PyException_SetCause(val2, val);
            PyException_SetContext(val2, val);
            PyErr_Restore(exc, val2, tb);
        }
        else {
            PyObject *exc, *val, *tb;

            /* Pop the exception before issuing a warning. */
            PyErr_Fetch(&exc, &val, &tb);

            if (PyErr_WarnFormat(PyExc_PendingDeprecationWarning, 1,
                                 "generator '%.50S' raised StopIteration",
                                 gen->gi_qualname)) {
                /* Warning was converted to an error. */
                Py_XDECREF(exc);
                Py_XDECREF(val);
                Py_XDECREF(tb);
            }
            else {
                PyErr_Restore(exc, val, tb);
            }
        }
    }

    if (!result || f->f_stacktop == NULL) {
        /* generator can't be rerun, so release the frame */
        /* first clean reference cycle through stored exception traceback */
        PyObject *t, *v, *tb;
        t = f->f_exc_type;
        v = f->f_exc_value;
        tb = f->f_exc_traceback;
        f->f_exc_type = NULL;
        f->f_exc_value = NULL;
        f->f_exc_traceback = NULL;
        Py_XDECREF(t);
        Py_XDECREF(v);
        Py_XDECREF(tb);
        gen->gi_frame->f_gen = NULL;
        gen->gi_frame = NULL;
        Py_DECREF(f);
    }

    return result;
}

PyDoc_STRVAR(send_doc,
"send(arg) -> send 'arg' into generator,\n\
return next yielded value or raise StopIteration.");

PyObject *
_PyGen_Send(PyGenObject *gen, PyObject *arg)
{
    return gen_send_ex(gen, arg, 0);
}

PyDoc_STRVAR(close_doc,
"close() -> raise GeneratorExit inside generator.");

/*
 *   This helper function is used by gen_close and gen_throw to
 *   close a subiterator being delegated to by yield-from.
 */

static int
gen_close_iter(PyObject *yf)
{
    PyObject *retval = NULL;
    _Py_IDENTIFIER(close);

    if (PyGen_CheckExact(yf)) {
        retval = gen_close((PyGenObject *)yf, NULL);
        if (retval == NULL)
            return -1;
    } else {
        PyObject *meth = _PyObject_GetAttrId(yf, &PyId_close);
        if (meth == NULL) {
            if (!PyErr_ExceptionMatches(PyExc_AttributeError))
                PyErr_WriteUnraisable(yf);
            PyErr_Clear();
        } else {
            retval = PyObject_CallFunction(meth, "");
            Py_DECREF(meth);
            if (retval == NULL)
                return -1;
        }
    }
    Py_XDECREF(retval);
    return 0;
}

static PyObject *
gen_yf(PyGenObject *gen)
{
    PyObject *yf = NULL;
    PyFrameObject *f = gen->gi_frame;

    if (f && f->f_stacktop) {
        PyObject *bytecode = f->f_code->co_code;
        unsigned char *code = (unsigned char *)PyBytes_AS_STRING(bytecode);

        if (code[f->f_lasti + 1] != YIELD_FROM)
            return NULL;
        yf = f->f_stacktop[-1];
        Py_INCREF(yf);
    }

    return yf;
}

static PyObject *
gen_close(PyGenObject *gen, PyObject *args)
{
    PyObject *retval;
    PyObject *yf = gen_yf(gen);
    int err = 0;

    if (yf) {
        gen->gi_running = 1;
        err = gen_close_iter(yf);
        gen->gi_running = 0;
        Py_DECREF(yf);
    }
    if (err == 0)
        PyErr_SetNone(PyExc_GeneratorExit);
    retval = gen_send_ex(gen, Py_None, 1);
    if (retval) {
        char *msg = "generator ignored GeneratorExit";
        if (PyCoro_CheckExact(gen))
            msg = "coroutine ignored GeneratorExit";
        Py_DECREF(retval);
        PyErr_SetString(PyExc_RuntimeError, msg);
        return NULL;
    }
    if (PyErr_ExceptionMatches(PyExc_StopIteration)
        || PyErr_ExceptionMatches(PyExc_GeneratorExit)) {
        PyErr_Clear();          /* ignore these errors */
        Py_INCREF(Py_None);
        return Py_None;
    }
    return NULL;
}


PyDoc_STRVAR(throw_doc,
"throw(typ[,val[,tb]]) -> raise exception in generator,\n\
return next yielded value or raise StopIteration.");

static PyObject *
gen_throw(PyGenObject *gen, PyObject *args)
{
    PyObject *typ;
    PyObject *tb = NULL;
    PyObject *val = NULL;
    PyObject *yf = gen_yf(gen);
    _Py_IDENTIFIER(throw);

    if (!PyArg_UnpackTuple(args, "throw", 1, 3, &typ, &val, &tb))
        return NULL;

    if (yf) {
        PyObject *ret;
        int err;
        if (PyErr_GivenExceptionMatches(typ, PyExc_GeneratorExit)) {
            gen->gi_running = 1;
            err = gen_close_iter(yf);
            gen->gi_running = 0;
            Py_DECREF(yf);
            if (err < 0)
                return gen_send_ex(gen, Py_None, 1);
            goto throw_here;
        }
        if (PyGen_CheckExact(yf)) {
            gen->gi_running = 1;
            ret = gen_throw((PyGenObject *)yf, args);
            gen->gi_running = 0;
        } else {
            PyObject *meth = _PyObject_GetAttrId(yf, &PyId_throw);
            if (meth == NULL) {
                if (!PyErr_ExceptionMatches(PyExc_AttributeError)) {
                    Py_DECREF(yf);
                    return NULL;
                }
                PyErr_Clear();
                Py_DECREF(yf);
                goto throw_here;
            }
            gen->gi_running = 1;
            ret = PyObject_CallObject(meth, args);
            gen->gi_running = 0;
            Py_DECREF(meth);
        }
        Py_DECREF(yf);
        if (!ret) {
            PyObject *val;
            /* Pop subiterator from stack */
            ret = *(--gen->gi_frame->f_stacktop);
            assert(ret == yf);
            Py_DECREF(ret);
            /* Termination repetition of YIELD_FROM */
            gen->gi_frame->f_lasti++;
            if (_PyGen_FetchStopIterationValue(&val) == 0) {
                ret = gen_send_ex(gen, val, 0);
                Py_DECREF(val);
            } else {
                ret = gen_send_ex(gen, Py_None, 1);
            }
        }
        return ret;
    }

throw_here:
    /* First, check the traceback argument, replacing None with
       NULL. */
    if (tb == Py_None) {
        tb = NULL;
    }
    else if (tb != NULL && !PyTraceBack_Check(tb)) {
        PyErr_SetString(PyExc_TypeError,
            "throw() third argument must be a traceback object");
        return NULL;
    }

    Py_INCREF(typ);
    Py_XINCREF(val);
    Py_XINCREF(tb);

    if (PyExceptionClass_Check(typ))
        PyErr_NormalizeException(&typ, &val, &tb);

    else if (PyExceptionInstance_Check(typ)) {
        /* Raising an instance.  The value should be a dummy. */
        if (val && val != Py_None) {
            PyErr_SetString(PyExc_TypeError,
              "instance exception may not have a separate value");
            goto failed_throw;
        }
        else {
            /* Normalize to raise <class>, <instance> */
            Py_XDECREF(val);
            val = typ;
            typ = PyExceptionInstance_Class(typ);
            Py_INCREF(typ);

            if (tb == NULL)
                /* Returns NULL if there's no traceback */
                tb = PyException_GetTraceback(val);
        }
    }
    else {
        /* Not something you can raise.  throw() fails. */
        PyErr_Format(PyExc_TypeError,
                     "exceptions must be classes or instances "
                     "deriving from BaseException, not %s",
                     Py_TYPE(typ)->tp_name);
            goto failed_throw;
    }

    PyErr_Restore(typ, val, tb);
    return gen_send_ex(gen, Py_None, 1);

failed_throw:
    /* Didn't use our arguments, so restore their original refcounts */
    Py_DECREF(typ);
    Py_XDECREF(val);
    Py_XDECREF(tb);
    return NULL;
}


static PyObject *
gen_iternext(PyGenObject *gen)
{
    return gen_send_ex(gen, NULL, 0);
}

/*
 *   If StopIteration exception is set, fetches its 'value'
 *   attribute if any, otherwise sets pvalue to None.
 *
 *   Returns 0 if no exception or StopIteration is set.
 *   If any other exception is set, returns -1 and leaves
 *   pvalue unchanged.
 */

int
_PyGen_FetchStopIterationValue(PyObject **pvalue) {
    PyObject *et, *ev, *tb;
    PyObject *value = NULL;

    if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
        PyErr_Fetch(&et, &ev, &tb);
        if (ev) {
            /* exception will usually be normalised already */
            if (PyObject_TypeCheck(ev, (PyTypeObject *) et)) {
                value = ((PyStopIterationObject *)ev)->value;
                Py_INCREF(value);
                Py_DECREF(ev);
            } else if (et == PyExc_StopIteration) {
                /* avoid normalisation and take ev as value */
                value = ev;
            } else {
                /* normalisation required */
                PyErr_NormalizeException(&et, &ev, &tb);
                if (!PyObject_TypeCheck(ev, (PyTypeObject *)PyExc_StopIteration)) {
                    PyErr_Restore(et, ev, tb);
                    return -1;
                }
                value = ((PyStopIterationObject *)ev)->value;
                Py_INCREF(value);
                Py_DECREF(ev);
            }
        }
        Py_XDECREF(et);
        Py_XDECREF(tb);
    } else if (PyErr_Occurred()) {
        return -1;
    }
    if (value == NULL) {
        value = Py_None;
        Py_INCREF(value);
    }
    *pvalue = value;
    return 0;
}

static PyObject *
gen_repr(PyGenObject *gen)
{
    return PyUnicode_FromFormat("<generator object %S at %p>",
                                gen->gi_qualname, gen);
}

static PyObject *
gen_get_name(PyGenObject *op)
{
    Py_INCREF(op->gi_name);
    return op->gi_name;
}

static int
gen_set_name(PyGenObject *op, PyObject *value)
{
    PyObject *tmp;

    /* Not legal to del gen.gi_name or to set it to anything
     * other than a string object. */
    if (value == NULL || !PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "__name__ must be set to a string object");
        return -1;
    }
    tmp = op->gi_name;
    Py_INCREF(value);
    op->gi_name = value;
    Py_DECREF(tmp);
    return 0;
}

static PyObject *
gen_get_qualname(PyGenObject *op)
{
    Py_INCREF(op->gi_qualname);
    return op->gi_qualname;
}

static int
gen_set_qualname(PyGenObject *op, PyObject *value)
{
    PyObject *tmp;

    /* Not legal to del gen.__qualname__ or to set it to anything
     * other than a string object. */
    if (value == NULL || !PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "__qualname__ must be set to a string object");
        return -1;
    }
    tmp = op->gi_qualname;
    Py_INCREF(value);
    op->gi_qualname = value;
    Py_DECREF(tmp);
    return 0;
}

static PyObject *
gen_getyieldfrom(PyGenObject *gen)
{
    PyObject *yf = gen_yf(gen);
    if (yf == NULL)
        Py_RETURN_NONE;
    return yf;
}

static PyGetSetDef gen_getsetlist[] = {
    {"__name__", (getter)gen_get_name, (setter)gen_set_name,
     PyDoc_STR("name of the generator")},
    {"__qualname__", (getter)gen_get_qualname, (setter)gen_set_qualname,
     PyDoc_STR("qualified name of the generator")},
    {"gi_yieldfrom", (getter)gen_getyieldfrom, NULL,
     PyDoc_STR("object being iterated by yield from, or None")},
    {NULL} /* Sentinel */
};

static PyMemberDef gen_memberlist[] = {
    {"gi_frame",     T_OBJECT, offsetof(PyGenObject, gi_frame),    READONLY},
    {"gi_running",   T_BOOL,   offsetof(PyGenObject, gi_running),  READONLY},
    {"gi_code",      T_OBJECT, offsetof(PyGenObject, gi_code),     READONLY},
    {NULL}      /* Sentinel */
};

static PyMethodDef gen_methods[] = {
    {"send",(PyCFunction)_PyGen_Send, METH_O, send_doc},
    {"throw",(PyCFunction)gen_throw, METH_VARARGS, throw_doc},
    {"close",(PyCFunction)gen_close, METH_NOARGS, close_doc},
    {NULL, NULL}        /* Sentinel */
};

PyTypeObject PyGen_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "generator",                                /* tp_name */
    sizeof(PyGenObject),                        /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)gen_dealloc,                    /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)gen_repr,                         /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_HAVE_FINALIZE,               /* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)gen_traverse,                 /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    offsetof(PyGenObject, gi_weakreflist),      /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)gen_iternext,                 /* tp_iternext */
    gen_methods,                                /* tp_methods */
    gen_memberlist,                             /* tp_members */
    gen_getsetlist,                             /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */

    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    0,                                          /* tp_new */
    0,                                          /* tp_free */
    0,                                          /* tp_is_gc */
    0,                                          /* tp_bases */
    0,                                          /* tp_mro */
    0,                                          /* tp_cache */
    0,                                          /* tp_subclasses */
    0,                                          /* tp_weaklist */
    0,                                          /* tp_del */
    0,                                          /* tp_version_tag */
    _PyGen_Finalize,                            /* tp_finalize */
};

static PyObject *
gen_new_with_qualname(PyTypeObject *type, PyFrameObject *f,
                      PyObject *name, PyObject *qualname)
{
    PyGenObject *gen = PyObject_GC_New(PyGenObject, type);
    if (gen == NULL) {
        Py_DECREF(f);
        return NULL;
    }
    gen->gi_frame = f;
    f->f_gen = (PyObject *) gen;
    Py_INCREF(f->f_code);
    gen->gi_code = (PyObject *)(f->f_code);
    gen->gi_running = 0;
    gen->gi_weakreflist = NULL;
    if (name != NULL)
        gen->gi_name = name;
    else
        gen->gi_name = ((PyCodeObject *)gen->gi_code)->co_name;
    Py_INCREF(gen->gi_name);
    if (qualname != NULL)
        gen->gi_qualname = qualname;
    else
        gen->gi_qualname = gen->gi_name;
    Py_INCREF(gen->gi_qualname);
    _PyObject_GC_TRACK(gen);
    return (PyObject *)gen;
}

PyObject *
PyGen_NewWithQualName(PyFrameObject *f, PyObject *name, PyObject *qualname)
{
    return gen_new_with_qualname(&PyGen_Type, f, name, qualname);
}

PyObject *
PyGen_New(PyFrameObject *f)
{
    return gen_new_with_qualname(&PyGen_Type, f, NULL, NULL);
}

int
PyGen_NeedsFinalizing(PyGenObject *gen)
{
    int i;
    PyFrameObject *f = gen->gi_frame;

    if (f == NULL || f->f_stacktop == NULL)
        return 0; /* no frame or empty blockstack == no finalization */

    /* Any block type besides a loop requires cleanup. */
    for (i = 0; i < f->f_iblock; i++)
        if (f->f_blockstack[i].b_type != SETUP_LOOP)
            return 1;

    /* No blocks except loops, it's safe to skip finalization. */
    return 0;
}

/* Coroutine Object */

typedef struct {
    PyObject_HEAD
    PyCoroObject *cw_coroutine;
} PyCoroWrapper;

static int
gen_is_coroutine(PyObject *o)
{
    if (PyGen_CheckExact(o)) {
        PyCodeObject *code = (PyCodeObject *)((PyGenObject*)o)->gi_code;
        if (code->co_flags & CO_ITERABLE_COROUTINE) {
            return 1;
        }
    }
    return 0;
}

/*
 *   This helper function returns an awaitable for `o`:
 *     - `o` if `o` is a coroutine-object;
 *     - `type(o)->tp_as_async->am_await(o)`
 *
 *   Raises a TypeError if it's not possible to return
 *   an awaitable and returns NULL.
 */
PyObject *
_PyCoro_GetAwaitableIter(PyObject *o)
{
    unaryfunc getter = NULL;
    PyTypeObject *ot;

    if (PyCoro_CheckExact(o) || gen_is_coroutine(o)) {
        /* 'o' is a coroutine. */
        Py_INCREF(o);
        return o;
    }

    ot = Py_TYPE(o);
    if (ot->tp_as_async != NULL) {
        getter = ot->tp_as_async->am_await;
    }
    if (getter != NULL) {
        PyObject *res = (*getter)(o);
        if (res != NULL) {
            if (PyCoro_CheckExact(res) || gen_is_coroutine(res)) {
                /* __await__ must return an *iterator*, not
                   a coroutine or another awaitable (see PEP 492) */
                PyErr_SetString(PyExc_TypeError,
                                "__await__() returned a coroutine");
                Py_CLEAR(res);
            } else if (!PyIter_Check(res)) {
                PyErr_Format(PyExc_TypeError,
                             "__await__() returned non-iterator "
                             "of type '%.100s'",
                             Py_TYPE(res)->tp_name);
                Py_CLEAR(res);
            }
        }
        return res;
    }

    PyErr_Format(PyExc_TypeError,
                 "object %.100s can't be used in 'await' expression",
                 ot->tp_name);
    return NULL;
}

static PyObject *
coro_repr(PyCoroObject *coro)
{
    return PyUnicode_FromFormat("<coroutine object %S at %p>",
                                coro->cr_qualname, coro);
}

static PyObject *
coro_await(PyCoroObject *coro)
{
    PyCoroWrapper *cw = PyObject_GC_New(PyCoroWrapper, &_PyCoroWrapper_Type);
    if (cw == NULL) {
        return NULL;
    }
    Py_INCREF(coro);
    cw->cw_coroutine = coro;
    _PyObject_GC_TRACK(cw);
    return (PyObject *)cw;
}

static PyObject *
coro_get_cr_await(PyCoroObject *coro)
{
    PyObject *yf = gen_yf((PyGenObject *) coro);
    if (yf == NULL)
        Py_RETURN_NONE;
    return yf;
}

static PyGetSetDef coro_getsetlist[] = {
    {"__name__", (getter)gen_get_name, (setter)gen_set_name,
     PyDoc_STR("name of the coroutine")},
    {"__qualname__", (getter)gen_get_qualname, (setter)gen_set_qualname,
     PyDoc_STR("qualified name of the coroutine")},
    {"cr_await", (getter)coro_get_cr_await, NULL,
     PyDoc_STR("object being awaited on, or None")},
    {NULL} /* Sentinel */
};

static PyMemberDef coro_memberlist[] = {
    {"cr_frame",     T_OBJECT, offsetof(PyCoroObject, cr_frame),    READONLY},
    {"cr_running",   T_BOOL,   offsetof(PyCoroObject, cr_running),  READONLY},
    {"cr_code",      T_OBJECT, offsetof(PyCoroObject, cr_code),     READONLY},
    {NULL}      /* Sentinel */
};

PyDoc_STRVAR(coro_send_doc,
"send(arg) -> send 'arg' into coroutine,\n\
return next iterated value or raise StopIteration.");

PyDoc_STRVAR(coro_throw_doc,
"throw(typ[,val[,tb]]) -> raise exception in coroutine,\n\
return next iterated value or raise StopIteration.");

PyDoc_STRVAR(coro_close_doc,
"close() -> raise GeneratorExit inside coroutine.");

static PyMethodDef coro_methods[] = {
    {"send",(PyCFunction)_PyGen_Send, METH_O, coro_send_doc},
    {"throw",(PyCFunction)gen_throw, METH_VARARGS, coro_throw_doc},
    {"close",(PyCFunction)gen_close, METH_NOARGS, coro_close_doc},
    {NULL, NULL}        /* Sentinel */
};

static PyAsyncMethods coro_as_async = {
    (unaryfunc)coro_await,                      /* am_await */
    0,                                          /* am_aiter */
    0                                           /* am_anext */
};

PyTypeObject PyCoro_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "coroutine",                                /* tp_name */
    sizeof(PyCoroObject),                       /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)gen_dealloc,                    /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    &coro_as_async,                             /* tp_as_async */
    (reprfunc)coro_repr,                        /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_HAVE_FINALIZE,               /* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)gen_traverse,                 /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    offsetof(PyCoroObject, cr_weakreflist),     /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    coro_methods,                               /* tp_methods */
    coro_memberlist,                            /* tp_members */
    coro_getsetlist,                            /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    0,                                          /* tp_new */
    0,                                          /* tp_free */
    0,                                          /* tp_is_gc */
    0,                                          /* tp_bases */
    0,                                          /* tp_mro */
    0,                                          /* tp_cache */
    0,                                          /* tp_subclasses */
    0,                                          /* tp_weaklist */
    0,                                          /* tp_del */
    0,                                          /* tp_version_tag */
    _PyGen_Finalize,                            /* tp_finalize */
};

static void
coro_wrapper_dealloc(PyCoroWrapper *cw)
{
    _PyObject_GC_UNTRACK((PyObject *)cw);
    Py_CLEAR(cw->cw_coroutine);
    PyObject_GC_Del(cw);
}

static PyObject *
coro_wrapper_iternext(PyCoroWrapper *cw)
{
    return gen_send_ex((PyGenObject *)cw->cw_coroutine, NULL, 0);
}

static PyObject *
coro_wrapper_send(PyCoroWrapper *cw, PyObject *arg)
{
    return gen_send_ex((PyGenObject *)cw->cw_coroutine, arg, 0);
}

static PyObject *
coro_wrapper_throw(PyCoroWrapper *cw, PyObject *args)
{
    return gen_throw((PyGenObject *)cw->cw_coroutine, args);
}

static PyObject *
coro_wrapper_close(PyCoroWrapper *cw, PyObject *args)
{
    return gen_close((PyGenObject *)cw->cw_coroutine, args);
}

static int
coro_wrapper_traverse(PyCoroWrapper *cw, visitproc visit, void *arg)
{
    Py_VISIT((PyObject *)cw->cw_coroutine);
    return 0;
}

static PyMethodDef coro_wrapper_methods[] = {
    {"send",(PyCFunction)coro_wrapper_send, METH_O, coro_send_doc},
    {"throw",(PyCFunction)coro_wrapper_throw, METH_VARARGS, coro_throw_doc},
    {"close",(PyCFunction)coro_wrapper_close, METH_NOARGS, coro_close_doc},
    {NULL, NULL}        /* Sentinel */
};

PyTypeObject _PyCoroWrapper_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "coroutine_wrapper",
    sizeof(PyCoroWrapper),                      /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)coro_wrapper_dealloc,           /* destructor tp_dealloc */
    0,                                          /* tp_print */
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
    "A wrapper object implementing __await__ for coroutines.",
    (traverseproc)coro_wrapper_traverse,        /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)coro_wrapper_iternext,        /* tp_iternext */
    coro_wrapper_methods,                       /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    0,                                          /* tp_new */
    PyObject_Del,                               /* tp_free */
};

PyObject *
PyCoro_New(PyFrameObject *f, PyObject *name, PyObject *qualname)
{
    return gen_new_with_qualname(&PyCoro_Type, f, name, qualname);
}
