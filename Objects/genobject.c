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

static void
gen_dealloc(PyGenObject *gen)
{
    PyObject *self = (PyObject *) gen;
    PyFrameObject *f = gen->gi_frame;

    _PyObject_GC_UNTRACK(gen);

    if (gen->gi_weakreflist != NULL)
        PyObject_ClearWeakRefs(self);

    gen->gi_frame = NULL;
    if (f) {
        /* Close the generator by finalizing the frame */
        PyObject *retval, *t, *v, *tb;
        PyErr_Fetch(&t, &v, &tb);
        f->f_gen = NULL;
        retval = _PyFrame_Finalize(f);
        if (retval)
            Py_DECREF(retval);
        else if (PyErr_Occurred())
            PyErr_WriteUnraisable((PyObject *) gen);
        Py_DECREF(f);
        PyErr_Restore(t, v, tb);
    }
    Py_CLEAR(gen->gi_code);
    PyObject_GC_Del(gen);
}

static PyObject *
gen_send_ex(PyGenObject *gen, PyObject *arg, int exc)
{
    PyFrameObject *f = gen->gi_frame;

    /* For compatibility, we check gi_running before f == NULL */
    if (gen->gi_running) {
        PyErr_SetString(PyExc_ValueError,
                        "generator already executing");
        return NULL;
    }
    if (f == NULL) {
        /* Only set exception if send() called, not throw() or next() */
        if (arg && !exc)
            PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }

    return _PyFrame_GeneratorSend(f, arg, exc);
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

static PyObject *
gen_yf(PyGenObject *gen)
{
    PyFrameObject *f = gen->gi_frame;
    if (f)
        return _PyFrame_YieldingFrom(f);
    else
        return NULL;
}

static PyObject *
gen_close(PyGenObject *gen, PyObject *args)
{
    PyFrameObject *f = gen->gi_frame;

    /* For compatibility, we check gi_running before f == NULL */
    if (gen->gi_running) {
        PyErr_SetString(PyExc_ValueError,
                        "generator already executing");
        return NULL;
    }
    if (f == NULL)
        Py_RETURN_NONE;

    return _PyFrame_Finalize(f);
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
            err = _PyFrame_CloseIterator(yf);
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
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
    return 0;
}
