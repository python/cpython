/* Generator object implementation */

#include "Python.h"
#include "pycore_ceval.h"         // _PyEval_EvalFrame()
#include "pycore_object.h"
#include "pycore_pyerrors.h"      // _PyErr_ClearExcState()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "frameobject.h"
#include "structmember.h"         // PyMemberDef
#include "opcode.h"

static PyObject *gen_close(PyGenObject *, PyObject *);
static PyObject *async_gen_asend_new(PyAsyncGenObject *, PyObject *);
static PyObject *async_gen_athrow_new(PyAsyncGenObject *, PyObject *);

static const char *NON_INIT_CORO_MSG = "can't send non-None value to a "
                                 "just-started coroutine";

static const char *ASYNC_GEN_IGNORED_EXIT_MSG =
                                 "async generator ignored GeneratorExit";

static inline int
exc_state_traverse(_PyErr_StackItem *exc_state, visitproc visit, void *arg)
{
    Py_VISIT(exc_state->exc_type);
    Py_VISIT(exc_state->exc_value);
    Py_VISIT(exc_state->exc_traceback);
    return 0;
}

static int
gen_traverse(PyGenObject *gen, visitproc visit, void *arg)
{
    Py_VISIT((PyObject *)gen->gi_frame);
    Py_VISIT(gen->gi_code);
    Py_VISIT(gen->gi_name);
    Py_VISIT(gen->gi_qualname);
    /* No need to visit cr_origin, because it's just tuples/str/int, so can't
       participate in a reference cycle. */
    return exc_state_traverse(&gen->gi_exc_state, visit, arg);
}

void
_PyGen_Finalize(PyObject *self)
{
    PyGenObject *gen = (PyGenObject *)self;
    PyObject *res = NULL;
    PyObject *error_type, *error_value, *error_traceback;

    if (gen->gi_frame == NULL ||  _PyFrameHasCompleted(gen->gi_frame)) {
        /* Generator isn't paused, so no need to close */
        return;
    }

    if (PyAsyncGen_CheckExact(self)) {
        PyAsyncGenObject *agen = (PyAsyncGenObject*)self;
        PyObject *finalizer = agen->ag_finalizer;
        if (finalizer && !agen->ag_closed) {
            /* Save the current exception, if any. */
            PyErr_Fetch(&error_type, &error_value, &error_traceback);

            res = PyObject_CallOneArg(finalizer, self);

            if (res == NULL) {
                PyErr_WriteUnraisable(self);
            } else {
                Py_DECREF(res);
            }
            /* Restore the saved exception. */
            PyErr_Restore(error_type, error_value, error_traceback);
            return;
        }
    }

    /* Save the current exception, if any. */
    PyErr_Fetch(&error_type, &error_value, &error_traceback);

    /* If `gen` is a coroutine, and if it was never awaited on,
       issue a RuntimeWarning. */
    if (gen->gi_code != NULL &&
        ((PyCodeObject *)gen->gi_code)->co_flags & CO_COROUTINE &&
        gen->gi_frame->f_lasti == -1)
    {
        _PyErr_WarnUnawaitedCoroutine((PyObject *)gen);
    }
    else {
        res = gen_close(gen, NULL);
    }

    if (res == NULL) {
        if (PyErr_Occurred()) {
            PyErr_WriteUnraisable(self);
        }
    }
    else {
        Py_DECREF(res);
    }

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
    if (PyAsyncGen_CheckExact(gen)) {
        /* We have to handle this case for asynchronous generators
           right here, because this code has to be between UNTRACK
           and GC_Del. */
        Py_CLEAR(((PyAsyncGenObject*)gen)->ag_finalizer);
    }
    if (gen->gi_frame != NULL) {
        gen->gi_frame->f_gen = NULL;
        Py_CLEAR(gen->gi_frame);
    }
    if (((PyCodeObject *)gen->gi_code)->co_flags & CO_COROUTINE) {
        Py_CLEAR(((PyCoroObject *)gen)->cr_origin);
    }
    Py_CLEAR(gen->gi_code);
    Py_CLEAR(gen->gi_name);
    Py_CLEAR(gen->gi_qualname);
    _PyErr_ClearExcState(&gen->gi_exc_state);
    PyObject_GC_Del(gen);
}

static PySendResult
gen_send_ex2(PyGenObject *gen, PyObject *arg, PyObject **presult,
             int exc, int closing)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyFrameObject *f = gen->gi_frame;
    PyObject *result;

    *presult = NULL;
    if (f != NULL && f->f_lasti < 0 && arg && arg != Py_None) {
        const char *msg = "can't send non-None value to a "
                            "just-started generator";
        if (PyCoro_CheckExact(gen)) {
            msg = NON_INIT_CORO_MSG;
        }
        else if (PyAsyncGen_CheckExact(gen)) {
            msg = "can't send non-None value to a "
                    "just-started async generator";
        }
        PyErr_SetString(PyExc_TypeError, msg);
        return PYGEN_ERROR;
    }
    if (f != NULL && _PyFrame_IsExecuting(f)) {
        const char *msg = "generator already executing";
        if (PyCoro_CheckExact(gen)) {
            msg = "coroutine already executing";
        }
        else if (PyAsyncGen_CheckExact(gen)) {
            msg = "async generator already executing";
        }
        PyErr_SetString(PyExc_ValueError, msg);
        return PYGEN_ERROR;
    }
    if (f == NULL || _PyFrameHasCompleted(f)) {
        if (PyCoro_CheckExact(gen) && !closing) {
            /* `gen` is an exhausted coroutine: raise an error,
               except when called from gen_close(), which should
               always be a silent method. */
            PyErr_SetString(
                PyExc_RuntimeError,
                "cannot reuse already awaited coroutine");
        }
        else if (arg && !exc) {
            /* `gen` is an exhausted generator:
               only return value if called from send(). */
            *presult = Py_None;
            Py_INCREF(*presult);
            return PYGEN_RETURN;
        }
        return PYGEN_ERROR;
    }

    assert(_PyFrame_IsRunnable(f));
    assert(f->f_lasti >= 0 || ((unsigned char *)PyBytes_AS_STRING(f->f_code->co_code))[0] == GEN_START);
    /* Push arg onto the frame's value stack */
    result = arg ? arg : Py_None;
    Py_INCREF(result);
    gen->gi_frame->f_valuestack[gen->gi_frame->f_stackdepth] = result;
    gen->gi_frame->f_stackdepth++;

    /* Generators always return to their most recent caller, not
     * necessarily their creator. */
    Py_XINCREF(tstate->frame);
    assert(f->f_back == NULL);
    f->f_back = tstate->frame;

    gen->gi_exc_state.previous_item = tstate->exc_info;
    tstate->exc_info = &gen->gi_exc_state;

    if (exc) {
        assert(_PyErr_Occurred(tstate));
        _PyErr_ChainStackItem(NULL);
    }

    result = _PyEval_EvalFrame(tstate, f, exc);
    tstate->exc_info = gen->gi_exc_state.previous_item;
    gen->gi_exc_state.previous_item = NULL;

    /* Don't keep the reference to f_back any longer than necessary.  It
     * may keep a chain of frames alive or it could create a reference
     * cycle. */
    assert(f->f_back == tstate->frame);
    Py_CLEAR(f->f_back);

    /* If the generator just returned (as opposed to yielding), signal
     * that the generator is exhausted. */
    if (result) {
        if (!_PyFrameHasCompleted(f)) {
            *presult = result;
            return PYGEN_NEXT;
        }
        assert(result == Py_None || !PyAsyncGen_CheckExact(gen));
        if (result == Py_None && !PyAsyncGen_CheckExact(gen) && !arg) {
            /* Return NULL if called by gen_iternext() */
            Py_CLEAR(result);
        }
    }
    else {
        if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
            const char *msg = "generator raised StopIteration";
            if (PyCoro_CheckExact(gen)) {
                msg = "coroutine raised StopIteration";
            }
            else if (PyAsyncGen_CheckExact(gen)) {
                msg = "async generator raised StopIteration";
            }
            _PyErr_FormatFromCause(PyExc_RuntimeError, "%s", msg);
        }
        else if (PyAsyncGen_CheckExact(gen) &&
                PyErr_ExceptionMatches(PyExc_StopAsyncIteration))
        {
            /* code in `gen` raised a StopAsyncIteration error:
               raise a RuntimeError.
            */
            const char *msg = "async generator raised StopAsyncIteration";
            _PyErr_FormatFromCause(PyExc_RuntimeError, "%s", msg);
        }
    }

    /* generator can't be rerun, so release the frame */
    /* first clean reference cycle through stored exception traceback */
    _PyErr_ClearExcState(&gen->gi_exc_state);
    gen->gi_frame->f_gen = NULL;
    gen->gi_frame = NULL;
    Py_DECREF(f);

    *presult = result;
    return result ? PYGEN_RETURN : PYGEN_ERROR;
}

static PySendResult
PyGen_am_send(PyGenObject *gen, PyObject *arg, PyObject **result)
{
    return gen_send_ex2(gen, arg, result, 0, 0);
}

static PyObject *
gen_send_ex(PyGenObject *gen, PyObject *arg, int exc, int closing)
{
    PyObject *result;
    if (gen_send_ex2(gen, arg, &result, exc, closing) == PYGEN_RETURN) {
        if (PyAsyncGen_CheckExact(gen)) {
            assert(result == Py_None);
            PyErr_SetNone(PyExc_StopAsyncIteration);
        }
        else if (result == Py_None) {
            PyErr_SetNone(PyExc_StopIteration);
        }
        else {
            _PyGen_SetStopIterationValue(result);
        }
        Py_CLEAR(result);
    }
    return result;
}

PyDoc_STRVAR(send_doc,
"send(arg) -> send 'arg' into generator,\n\
return next yielded value or raise StopIteration.");

static PyObject *
gen_send(PyGenObject *gen, PyObject *arg)
{
    return gen_send_ex(gen, arg, 0, 0);
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

    if (PyGen_CheckExact(yf) || PyCoro_CheckExact(yf)) {
        retval = gen_close((PyGenObject *)yf, NULL);
        if (retval == NULL)
            return -1;
    }
    else {
        PyObject *meth;
        if (_PyObject_LookupAttrId(yf, &PyId_close, &meth) < 0) {
            PyErr_WriteUnraisable(yf);
        }
        if (meth) {
            retval = _PyObject_CallNoArg(meth);
            Py_DECREF(meth);
            if (retval == NULL)
                return -1;
        }
    }
    Py_XDECREF(retval);
    return 0;
}

PyObject *
_PyGen_yf(PyGenObject *gen)
{
    PyObject *yf = NULL;
    PyFrameObject *f = gen->gi_frame;

    if (f) {
        PyObject *bytecode = f->f_code->co_code;
        unsigned char *code = (unsigned char *)PyBytes_AS_STRING(bytecode);

        if (f->f_lasti < 0) {
            /* Return immediately if the frame didn't start yet. YIELD_FROM
               always come after LOAD_CONST: a code object should not start
               with YIELD_FROM */
            assert(code[0] != YIELD_FROM);
            return NULL;
        }

        if (code[(f->f_lasti+1)*sizeof(_Py_CODEUNIT)] != YIELD_FROM)
            return NULL;
        assert(f->f_stackdepth > 0);
        yf = f->f_valuestack[f->f_stackdepth-1];
        Py_INCREF(yf);
    }

    return yf;
}

static PyObject *
gen_close(PyGenObject *gen, PyObject *args)
{
    PyObject *retval;
    PyObject *yf = _PyGen_yf(gen);
    int err = 0;

    if (yf) {
        PyFrameState state = gen->gi_frame->f_state;
        gen->gi_frame->f_state = FRAME_EXECUTING;
        err = gen_close_iter(yf);
        gen->gi_frame->f_state = state;
        Py_DECREF(yf);
    }
    if (err == 0)
        PyErr_SetNone(PyExc_GeneratorExit);
    retval = gen_send_ex(gen, Py_None, 1, 1);
    if (retval) {
        const char *msg = "generator ignored GeneratorExit";
        if (PyCoro_CheckExact(gen)) {
            msg = "coroutine ignored GeneratorExit";
        } else if (PyAsyncGen_CheckExact(gen)) {
            msg = ASYNC_GEN_IGNORED_EXIT_MSG;
        }
        Py_DECREF(retval);
        PyErr_SetString(PyExc_RuntimeError, msg);
        return NULL;
    }
    if (PyErr_ExceptionMatches(PyExc_StopIteration)
        || PyErr_ExceptionMatches(PyExc_GeneratorExit)) {
        PyErr_Clear();          /* ignore these errors */
        Py_RETURN_NONE;
    }
    return NULL;
}


PyDoc_STRVAR(throw_doc,
"throw(value)\n\
throw(type[,value[,tb]])\n\
\n\
Raise exception in generator, return next yielded value or raise\n\
StopIteration.");

static PyObject *
_gen_throw(PyGenObject *gen, int close_on_genexit,
           PyObject *typ, PyObject *val, PyObject *tb)
{
    PyObject *yf = _PyGen_yf(gen);
    _Py_IDENTIFIER(throw);

    if (yf) {
        PyObject *ret;
        int err;
        if (PyErr_GivenExceptionMatches(typ, PyExc_GeneratorExit) &&
            close_on_genexit
        ) {
            /* Asynchronous generators *should not* be closed right away.
               We have to allow some awaits to work it through, hence the
               `close_on_genexit` parameter here.
            */
            PyFrameState state = gen->gi_frame->f_state;
            gen->gi_frame->f_state = FRAME_EXECUTING;
            err = gen_close_iter(yf);
            gen->gi_frame->f_state = state;
            Py_DECREF(yf);
            if (err < 0)
                return gen_send_ex(gen, Py_None, 1, 0);
            goto throw_here;
        }
        if (PyGen_CheckExact(yf) || PyCoro_CheckExact(yf)) {
            /* `yf` is a generator or a coroutine. */
            PyThreadState *tstate = _PyThreadState_GET();
            PyFrameObject *f = tstate->frame;
            PyFrameObject *gf = gen->gi_frame;

            /* Since we are fast-tracking things by skipping the eval loop,
               we need to update the current frame so the stack trace
               will be reported correctly to the user. */
            /* XXX We should probably be updating the current frame
               somewhere in ceval.c. */
            assert(gf->f_back == NULL);
            Py_XINCREF(f);
            gf->f_back = f;
            tstate->frame = gf;
            /* Close the generator that we are currently iterating with
               'yield from' or awaiting on with 'await'. */
            PyFrameState state = gf->f_state;
            gf->f_state = FRAME_EXECUTING;
            ret = _gen_throw((PyGenObject *)yf, close_on_genexit,
                             typ, val, tb);
            gf->f_state = state;
            Py_CLEAR(gf->f_back);
            tstate->frame = f;
        } else {
            /* `yf` is an iterator or a coroutine-like object. */
            PyObject *meth;
            if (_PyObject_LookupAttrId(yf, &PyId_throw, &meth) < 0) {
                Py_DECREF(yf);
                return NULL;
            }
            if (meth == NULL) {
                Py_DECREF(yf);
                goto throw_here;
            }
            PyFrameState state = gen->gi_frame->f_state;
            gen->gi_frame->f_state = FRAME_EXECUTING;
            ret = PyObject_CallFunctionObjArgs(meth, typ, val, tb, NULL);
            gen->gi_frame->f_state = state;
            Py_DECREF(meth);
        }
        Py_DECREF(yf);
        if (!ret) {
            PyObject *val;
            /* Pop subiterator from stack */
            assert(gen->gi_frame->f_stackdepth > 0);
            gen->gi_frame->f_stackdepth--;
            ret = gen->gi_frame->f_valuestack[gen->gi_frame->f_stackdepth];
            assert(ret == yf);
            Py_DECREF(ret);
            /* Termination repetition of YIELD_FROM */
            assert(gen->gi_frame->f_lasti >= 0);
            gen->gi_frame->f_lasti += 1;
            if (_PyGen_FetchStopIterationValue(&val) == 0) {
                ret = gen_send(gen, val);
                Py_DECREF(val);
            } else {
                ret = gen_send_ex(gen, Py_None, 1, 0);
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
    return gen_send_ex(gen, Py_None, 1, 0);

failed_throw:
    /* Didn't use our arguments, so restore their original refcounts */
    Py_DECREF(typ);
    Py_XDECREF(val);
    Py_XDECREF(tb);
    return NULL;
}


static PyObject *
gen_throw(PyGenObject *gen, PyObject *args)
{
    PyObject *typ;
    PyObject *tb = NULL;
    PyObject *val = NULL;

    if (!PyArg_UnpackTuple(args, "throw", 1, 3, &typ, &val, &tb)) {
        return NULL;
    }

    return _gen_throw(gen, 1, typ, val, tb);
}


static PyObject *
gen_iternext(PyGenObject *gen)
{
    PyObject *result;
    assert(PyGen_CheckExact(gen) || PyCoro_CheckExact(gen));
    if (gen_send_ex2(gen, NULL, &result, 0, 0) == PYGEN_RETURN) {
        if (result != Py_None) {
            _PyGen_SetStopIterationValue(result);
        }
        Py_CLEAR(result);
    }
    return result;
}

/*
 * Set StopIteration with specified value.  Value can be arbitrary object
 * or NULL.
 *
 * Returns 0 if StopIteration is set and -1 if any other exception is set.
 */
int
_PyGen_SetStopIterationValue(PyObject *value)
{
    PyObject *e;

    if (value == NULL ||
        (!PyTuple_Check(value) && !PyExceptionInstance_Check(value)))
    {
        /* Delay exception instantiation if we can */
        PyErr_SetObject(PyExc_StopIteration, value);
        return 0;
    }
    /* Construct an exception instance manually with
     * PyObject_CallOneArg and pass it to PyErr_SetObject.
     *
     * We do this to handle a situation when "value" is a tuple, in which
     * case PyErr_SetObject would set the value of StopIteration to
     * the first element of the tuple.
     *
     * (See PyErr_SetObject/_PyErr_CreateException code for details.)
     */
    e = PyObject_CallOneArg(PyExc_StopIteration, value);
    if (e == NULL) {
        return -1;
    }
    PyErr_SetObject(PyExc_StopIteration, e);
    Py_DECREF(e);
    return 0;
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
_PyGen_FetchStopIterationValue(PyObject **pvalue)
{
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
            } else if (et == PyExc_StopIteration && !PyTuple_Check(ev)) {
                /* Avoid normalisation and take ev as value.
                 *
                 * Normalization is required if the value is a tuple, in
                 * that case the value of StopIteration would be set to
                 * the first element of the tuple.
                 *
                 * (See _PyErr_CreateException code for details.)
                 */
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
gen_get_name(PyGenObject *op, void *Py_UNUSED(ignored))
{
    Py_INCREF(op->gi_name);
    return op->gi_name;
}

static int
gen_set_name(PyGenObject *op, PyObject *value, void *Py_UNUSED(ignored))
{
    /* Not legal to del gen.gi_name or to set it to anything
     * other than a string object. */
    if (value == NULL || !PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "__name__ must be set to a string object");
        return -1;
    }
    Py_INCREF(value);
    Py_XSETREF(op->gi_name, value);
    return 0;
}

static PyObject *
gen_get_qualname(PyGenObject *op, void *Py_UNUSED(ignored))
{
    Py_INCREF(op->gi_qualname);
    return op->gi_qualname;
}

static int
gen_set_qualname(PyGenObject *op, PyObject *value, void *Py_UNUSED(ignored))
{
    /* Not legal to del gen.__qualname__ or to set it to anything
     * other than a string object. */
    if (value == NULL || !PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "__qualname__ must be set to a string object");
        return -1;
    }
    Py_INCREF(value);
    Py_XSETREF(op->gi_qualname, value);
    return 0;
}

static PyObject *
gen_getyieldfrom(PyGenObject *gen, void *Py_UNUSED(ignored))
{
    PyObject *yf = _PyGen_yf(gen);
    if (yf == NULL)
        Py_RETURN_NONE;
    return yf;
}


static PyObject *
gen_getrunning(PyGenObject *gen, void *Py_UNUSED(ignored))
{
    if (gen->gi_frame == NULL) {
        Py_RETURN_FALSE;
    }
    return PyBool_FromLong(_PyFrame_IsExecuting(gen->gi_frame));
}

static PyGetSetDef gen_getsetlist[] = {
    {"__name__", (getter)gen_get_name, (setter)gen_set_name,
     PyDoc_STR("name of the generator")},
    {"__qualname__", (getter)gen_get_qualname, (setter)gen_set_qualname,
     PyDoc_STR("qualified name of the generator")},
    {"gi_yieldfrom", (getter)gen_getyieldfrom, NULL,
     PyDoc_STR("object being iterated by yield from, or None")},
    {"gi_running", (getter)gen_getrunning, NULL, NULL},
    {NULL} /* Sentinel */
};

static PyMemberDef gen_memberlist[] = {
    {"gi_frame",     T_OBJECT, offsetof(PyGenObject, gi_frame),    READONLY|PY_AUDIT_READ},
    {"gi_code",      T_OBJECT, offsetof(PyGenObject, gi_code),     READONLY|PY_AUDIT_READ},
    {NULL}      /* Sentinel */
};

static PyMethodDef gen_methods[] = {
    {"send",(PyCFunction)gen_send, METH_O, send_doc},
    {"throw",(PyCFunction)gen_throw, METH_VARARGS, throw_doc},
    {"close",(PyCFunction)gen_close, METH_NOARGS, close_doc},
    {NULL, NULL}        /* Sentinel */
};

static PyAsyncMethods gen_as_async = {
    0,                                          /* am_await */
    0,                                          /* am_aiter */
    0,                                          /* am_anext */
    (sendfunc)PyGen_am_send,                    /* am_send  */
};


PyTypeObject PyGen_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "generator",                                /* tp_name */
    sizeof(PyGenObject),                        /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)gen_dealloc,                    /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    &gen_as_async,                              /* tp_as_async */
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
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
    gen->gi_weakreflist = NULL;
    gen->gi_exc_state.exc_type = NULL;
    gen->gi_exc_state.exc_value = NULL;
    gen->gi_exc_state.exc_traceback = NULL;
    gen->gi_exc_state.previous_item = NULL;
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
coro_get_cr_await(PyCoroObject *coro, void *Py_UNUSED(ignored))
{
    PyObject *yf = _PyGen_yf((PyGenObject *) coro);
    if (yf == NULL)
        Py_RETURN_NONE;
    return yf;
}

static PyObject *
cr_getrunning(PyCoroObject *coro, void *Py_UNUSED(ignored))
{
    if (coro->cr_frame == NULL) {
        Py_RETURN_FALSE;
    }
    return PyBool_FromLong(_PyFrame_IsExecuting(coro->cr_frame));
}

static PyGetSetDef coro_getsetlist[] = {
    {"__name__", (getter)gen_get_name, (setter)gen_set_name,
     PyDoc_STR("name of the coroutine")},
    {"__qualname__", (getter)gen_get_qualname, (setter)gen_set_qualname,
     PyDoc_STR("qualified name of the coroutine")},
    {"cr_await", (getter)coro_get_cr_await, NULL,
     PyDoc_STR("object being awaited on, or None")},
    {"cr_running", (getter)cr_getrunning, NULL, NULL},
    {NULL} /* Sentinel */
};

static PyMemberDef coro_memberlist[] = {
    {"cr_frame",     T_OBJECT, offsetof(PyCoroObject, cr_frame),    READONLY|PY_AUDIT_READ},
    {"cr_code",      T_OBJECT, offsetof(PyCoroObject, cr_code),     READONLY|PY_AUDIT_READ},
    {"cr_origin",    T_OBJECT, offsetof(PyCoroObject, cr_origin),   READONLY},
    {NULL}      /* Sentinel */
};

PyDoc_STRVAR(coro_send_doc,
"send(arg) -> send 'arg' into coroutine,\n\
return next iterated value or raise StopIteration.");

PyDoc_STRVAR(coro_throw_doc,
"throw(value)\n\
throw(type[,value[,traceback]])\n\
\n\
Raise exception in coroutine, return next iterated value or raise\n\
StopIteration.");

PyDoc_STRVAR(coro_close_doc,
"close() -> raise GeneratorExit inside coroutine.");

static PyMethodDef coro_methods[] = {
    {"send",(PyCFunction)gen_send, METH_O, coro_send_doc},
    {"throw",(PyCFunction)gen_throw, METH_VARARGS, coro_throw_doc},
    {"close",(PyCFunction)gen_close, METH_NOARGS, coro_close_doc},
    {NULL, NULL}        /* Sentinel */
};

static PyAsyncMethods coro_as_async = {
    (unaryfunc)coro_await,                      /* am_await */
    0,                                          /* am_aiter */
    0,                                          /* am_anext */
    (sendfunc)PyGen_am_send,                    /* am_send  */
};

PyTypeObject PyCoro_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "coroutine",                                /* tp_name */
    sizeof(PyCoroObject),                       /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)gen_dealloc,                    /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
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
    return gen_iternext((PyGenObject *)cw->cw_coroutine);
}

static PyObject *
coro_wrapper_send(PyCoroWrapper *cw, PyObject *arg)
{
    return gen_send((PyGenObject *)cw->cw_coroutine, arg);
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
    0,                                          /* tp_free */
};

static PyObject *
compute_cr_origin(int origin_depth)
{
    PyFrameObject *frame = PyEval_GetFrame();
    /* First count how many frames we have */
    int frame_count = 0;
    for (; frame && frame_count < origin_depth; ++frame_count) {
        frame = frame->f_back;
    }

    /* Now collect them */
    PyObject *cr_origin = PyTuple_New(frame_count);
    if (cr_origin == NULL) {
        return NULL;
    }
    frame = PyEval_GetFrame();
    for (int i = 0; i < frame_count; ++i) {
        PyCodeObject *code = frame->f_code;
        PyObject *frameinfo = Py_BuildValue("OiO",
                                            code->co_filename,
                                            PyFrame_GetLineNumber(frame),
                                            code->co_name);
        if (!frameinfo) {
            Py_DECREF(cr_origin);
            return NULL;
        }
        PyTuple_SET_ITEM(cr_origin, i, frameinfo);
        frame = frame->f_back;
    }

    return cr_origin;
}

PyObject *
PyCoro_New(PyFrameObject *f, PyObject *name, PyObject *qualname)
{
    PyObject *coro = gen_new_with_qualname(&PyCoro_Type, f, name, qualname);
    if (!coro) {
        return NULL;
    }

    PyThreadState *tstate = _PyThreadState_GET();
    int origin_depth = tstate->coroutine_origin_tracking_depth;

    if (origin_depth == 0) {
        ((PyCoroObject *)coro)->cr_origin = NULL;
    } else {
        PyObject *cr_origin = compute_cr_origin(origin_depth);
        ((PyCoroObject *)coro)->cr_origin = cr_origin;
        if (!cr_origin) {
            Py_DECREF(coro);
            return NULL;
        }
    }

    return coro;
}


/* ========= Asynchronous Generators ========= */


typedef enum {
    AWAITABLE_STATE_INIT,   /* new awaitable, has not yet been iterated */
    AWAITABLE_STATE_ITER,   /* being iterated */
    AWAITABLE_STATE_CLOSED, /* closed */
} AwaitableState;


typedef struct PyAsyncGenASend {
    PyObject_HEAD
    PyAsyncGenObject *ags_gen;

    /* Can be NULL, when in the __anext__() mode
       (equivalent of "asend(None)") */
    PyObject *ags_sendval;

    AwaitableState ags_state;
} PyAsyncGenASend;


typedef struct PyAsyncGenAThrow {
    PyObject_HEAD
    PyAsyncGenObject *agt_gen;

    /* Can be NULL, when in the "aclose()" mode
       (equivalent of "athrow(GeneratorExit)") */
    PyObject *agt_args;

    AwaitableState agt_state;
} PyAsyncGenAThrow;


typedef struct _PyAsyncGenWrappedValue {
    PyObject_HEAD
    PyObject *agw_val;
} _PyAsyncGenWrappedValue;


#define _PyAsyncGenWrappedValue_CheckExact(o) \
                    Py_IS_TYPE(o, &_PyAsyncGenWrappedValue_Type)

#define PyAsyncGenASend_CheckExact(o) \
                    Py_IS_TYPE(o, &_PyAsyncGenASend_Type)


static int
async_gen_traverse(PyAsyncGenObject *gen, visitproc visit, void *arg)
{
    Py_VISIT(gen->ag_finalizer);
    return gen_traverse((PyGenObject*)gen, visit, arg);
}


static PyObject *
async_gen_repr(PyAsyncGenObject *o)
{
    return PyUnicode_FromFormat("<async_generator object %S at %p>",
                                o->ag_qualname, o);
}


static int
async_gen_init_hooks(PyAsyncGenObject *o)
{
    PyThreadState *tstate;
    PyObject *finalizer;
    PyObject *firstiter;

    if (o->ag_hooks_inited) {
        return 0;
    }

    o->ag_hooks_inited = 1;

    tstate = _PyThreadState_GET();

    finalizer = tstate->async_gen_finalizer;
    if (finalizer) {
        Py_INCREF(finalizer);
        o->ag_finalizer = finalizer;
    }

    firstiter = tstate->async_gen_firstiter;
    if (firstiter) {
        PyObject *res;

        Py_INCREF(firstiter);
        res = PyObject_CallOneArg(firstiter, (PyObject *)o);
        Py_DECREF(firstiter);
        if (res == NULL) {
            return 1;
        }
        Py_DECREF(res);
    }

    return 0;
}


static PyObject *
async_gen_anext(PyAsyncGenObject *o)
{
    if (async_gen_init_hooks(o)) {
        return NULL;
    }
    return async_gen_asend_new(o, NULL);
}


static PyObject *
async_gen_asend(PyAsyncGenObject *o, PyObject *arg)
{
    if (async_gen_init_hooks(o)) {
        return NULL;
    }
    return async_gen_asend_new(o, arg);
}


static PyObject *
async_gen_aclose(PyAsyncGenObject *o, PyObject *arg)
{
    if (async_gen_init_hooks(o)) {
        return NULL;
    }
    return async_gen_athrow_new(o, NULL);
}

static PyObject *
async_gen_athrow(PyAsyncGenObject *o, PyObject *args)
{
    if (async_gen_init_hooks(o)) {
        return NULL;
    }
    return async_gen_athrow_new(o, args);
}


static PyGetSetDef async_gen_getsetlist[] = {
    {"__name__", (getter)gen_get_name, (setter)gen_set_name,
     PyDoc_STR("name of the async generator")},
    {"__qualname__", (getter)gen_get_qualname, (setter)gen_set_qualname,
     PyDoc_STR("qualified name of the async generator")},
    {"ag_await", (getter)coro_get_cr_await, NULL,
     PyDoc_STR("object being awaited on, or None")},
    {NULL} /* Sentinel */
};

static PyMemberDef async_gen_memberlist[] = {
    {"ag_frame",   T_OBJECT, offsetof(PyAsyncGenObject, ag_frame),   READONLY|PY_AUDIT_READ},
    {"ag_running", T_BOOL,   offsetof(PyAsyncGenObject, ag_running_async),
        READONLY},
    {"ag_code",    T_OBJECT, offsetof(PyAsyncGenObject, ag_code),    READONLY|PY_AUDIT_READ},
    {NULL}      /* Sentinel */
};

PyDoc_STRVAR(async_aclose_doc,
"aclose() -> raise GeneratorExit inside generator.");

PyDoc_STRVAR(async_asend_doc,
"asend(v) -> send 'v' in generator.");

PyDoc_STRVAR(async_athrow_doc,
"athrow(typ[,val[,tb]]) -> raise exception in generator.");

static PyMethodDef async_gen_methods[] = {
    {"asend", (PyCFunction)async_gen_asend, METH_O, async_asend_doc},
    {"athrow",(PyCFunction)async_gen_athrow, METH_VARARGS, async_athrow_doc},
    {"aclose", (PyCFunction)async_gen_aclose, METH_NOARGS, async_aclose_doc},
    {"__class_getitem__",    (PyCFunction)Py_GenericAlias,
    METH_O|METH_CLASS,       PyDoc_STR("See PEP 585")},
    {NULL, NULL}        /* Sentinel */
};


static PyAsyncMethods async_gen_as_async = {
    0,                                          /* am_await */
    PyObject_SelfIter,                          /* am_aiter */
    (unaryfunc)async_gen_anext,                 /* am_anext */
    (sendfunc)PyGen_am_send,                    /* am_send  */
};


PyTypeObject PyAsyncGen_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "async_generator",                          /* tp_name */
    sizeof(PyAsyncGenObject),                   /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)gen_dealloc,                    /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    &async_gen_as_async,                        /* tp_as_async */
    (reprfunc)async_gen_repr,                   /* tp_repr */
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
    (traverseproc)async_gen_traverse,           /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    offsetof(PyAsyncGenObject, ag_weakreflist), /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    async_gen_methods,                          /* tp_methods */
    async_gen_memberlist,                       /* tp_members */
    async_gen_getsetlist,                       /* tp_getset */
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


static struct _Py_async_gen_state *
get_async_gen_state(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return &interp->async_gen;
}


PyObject *
PyAsyncGen_New(PyFrameObject *f, PyObject *name, PyObject *qualname)
{
    PyAsyncGenObject *o;
    o = (PyAsyncGenObject *)gen_new_with_qualname(
        &PyAsyncGen_Type, f, name, qualname);
    if (o == NULL) {
        return NULL;
    }
    o->ag_finalizer = NULL;
    o->ag_closed = 0;
    o->ag_hooks_inited = 0;
    o->ag_running_async = 0;
    return (PyObject*)o;
}


void
_PyAsyncGen_ClearFreeLists(PyInterpreterState *interp)
{
    struct _Py_async_gen_state *state = &interp->async_gen;

    while (state->value_numfree) {
        _PyAsyncGenWrappedValue *o;
        o = state->value_freelist[--state->value_numfree];
        assert(_PyAsyncGenWrappedValue_CheckExact(o));
        PyObject_GC_Del(o);
    }

    while (state->asend_numfree) {
        PyAsyncGenASend *o;
        o = state->asend_freelist[--state->asend_numfree];
        assert(Py_IS_TYPE(o, &_PyAsyncGenASend_Type));
        PyObject_GC_Del(o);
    }
}

void
_PyAsyncGen_Fini(PyInterpreterState *interp)
{
    _PyAsyncGen_ClearFreeLists(interp);
#ifdef Py_DEBUG
    struct _Py_async_gen_state *state = &interp->async_gen;
    state->value_numfree = -1;
    state->asend_numfree = -1;
#endif
}


static PyObject *
async_gen_unwrap_value(PyAsyncGenObject *gen, PyObject *result)
{
    if (result == NULL) {
        if (!PyErr_Occurred()) {
            PyErr_SetNone(PyExc_StopAsyncIteration);
        }

        if (PyErr_ExceptionMatches(PyExc_StopAsyncIteration)
            || PyErr_ExceptionMatches(PyExc_GeneratorExit)
        ) {
            gen->ag_closed = 1;
        }

        gen->ag_running_async = 0;
        return NULL;
    }

    if (_PyAsyncGenWrappedValue_CheckExact(result)) {
        /* async yield */
        _PyGen_SetStopIterationValue(((_PyAsyncGenWrappedValue*)result)->agw_val);
        Py_DECREF(result);
        gen->ag_running_async = 0;
        return NULL;
    }

    return result;
}


/* ---------- Async Generator ASend Awaitable ------------ */


static void
async_gen_asend_dealloc(PyAsyncGenASend *o)
{
    _PyObject_GC_UNTRACK((PyObject *)o);
    Py_CLEAR(o->ags_gen);
    Py_CLEAR(o->ags_sendval);
    struct _Py_async_gen_state *state = get_async_gen_state();
#ifdef Py_DEBUG
    // async_gen_asend_dealloc() must not be called after _PyAsyncGen_Fini()
    assert(state->asend_numfree != -1);
#endif
    if (state->asend_numfree < _PyAsyncGen_MAXFREELIST) {
        assert(PyAsyncGenASend_CheckExact(o));
        state->asend_freelist[state->asend_numfree++] = o;
    }
    else {
        PyObject_GC_Del(o);
    }
}

static int
async_gen_asend_traverse(PyAsyncGenASend *o, visitproc visit, void *arg)
{
    Py_VISIT(o->ags_gen);
    Py_VISIT(o->ags_sendval);
    return 0;
}


static PyObject *
async_gen_asend_send(PyAsyncGenASend *o, PyObject *arg)
{
    PyObject *result;

    if (o->ags_state == AWAITABLE_STATE_CLOSED) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "cannot reuse already awaited __anext__()/asend()");
        return NULL;
    }

    if (o->ags_state == AWAITABLE_STATE_INIT) {
        if (o->ags_gen->ag_running_async) {
            PyErr_SetString(
                PyExc_RuntimeError,
                "anext(): asynchronous generator is already running");
            return NULL;
        }

        if (arg == NULL || arg == Py_None) {
            arg = o->ags_sendval;
        }
        o->ags_state = AWAITABLE_STATE_ITER;
    }

    o->ags_gen->ag_running_async = 1;
    result = gen_send((PyGenObject*)o->ags_gen, arg);
    result = async_gen_unwrap_value(o->ags_gen, result);

    if (result == NULL) {
        o->ags_state = AWAITABLE_STATE_CLOSED;
    }

    return result;
}


static PyObject *
async_gen_asend_iternext(PyAsyncGenASend *o)
{
    return async_gen_asend_send(o, NULL);
}


static PyObject *
async_gen_asend_throw(PyAsyncGenASend *o, PyObject *args)
{
    PyObject *result;

    if (o->ags_state == AWAITABLE_STATE_CLOSED) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "cannot reuse already awaited __anext__()/asend()");
        return NULL;
    }

    result = gen_throw((PyGenObject*)o->ags_gen, args);
    result = async_gen_unwrap_value(o->ags_gen, result);

    if (result == NULL) {
        o->ags_state = AWAITABLE_STATE_CLOSED;
    }

    return result;
}


static PyObject *
async_gen_asend_close(PyAsyncGenASend *o, PyObject *args)
{
    o->ags_state = AWAITABLE_STATE_CLOSED;
    Py_RETURN_NONE;
}


static PyMethodDef async_gen_asend_methods[] = {
    {"send", (PyCFunction)async_gen_asend_send, METH_O, send_doc},
    {"throw", (PyCFunction)async_gen_asend_throw, METH_VARARGS, throw_doc},
    {"close", (PyCFunction)async_gen_asend_close, METH_NOARGS, close_doc},
    {NULL, NULL}        /* Sentinel */
};


static PyAsyncMethods async_gen_asend_as_async = {
    PyObject_SelfIter,                          /* am_await */
    0,                                          /* am_aiter */
    0,                                          /* am_anext */
    0,                                          /* am_send  */
};


PyTypeObject _PyAsyncGenASend_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "async_generator_asend",                    /* tp_name */
    sizeof(PyAsyncGenASend),                    /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)async_gen_asend_dealloc,        /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    &async_gen_asend_as_async,                  /* tp_as_async */
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
    (traverseproc)async_gen_asend_traverse,     /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)async_gen_asend_iternext,     /* tp_iternext */
    async_gen_asend_methods,                    /* tp_methods */
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
};


static PyObject *
async_gen_asend_new(PyAsyncGenObject *gen, PyObject *sendval)
{
    PyAsyncGenASend *o;
    struct _Py_async_gen_state *state = get_async_gen_state();
#ifdef Py_DEBUG
    // async_gen_asend_new() must not be called after _PyAsyncGen_Fini()
    assert(state->asend_numfree != -1);
#endif
    if (state->asend_numfree) {
        state->asend_numfree--;
        o = state->asend_freelist[state->asend_numfree];
        _Py_NewReference((PyObject *)o);
    }
    else {
        o = PyObject_GC_New(PyAsyncGenASend, &_PyAsyncGenASend_Type);
        if (o == NULL) {
            return NULL;
        }
    }

    Py_INCREF(gen);
    o->ags_gen = gen;

    Py_XINCREF(sendval);
    o->ags_sendval = sendval;

    o->ags_state = AWAITABLE_STATE_INIT;

    _PyObject_GC_TRACK((PyObject*)o);
    return (PyObject*)o;
}


/* ---------- Async Generator Value Wrapper ------------ */


static void
async_gen_wrapped_val_dealloc(_PyAsyncGenWrappedValue *o)
{
    _PyObject_GC_UNTRACK((PyObject *)o);
    Py_CLEAR(o->agw_val);
    struct _Py_async_gen_state *state = get_async_gen_state();
#ifdef Py_DEBUG
    // async_gen_wrapped_val_dealloc() must not be called after _PyAsyncGen_Fini()
    assert(state->value_numfree != -1);
#endif
    if (state->value_numfree < _PyAsyncGen_MAXFREELIST) {
        assert(_PyAsyncGenWrappedValue_CheckExact(o));
        state->value_freelist[state->value_numfree++] = o;
    }
    else {
        PyObject_GC_Del(o);
    }
}


static int
async_gen_wrapped_val_traverse(_PyAsyncGenWrappedValue *o,
                               visitproc visit, void *arg)
{
    Py_VISIT(o->agw_val);
    return 0;
}


PyTypeObject _PyAsyncGenWrappedValue_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "async_generator_wrapped_value",            /* tp_name */
    sizeof(_PyAsyncGenWrappedValue),            /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)async_gen_wrapped_val_dealloc,  /* tp_dealloc */
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
    (traverseproc)async_gen_wrapped_val_traverse, /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
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
};


PyObject *
_PyAsyncGenValueWrapperNew(PyObject *val)
{
    _PyAsyncGenWrappedValue *o;
    assert(val);

    struct _Py_async_gen_state *state = get_async_gen_state();
#ifdef Py_DEBUG
    // _PyAsyncGenValueWrapperNew() must not be called after _PyAsyncGen_Fini()
    assert(state->value_numfree != -1);
#endif
    if (state->value_numfree) {
        state->value_numfree--;
        o = state->value_freelist[state->value_numfree];
        assert(_PyAsyncGenWrappedValue_CheckExact(o));
        _Py_NewReference((PyObject*)o);
    }
    else {
        o = PyObject_GC_New(_PyAsyncGenWrappedValue,
                            &_PyAsyncGenWrappedValue_Type);
        if (o == NULL) {
            return NULL;
        }
    }
    o->agw_val = val;
    Py_INCREF(val);
    _PyObject_GC_TRACK((PyObject*)o);
    return (PyObject*)o;
}


/* ---------- Async Generator AThrow awaitable ------------ */


static void
async_gen_athrow_dealloc(PyAsyncGenAThrow *o)
{
    _PyObject_GC_UNTRACK((PyObject *)o);
    Py_CLEAR(o->agt_gen);
    Py_CLEAR(o->agt_args);
    PyObject_GC_Del(o);
}


static int
async_gen_athrow_traverse(PyAsyncGenAThrow *o, visitproc visit, void *arg)
{
    Py_VISIT(o->agt_gen);
    Py_VISIT(o->agt_args);
    return 0;
}


static PyObject *
async_gen_athrow_send(PyAsyncGenAThrow *o, PyObject *arg)
{
    PyGenObject *gen = (PyGenObject*)o->agt_gen;
    PyFrameObject *f = gen->gi_frame;
    PyObject *retval;

    if (o->agt_state == AWAITABLE_STATE_CLOSED) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "cannot reuse already awaited aclose()/athrow()");
        return NULL;
    }

    if (f == NULL || _PyFrameHasCompleted(f)) {
        o->agt_state = AWAITABLE_STATE_CLOSED;
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }

    if (o->agt_state == AWAITABLE_STATE_INIT) {
        if (o->agt_gen->ag_running_async) {
            o->agt_state = AWAITABLE_STATE_CLOSED;
            if (o->agt_args == NULL) {
                PyErr_SetString(
                    PyExc_RuntimeError,
                    "aclose(): asynchronous generator is already running");
            }
            else {
                PyErr_SetString(
                    PyExc_RuntimeError,
                    "athrow(): asynchronous generator is already running");
            }
            return NULL;
        }

        if (o->agt_gen->ag_closed) {
            o->agt_state = AWAITABLE_STATE_CLOSED;
            PyErr_SetNone(PyExc_StopAsyncIteration);
            return NULL;
        }

        if (arg != Py_None) {
            PyErr_SetString(PyExc_RuntimeError, NON_INIT_CORO_MSG);
            return NULL;
        }

        o->agt_state = AWAITABLE_STATE_ITER;
        o->agt_gen->ag_running_async = 1;

        if (o->agt_args == NULL) {
            /* aclose() mode */
            o->agt_gen->ag_closed = 1;

            retval = _gen_throw((PyGenObject *)gen,
                                0,  /* Do not close generator when
                                       PyExc_GeneratorExit is passed */
                                PyExc_GeneratorExit, NULL, NULL);

            if (retval && _PyAsyncGenWrappedValue_CheckExact(retval)) {
                Py_DECREF(retval);
                goto yield_close;
            }
        } else {
            PyObject *typ;
            PyObject *tb = NULL;
            PyObject *val = NULL;

            if (!PyArg_UnpackTuple(o->agt_args, "athrow", 1, 3,
                                   &typ, &val, &tb)) {
                return NULL;
            }

            retval = _gen_throw((PyGenObject *)gen,
                                0,  /* Do not close generator when
                                       PyExc_GeneratorExit is passed */
                                typ, val, tb);
            retval = async_gen_unwrap_value(o->agt_gen, retval);
        }
        if (retval == NULL) {
            goto check_error;
        }
        return retval;
    }

    assert(o->agt_state == AWAITABLE_STATE_ITER);

    retval = gen_send((PyGenObject *)gen, arg);
    if (o->agt_args) {
        return async_gen_unwrap_value(o->agt_gen, retval);
    } else {
        /* aclose() mode */
        if (retval) {
            if (_PyAsyncGenWrappedValue_CheckExact(retval)) {
                Py_DECREF(retval);
                goto yield_close;
            }
            else {
                return retval;
            }
        }
        else {
            goto check_error;
        }
    }

yield_close:
    o->agt_gen->ag_running_async = 0;
    o->agt_state = AWAITABLE_STATE_CLOSED;
    PyErr_SetString(
        PyExc_RuntimeError, ASYNC_GEN_IGNORED_EXIT_MSG);
    return NULL;

check_error:
    o->agt_gen->ag_running_async = 0;
    o->agt_state = AWAITABLE_STATE_CLOSED;
    if (PyErr_ExceptionMatches(PyExc_StopAsyncIteration) ||
            PyErr_ExceptionMatches(PyExc_GeneratorExit))
    {
        if (o->agt_args == NULL) {
            /* when aclose() is called we don't want to propagate
               StopAsyncIteration or GeneratorExit; just raise
               StopIteration, signalling that this 'aclose()' await
               is done.
            */
            PyErr_Clear();
            PyErr_SetNone(PyExc_StopIteration);
        }
    }
    return NULL;
}


static PyObject *
async_gen_athrow_throw(PyAsyncGenAThrow *o, PyObject *args)
{
    PyObject *retval;

    if (o->agt_state == AWAITABLE_STATE_CLOSED) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "cannot reuse already awaited aclose()/athrow()");
        return NULL;
    }

    retval = gen_throw((PyGenObject*)o->agt_gen, args);
    if (o->agt_args) {
        return async_gen_unwrap_value(o->agt_gen, retval);
    } else {
        /* aclose() mode */
        if (retval && _PyAsyncGenWrappedValue_CheckExact(retval)) {
            o->agt_gen->ag_running_async = 0;
            o->agt_state = AWAITABLE_STATE_CLOSED;
            Py_DECREF(retval);
            PyErr_SetString(PyExc_RuntimeError, ASYNC_GEN_IGNORED_EXIT_MSG);
            return NULL;
        }
        if (PyErr_ExceptionMatches(PyExc_StopAsyncIteration) ||
            PyErr_ExceptionMatches(PyExc_GeneratorExit))
        {
            /* when aclose() is called we don't want to propagate
               StopAsyncIteration or GeneratorExit; just raise
               StopIteration, signalling that this 'aclose()' await
               is done.
            */
            PyErr_Clear();
            PyErr_SetNone(PyExc_StopIteration);
        }
        return retval;
    }
}


static PyObject *
async_gen_athrow_iternext(PyAsyncGenAThrow *o)
{
    return async_gen_athrow_send(o, Py_None);
}


static PyObject *
async_gen_athrow_close(PyAsyncGenAThrow *o, PyObject *args)
{
    o->agt_state = AWAITABLE_STATE_CLOSED;
    Py_RETURN_NONE;
}


static PyMethodDef async_gen_athrow_methods[] = {
    {"send", (PyCFunction)async_gen_athrow_send, METH_O, send_doc},
    {"throw", (PyCFunction)async_gen_athrow_throw, METH_VARARGS, throw_doc},
    {"close", (PyCFunction)async_gen_athrow_close, METH_NOARGS, close_doc},
    {NULL, NULL}        /* Sentinel */
};


static PyAsyncMethods async_gen_athrow_as_async = {
    PyObject_SelfIter,                          /* am_await */
    0,                                          /* am_aiter */
    0,                                          /* am_anext */
    0,                                          /* am_send  */
};


PyTypeObject _PyAsyncGenAThrow_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "async_generator_athrow",                   /* tp_name */
    sizeof(PyAsyncGenAThrow),                   /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)async_gen_athrow_dealloc,       /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    &async_gen_athrow_as_async,                 /* tp_as_async */
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
    (traverseproc)async_gen_athrow_traverse,    /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)async_gen_athrow_iternext,    /* tp_iternext */
    async_gen_athrow_methods,                   /* tp_methods */
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
};


static PyObject *
async_gen_athrow_new(PyAsyncGenObject *gen, PyObject *args)
{
    PyAsyncGenAThrow *o;
    o = PyObject_GC_New(PyAsyncGenAThrow, &_PyAsyncGenAThrow_Type);
    if (o == NULL) {
        return NULL;
    }
    o->agt_gen = gen;
    o->agt_args = args;
    o->agt_state = AWAITABLE_STATE_INIT;
    Py_INCREF(gen);
    Py_XINCREF(args);
    _PyObject_GC_TRACK((PyObject*)o);
    return (PyObject*)o;
}
