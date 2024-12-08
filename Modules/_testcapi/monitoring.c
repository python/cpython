#include "parts.h"
#include "util.h"

#include "monitoring.h"

#define Py_BUILD_CORE
#include "internal/pycore_instruments.h"

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
    PyObject *res = NULL;
    PyObject *sep = NULL;
    PyObject *parts = NULL;
    if (self->monitoring_states) {
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
            PyObject *part = PyUnicode_FromFormat(" %d", self->monitoring_states[i].active);
            if (part == NULL) {
                goto end;
            }
            int err = PyList_Append(parts, part);
            Py_XDECREF(part);
            if (err < 0) {
                goto end;
            }
        }
        sep = PyUnicode_FromString(": ");
        if (sep == NULL) {
            goto end;
        }
        res = PyUnicode_Join(sep, parts);
    }
end:
    Py_XDECREF(sep);
    Py_XDECREF(parts);
    return res;
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

#define RAISE_UNLESS_CODELIKE(v)  if (!Py_IS_TYPE((v), &PyCodeLike_Type)) { \
        PyErr_Format(PyExc_TypeError, "expected a code-like, got %s", Py_TYPE(v)->tp_name); \
        return NULL; \
    }

/*******************************************************************/

static PyMonitoringState *
setup_fire(PyObject *codelike, int offset, PyObject *exc)
{
    RAISE_UNLESS_CODELIKE(codelike);
    PyCodeLikeObject *cl = ((PyCodeLikeObject *)codelike);
    assert(offset >= 0 && offset < cl->num_events);
    PyMonitoringState *state = &cl->monitoring_states[offset];

    if (exc != NULL) {
        PyErr_SetRaisedException(Py_NewRef(exc));
    }
    return state;
}

static int
teardown_fire(int res, PyMonitoringState *state, PyObject *exception)
{
    if (res == -1) {
        return -1;
    }
    if (exception) {
        assert(PyErr_Occurred());
        assert(((PyObject*)Py_TYPE(exception)) == PyErr_Occurred());
    }

    else {
        assert(!PyErr_Occurred());
    }
    PyErr_Clear();
    return state->active;
}

static PyObject *
fire_event_py_start(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset;
    if (!PyArg_ParseTuple(args, "Oi", &codelike, &offset)) {
        return NULL;
    }
    PyObject *exception = NULL;
    PyMonitoringState *state = setup_fire(codelike, offset, exception);
    if (state == NULL) {
        return NULL;
    }
    int res = PyMonitoring_FirePyStartEvent(state, codelike, offset);
    RETURN_INT(teardown_fire(res, state, exception));
}

static PyObject *
fire_event_py_resume(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset;
    if (!PyArg_ParseTuple(args, "Oi", &codelike, &offset)) {
        return NULL;
    }
    PyObject *exception = NULL;
    PyMonitoringState *state = setup_fire(codelike, offset, exception);
    if (state == NULL) {
        return NULL;
    }
    int res = PyMonitoring_FirePyResumeEvent(state, codelike, offset);
    RETURN_INT(teardown_fire(res, state, exception));
}

static PyObject *
fire_event_py_return(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset;
    PyObject *retval;
    if (!PyArg_ParseTuple(args, "OiO", &codelike, &offset, &retval)) {
        return NULL;
    }
    PyObject *exception = NULL;
    PyMonitoringState *state = setup_fire(codelike, offset, exception);
    if (state == NULL) {
        return NULL;
    }
    int res = PyMonitoring_FirePyReturnEvent(state, codelike, offset, retval);
    RETURN_INT(teardown_fire(res, state, exception));
}

static PyObject *
fire_event_c_return(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset;
    PyObject *retval;
    if (!PyArg_ParseTuple(args, "OiO", &codelike, &offset, &retval)) {
        return NULL;
    }
    PyObject *exception = NULL;
    PyMonitoringState *state = setup_fire(codelike, offset, exception);
    if (state == NULL) {
        return NULL;
    }
    int res = PyMonitoring_FireCReturnEvent(state, codelike, offset, retval);
    RETURN_INT(teardown_fire(res, state, exception));
}

static PyObject *
fire_event_py_yield(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset;
    PyObject *retval;
    if (!PyArg_ParseTuple(args, "OiO", &codelike, &offset, &retval)) {
        return NULL;
    }
    PyObject *exception = NULL;
    PyMonitoringState *state = setup_fire(codelike, offset, exception);
    if (state == NULL) {
        return NULL;
    }
    int res = PyMonitoring_FirePyYieldEvent(state, codelike, offset, retval);
    RETURN_INT(teardown_fire(res, state, exception));
}

static PyObject *
fire_event_call(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset;
    PyObject *callable, *arg0;
    if (!PyArg_ParseTuple(args, "OiOO", &codelike, &offset, &callable, &arg0)) {
        return NULL;
    }
    PyObject *exception = NULL;
    PyMonitoringState *state = setup_fire(codelike, offset, exception);
    if (state == NULL) {
        return NULL;
    }
    int res = PyMonitoring_FireCallEvent(state, codelike, offset, callable, arg0);
    RETURN_INT(teardown_fire(res, state, exception));
}

static PyObject *
fire_event_line(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset, lineno;
    if (!PyArg_ParseTuple(args, "Oii", &codelike, &offset, &lineno)) {
        return NULL;
    }
    PyObject *exception = NULL;
    PyMonitoringState *state = setup_fire(codelike, offset, exception);
    if (state == NULL) {
        return NULL;
    }
    int res = PyMonitoring_FireLineEvent(state, codelike, offset, lineno);
    RETURN_INT(teardown_fire(res, state, exception));
}

static PyObject *
fire_event_jump(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset;
    PyObject *target_offset;
    if (!PyArg_ParseTuple(args, "OiO", &codelike, &offset, &target_offset)) {
        return NULL;
    }
    PyObject *exception = NULL;
    PyMonitoringState *state = setup_fire(codelike, offset, exception);
    if (state == NULL) {
        return NULL;
    }
    int res = PyMonitoring_FireJumpEvent(state, codelike, offset, target_offset);
    RETURN_INT(teardown_fire(res, state, exception));
}

static PyObject *
fire_event_branch(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset;
    PyObject *target_offset;
    if (!PyArg_ParseTuple(args, "OiO", &codelike, &offset, &target_offset)) {
        return NULL;
    }
    PyObject *exception = NULL;
    PyMonitoringState *state = setup_fire(codelike, offset, exception);
    if (state == NULL) {
        return NULL;
    }
    int res = PyMonitoring_FireBranchEvent(state, codelike, offset, target_offset);
    RETURN_INT(teardown_fire(res, state, exception));
}

static PyObject *
fire_event_py_throw(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset;
    PyObject *exception;
    if (!PyArg_ParseTuple(args, "OiO", &codelike, &offset, &exception)) {
        return NULL;
    }
    NULLABLE(exception);
    PyMonitoringState *state = setup_fire(codelike, offset, exception);
    if (state == NULL) {
        return NULL;
    }
    int res = PyMonitoring_FirePyThrowEvent(state, codelike, offset);
    RETURN_INT(teardown_fire(res, state, exception));
}

static PyObject *
fire_event_raise(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset;
    PyObject *exception;
    if (!PyArg_ParseTuple(args, "OiO", &codelike, &offset, &exception)) {
        return NULL;
    }
    NULLABLE(exception);
    PyMonitoringState *state = setup_fire(codelike, offset, exception);
    if (state == NULL) {
        return NULL;
    }
    int res = PyMonitoring_FireRaiseEvent(state, codelike, offset);
    RETURN_INT(teardown_fire(res, state, exception));
}

static PyObject *
fire_event_c_raise(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset;
    PyObject *exception;
    if (!PyArg_ParseTuple(args, "OiO", &codelike, &offset, &exception)) {
        return NULL;
    }
    NULLABLE(exception);
    PyMonitoringState *state = setup_fire(codelike, offset, exception);
    if (state == NULL) {
        return NULL;
    }
    int res = PyMonitoring_FireCRaiseEvent(state, codelike, offset);
    RETURN_INT(teardown_fire(res, state, exception));
}

static PyObject *
fire_event_reraise(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset;
    PyObject *exception;
    if (!PyArg_ParseTuple(args, "OiO", &codelike, &offset, &exception)) {
        return NULL;
    }
    NULLABLE(exception);
    PyMonitoringState *state = setup_fire(codelike, offset, exception);
    if (state == NULL) {
        return NULL;
    }
    int res = PyMonitoring_FireReraiseEvent(state, codelike, offset);
    RETURN_INT(teardown_fire(res, state, exception));
}

static PyObject *
fire_event_exception_handled(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset;
    PyObject *exception;
    if (!PyArg_ParseTuple(args, "OiO", &codelike, &offset, &exception)) {
        return NULL;
    }
    NULLABLE(exception);
    PyMonitoringState *state = setup_fire(codelike, offset, exception);
    if (state == NULL) {
        return NULL;
    }
    int res = PyMonitoring_FireExceptionHandledEvent(state, codelike, offset);
    RETURN_INT(teardown_fire(res, state, exception));
}

static PyObject *
fire_event_py_unwind(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset;
    PyObject *exception;
    if (!PyArg_ParseTuple(args, "OiO", &codelike, &offset, &exception)) {
        return NULL;
    }
    NULLABLE(exception);
    PyMonitoringState *state = setup_fire(codelike, offset, exception);
    if (state == NULL) {
        return NULL;
    }
    int res = PyMonitoring_FirePyUnwindEvent(state, codelike, offset);
    RETURN_INT(teardown_fire(res, state, exception));
}

static PyObject *
fire_event_stop_iteration(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int offset;
    PyObject *value;
    if (!PyArg_ParseTuple(args, "OiO", &codelike, &offset, &value)) {
        return NULL;
    }
    NULLABLE(value);
    PyObject *exception = NULL;
    PyMonitoringState *state = setup_fire(codelike, offset, exception);
    if (state == NULL) {
        return NULL;
    }
    int res = PyMonitoring_FireStopIterationEvent(state, codelike, offset, value);
    RETURN_INT(teardown_fire(res, state, exception));
}

/*******************************************************************/

static PyObject *
enter_scope(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    int event1, event2=0;
    Py_ssize_t num_events = PyTuple_Size(args) - 1;
    if (num_events == 1) {
        if (!PyArg_ParseTuple(args, "Oi", &codelike, &event1)) {
            return NULL;
        }
    }
    else {
        assert(num_events == 2);
        if (!PyArg_ParseTuple(args, "Oii", &codelike, &event1, &event2)) {
            return NULL;
        }
    }
    RAISE_UNLESS_CODELIKE(codelike);
    PyCodeLikeObject *cl = (PyCodeLikeObject *) codelike;

    uint8_t events[] = { event1, event2 };

    PyMonitoring_EnterScope(cl->monitoring_states,
                            &cl->version,
                            events,
                            num_events);

    Py_RETURN_NONE;
}

static PyObject *
exit_scope(PyObject *self, PyObject *args)
{
    PyMonitoring_ExitScope();
    Py_RETURN_NONE;
}

static PyMethodDef TestMethods[] = {
    {"fire_event_py_start", fire_event_py_start, METH_VARARGS},
    {"fire_event_py_resume", fire_event_py_resume, METH_VARARGS},
    {"fire_event_py_return", fire_event_py_return, METH_VARARGS},
    {"fire_event_c_return", fire_event_c_return, METH_VARARGS},
    {"fire_event_py_yield", fire_event_py_yield, METH_VARARGS},
    {"fire_event_call", fire_event_call, METH_VARARGS},
    {"fire_event_line", fire_event_line, METH_VARARGS},
    {"fire_event_jump", fire_event_jump, METH_VARARGS},
    {"fire_event_branch", fire_event_branch, METH_VARARGS},
    {"fire_event_py_throw", fire_event_py_throw, METH_VARARGS},
    {"fire_event_raise", fire_event_raise, METH_VARARGS},
    {"fire_event_c_raise", fire_event_c_raise, METH_VARARGS},
    {"fire_event_reraise", fire_event_reraise, METH_VARARGS},
    {"fire_event_exception_handled", fire_event_exception_handled, METH_VARARGS},
    {"fire_event_py_unwind", fire_event_py_unwind, METH_VARARGS},
    {"fire_event_stop_iteration", fire_event_stop_iteration, METH_VARARGS},
    {"monitoring_enter_scope", enter_scope, METH_VARARGS},
    {"monitoring_exit_scope", exit_scope, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Monitoring(PyObject *m)
{
    if (PyType_Ready(&PyCodeLike_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "CodeLike", (PyObject *) &PyCodeLike_Type) < 0) {
        Py_DECREF(m);
        return -1;
    }
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }
    return 0;
}
