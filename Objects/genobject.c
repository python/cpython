/* Generator object implementation */

#define _PY_INTERPRETER

#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_ceval.h"         // _PyEval_EvalFrame()
#include "pycore_frame.h"         // _PyInterpreterFrame
#include "pycore_freelist.h"      // _Py_FREELIST_FREE()
#include "pycore_gc.h"            // _PyGC_CLEAR_FINALIZED()
#include "pycore_genobject.h"     // _PyGen_SetStopIterationValue()
#include "pycore_interpframe.h"   // _PyFrame_GetCode()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_object.h"        // _PyObject_GC_UNTRACK()
#include "pycore_opcode_utils.h"  // RESUME_AFTER_YIELD_FROM
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_LOAD_UINT8_RELAXED()
#include "pycore_pyerrors.h"      // _PyErr_ClearExcState()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_warnings.h"      // _PyErr_WarnUnawaitedCoroutine()


#include "opcode_ids.h"           // RESUME, etc

// Forward declarations
static PyObject* gen_close(PyObject *, PyObject *);
static PyObject* async_gen_asend_new(PyAsyncGenObject *, PyObject *);
static PyObject* async_gen_athrow_new(PyAsyncGenObject *, PyObject *);


#define _PyGen_CAST(op) \
    _Py_CAST(PyGenObject*, (op))
#define _PyCoroObject_CAST(op) \
    (assert(PyCoro_CheckExact(op)), \
     _Py_CAST(PyCoroObject*, (op)))
#define _PyAsyncGenObject_CAST(op) \
    _Py_CAST(PyAsyncGenObject*, (op))


static const char *NON_INIT_CORO_MSG = "can't send non-None value to a "
                                 "just-started coroutine";

static const char *ASYNC_GEN_IGNORED_EXIT_MSG =
                                 "async generator ignored GeneratorExit";

/* Returns a borrowed reference */
static inline PyCodeObject *
_PyGen_GetCode(PyGenObject *gen) {
    return _PyFrame_GetCode(&gen->gi_iframe);
}

PyCodeObject *
PyGen_GetCode(PyGenObject *gen) {
    assert(PyGen_Check(gen));
    PyCodeObject *res = _PyGen_GetCode(gen);
    Py_INCREF(res);
    return res;
}

static int
gen_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyGenObject *gen = _PyGen_CAST(self);
    Py_VISIT(gen->gi_name);
    Py_VISIT(gen->gi_qualname);
    if (gen->gi_frame_state != FRAME_CLEARED) {
        _PyInterpreterFrame *frame = &gen->gi_iframe;
        assert(frame->frame_obj == NULL ||
               frame->frame_obj->f_frame->owner == FRAME_OWNED_BY_GENERATOR);
        int err = _PyFrame_Traverse(frame, visit, arg);
        if (err) {
            return err;
        }
    }
    else {
        // We still need to visit the code object when the frame is cleared to
        // ensure that it's kept alive if the reference is deferred.
        _Py_VISIT_STACKREF(gen->gi_iframe.f_executable);
    }
    /* No need to visit cr_origin, because it's just tuples/str/int, so can't
       participate in a reference cycle. */
    Py_VISIT(gen->gi_exc_state.exc_value);
    return 0;
}

void
_PyGen_Finalize(PyObject *self)
{
    PyGenObject *gen = (PyGenObject *)self;

    if (FRAME_STATE_FINISHED(gen->gi_frame_state)) {
        /* Generator isn't paused, so no need to close */
        return;
    }

    if (PyAsyncGen_CheckExact(self)) {
        PyAsyncGenObject *agen = (PyAsyncGenObject*)self;
        PyObject *finalizer = agen->ag_origin_or_finalizer;
        if (finalizer && !agen->ag_closed) {
            /* Save the current exception, if any. */
            PyObject *exc = PyErr_GetRaisedException();

            PyObject *res = PyObject_CallOneArg(finalizer, self);
            if (res == NULL) {
                PyErr_FormatUnraisable("Exception ignored while "
                                       "finalizing generator %R", self);
            }
            else {
                Py_DECREF(res);
            }
            /* Restore the saved exception. */
            PyErr_SetRaisedException(exc);
            return;
        }
    }

    /* Save the current exception, if any. */
    PyObject *exc = PyErr_GetRaisedException();

    /* If `gen` is a coroutine, and if it was never awaited on,
       issue a RuntimeWarning. */
    assert(_PyGen_GetCode(gen) != NULL);
    if (_PyGen_GetCode(gen)->co_flags & CO_COROUTINE &&
        gen->gi_frame_state == FRAME_CREATED)
    {
        _PyErr_WarnUnawaitedCoroutine((PyObject *)gen);
    }
    else {
        PyObject *res = gen_close((PyObject*)gen, NULL);
        if (res == NULL) {
            if (PyErr_Occurred()) {
                PyErr_FormatUnraisable("Exception ignored while "
                                       "closing generator %R", self);
            }
        }
        else {
            Py_DECREF(res);
        }
    }

    /* Restore the saved exception. */
    PyErr_SetRaisedException(exc);
}

static void
gen_clear_frame(PyGenObject *gen)
{
    if (gen->gi_frame_state == FRAME_CLEARED)
        return;

    gen->gi_frame_state = FRAME_CLEARED;
    _PyInterpreterFrame *frame = &gen->gi_iframe;
    frame->previous = NULL;
    _PyFrame_ClearExceptCode(frame);
    _PyErr_ClearExcState(&gen->gi_exc_state);
}

static void
gen_dealloc(PyObject *self)
{
    PyGenObject *gen = _PyGen_CAST(self);

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
        Py_CLEAR(((PyAsyncGenObject*)gen)->ag_origin_or_finalizer);
    }
    if (PyCoro_CheckExact(gen)) {
        Py_CLEAR(((PyCoroObject *)gen)->cr_origin_or_finalizer);
    }
    gen_clear_frame(gen);
    assert(gen->gi_exc_state.exc_value == NULL);
    PyStackRef_CLEAR(gen->gi_iframe.f_executable);
    Py_CLEAR(gen->gi_name);
    Py_CLEAR(gen->gi_qualname);

    PyObject_GC_Del(gen);
}

static PySendResult
gen_send_ex2(PyGenObject *gen, PyObject *arg, PyObject **presult,
             int exc, int closing)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _PyInterpreterFrame *frame = &gen->gi_iframe;

    *presult = NULL;
    if (gen->gi_frame_state == FRAME_CREATED && arg && arg != Py_None) {
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
    if (gen->gi_frame_state == FRAME_EXECUTING) {
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
    if (FRAME_STATE_FINISHED(gen->gi_frame_state)) {
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
            *presult = Py_NewRef(Py_None);
            return PYGEN_RETURN;
        }
        return PYGEN_ERROR;
    }

    assert((gen->gi_frame_state == FRAME_CREATED) ||
           FRAME_STATE_SUSPENDED(gen->gi_frame_state));

    /* Push arg onto the frame's value stack */
    PyObject *arg_obj = arg ? arg : Py_None;
    _PyFrame_StackPush(frame, PyStackRef_FromPyObjectNew(arg_obj));

    _PyErr_StackItem *prev_exc_info = tstate->exc_info;
    gen->gi_exc_state.previous_item = prev_exc_info;
    tstate->exc_info = &gen->gi_exc_state;

    if (exc) {
        assert(_PyErr_Occurred(tstate));
        _PyErr_ChainStackItem();
    }

    gen->gi_frame_state = FRAME_EXECUTING;
    EVAL_CALL_STAT_INC(EVAL_CALL_GENERATOR);
    PyObject *result = _PyEval_EvalFrame(tstate, frame, exc);
    assert(tstate->exc_info == prev_exc_info);
    assert(gen->gi_exc_state.previous_item == NULL);
    assert(gen->gi_frame_state != FRAME_EXECUTING);
    assert(frame->previous == NULL);

    /* If the generator just returned (as opposed to yielding), signal
     * that the generator is exhausted. */
    if (result) {
        if (FRAME_STATE_SUSPENDED(gen->gi_frame_state)) {
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
        assert(!PyErr_ExceptionMatches(PyExc_StopIteration));
        assert(!PyAsyncGen_CheckExact(gen) ||
            !PyErr_ExceptionMatches(PyExc_StopAsyncIteration));
    }

    assert(gen->gi_exc_state.exc_value == NULL);
    assert(gen->gi_frame_state == FRAME_CLEARED);
    *presult = result;
    return result ? PYGEN_RETURN : PYGEN_ERROR;
}

static PySendResult
PyGen_am_send(PyObject *self, PyObject *arg, PyObject **result)
{
    PyGenObject *gen = _PyGen_CAST(self);
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
gen_send(PyObject *gen, PyObject *arg)
{
    return gen_send_ex((PyGenObject*)gen, arg, 0, 0);
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

    if (PyGen_CheckExact(yf) || PyCoro_CheckExact(yf)) {
        retval = gen_close((PyObject *)yf, NULL);
        if (retval == NULL)
            return -1;
    }
    else {
        PyObject *meth;
        if (PyObject_GetOptionalAttr(yf, &_Py_ID(close), &meth) < 0) {
            PyErr_FormatUnraisable("Exception ignored while "
                                   "closing generator %R", yf);
        }
        if (meth) {
            retval = _PyObject_CallNoArgs(meth);
            Py_DECREF(meth);
            if (retval == NULL)
                return -1;
        }
    }
    Py_XDECREF(retval);
    return 0;
}

static inline bool
is_resume(_Py_CODEUNIT *instr)
{
    uint8_t code = FT_ATOMIC_LOAD_UINT8_RELAXED(instr->op.code);
    return (
        code == RESUME ||
        code == RESUME_CHECK ||
        code == INSTRUMENTED_RESUME
    );
}

PyObject *
_PyGen_yf(PyGenObject *gen)
{
    if (gen->gi_frame_state == FRAME_SUSPENDED_YIELD_FROM) {
        _PyInterpreterFrame *frame = &gen->gi_iframe;
        // GH-122390: These asserts are wrong in the presence of ENTER_EXECUTOR!
        // assert(is_resume(frame->instr_ptr));
        // assert((frame->instr_ptr->op.arg & RESUME_OPARG_LOCATION_MASK) >= RESUME_AFTER_YIELD_FROM);
        return PyStackRef_AsPyObjectNew(_PyFrame_StackPeek(frame));
    }
    return NULL;
}

static PyObject *
gen_close(PyObject *self, PyObject *args)
{
    PyGenObject *gen = _PyGen_CAST(self);

    if (gen->gi_frame_state == FRAME_CREATED) {
        gen->gi_frame_state = FRAME_COMPLETED;
        Py_RETURN_NONE;
    }
    if (FRAME_STATE_FINISHED(gen->gi_frame_state)) {
        Py_RETURN_NONE;
    }

    PyObject *yf = _PyGen_yf(gen);
    int err = 0;
    if (yf) {
        PyFrameState state = gen->gi_frame_state;
        gen->gi_frame_state = FRAME_EXECUTING;
        err = gen_close_iter(yf);
        gen->gi_frame_state = state;
        Py_DECREF(yf);
    }
    _PyInterpreterFrame *frame = &gen->gi_iframe;
    if (is_resume(frame->instr_ptr)) {
        /* We can safely ignore the outermost try block
         * as it is automatically generated to handle
         * StopIteration. */
        int oparg = frame->instr_ptr->op.arg;
        if (oparg & RESUME_OPARG_DEPTH1_MASK) {
            // RESUME after YIELD_VALUE and exception depth is 1
            assert((oparg & RESUME_OPARG_LOCATION_MASK) != RESUME_AT_FUNC_START);
            gen->gi_frame_state = FRAME_COMPLETED;
            gen_clear_frame(gen);
            Py_RETURN_NONE;
        }
    }
    if (err == 0) {
        PyErr_SetNone(PyExc_GeneratorExit);
    }

    PyObject *retval = gen_send_ex(gen, Py_None, 1, 1);
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
    assert(PyErr_Occurred());

    if (PyErr_ExceptionMatches(PyExc_GeneratorExit)) {
        PyErr_Clear();          /* ignore this error */
        Py_RETURN_NONE;
    }

    /* if the generator returned a value while closing, StopIteration was
     * raised in gen_send_ex() above; retrieve and return the value here */
    if (_PyGen_FetchStopIterationValue(&retval) == 0) {
        return retval;
    }
    return NULL;
}


PyDoc_STRVAR(throw_doc,
"throw(value)\n\
throw(type[,value[,tb]])\n\
\n\
Raise exception in generator, return next yielded value or raise\n\
StopIteration.\n\
the (type, val, tb) signature is deprecated, \n\
and may be removed in a future version of Python.");

static PyObject *
_gen_throw(PyGenObject *gen, int close_on_genexit,
           PyObject *typ, PyObject *val, PyObject *tb)
{
    PyObject *yf = _PyGen_yf(gen);

    if (yf) {
        _PyInterpreterFrame *frame = &gen->gi_iframe;
        PyObject *ret;
        int err;
        if (PyErr_GivenExceptionMatches(typ, PyExc_GeneratorExit) &&
            close_on_genexit
        ) {
            /* Asynchronous generators *should not* be closed right away.
               We have to allow some awaits to work it through, hence the
               `close_on_genexit` parameter here.
            */
            PyFrameState state = gen->gi_frame_state;
            gen->gi_frame_state = FRAME_EXECUTING;
            err = gen_close_iter(yf);
            gen->gi_frame_state = state;
            Py_DECREF(yf);
            if (err < 0)
                return gen_send_ex(gen, Py_None, 1, 0);
            goto throw_here;
        }
        PyThreadState *tstate = _PyThreadState_GET();
        assert(tstate != NULL);
        if (PyGen_CheckExact(yf) || PyCoro_CheckExact(yf)) {
            /* `yf` is a generator or a coroutine. */

            /* Link frame into the stack to enable complete backtraces. */
            /* XXX We should probably be updating the current frame somewhere in
               ceval.c. */
            _PyInterpreterFrame *prev = tstate->current_frame;
            frame->previous = prev;
            tstate->current_frame = frame;
            /* Close the generator that we are currently iterating with
               'yield from' or awaiting on with 'await'. */
            PyFrameState state = gen->gi_frame_state;
            gen->gi_frame_state = FRAME_EXECUTING;
            ret = _gen_throw((PyGenObject *)yf, close_on_genexit,
                             typ, val, tb);
            gen->gi_frame_state = state;
            tstate->current_frame = prev;
            frame->previous = NULL;
        } else {
            /* `yf` is an iterator or a coroutine-like object. */
            PyObject *meth;
            if (PyObject_GetOptionalAttr(yf, &_Py_ID(throw), &meth) < 0) {
                Py_DECREF(yf);
                return NULL;
            }
            if (meth == NULL) {
                Py_DECREF(yf);
                goto throw_here;
            }

            _PyInterpreterFrame *prev = tstate->current_frame;
            frame->previous = prev;
            tstate->current_frame = frame;
            PyFrameState state = gen->gi_frame_state;
            gen->gi_frame_state = FRAME_EXECUTING;
            ret = PyObject_CallFunctionObjArgs(meth, typ, val, tb, NULL);
            gen->gi_frame_state = state;
            tstate->current_frame = prev;
            frame->previous = NULL;
            Py_DECREF(meth);
        }
        Py_DECREF(yf);
        if (!ret) {
            ret = gen_send_ex(gen, Py_None, 1, 0);
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
            Py_XSETREF(val, typ);
            typ = Py_NewRef(PyExceptionInstance_Class(typ));

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
gen_throw(PyObject *op, PyObject *const *args, Py_ssize_t nargs)
{
    PyGenObject *gen = _PyGen_CAST(op);
    PyObject *typ;
    PyObject *tb = NULL;
    PyObject *val = NULL;

    if (!_PyArg_CheckPositional("throw", nargs, 1, 3)) {
        return NULL;
    }
    if (nargs > 1) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                            "the (type, exc, tb) signature of throw() is deprecated, "
                            "use the single-arg signature instead.",
                            1) < 0) {
            return NULL;
        }
    }
    typ = args[0];
    if (nargs == 3) {
        val = args[1];
        tb = args[2];
    }
    else if (nargs == 2) {
        val = args[1];
    }
    return _gen_throw(gen, 1, typ, val, tb);
}


static PyObject *
gen_iternext(PyObject *self)
{
    assert(PyGen_CheckExact(self) || PyCoro_CheckExact(self));
    PyGenObject *gen = _PyGen_CAST(self);

    PyObject *result;
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
    assert(!PyErr_Occurred());
    // Construct an exception instance manually with PyObject_CallOneArg()
    // but use PyErr_SetRaisedException() instead of PyErr_SetObject() as
    // PyErr_SetObject(exc_type, value) has a fast path when 'value'
    // is a tuple, where the value of the StopIteration exception would be
    // set to 'value[0]' instead of 'value'.
    PyObject *exc = value == NULL
        ? PyObject_CallNoArgs(PyExc_StopIteration)
        : PyObject_CallOneArg(PyExc_StopIteration, value);
    if (exc == NULL) {
        return -1;
    }
    PyErr_SetRaisedException(exc /* stolen */);
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
    PyObject *value = NULL;
    if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
        PyObject *exc = PyErr_GetRaisedException();
        value = Py_NewRef(((PyStopIterationObject *)exc)->value);
        Py_DECREF(exc);
    } else if (PyErr_Occurred()) {
        return -1;
    }
    if (value == NULL) {
        value = Py_NewRef(Py_None);
    }
    *pvalue = value;
    return 0;
}

static PyObject *
gen_repr(PyObject *self)
{
    PyGenObject *gen = _PyGen_CAST(self);
    return PyUnicode_FromFormat("<generator object %S at %p>",
                                gen->gi_qualname, gen);
}

static PyObject *
gen_get_name(PyObject *self, void *Py_UNUSED(ignored))
{
    PyGenObject *op = _PyGen_CAST(self);
    PyObject *name = FT_ATOMIC_LOAD_PTR_ACQUIRE(op->gi_name);
    return Py_NewRef(name);
}

static int
gen_set_name(PyObject *self, PyObject *value, void *Py_UNUSED(ignored))
{
    PyGenObject *op = _PyGen_CAST(self);
    /* Not legal to del gen.gi_name or to set it to anything
     * other than a string object. */
    if (value == NULL || !PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "__name__ must be set to a string object");
        return -1;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    // gh-133931: To prevent use-after-free from other threads that reference
    // the gi_name.
    _PyObject_XSetRefDelayed(&op->gi_name, Py_NewRef(value));
    Py_END_CRITICAL_SECTION();
    return 0;
}

static PyObject *
gen_get_qualname(PyObject *self, void *Py_UNUSED(ignored))
{
    PyGenObject *op = _PyGen_CAST(self);
    PyObject *qualname = FT_ATOMIC_LOAD_PTR_ACQUIRE(op->gi_qualname);
    return Py_NewRef(qualname);
}

static int
gen_set_qualname(PyObject *self, PyObject *value, void *Py_UNUSED(ignored))
{
    PyGenObject *op = _PyGen_CAST(self);
    /* Not legal to del gen.__qualname__ or to set it to anything
     * other than a string object. */
    if (value == NULL || !PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "__qualname__ must be set to a string object");
        return -1;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    // gh-133931: To prevent use-after-free from other threads that reference
    // the gi_qualname.
    _PyObject_XSetRefDelayed(&op->gi_qualname, Py_NewRef(value));
    Py_END_CRITICAL_SECTION();
    return 0;
}

static PyObject *
gen_getyieldfrom(PyObject *gen, void *Py_UNUSED(ignored))
{
    PyObject *yf = _PyGen_yf(_PyGen_CAST(gen));
    if (yf == NULL) {
        Py_RETURN_NONE;
    }
    return yf;
}


static PyObject *
gen_getrunning(PyObject *self, void *Py_UNUSED(ignored))
{
    PyGenObject *gen = _PyGen_CAST(self);
    if (gen->gi_frame_state == FRAME_EXECUTING) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *
gen_getsuspended(PyObject *self, void *Py_UNUSED(ignored))
{
    PyGenObject *gen = _PyGen_CAST(self);
    return PyBool_FromLong(FRAME_STATE_SUSPENDED(gen->gi_frame_state));
}

static PyObject *
_gen_getframe(PyGenObject *gen, const char *const name)
{
    if (PySys_Audit("object.__getattr__", "Os", gen, name) < 0) {
        return NULL;
    }
    if (FRAME_STATE_FINISHED(gen->gi_frame_state)) {
        Py_RETURN_NONE;
    }
    return _Py_XNewRef((PyObject *)_PyFrame_GetFrameObject(&gen->gi_iframe));
}

static PyObject *
gen_getframe(PyObject *self, void *Py_UNUSED(ignored))
{
    PyGenObject *gen = _PyGen_CAST(self);
    return _gen_getframe(gen, "gi_frame");
}

static PyObject *
_gen_getcode(PyGenObject *gen, const char *const name)
{
    if (PySys_Audit("object.__getattr__", "Os", gen, name) < 0) {
        return NULL;
    }
    return Py_NewRef(_PyGen_GetCode(gen));
}

static PyObject *
gen_getcode(PyObject *self, void *Py_UNUSED(ignored))
{
    PyGenObject *gen = _PyGen_CAST(self);
    return _gen_getcode(gen, "gi_code");
}

static PyGetSetDef gen_getsetlist[] = {
    {"__name__", gen_get_name, gen_set_name,
     PyDoc_STR("name of the generator")},
    {"__qualname__", gen_get_qualname, gen_set_qualname,
     PyDoc_STR("qualified name of the generator")},
    {"gi_yieldfrom", gen_getyieldfrom, NULL,
     PyDoc_STR("object being iterated by yield from, or None")},
    {"gi_running", gen_getrunning, NULL, NULL},
    {"gi_frame", gen_getframe,  NULL, NULL},
    {"gi_suspended", gen_getsuspended,  NULL, NULL},
    {"gi_code", gen_getcode,  NULL, NULL},
    {NULL} /* Sentinel */
};

static PyMemberDef gen_memberlist[] = {
    {NULL}      /* Sentinel */
};

static PyObject *
gen_sizeof(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    PyGenObject *gen = _PyGen_CAST(op);
    Py_ssize_t res;
    res = offsetof(PyGenObject, gi_iframe) + offsetof(_PyInterpreterFrame, localsplus);
    PyCodeObject *code = _PyGen_GetCode(gen);
    res += _PyFrame_NumSlotsForCodeObject(code) * sizeof(PyObject *);
    return PyLong_FromSsize_t(res);
}

PyDoc_STRVAR(sizeof__doc__,
"gen.__sizeof__() -> size of gen in memory, in bytes");

static PyMethodDef gen_methods[] = {
    {"send", gen_send, METH_O, send_doc},
    {"throw", _PyCFunction_CAST(gen_throw), METH_FASTCALL, throw_doc},
    {"close", gen_close, METH_NOARGS, close_doc},
    {"__sizeof__", gen_sizeof, METH_NOARGS, sizeof__doc__},
    {"__class_getitem__", Py_GenericAlias, METH_O|METH_CLASS, PyDoc_STR("See PEP 585")},
    {NULL, NULL}        /* Sentinel */
};

static PyAsyncMethods gen_as_async = {
    0,                                          /* am_await */
    0,                                          /* am_aiter */
    0,                                          /* am_anext */
    PyGen_am_send,                              /* am_send  */
};


PyTypeObject PyGen_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "generator",                                /* tp_name */
    offsetof(PyGenObject, gi_iframe.localsplus), /* tp_basicsize */
    sizeof(PyObject *),                         /* tp_itemsize */
    /* methods */
    gen_dealloc,                                /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    &gen_as_async,                              /* tp_as_async */
    gen_repr,                                   /* tp_repr */
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
    gen_traverse,                               /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    offsetof(PyGenObject, gi_weakreflist),      /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    gen_iternext,                               /* tp_iternext */
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
make_gen(PyTypeObject *type, PyFunctionObject *func)
{
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    int slots = _PyFrame_NumSlotsForCodeObject(code);
    PyGenObject *gen = PyObject_GC_NewVar(PyGenObject, type, slots);
    if (gen == NULL) {
        return NULL;
    }
    gen->gi_frame_state = FRAME_CLEARED;
    gen->gi_weakreflist = NULL;
    gen->gi_exc_state.exc_value = NULL;
    gen->gi_exc_state.previous_item = NULL;
    assert(func->func_name != NULL);
    gen->gi_name = Py_NewRef(func->func_name);
    assert(func->func_qualname != NULL);
    gen->gi_qualname = Py_NewRef(func->func_qualname);
    _PyObject_GC_TRACK(gen);
    return (PyObject *)gen;
}

static PyObject *
compute_cr_origin(int origin_depth, _PyInterpreterFrame *current_frame);

PyObject *
_Py_MakeCoro(PyFunctionObject *func)
{
    int coro_flags = ((PyCodeObject *)func->func_code)->co_flags &
        (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR);
    assert(coro_flags);
    if (coro_flags == CO_GENERATOR) {
        return make_gen(&PyGen_Type, func);
    }
    if (coro_flags == CO_ASYNC_GENERATOR) {
        PyAsyncGenObject *ag;
        ag = (PyAsyncGenObject *)make_gen(&PyAsyncGen_Type, func);
        if (ag == NULL) {
            return NULL;
        }
        ag->ag_origin_or_finalizer = NULL;
        ag->ag_closed = 0;
        ag->ag_hooks_inited = 0;
        ag->ag_running_async = 0;
        return (PyObject*)ag;
    }

    assert (coro_flags == CO_COROUTINE);
    PyObject *coro = make_gen(&PyCoro_Type, func);
    if (!coro) {
        return NULL;
    }
    PyThreadState *tstate = _PyThreadState_GET();
    int origin_depth = tstate->coroutine_origin_tracking_depth;

    if (origin_depth == 0) {
        ((PyCoroObject *)coro)->cr_origin_or_finalizer = NULL;
    } else {
        _PyInterpreterFrame *frame = tstate->current_frame;
        assert(frame);
        assert(_PyFrame_IsIncomplete(frame));
        frame = _PyFrame_GetFirstComplete(frame->previous);
        PyObject *cr_origin = compute_cr_origin(origin_depth, frame);
        ((PyCoroObject *)coro)->cr_origin_or_finalizer = cr_origin;
        if (!cr_origin) {
            Py_DECREF(coro);
            return NULL;
        }
    }
    return coro;
}

static PyObject *
gen_new_with_qualname(PyTypeObject *type, PyFrameObject *f,
                      PyObject *name, PyObject *qualname)
{
    PyCodeObject *code = _PyFrame_GetCode(f->f_frame);
    int size = code->co_nlocalsplus + code->co_stacksize;
    PyGenObject *gen = PyObject_GC_NewVar(PyGenObject, type, size);
    if (gen == NULL) {
        Py_DECREF(f);
        return NULL;
    }
    /* Copy the frame */
    assert(f->f_frame->frame_obj == NULL);
    assert(f->f_frame->owner == FRAME_OWNED_BY_FRAME_OBJECT);
    _PyInterpreterFrame *frame = &gen->gi_iframe;
    _PyFrame_Copy((_PyInterpreterFrame *)f->_f_frame_data, frame);
    gen->gi_frame_state = FRAME_CREATED;
    assert(frame->frame_obj == f);
    f->f_frame = frame;
    frame->owner = FRAME_OWNED_BY_GENERATOR;
    assert(PyObject_GC_IsTracked((PyObject *)f));
    Py_DECREF(f);
    gen->gi_weakreflist = NULL;
    gen->gi_exc_state.exc_value = NULL;
    gen->gi_exc_state.previous_item = NULL;
    if (name != NULL)
        gen->gi_name = Py_NewRef(name);
    else
        gen->gi_name = Py_NewRef(_PyGen_GetCode(gen)->co_name);
    if (qualname != NULL)
        gen->gi_qualname = Py_NewRef(qualname);
    else
        gen->gi_qualname = Py_NewRef(_PyGen_GetCode(gen)->co_qualname);
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

#define _PyCoroWrapper_CAST(op) \
    (assert(Py_IS_TYPE((op), &_PyCoroWrapper_Type)), \
     _Py_CAST(PyCoroWrapper*, (op)))


static int
gen_is_coroutine(PyObject *o)
{
    if (PyGen_CheckExact(o)) {
        PyCodeObject *code = _PyGen_GetCode((PyGenObject*)o);
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
        return Py_NewRef(o);
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
                 "'%.100s' object can't be awaited",
                 ot->tp_name);
    return NULL;
}

static PyObject *
coro_repr(PyObject *self)
{
    PyCoroObject *coro = _PyCoroObject_CAST(self);
    return PyUnicode_FromFormat("<coroutine object %S at %p>",
                                coro->cr_qualname, coro);
}

static PyObject *
coro_await(PyObject *coro)
{
    PyCoroWrapper *cw = PyObject_GC_New(PyCoroWrapper, &_PyCoroWrapper_Type);
    if (cw == NULL) {
        return NULL;
    }
    cw->cw_coroutine = (PyCoroObject*)Py_NewRef(coro);
    _PyObject_GC_TRACK(cw);
    return (PyObject *)cw;
}

static PyObject *
coro_get_cr_await(PyObject *coro, void *Py_UNUSED(ignored))
{
    PyObject *yf = _PyGen_yf((PyGenObject *) coro);
    if (yf == NULL)
        Py_RETURN_NONE;
    return yf;
}

static PyObject *
cr_getsuspended(PyObject *self, void *Py_UNUSED(ignored))
{
    PyCoroObject *coro = _PyCoroObject_CAST(self);
    if (FRAME_STATE_SUSPENDED(coro->cr_frame_state)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *
cr_getrunning(PyObject *self, void *Py_UNUSED(ignored))
{
    PyCoroObject *coro = _PyCoroObject_CAST(self);
    if (coro->cr_frame_state == FRAME_EXECUTING) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *
cr_getframe(PyObject *coro, void *Py_UNUSED(ignored))
{
    return _gen_getframe(_PyGen_CAST(coro), "cr_frame");
}

static PyObject *
cr_getcode(PyObject *coro, void *Py_UNUSED(ignored))
{
    return _gen_getcode(_PyGen_CAST(coro), "cr_code");
}

static PyGetSetDef coro_getsetlist[] = {
    {"__name__", gen_get_name, gen_set_name,
     PyDoc_STR("name of the coroutine")},
    {"__qualname__", gen_get_qualname, gen_set_qualname,
     PyDoc_STR("qualified name of the coroutine")},
    {"cr_await", coro_get_cr_await, NULL,
     PyDoc_STR("object being awaited on, or None")},
    {"cr_running", cr_getrunning, NULL, NULL},
    {"cr_frame", cr_getframe, NULL, NULL},
    {"cr_code", cr_getcode, NULL, NULL},
    {"cr_suspended", cr_getsuspended, NULL, NULL},
    {NULL} /* Sentinel */
};

static PyMemberDef coro_memberlist[] = {
    {"cr_origin",    _Py_T_OBJECT, offsetof(PyCoroObject, cr_origin_or_finalizer),   Py_READONLY},
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
StopIteration.\n\
the (type, val, tb) signature is deprecated, \n\
and may be removed in a future version of Python.");


PyDoc_STRVAR(coro_close_doc,
"close() -> raise GeneratorExit inside coroutine.");

static PyMethodDef coro_methods[] = {
    {"send", gen_send, METH_O, coro_send_doc},
    {"throw",_PyCFunction_CAST(gen_throw), METH_FASTCALL, coro_throw_doc},
    {"close", gen_close, METH_NOARGS, coro_close_doc},
    {"__sizeof__", gen_sizeof, METH_NOARGS, sizeof__doc__},
    {"__class_getitem__", Py_GenericAlias, METH_O|METH_CLASS, PyDoc_STR("See PEP 585")},
    {NULL, NULL}        /* Sentinel */
};

static PyAsyncMethods coro_as_async = {
    coro_await,                                 /* am_await */
    0,                                          /* am_aiter */
    0,                                          /* am_anext */
    PyGen_am_send,                              /* am_send  */
};

PyTypeObject PyCoro_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "coroutine",                                /* tp_name */
    offsetof(PyCoroObject, cr_iframe.localsplus),/* tp_basicsize */
    sizeof(PyObject *),                         /* tp_itemsize */
    /* methods */
    gen_dealloc,                                /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    &coro_as_async,                             /* tp_as_async */
    coro_repr,                                  /* tp_repr */
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
    gen_traverse,                               /* tp_traverse */
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
coro_wrapper_dealloc(PyObject *self)
{
    PyCoroWrapper *cw = _PyCoroWrapper_CAST(self);
    _PyObject_GC_UNTRACK((PyObject *)cw);
    Py_CLEAR(cw->cw_coroutine);
    PyObject_GC_Del(cw);
}

static PyObject *
coro_wrapper_iternext(PyObject *self)
{
    PyCoroWrapper *cw = _PyCoroWrapper_CAST(self);
    return gen_iternext((PyObject *)cw->cw_coroutine);
}

static PyObject *
coro_wrapper_send(PyObject *self, PyObject *arg)
{
    PyCoroWrapper *cw = _PyCoroWrapper_CAST(self);
    return gen_send((PyObject *)cw->cw_coroutine, arg);
}

static PyObject *
coro_wrapper_throw(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyCoroWrapper *cw = _PyCoroWrapper_CAST(self);
    return gen_throw((PyObject*)cw->cw_coroutine, args, nargs);
}

static PyObject *
coro_wrapper_close(PyObject *self, PyObject *args)
{
    PyCoroWrapper *cw = _PyCoroWrapper_CAST(self);
    return gen_close((PyObject *)cw->cw_coroutine, args);
}

static int
coro_wrapper_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyCoroWrapper *cw = _PyCoroWrapper_CAST(self);
    Py_VISIT((PyObject *)cw->cw_coroutine);
    return 0;
}

static PyMethodDef coro_wrapper_methods[] = {
    {"send", coro_wrapper_send, METH_O, coro_send_doc},
    {"throw", _PyCFunction_CAST(coro_wrapper_throw), METH_FASTCALL,
     coro_throw_doc},
    {"close", coro_wrapper_close, METH_NOARGS, coro_close_doc},
    {NULL, NULL}        /* Sentinel */
};

PyTypeObject _PyCoroWrapper_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "coroutine_wrapper",
    sizeof(PyCoroWrapper),                      /* tp_basicsize */
    0,                                          /* tp_itemsize */
    coro_wrapper_dealloc,                       /* destructor tp_dealloc */
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
    coro_wrapper_traverse,                      /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    coro_wrapper_iternext,                      /* tp_iternext */
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
compute_cr_origin(int origin_depth, _PyInterpreterFrame *current_frame)
{
    _PyInterpreterFrame *frame = current_frame;
    /* First count how many frames we have */
    int frame_count = 0;
    for (; frame && frame_count < origin_depth; ++frame_count) {
        frame = _PyFrame_GetFirstComplete(frame->previous);
    }

    /* Now collect them */
    PyObject *cr_origin = PyTuple_New(frame_count);
    if (cr_origin == NULL) {
        return NULL;
    }
    frame = current_frame;
    for (int i = 0; i < frame_count; ++i) {
        PyCodeObject *code = _PyFrame_GetCode(frame);
        int line = PyUnstable_InterpreterFrame_GetLine(frame);
        PyObject *frameinfo = Py_BuildValue("OiO", code->co_filename, line,
                                            code->co_name);
        if (!frameinfo) {
            Py_DECREF(cr_origin);
            return NULL;
        }
        PyTuple_SET_ITEM(cr_origin, i, frameinfo);
        frame = _PyFrame_GetFirstComplete(frame->previous);
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
        ((PyCoroObject *)coro)->cr_origin_or_finalizer = NULL;
    } else {
        PyObject *cr_origin = compute_cr_origin(origin_depth, _PyEval_GetFrame());
        ((PyCoroObject *)coro)->cr_origin_or_finalizer = cr_origin;
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

#define _PyAsyncGenASend_CAST(op) \
    _Py_CAST(PyAsyncGenASend*, (op))


typedef struct PyAsyncGenAThrow {
    PyObject_HEAD
    PyAsyncGenObject *agt_gen;

    /* Can be NULL, when in the "aclose()" mode
       (equivalent of "athrow(GeneratorExit)") */
    PyObject *agt_typ;
    PyObject *agt_tb;
    PyObject *agt_val;

    AwaitableState agt_state;
} PyAsyncGenAThrow;


typedef struct _PyAsyncGenWrappedValue {
    PyObject_HEAD
    PyObject *agw_val;
} _PyAsyncGenWrappedValue;


#define _PyAsyncGenWrappedValue_CheckExact(o) \
                    Py_IS_TYPE(o, &_PyAsyncGenWrappedValue_Type)
#define _PyAsyncGenWrappedValue_CAST(op) \
    (assert(_PyAsyncGenWrappedValue_CheckExact(op)), \
     _Py_CAST(_PyAsyncGenWrappedValue*, (op)))


static int
async_gen_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyAsyncGenObject *ag = _PyAsyncGenObject_CAST(self);
    Py_VISIT(ag->ag_origin_or_finalizer);
    return gen_traverse((PyObject*)ag, visit, arg);
}


static PyObject *
async_gen_repr(PyObject *self)
{
    PyAsyncGenObject *o = _PyAsyncGenObject_CAST(self);
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
        o->ag_origin_or_finalizer = Py_NewRef(finalizer);
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
async_gen_anext(PyObject *self)
{
    PyAsyncGenObject *ag = _PyAsyncGenObject_CAST(self);
    if (async_gen_init_hooks(ag)) {
        return NULL;
    }
    return async_gen_asend_new(ag, NULL);
}


static PyObject *
async_gen_asend(PyObject *op, PyObject *arg)
{
    PyAsyncGenObject *o = (PyAsyncGenObject*)op;
    if (async_gen_init_hooks(o)) {
        return NULL;
    }
    return async_gen_asend_new(o, arg);
}


static PyObject *
async_gen_aclose(PyObject *op, PyObject *arg)
{
    PyAsyncGenObject *o = (PyAsyncGenObject*)op;
    if (async_gen_init_hooks(o)) {
        return NULL;
    }
    return async_gen_athrow_new(o, NULL);
}

static PyObject *
async_gen_athrow(PyObject *op, PyObject *args)
{
    PyAsyncGenObject *o = (PyAsyncGenObject*)op;
    if (PyTuple_GET_SIZE(args) > 1) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                            "the (type, exc, tb) signature of athrow() is deprecated, "
                            "use the single-arg signature instead.",
                            1) < 0) {
            return NULL;
        }
    }
    if (async_gen_init_hooks(o)) {
        return NULL;
    }
    return async_gen_athrow_new(o, args);
}

static PyObject *
ag_getframe(PyObject *ag, void *Py_UNUSED(ignored))
{
    return _gen_getframe((PyGenObject *)ag, "ag_frame");
}

static PyObject *
ag_getcode(PyObject *gen, void *Py_UNUSED(ignored))
{
    return _gen_getcode((PyGenObject*)gen, "ag_code");
}

static PyObject *
ag_getsuspended(PyObject *self, void *Py_UNUSED(ignored))
{
    PyAsyncGenObject *ag = _PyAsyncGenObject_CAST(self);
    if (FRAME_STATE_SUSPENDED(ag->ag_frame_state)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyGetSetDef async_gen_getsetlist[] = {
    {"__name__", gen_get_name, gen_set_name,
     PyDoc_STR("name of the async generator")},
    {"__qualname__", gen_get_qualname, gen_set_qualname,
     PyDoc_STR("qualified name of the async generator")},
    {"ag_await", coro_get_cr_await, NULL,
     PyDoc_STR("object being awaited on, or None")},
     {"ag_frame", ag_getframe, NULL, NULL},
     {"ag_code", ag_getcode, NULL, NULL},
     {"ag_suspended", ag_getsuspended, NULL, NULL},
    {NULL} /* Sentinel */
};

static PyMemberDef async_gen_memberlist[] = {
    {"ag_running", Py_T_BOOL,   offsetof(PyAsyncGenObject, ag_running_async),
        Py_READONLY},
    {NULL}      /* Sentinel */
};

PyDoc_STRVAR(async_aclose_doc,
"aclose() -> raise GeneratorExit inside generator.");

PyDoc_STRVAR(async_asend_doc,
"asend(v) -> send 'v' in generator.");

PyDoc_STRVAR(async_athrow_doc,
"athrow(value)\n\
athrow(type[,value[,tb]])\n\
\n\
raise exception in generator.\n\
the (type, val, tb) signature is deprecated, \n\
and may be removed in a future version of Python.");

static PyMethodDef async_gen_methods[] = {
    {"asend", async_gen_asend, METH_O, async_asend_doc},
    {"athrow", async_gen_athrow, METH_VARARGS, async_athrow_doc},
    {"aclose", async_gen_aclose, METH_NOARGS, async_aclose_doc},
    {"__sizeof__", gen_sizeof, METH_NOARGS, sizeof__doc__},
    {"__class_getitem__",    Py_GenericAlias,
    METH_O|METH_CLASS,       PyDoc_STR("See PEP 585")},
    {NULL, NULL}        /* Sentinel */
};


static PyAsyncMethods async_gen_as_async = {
    0,                                          /* am_await */
    PyObject_SelfIter,                          /* am_aiter */
    async_gen_anext,                            /* am_anext */
    PyGen_am_send,                              /* am_send  */
};


PyTypeObject PyAsyncGen_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "async_generator",                          /* tp_name */
    offsetof(PyAsyncGenObject, ag_iframe.localsplus), /* tp_basicsize */
    sizeof(PyObject *),                         /* tp_itemsize */
    /* methods */
    gen_dealloc,                                /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    &async_gen_as_async,                        /* tp_as_async */
    async_gen_repr,                             /* tp_repr */
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
    async_gen_traverse,                         /* tp_traverse */
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


PyObject *
PyAsyncGen_New(PyFrameObject *f, PyObject *name, PyObject *qualname)
{
    PyAsyncGenObject *ag;
    ag = (PyAsyncGenObject *)gen_new_with_qualname(&PyAsyncGen_Type, f,
                                                   name, qualname);
    if (ag == NULL) {
        return NULL;
    }

    ag->ag_origin_or_finalizer = NULL;
    ag->ag_closed = 0;
    ag->ag_hooks_inited = 0;
    ag->ag_running_async = 0;
    return (PyObject*)ag;
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
async_gen_asend_dealloc(PyObject *self)
{
    assert(PyAsyncGenASend_CheckExact(self));
    PyAsyncGenASend *ags = _PyAsyncGenASend_CAST(self);

    if (PyObject_CallFinalizerFromDealloc(self)) {
        return;
    }

    _PyObject_GC_UNTRACK(self);
    Py_CLEAR(ags->ags_gen);
    Py_CLEAR(ags->ags_sendval);

    _PyGC_CLEAR_FINALIZED(self);

    _Py_FREELIST_FREE(async_gen_asends, self, PyObject_GC_Del);
}

static int
async_gen_asend_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyAsyncGenASend *ags = _PyAsyncGenASend_CAST(self);
    Py_VISIT(ags->ags_gen);
    Py_VISIT(ags->ags_sendval);
    return 0;
}


static PyObject *
async_gen_asend_send(PyObject *self, PyObject *arg)
{
    PyAsyncGenASend *o = _PyAsyncGenASend_CAST(self);
    if (o->ags_state == AWAITABLE_STATE_CLOSED) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "cannot reuse already awaited __anext__()/asend()");
        return NULL;
    }

    if (o->ags_state == AWAITABLE_STATE_INIT) {
        if (o->ags_gen->ag_running_async) {
            o->ags_state = AWAITABLE_STATE_CLOSED;
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
    PyObject *result = gen_send((PyObject*)o->ags_gen, arg);
    result = async_gen_unwrap_value(o->ags_gen, result);

    if (result == NULL) {
        o->ags_state = AWAITABLE_STATE_CLOSED;
    }

    return result;
}


static PyObject *
async_gen_asend_iternext(PyObject *ags)
{
    return async_gen_asend_send(ags, NULL);
}


static PyObject *
async_gen_asend_throw(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyAsyncGenASend *o = _PyAsyncGenASend_CAST(self);

    if (o->ags_state == AWAITABLE_STATE_CLOSED) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "cannot reuse already awaited __anext__()/asend()");
        return NULL;
    }

    if (o->ags_state == AWAITABLE_STATE_INIT) {
        if (o->ags_gen->ag_running_async) {
            o->ags_state = AWAITABLE_STATE_CLOSED;
            PyErr_SetString(
                PyExc_RuntimeError,
                "anext(): asynchronous generator is already running");
            return NULL;
        }

        o->ags_state = AWAITABLE_STATE_ITER;
        o->ags_gen->ag_running_async = 1;
    }

    PyObject *result = gen_throw((PyObject*)o->ags_gen, args, nargs);
    result = async_gen_unwrap_value(o->ags_gen, result);

    if (result == NULL) {
        o->ags_gen->ag_running_async = 0;
        o->ags_state = AWAITABLE_STATE_CLOSED;
    }

    return result;
}


static PyObject *
async_gen_asend_close(PyObject *self, PyObject *args)
{
    PyAsyncGenASend *o = _PyAsyncGenASend_CAST(self);
    if (o->ags_state == AWAITABLE_STATE_CLOSED) {
        Py_RETURN_NONE;
    }

    PyObject *result = async_gen_asend_throw(self, &PyExc_GeneratorExit, 1);
    if (result == NULL) {
        if (PyErr_ExceptionMatches(PyExc_StopIteration) ||
            PyErr_ExceptionMatches(PyExc_StopAsyncIteration) ||
            PyErr_ExceptionMatches(PyExc_GeneratorExit))
        {
            PyErr_Clear();
            Py_RETURN_NONE;
        }
        return result;
    }

    Py_DECREF(result);
    PyErr_SetString(PyExc_RuntimeError, "coroutine ignored GeneratorExit");
    return NULL;
}

static void
async_gen_asend_finalize(PyObject *self)
{
    PyAsyncGenASend *ags = _PyAsyncGenASend_CAST(self);
    if (ags->ags_state == AWAITABLE_STATE_INIT) {
        _PyErr_WarnUnawaitedAgenMethod(ags->ags_gen, &_Py_ID(asend));
    }
}

static PyMethodDef async_gen_asend_methods[] = {
    {"send", async_gen_asend_send, METH_O, send_doc},
    {"throw", _PyCFunction_CAST(async_gen_asend_throw), METH_FASTCALL, throw_doc},
    {"close", async_gen_asend_close, METH_NOARGS, close_doc},
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
    async_gen_asend_dealloc,                    /* tp_dealloc */
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
    async_gen_asend_traverse,                   /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    async_gen_asend_iternext,                   /* tp_iternext */
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
    .tp_finalize = async_gen_asend_finalize,
};


static PyObject *
async_gen_asend_new(PyAsyncGenObject *gen, PyObject *sendval)
{
    PyAsyncGenASend *ags = _Py_FREELIST_POP(PyAsyncGenASend, async_gen_asends);
    if (ags == NULL) {
        ags = PyObject_GC_New(PyAsyncGenASend, &_PyAsyncGenASend_Type);
        if (ags == NULL) {
            return NULL;
        }
    }

    ags->ags_gen = (PyAsyncGenObject*)Py_NewRef(gen);
    ags->ags_sendval = Py_XNewRef(sendval);
    ags->ags_state = AWAITABLE_STATE_INIT;

    _PyObject_GC_TRACK((PyObject*)ags);
    return (PyObject*)ags;
}


/* ---------- Async Generator Value Wrapper ------------ */


static void
async_gen_wrapped_val_dealloc(PyObject *self)
{
    _PyAsyncGenWrappedValue *agw = _PyAsyncGenWrappedValue_CAST(self);
    _PyObject_GC_UNTRACK(self);
    Py_CLEAR(agw->agw_val);
    _Py_FREELIST_FREE(async_gens, self, PyObject_GC_Del);
}


static int
async_gen_wrapped_val_traverse(PyObject *self, visitproc visit, void *arg)
{
    _PyAsyncGenWrappedValue *agw = _PyAsyncGenWrappedValue_CAST(self);
    Py_VISIT(agw->agw_val);
    return 0;
}


PyTypeObject _PyAsyncGenWrappedValue_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "async_generator_wrapped_value",            /* tp_name */
    sizeof(_PyAsyncGenWrappedValue),            /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    async_gen_wrapped_val_dealloc,              /* tp_dealloc */
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
    async_gen_wrapped_val_traverse,             /* tp_traverse */
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
_PyAsyncGenValueWrapperNew(PyThreadState *tstate, PyObject *val)
{
    assert(val);

    _PyAsyncGenWrappedValue *o = _Py_FREELIST_POP(_PyAsyncGenWrappedValue, async_gens);
    if (o == NULL) {
        o = PyObject_GC_New(_PyAsyncGenWrappedValue,
                            &_PyAsyncGenWrappedValue_Type);
        if (o == NULL) {
            return NULL;
        }
    }
    assert(_PyAsyncGenWrappedValue_CheckExact(o));
    o->agw_val = Py_NewRef(val);
    _PyObject_GC_TRACK((PyObject*)o);
    return (PyObject*)o;
}


/* ---------- Async Generator AThrow awaitable ------------ */

#define _PyAsyncGenAThrow_CAST(op) \
    (assert(Py_IS_TYPE((op), &_PyAsyncGenAThrow_Type)), \
     _Py_CAST(PyAsyncGenAThrow*, (op)))

static void
async_gen_athrow_dealloc(PyObject *self)
{
    PyAsyncGenAThrow *agt = _PyAsyncGenAThrow_CAST(self);
    if (PyObject_CallFinalizerFromDealloc(self)) {
        return;
    }

    _PyObject_GC_UNTRACK(self);
    Py_CLEAR(agt->agt_gen);
    Py_XDECREF(agt->agt_typ);
    Py_XDECREF(agt->agt_tb);
    Py_XDECREF(agt->agt_val);
    PyObject_GC_Del(self);
}


static int
async_gen_athrow_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyAsyncGenAThrow *agt = _PyAsyncGenAThrow_CAST(self);
    Py_VISIT(agt->agt_gen);
    Py_VISIT(agt->agt_typ);
    Py_VISIT(agt->agt_tb);
    Py_VISIT(agt->agt_val);
    return 0;
}


static PyObject *
async_gen_athrow_send(PyObject *self, PyObject *arg)
{
    PyAsyncGenAThrow *o = _PyAsyncGenAThrow_CAST(self);
    PyGenObject *gen = _PyGen_CAST(o->agt_gen);
    PyObject *retval;

    if (o->agt_state == AWAITABLE_STATE_CLOSED) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "cannot reuse already awaited aclose()/athrow()");
        return NULL;
    }

    if (FRAME_STATE_FINISHED(gen->gi_frame_state)) {
        o->agt_state = AWAITABLE_STATE_CLOSED;
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }

    if (o->agt_state == AWAITABLE_STATE_INIT) {
        if (o->agt_gen->ag_running_async) {
            o->agt_state = AWAITABLE_STATE_CLOSED;
            if (o->agt_typ == NULL) {
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

        if (o->agt_typ == NULL) {
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
            retval = _gen_throw((PyGenObject *)gen,
                                0,  /* Do not close generator when
                                       PyExc_GeneratorExit is passed */
                                o->agt_typ, o->agt_val, o->agt_tb);
            retval = async_gen_unwrap_value(o->agt_gen, retval);
        }
        if (retval == NULL) {
            goto check_error;
        }
        return retval;
    }

    assert(o->agt_state == AWAITABLE_STATE_ITER);

    retval = gen_send((PyObject *)gen, arg);
    if (o->agt_typ) {
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
        if (o->agt_typ == NULL) {
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
async_gen_athrow_throw(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyAsyncGenAThrow *o = _PyAsyncGenAThrow_CAST(self);

    if (o->agt_state == AWAITABLE_STATE_CLOSED) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "cannot reuse already awaited aclose()/athrow()");
        return NULL;
    }

    if (o->agt_state == AWAITABLE_STATE_INIT) {
        if (o->agt_gen->ag_running_async) {
            o->agt_state = AWAITABLE_STATE_CLOSED;
            if (o->agt_typ == NULL) {
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

        o->agt_state = AWAITABLE_STATE_ITER;
        o->agt_gen->ag_running_async = 1;
    }

    PyObject *retval = gen_throw((PyObject*)o->agt_gen, args, nargs);
    if (o->agt_typ) {
        retval = async_gen_unwrap_value(o->agt_gen, retval);
        if (retval == NULL) {
            o->agt_gen->ag_running_async = 0;
            o->agt_state = AWAITABLE_STATE_CLOSED;
        }
        return retval;
    }
    else {
        /* aclose() mode */
        if (retval && _PyAsyncGenWrappedValue_CheckExact(retval)) {
            o->agt_gen->ag_running_async = 0;
            o->agt_state = AWAITABLE_STATE_CLOSED;
            Py_DECREF(retval);
            PyErr_SetString(PyExc_RuntimeError, ASYNC_GEN_IGNORED_EXIT_MSG);
            return NULL;
        }
        if (retval == NULL) {
            o->agt_gen->ag_running_async = 0;
            o->agt_state = AWAITABLE_STATE_CLOSED;
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
async_gen_athrow_iternext(PyObject *agt)
{
    return async_gen_athrow_send(agt, Py_None);
}


static PyObject *
async_gen_athrow_close(PyObject *self, PyObject *args)
{
    PyAsyncGenAThrow *agt = _PyAsyncGenAThrow_CAST(self);
    if (agt->agt_state == AWAITABLE_STATE_CLOSED) {
        Py_RETURN_NONE;
    }
    PyObject *result = async_gen_athrow_throw((PyObject*)agt,
                                              &PyExc_GeneratorExit, 1);
    if (result == NULL) {
        if (PyErr_ExceptionMatches(PyExc_StopIteration) ||
            PyErr_ExceptionMatches(PyExc_StopAsyncIteration) ||
            PyErr_ExceptionMatches(PyExc_GeneratorExit))
        {
            PyErr_Clear();
            Py_RETURN_NONE;
        }
        return result;
    } else {
        Py_DECREF(result);
        PyErr_SetString(PyExc_RuntimeError, "coroutine ignored GeneratorExit");
        return NULL;
    }
}


static void
async_gen_athrow_finalize(PyObject *op)
{
    PyAsyncGenAThrow *o = (PyAsyncGenAThrow*)op;
    if (o->agt_state == AWAITABLE_STATE_INIT) {
        PyObject *method = o->agt_typ ? &_Py_ID(athrow) : &_Py_ID(aclose);
        _PyErr_WarnUnawaitedAgenMethod(o->agt_gen, method);
    }
}

static PyMethodDef async_gen_athrow_methods[] = {
    {"send", async_gen_athrow_send, METH_O, send_doc},
    {"throw", _PyCFunction_CAST(async_gen_athrow_throw),
    METH_FASTCALL, throw_doc},
    {"close", async_gen_athrow_close, METH_NOARGS, close_doc},
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
    async_gen_athrow_dealloc,                   /* tp_dealloc */
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
    async_gen_athrow_traverse,                  /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    async_gen_athrow_iternext,                  /* tp_iternext */
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
    .tp_finalize = async_gen_athrow_finalize,
};


static PyObject *
async_gen_athrow_new(PyAsyncGenObject *gen, PyObject *args)
{
    PyObject *typ = NULL;
    PyObject *tb = NULL;
    PyObject *val = NULL;
    if (args && !PyArg_UnpackTuple(args, "athrow", 1, 3, &typ, &val, &tb)) {
        return NULL;
    }

    PyAsyncGenAThrow *o;
    o = PyObject_GC_New(PyAsyncGenAThrow, &_PyAsyncGenAThrow_Type);
    if (o == NULL) {
        return NULL;
    }
    o->agt_gen = (PyAsyncGenObject*)Py_NewRef(gen);
    o->agt_typ = Py_XNewRef(typ);
    o->agt_tb = Py_XNewRef(tb);
    o->agt_val = Py_XNewRef(val);

    o->agt_state = AWAITABLE_STATE_INIT;
    _PyObject_GC_TRACK((PyObject*)o);
    return (PyObject*)o;
}
