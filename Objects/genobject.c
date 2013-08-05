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
    return 0;
}

void
_PyGen_Finalize(PyObject *self)
{
    PyGenObject *gen = (PyGenObject *)self;
    PyObject *res;
    PyObject *error_type, *error_value, *error_traceback;

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
    PyObject_GC_Del(gen);
}

static PyObject *
gen_send_ex(PyGenObject *gen, PyObject *arg, int exc)
{
    PyThreadState *tstate = PyThreadState_GET();
    PyFrameObject *f = gen->gi_frame;
    PyObject *result;

    if (gen->gi_running) {
        PyErr_SetString(PyExc_ValueError,
                        "generator already executing");
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
            PyErr_SetString(PyExc_TypeError,
                            "can't send non-None value to a "
                            "just-started generator");
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
        Py_DECREF(retval);
        PyErr_SetString(PyExc_RuntimeError,
                        "generator ignored GeneratorExit");
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
    PyObject *val = NULL;
    PyObject *ret;
    ret = gen_send_ex(gen, val, 0);
    Py_XDECREF(val);
    return ret;
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
        Py_XDECREF(et);
        Py_XDECREF(tb);
        if (ev) {
            value = ((PyStopIterationObject *)ev)->value;
            Py_INCREF(value);
            Py_DECREF(ev);
        }
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
                                ((PyCodeObject *)gen->gi_code)->co_name,
                                gen);
}


static PyObject *
gen_get_name(PyGenObject *gen)
{
    PyObject *name = ((PyCodeObject *)gen->gi_code)->co_name;
    Py_INCREF(name);
    return name;
}


PyDoc_STRVAR(gen__name__doc__,
"Return the name of the generator's associated code object.");

static PyGetSetDef gen_getsetlist[] = {
    {"__name__", (getter)gen_get_name, NULL, gen__name__doc__},
    {NULL}
};


static PyMemberDef gen_memberlist[] = {
    {"gi_frame",        T_OBJECT, offsetof(PyGenObject, gi_frame),      READONLY},
    {"gi_running",      T_BOOL,    offsetof(PyGenObject, gi_running),    READONLY},
    {"gi_code",     T_OBJECT, offsetof(PyGenObject, gi_code),  READONLY},
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
    0,                                          /* tp_reserved */
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

PyObject *
PyGen_New(PyFrameObject *f)
{
    PyGenObject *gen = PyObject_GC_New(PyGenObject, &PyGen_Type);
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
    _PyObject_GC_TRACK(gen);
    return (PyObject *)gen;
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
