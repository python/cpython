#include "parts.h"
#include "../_testcapi/util.h"  // NULLABLE, RETURN_INT

#include "pycore_instruments.h"


static PyObject *
fire_event_py_start(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    uint32_t offset;
    uint8_t active;
    if (!PyArg_ParseTuple(args, "Oii", &codelike, &offset, &active)) {
        return NULL;
    }
    NULLABLE(codelike);
    PyMonitoringState state = {active, 0};
    RETURN_INT(_PyMonitoring_FirePyStartEvent(&state, codelike, offset));
}

static PyObject *
fire_event_py_resume(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    uint32_t offset;
    uint8_t active;
    if (!PyArg_ParseTuple(args, "Oii", &codelike, &offset, &active)) {
        return NULL;
    }
    NULLABLE(codelike);
    PyMonitoringState state = {active, 0};
    RETURN_INT(_PyMonitoring_FirePyResumeEvent(&state, codelike, offset));
}

static PyObject *
fire_event_py_return(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    uint32_t offset;
    uint8_t active;
    PyObject *retval;
    if (!PyArg_ParseTuple(args, "OiiO", &codelike, &offset, &active, &retval)) {
        return NULL;
    }
    NULLABLE(codelike);
    PyMonitoringState state = {active, 0};
    RETURN_INT(_PyMonitoring_FirePyReturnEvent(&state, codelike, offset, retval));
}

static PyObject *
fire_event_py_yield(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    uint32_t offset;
    uint8_t active;
    PyObject *retval;
    if (!PyArg_ParseTuple(args, "OiiO", &codelike, &offset, &active, &retval)) {
        return NULL;
    }
    NULLABLE(codelike);
    PyMonitoringState state = {active, 0};
    RETURN_INT(_PyMonitoring_FirePyYieldEvent(&state, codelike, offset, retval));
}

static PyObject *
fire_event_py_call(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    uint32_t offset;
    uint8_t active;
    PyObject *callable, *arg0;
    if (!PyArg_ParseTuple(args, "OiiOO", &codelike, &offset, &active, &callable, &arg0)) {
        return NULL;
    }
    NULLABLE(callable);
    NULLABLE(arg0);
    PyMonitoringState state = {active, 0};
    RETURN_INT(_PyMonitoring_FirePyCallEvent(&state, codelike, offset, callable, arg0));
}

static PyObject *
fire_event_call(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    uint32_t offset;
    uint8_t active;
    PyObject *callable, *arg0;
    if (!PyArg_ParseTuple(args, "OiiOO", &codelike, &offset, &active, &callable, &arg0)) {
        return NULL;
    }
    NULLABLE(callable);
    NULLABLE(arg0);
    PyMonitoringState state = {active, 0};
    RETURN_INT(_PyMonitoring_FireCallEvent(&state, codelike, offset, callable, arg0));
}

static PyObject *
fire_event_line(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    uint32_t offset;
    uint8_t active;
    PyObject *lineno;
    if (!PyArg_ParseTuple(args, "OiiO", &codelike, &offset, &active, &lineno)) {
        return NULL;
    }
    NULLABLE(lineno);
    PyMonitoringState state = {active, 0};
    RETURN_INT(_PyMonitoring_FireLineEvent(&state, codelike, offset, lineno));
}

static PyObject *
fire_event_instruction(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    uint32_t offset;
    uint8_t active;
    if (!PyArg_ParseTuple(args, "Oii", &codelike, &offset, &active)) {
        return NULL;
    }
    PyMonitoringState state = {active, 0};
    RETURN_INT(_PyMonitoring_FireInstructionEvent(&state, codelike, offset));
}

static PyObject *
fire_event_jump(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    uint32_t offset;
    uint8_t active;
    PyObject *target_offset;
    if (!PyArg_ParseTuple(args, "OiiO", &codelike, &offset, &active, &target_offset)) {
        return NULL;
    }
    PyMonitoringState state = {active, 0};
    RETURN_INT(_PyMonitoring_FireJumpEvent(&state, codelike, offset, target_offset));
}

static PyObject *
fire_event_branch(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    uint32_t offset;
    uint8_t active;
    PyObject *target_offset;
    if (!PyArg_ParseTuple(args, "OiiO", &codelike, &offset, &active, &target_offset)) {
        return NULL;
    }
    PyMonitoringState state = {active, 0};
    RETURN_INT(_PyMonitoring_FireBranchEvent(&state, codelike, offset, target_offset));
}

static PyObject *
fire_event_c_return(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    uint32_t offset;
    uint8_t active;
    PyObject *callable, *arg0;
    if (!PyArg_ParseTuple(args, "OiiOO", &codelike, &offset, &active, &callable, &arg0)) {
        return NULL;
    }
    NULLABLE(callable);
    NULLABLE(arg0);
    PyMonitoringState state = {active, 0};
    RETURN_INT(_PyMonitoring_FireCReturnEvent(&state, codelike, offset, callable, arg0));
}

static PyObject *
fire_event_py_throw(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    uint32_t offset;
    uint8_t active;
    PyObject *exception;
    if (!PyArg_ParseTuple(args, "OiiOO", &codelike, &offset, &active, &exception)) {
        return NULL;
    }
    NULLABLE(exception);
    PyMonitoringState state = {active, 0};
    RETURN_INT(_PyMonitoring_FirePyThrowEvent(&state, codelike, offset, exception));
}

static PyObject *
fire_event_raise(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    uint32_t offset;
    uint8_t active;
    PyObject *exception;
    if (!PyArg_ParseTuple(args, "OiiOO", &codelike, &offset, &active, &exception)) {
        return NULL;
    }
    NULLABLE(exception);
    PyMonitoringState state = {active, 0};
    RETURN_INT(_PyMonitoring_FireRaiseEvent(&state, codelike, offset, exception));
}

static PyObject *
fire_event_reraise(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    uint32_t offset;
    uint8_t active;
    PyObject *exception;
    if (!PyArg_ParseTuple(args, "OiiOO", &codelike, &offset, &active, &exception)) {
        return NULL;
    }
    NULLABLE(exception);
    PyMonitoringState state = {active, 0};
    RETURN_INT(_PyMonitoring_FireReraiseEvent(&state, codelike, offset, exception));
}

static PyObject *
fire_event_exception_handled(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    uint32_t offset;
    uint8_t active;
    PyObject *exception;
    if (!PyArg_ParseTuple(args, "OiiOO", &codelike, &offset, &active, &exception)) {
        return NULL;
    }
    NULLABLE(exception);
    PyMonitoringState state = {active, 0};
    RETURN_INT(_PyMonitoring_FireExceptionHandledEvent(&state, codelike, offset, exception));
}

static PyObject *
fire_event_c_raise(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    uint32_t offset;
    uint8_t active;
    PyObject *callable, *arg0;
    if (!PyArg_ParseTuple(args, "OiiOO", &codelike, &offset, &active, &callable, &arg0)) {
        return NULL;
    }
    NULLABLE(callable);
    NULLABLE(arg0);
    PyMonitoringState state = {active, 0};
    RETURN_INT(_PyMonitoring_FireCRaiseEvent(&state, codelike, offset, callable, arg0));
}

static PyObject *
fire_event_py_unwind(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    uint32_t offset;
    uint8_t active;
    PyObject *exception;
    if (!PyArg_ParseTuple(args, "OiiOO", &codelike, &offset, &active, &exception)) {
        return NULL;
    }
    NULLABLE(exception);
    PyMonitoringState state = {active, 0};
    RETURN_INT(_PyMonitoring_FirePyUnwindEvent(&state, codelike, offset, exception));
}

static PyObject *
fire_event_stop_iteration(PyObject *self, PyObject *args)
{
    PyObject *codelike;
    uint32_t offset;
    uint8_t active;
    PyObject *exception;
    if (!PyArg_ParseTuple(args, "OiiOO", &codelike, &offset, &active, &exception)) {
        return NULL;
    }
    NULLABLE(exception);
    PyMonitoringState state = {active, 0};
    RETURN_INT(_PyMonitoring_FireStopIterationEvent(&state, codelike, offset, exception));
}

static PyMethodDef TestMethods[] = {
    {"fire_event_py_start", fire_event_py_start, METH_VARARGS},
    {"fire_event_py_resume", fire_event_py_resume, METH_VARARGS},
    {"fire_event_py_return", fire_event_py_return, METH_VARARGS},
    {"fire_event_py_yield", fire_event_py_yield, METH_VARARGS},
    {"fire_event_py_call", fire_event_py_call, METH_VARARGS},
    {"fire_event_call", fire_event_call, METH_VARARGS},
    {"fire_event_line", fire_event_line, METH_VARARGS},
    {"fire_event_instruction", fire_event_instruction, METH_VARARGS},
    {"fire_event_jump", fire_event_jump, METH_VARARGS},
    {"fire_event_branch", fire_event_branch, METH_VARARGS},
    {"fire_event_c_return", fire_event_c_return, METH_VARARGS},
    {"fire_event_py_throw", fire_event_py_throw, METH_VARARGS},
    {"fire_event_raise", fire_event_raise, METH_VARARGS},
    {"fire_event_reraise", fire_event_reraise, METH_VARARGS},
    {"fire_event_exception_handled", fire_event_exception_handled, METH_VARARGS},
    {"fire_event_c_raise", fire_event_c_raise, METH_VARARGS},
    {"fire_event_py_unwind", fire_event_py_unwind, METH_VARARGS},
    {"fire_event_stop_iteration", fire_event_stop_iteration, METH_VARARGS},
    {NULL},
};

int
_PyTestInternalCapi_Init_Monitoring(PyObject *m)
{
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }
    return 0;
}
