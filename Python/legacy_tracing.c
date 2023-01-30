/* Support for legacy tracing on top of PEP 669 instrumentation
 * Provides callables to forward PEP 669 events to legacy events.
 */

#include <stddef.h>
#include "Python.h"
#include "opcode.h"
#include "pycore_ceval.h"
#include "pycore_instruments.h"
#include "pycore_pyerrors.h"
#include "pycore_pymem.h"
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_sysmodule.h"

typedef struct _PyLegacyEventHandler {
    PyObject_HEAD
    vectorcallfunc vectorcall;
    int event;
} _PyLegacyEventHandler;

static void
dealloc(_PyLegacyEventHandler *self)
{
    PyObject_Free(self);
}


/* The Py_tracefunc function expects the following arguments:
 *   frame: FrameObject
 *   kind: c-int
 *   arg: The arg
 */

static PyObject *
call_profile_func(_PyLegacyEventHandler *self, PyObject *arg)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate->c_profilefunc == NULL) {
        Py_RETURN_NONE;
    }
    PyFrameObject* frame = PyEval_GetFrame();
    if (frame == NULL) {
        PyErr_SetString(PyExc_SystemError,
                        "Missing frame when calling profile function.");
        return NULL;
    }
    Py_INCREF(frame);
    int err = tstate->c_profilefunc(tstate->c_profileobj, frame, self->event, arg);
    Py_DECREF(frame);
    if (err) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
sys_profile_func2(
    _PyLegacyEventHandler *self, PyObject *const *args,
    size_t nargsf, PyObject *kwnames
) {
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 2);
    return call_profile_func(self, Py_None);
}

static PyObject *
sys_profile_func3(
    _PyLegacyEventHandler *self, PyObject *const *args,
    size_t nargsf, PyObject *kwnames
) {
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 3);
    return call_profile_func(self, args[2]);
}

static PyObject *
sys_profile_call_or_return(
    _PyLegacyEventHandler *self, PyObject *const *args,
    size_t nargsf, PyObject *kwnames
) {
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 3);
    PyObject *callable = args[2];
    if (!PyCFunction_Check(callable) && Py_TYPE(callable) != &PyMethodDescr_Type) {
        Py_RETURN_NONE;
    }
    return call_profile_func(self, callable);
}

static PyObject *
call_trace_func(_PyLegacyEventHandler *self, PyObject *arg)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate->c_tracefunc == NULL) {
        Py_RETURN_NONE;
    }
    PyFrameObject* frame = PyEval_GetFrame();
    if (frame == NULL) {
        PyErr_SetString(PyExc_SystemError,
                        "Missing frame when calling trace function.");
        return NULL;
    }
    Py_INCREF(frame);
    int err = tstate->c_tracefunc(tstate->c_traceobj, frame, self->event, arg);
    Py_DECREF(frame);
    if (err) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
sys_trace_exception_func(
    _PyLegacyEventHandler *self, PyObject *const *args,
    size_t nargsf, PyObject *kwnames
) {
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 3);
    PyObject *arg = args[2];
    assert(PyTuple_CheckExact(arg));
    PyObject *res = call_trace_func(self, arg);
    return res;
}

static PyObject *
sys_trace_func2(
    _PyLegacyEventHandler *self, PyObject *const *args,
    size_t nargsf, PyObject *kwnames
) {
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 2);
    return call_trace_func(self, Py_None);
}

static PyObject *
sys_trace_func3(
    _PyLegacyEventHandler *self, PyObject *const *args,
    size_t nargsf, PyObject *kwnames
) {
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 3);
    return call_trace_func(self, args[2]);
}

static PyObject *
sys_trace_instruction_func(
    _PyLegacyEventHandler *self, PyObject *const *args,
    size_t nargsf, PyObject *kwnames
) {
    assert(kwnames == NULL);
    assert(PyVectorcall_NARGS(nargsf) == 2);
    PyFrameObject* frame = PyEval_GetFrame();
    if (frame == NULL) {
        PyErr_SetString(PyExc_SystemError,
                        "Missing frame when calling trace function.");
        return NULL;
    }
    if (!frame->f_trace_opcodes) {
        Py_RETURN_NONE;
    }
    Py_INCREF(frame);
    PyThreadState *tstate = _PyThreadState_GET();
    int err = tstate->c_tracefunc(tstate->c_traceobj, frame, self->event, Py_None);
    frame->f_lineno = 0;
    Py_DECREF(frame);
    if (err) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
trace_line(
    PyThreadState *tstate, _PyLegacyEventHandler *self,
    PyFrameObject* frame, int line
) {
    if (line < 0) {
        Py_RETURN_NONE;
    }
    frame ->f_last_traced_line = line;
    Py_INCREF(frame);
    frame->f_lineno = line;
    int err = tstate->c_tracefunc(tstate->c_traceobj, frame, self->event, Py_None);
    frame->f_lineno = 0;
    Py_DECREF(frame);
    if (err) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
sys_trace_line_func(
    _PyLegacyEventHandler *self, PyObject *const *args,
    size_t nargsf, PyObject *kwnames
) {
    assert(kwnames == NULL);
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate->c_tracefunc == NULL) {
        Py_RETURN_NONE;
    }
    assert(PyVectorcall_NARGS(nargsf) == 2);
    int line = _PyLong_AsInt(args[1]);
    assert(line >= 0);
    PyFrameObject* frame = PyEval_GetFrame();
    if (frame == NULL) {
        PyErr_SetString(PyExc_SystemError,
                        "Missing frame when calling trace function.");
        return NULL;
    }
    assert(args[0] == (PyObject *)frame->f_frame->f_code);
    if (!frame->f_trace_lines) {
        Py_RETURN_NONE;
    }
    if (frame ->f_last_traced_line == line) {
        /* Already traced this line */
        Py_RETURN_NONE;
    }
    return trace_line(tstate, self, frame, line);
}


static PyObject *
sys_trace_jump_func(
    _PyLegacyEventHandler *self, PyObject *const *args,
    size_t nargsf, PyObject *kwnames
) {
    assert(kwnames == NULL);
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate->c_tracefunc == NULL) {
        Py_RETURN_NONE;
    }
    assert(PyVectorcall_NARGS(nargsf) == 3);
    int from = _PyLong_AsInt(args[1]);
    assert(from >= 0);
    int to = _PyLong_AsInt(args[2]);
    assert(to >= 0);
    PyFrameObject* frame = PyEval_GetFrame();
    if (frame == NULL) {
        PyErr_SetString(PyExc_SystemError,
                        "Missing frame when calling trace function.");
        return NULL;
    }
    if (!frame->f_trace_lines) {
        Py_RETURN_NONE;
    }
    PyCodeObject *code = (PyCodeObject *)args[0];
    assert(PyCode_Check(code));
    assert(code == frame->f_frame->f_code);
    /* We can call _Py_Instrumentation_GetLine because we always set
    * line events for tracing */
    int to_line = _Py_Instrumentation_GetLine(code, to);
    /* Backward jump: Always generate event
     * Forward jump: Only generate event if jumping to different line. */
    if (to > from) {
        /* Forwards jump */
        if (frame->f_last_traced_line == to_line) {
            /* Already traced this line */
            Py_RETURN_NONE;
        }
    }
    return trace_line(tstate, self, frame, to_line);
}


PyTypeObject _PyLegacyEventHandler_Type = {

    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "sys.legacy_event_handler",
    sizeof(_PyLegacyEventHandler),
    .tp_dealloc = (destructor)dealloc,
    .tp_vectorcall_offset = offsetof(_PyLegacyEventHandler, vectorcall),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_VECTORCALL,
    .tp_call = PyVectorcall_Call,
};

static int
set_callbacks(int tool, vectorcallfunc vectorcall, int legacy_event, int event1, int event2)
{
    _PyLegacyEventHandler *callback =
        PyObject_NEW(_PyLegacyEventHandler, &_PyLegacyEventHandler_Type);
    if (callback == NULL) {
        return -1;
    }
    callback->vectorcall = vectorcall;
    callback->event = legacy_event;
    Py_XDECREF(_PyMonitoring_RegisterCallback(tool, event1, (PyObject *)callback));
    if (event2 >= 0) {
        Py_XDECREF(_PyMonitoring_RegisterCallback(tool, event2, (PyObject *)callback));
    }
    Py_DECREF(callback);
    return 0;
}

#ifndef NDEBUG
/* Ensure that tstate is valid: sanity check for PyEval_AcquireThread() and
   PyEval_RestoreThread(). Detect if tstate memory was freed. It can happen
   when a thread continues to run after Python finalization, especially
   daemon threads. */
static int
is_tstate_valid(PyThreadState *tstate)
{
    assert(!_PyMem_IsPtrFreed(tstate));
    assert(!_PyMem_IsPtrFreed(tstate->interp));
    return 1;
}
#endif

int
_PyEval_SetProfile(PyThreadState *tstate, Py_tracefunc func, PyObject *arg)
{
    assert(is_tstate_valid(tstate));
    /* The caller must hold the GIL */
    assert(PyGILState_Check());

    /* Call _PySys_Audit() in the context of the current thread state,
       even if tstate is not the current thread state. */
    PyThreadState *current_tstate = _PyThreadState_GET();
    if (_PySys_Audit(current_tstate, "sys.setprofile", NULL) < 0) {
        return -1;
    }
    /* Setup PEP 669 monitoring callbacks and events. */
    if (!tstate->interp->sys_profile_initialized) {
        tstate->interp->sys_profile_initialized = true;
        if (set_callbacks(PY_INSTRUMENT_SYS_PROFILE,
            (vectorcallfunc)sys_profile_func2, PyTrace_CALL,
                        PY_MONITORING_EVENT_PY_START, PY_MONITORING_EVENT_PY_RESUME)) {
            return -1;
        }
        if (set_callbacks(PY_INSTRUMENT_SYS_PROFILE,
            (vectorcallfunc)sys_profile_func3, PyTrace_RETURN,
                        PY_MONITORING_EVENT_PY_RETURN, PY_MONITORING_EVENT_PY_YIELD)) {
            return -1;
        }
        if (set_callbacks(PY_INSTRUMENT_SYS_PROFILE,
            (vectorcallfunc)sys_profile_func2, PyTrace_RETURN,
                        PY_MONITORING_EVENT_PY_UNWIND, -1)) {
            return -1;
        }
        if (set_callbacks(PY_INSTRUMENT_SYS_PROFILE,
            (vectorcallfunc)sys_profile_call_or_return, PyTrace_C_CALL,
                        PY_MONITORING_EVENT_CALL, -1)) {
            return -1;
        }
        if (set_callbacks(PY_INSTRUMENT_SYS_PROFILE,
            (vectorcallfunc)sys_profile_call_or_return, PyTrace_C_RETURN,
                        PY_MONITORING_EVENT_C_RETURN, -1)) {
            return -1;
        }
        if (set_callbacks(PY_INSTRUMENT_SYS_PROFILE,
            (vectorcallfunc)sys_profile_call_or_return, PyTrace_C_EXCEPTION,
                        PY_MONITORING_EVENT_C_RAISE, -1)) {
            return -1;
        }
    }

    int delta = (func != NULL) - (tstate->c_profilefunc != NULL);
    tstate->c_profilefunc = func;
    PyObject *old_profileobj = tstate->c_profileobj;
    tstate->c_profileobj = Py_XNewRef(arg);
    Py_XDECREF(old_profileobj);
    tstate->interp->sys_profiling_threads += delta;
    assert(tstate->interp->sys_profiling_threads >= 0);

    if (tstate->interp->sys_profiling_threads) {
        uint32_t events =
            (1 << PY_MONITORING_EVENT_PY_START) | (1 << PY_MONITORING_EVENT_PY_RESUME) |
            (1 << PY_MONITORING_EVENT_PY_RETURN) | (1 << PY_MONITORING_EVENT_PY_YIELD) |
            (1 << PY_MONITORING_EVENT_CALL) | (1 << PY_MONITORING_EVENT_PY_UNWIND) |
            (1 << PY_MONITORING_EVENT_C_RETURN) | (1 << PY_MONITORING_EVENT_C_RAISE);
        _PyMonitoring_SetEvents(PY_INSTRUMENT_SYS_PROFILE, events);
    }
    else {
        _PyMonitoring_SetEvents(PY_INSTRUMENT_SYS_PROFILE, 0);
    }
    return 0;
}

int
_PyEval_SetTrace(PyThreadState *tstate, Py_tracefunc func, PyObject *arg)
{
    assert(is_tstate_valid(tstate));
    /* The caller must hold the GIL */
    assert(PyGILState_Check());

    /* Call _PySys_Audit() in the context of the current thread state,
       even if tstate is not the current thread state. */
    PyThreadState *current_tstate = _PyThreadState_GET();
    if (_PySys_Audit(current_tstate, "sys.settrace", NULL) < 0) {
        return -1;
    }

    assert(tstate->interp->sys_tracing_threads >= 0);
    /* Setup PEP 669 monitoring callbacks and events. */
    if (!tstate->interp->sys_trace_initialized) {
        tstate->interp->sys_trace_initialized = true;
        if (set_callbacks(PY_INSTRUMENT_SYS_TRACE,
            (vectorcallfunc)sys_trace_func2, PyTrace_CALL,
                        PY_MONITORING_EVENT_PY_START, PY_MONITORING_EVENT_PY_RESUME)) {
            return -1;
        }
        if (set_callbacks(PY_INSTRUMENT_SYS_TRACE,
            (vectorcallfunc)sys_trace_func2, PyTrace_CALL,
                        PY_MONITORING_EVENT_PY_THROW, -1)) {
            return -1;
        }
        if (set_callbacks(PY_INSTRUMENT_SYS_TRACE,
            (vectorcallfunc)sys_trace_func3, PyTrace_RETURN,
                        PY_MONITORING_EVENT_PY_RETURN, PY_MONITORING_EVENT_PY_YIELD)) {
            return -1;
        }
        if (set_callbacks(PY_INSTRUMENT_SYS_TRACE,
            (vectorcallfunc)sys_trace_exception_func, PyTrace_EXCEPTION,
                        PY_MONITORING_EVENT_RAISE, -1)) {
            return -1;
        }
        if (set_callbacks(PY_INSTRUMENT_SYS_TRACE,
            (vectorcallfunc)sys_trace_line_func, PyTrace_LINE,
                        PY_MONITORING_EVENT_LINE, -1)) {
            return -1;
        }
        if (set_callbacks(PY_INSTRUMENT_SYS_TRACE,
            (vectorcallfunc)sys_trace_func2, PyTrace_RETURN,
                        PY_MONITORING_EVENT_PY_UNWIND, -1)) {
            return -1;
        }
        if (set_callbacks(PY_INSTRUMENT_SYS_TRACE,
            (vectorcallfunc)sys_trace_jump_func, PyTrace_LINE,
                        PY_MONITORING_EVENT_JUMP, PY_MONITORING_EVENT_BRANCH)) {
            return -1;
        }
        if (set_callbacks(PY_INSTRUMENT_SYS_TRACE,
            (vectorcallfunc)sys_trace_instruction_func, PyTrace_OPCODE,
                        PY_MONITORING_EVENT_INSTRUCTION, -1)) {
            return -1;
        }
        /* TO DO: Set up callback for PY_MONITORING_EVENT_EXCEPTION_HANDLED */
    }

    int delta = (func != NULL) - (tstate->c_tracefunc != NULL);
    tstate->c_tracefunc = func;
    PyObject *old_traceobj = tstate->c_traceobj;
    tstate->c_traceobj = Py_XNewRef(arg);
    Py_XDECREF(old_traceobj);
    tstate->interp->sys_tracing_threads += delta;
    assert(tstate->interp->sys_tracing_threads >= 0);

    if (tstate->interp->sys_tracing_threads) {
        uint32_t events =
            (1 << PY_MONITORING_EVENT_PY_START) | (1 << PY_MONITORING_EVENT_PY_RESUME) |
            (1 << PY_MONITORING_EVENT_PY_RETURN) | (1 << PY_MONITORING_EVENT_PY_YIELD) |
            (1 << PY_MONITORING_EVENT_RAISE) | (1 << PY_MONITORING_EVENT_LINE) |
            (1 << PY_MONITORING_EVENT_JUMP) | (1 << PY_MONITORING_EVENT_BRANCH) |
            (1 << PY_MONITORING_EVENT_PY_UNWIND) | (1 << PY_MONITORING_EVENT_PY_THROW);
        if (tstate->interp->f_opcode_trace_set) {
            events |= (1 << PY_MONITORING_EVENT_INSTRUCTION);
        }
        _PyMonitoring_SetEvents(PY_INSTRUMENT_SYS_TRACE, events);
    }
    else {
        _PyMonitoring_SetEvents(PY_INSTRUMENT_SYS_TRACE, 0);
    }

    return 0;
}
