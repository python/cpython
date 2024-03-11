#include "parts.h"
#include "../_testcapi/util.h"  // NULLABLE, RETURN_INT

#include "pycore_instruments.h"


static PyObject *
fire_event_py_start(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset, active;
    if (!PyArg_ParseTuple(args, "Oii", &codelike, &offset, &active)) {
        return NULL;
    }
    NULLABLE(codelike);
    PyMonitoringState state = {active, 0};
    int res = _PyMonitoring_FirePyStartEvent(&state, codelike, offset);
    RETURN_INT(res == -1 ? -1 : state.active);
}

static PyObject *
fire_event_py_resume(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset, active;
    if (!PyArg_ParseTuple(args, "Oii", &codelike, &offset, &active)) {
        return NULL;
    }
    NULLABLE(codelike);
    PyMonitoringState state = {active, 0};
    int res = _PyMonitoring_FirePyResumeEvent(&state, codelike, offset);
    RETURN_INT(res == -1 ? -1 : state.active);
}

static PyObject *
fire_event_py_return(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset, active;
    PyObject *retval;
    if (!PyArg_ParseTuple(args, "OiiO", &codelike, &offset, &active, &retval)) {
        return NULL;
    }
    NULLABLE(codelike);
    PyMonitoringState state = {active, 0};
    int res = _PyMonitoring_FirePyReturnEvent(&state, codelike, offset, retval);
    RETURN_INT(res == -1 ? -1 : state.active);
}

static PyObject *
fire_event_py_yield(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset, active;
    PyObject *retval;
    if (!PyArg_ParseTuple(args, "OiiO", &codelike, &offset, &active, &retval)) {
        return NULL;
    }
    NULLABLE(codelike);
    PyMonitoringState state = {active, 0};
    int res = _PyMonitoring_FirePyYieldEvent(&state, codelike, offset, retval);
    RETURN_INT(res == -1 ? -1 : state.active);
}

static PyObject *
fire_event_call(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset, active;
    PyObject *callable, *arg0;
    if (!PyArg_ParseTuple(args, "OiiOO", &codelike, &offset, &active, &callable, &arg0)) {
        return NULL;
    }
    NULLABLE(callable);
    NULLABLE(arg0);
    PyMonitoringState state = {active, 0};
    int res = _PyMonitoring_FireCallEvent(&state, codelike, offset, callable, arg0);
    RETURN_INT(res == -1 ? -1 : state.active);
}

static PyObject *
fire_event_line(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset, active;
    PyObject *lineno;
    if (!PyArg_ParseTuple(args, "OiiO", &codelike, &offset, &active, &lineno)) {
        return NULL;
    }
    NULLABLE(lineno);
    PyMonitoringState state = {active, 0};
    int res = _PyMonitoring_FireLineEvent(&state, codelike, offset, lineno);
    RETURN_INT(res == -1 ? -1 : state.active);
}

static PyObject *
fire_event_jump(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset, active;
    PyObject *target_offset;
    if (!PyArg_ParseTuple(args, "OiiO", &codelike, &offset, &active, &target_offset)) {
        return NULL;
    }
    PyMonitoringState state = {active, 0};
    int res = _PyMonitoring_FireJumpEvent(&state, codelike, offset, target_offset);
    RETURN_INT(res == -1 ? -1 : state.active);
}

static PyObject *
fire_event_branch(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset, active;
    PyObject *target_offset;
    if (!PyArg_ParseTuple(args, "OiiO", &codelike, &offset, &active, &target_offset)) {
        return NULL;
    }
    PyMonitoringState state = {active, 0};
    int res = _PyMonitoring_FireBranchEvent(&state, codelike, offset, target_offset);
    RETURN_INT(res == -1 ? -1 : state.active);
}

static PyObject *
fire_event_py_throw(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset, active;
    PyObject *exception;
    if (!PyArg_ParseTuple(args, "OiiO", &codelike, &offset, &active, &exception)) {
        return NULL;
    }
    NULLABLE(exception);
    PyMonitoringState state = {active, 0};
    int res = _PyMonitoring_FirePyThrowEvent(&state, codelike, offset, exception);
    RETURN_INT(res == -1 ? -1 : state.active);
}

static PyObject *
fire_event_raise(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset, active;
    PyObject *exception;
    if (!PyArg_ParseTuple(args, "OiiO", &codelike, &offset, &active, &exception)) {
        return NULL;
    }
    NULLABLE(exception);
    PyMonitoringState state = {active, 0};
    int res = _PyMonitoring_FireRaiseEvent(&state, codelike, offset, exception);
    RETURN_INT(res == -1 ? -1 : state.active);
}

static PyObject *
fire_event_reraise(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset, active;
    PyObject *exception;
    if (!PyArg_ParseTuple(args, "OiiO", &codelike, &offset, &active, &exception)) {
        return NULL;
    }
    NULLABLE(exception);
    PyMonitoringState state = {active, 0};
    int res = _PyMonitoring_FireReraiseEvent(&state, codelike, offset, exception);
    RETURN_INT(res == -1 ? -1 : state.active);
}

static PyObject *
fire_event_exception_handled(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset, active;
    PyObject *exception;
    if (!PyArg_ParseTuple(args, "OiiO", &codelike, &offset, &active, &exception)) {
        return NULL;
    }
    NULLABLE(exception);
    PyMonitoringState state = {active, 0};
    int res = _PyMonitoring_FireExceptionHandledEvent(&state, codelike, offset, exception);
    RETURN_INT(res == -1 ? -1 : state.active);
}

static PyObject *
fire_event_py_unwind(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset, active;
    PyObject *exception;
    if (!PyArg_ParseTuple(args, "OiiO", &codelike, &offset, &active, &exception)) {
        return NULL;
    }
    NULLABLE(exception);
    PyMonitoringState state = {active, 0};
    int res = _PyMonitoring_FirePyUnwindEvent(&state, codelike, offset, exception);
    RETURN_INT(res == -1 ? -1 : state.active);
}

static PyObject *
fire_event_stop_iteration(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset, active;
    PyObject *exception;
    if (!PyArg_ParseTuple(args, "OiiO", &codelike, &offset, &active, &exception)) {
        return NULL;
    }
    NULLABLE(exception);
    PyMonitoringState state = {active, 0};
    int res = _PyMonitoring_FireStopIterationEvent(&state, codelike, offset, exception);
    RETURN_INT(res == -1 ? -1 : state.active);
}

typedef struct {
    PyObject_HEAD
    PyMonitoringState *monitoring_states;
    uint64_t version;
    int num_events;
    /* Other fields */
} PyCodeLikeObject;


static PyObject *
CodeLike_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int num_events;
    if (!PyArg_ParseTuple(args, "i", &num_events)) {
        return NULL;
    }
    PyMonitoringState *states = (PyMonitoringState *)PyMem_Calloc(
            num_events, sizeof(PyMonitoringState));
    if (states == NULL) {
        return NULL;
    }
    PyCodeLikeObject *self = (PyCodeLikeObject *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->version = 0;
        self->monitoring_states = states;
        self->num_events = num_events;
    }
    else {
        PyMem_Free(states);
    }
    return (PyObject *) self;
}

static void
CodeLike_dealloc(PyCodeLikeObject *self)
{
    if (self->monitoring_states) {
        PyMem_Free(self->monitoring_states);
    }
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
CodeLike_str(PyCodeLikeObject *self)
{
    if (self->monitoring_states) {
        PyObject *res = NULL;
        PyObject *sep = NULL;
        PyObject *parts = NULL;

        parts = PyList_New(0);
        if (parts == NULL) {
            goto end;
        }

        PyObject *heading = PyUnicode_FromString("PyCodeLikeObject");
        if (heading == NULL) {
            goto end;
        }
        int err = PyList_Append(parts, heading);
        Py_DECREF(heading);
        if (err < 0) {
            goto end;
        }

        for (int i = 0; i < self->num_events; i++) {
            PyObject *part = PyUnicode_FromFormat("(active=%dl, opaque=%dl)",
                                                  self->monitoring_states[i].active,
                                                  self->monitoring_states[i].opaque);
            if (part == NULL) {
                goto end;
            }
            int err = PyList_Append(parts, part);
            Py_XDECREF(part);
            if (err < 0) {
                goto end;
            }
        }
        sep = PyUnicode_FromString(" ");
        if (sep == NULL) {
            goto end;
        }
        res = PyUnicode_Join(sep, parts);
end:
        Py_XDECREF(sep);
        Py_XDECREF(parts);
        return res;
    }
    else {
        return PyUnicode_FromString("PyCodeLikeObject");
    }
}

static PyTypeObject PyCodeLike_Type = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "monitoring.CodeLike",
    .tp_doc = PyDoc_STR("CodeLike objects"),
    .tp_basicsize = sizeof(PyCodeLikeObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = CodeLike_new,
    .tp_dealloc = (destructor) CodeLike_dealloc,
    .tp_str = (reprfunc) CodeLike_str,
};

static PyObject *
enter_scope_py_start_py_return(PyObject *self, PyObject *args)
{
    PyObject *codelike_obj;
    if (!PyArg_ParseTuple(args, "O", &codelike_obj)) {
        return NULL;
    }
    PyCodeLikeObject *codelike = (PyCodeLikeObject *) codelike_obj;

    uint8_t events[] = { PY_MONITORING_EVENT_PY_START, PY_MONITORING_EVENT_PY_RETURN };
    _PyMonitoringScopeBegin(codelike->monitoring_states, &codelike->version, events, 2);

    Py_RETURN_NONE;
}

static PyMethodDef TestMethods[] = {
    {"fire_event_py_start", fire_event_py_start, METH_VARARGS},
    {"fire_event_py_resume", fire_event_py_resume, METH_VARARGS},
    {"fire_event_py_return", fire_event_py_return, METH_VARARGS},
    {"fire_event_py_yield", fire_event_py_yield, METH_VARARGS},
    {"fire_event_call", fire_event_call, METH_VARARGS},
    {"fire_event_line", fire_event_line, METH_VARARGS},
    {"fire_event_jump", fire_event_jump, METH_VARARGS},
    {"fire_event_branch", fire_event_branch, METH_VARARGS},
    {"fire_event_py_throw", fire_event_py_throw, METH_VARARGS},
    {"fire_event_raise", fire_event_raise, METH_VARARGS},
    {"fire_event_reraise", fire_event_reraise, METH_VARARGS},
    {"fire_event_exception_handled", fire_event_exception_handled, METH_VARARGS},
    {"fire_event_py_unwind", fire_event_py_unwind, METH_VARARGS},
    {"fire_event_stop_iteration", fire_event_stop_iteration, METH_VARARGS},
    {"enter_scope_py_start_py_return", enter_scope_py_start_py_return, METH_VARARGS},
    {NULL},
};

int
_PyTestInternalCapi_Init_Monitoring(PyObject *m)
{
    if (PyType_Ready(&PyCodeLike_Type) < 0) {
        return NULL;
    }
    if (PyModule_AddObjectRef(m, "CodeLike", (PyObject *) &PyCodeLike_Type) < 0) {
        Py_DECREF(m);
        return NULL;
    }
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }
    return 0;
}
