#ifndef Py_MONITORING_H
#define Py_MONITORING_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PyMonitoringState {
    uint8_t active;
    uint8_t opaque;
} PyMonitoringState;


#  define Py_CPYTHON_MONITORING_H
#  include "cpython/monitoring.h"
#  undef Py_CPYTHON_MONITORING_H


#ifndef Py_LIMITED_API

#define _PYMONITORING_IF_ACTIVE(STATE, X)  \
    if ((STATE)->active) { \
        return (X); \
    } \
    else { \
        return 0; \
    }


static inline int
PyMonitoring_FirePyStartEvent(PyMonitoringState *state, PyObject *codelike, int offset)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FirePyStartEvent(state, codelike, offset));
}

static inline int
PyMonitoring_FirePyResumeEvent(PyMonitoringState *state, PyObject *codelike, int offset)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FirePyResumeEvent(state, codelike, offset));
}

static inline int
PyMonitoring_FirePyReturnEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                               PyObject *retval)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FirePyReturnEvent(state, codelike, offset, retval));
}

static inline int
PyMonitoring_FirePyYieldEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                              PyObject *retval)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FirePyYieldEvent(state, codelike, offset, retval));
}

static inline int
PyMonitoring_FireCallEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                           PyObject* callable, PyObject *arg0)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireCallEvent(state, codelike, offset, callable, arg0));
}

static inline int
PyMonitoring_FireLineEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                           int lineno)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireLineEvent(state, codelike, offset, lineno));
}

static inline int
PyMonitoring_FireInstructionEvent(PyMonitoringState *state, PyObject *codelike, int offset)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireInstructionEvent(state, codelike, offset));
}

static inline int
PyMonitoring_FireJumpEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                           PyObject *target_offset)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireJumpEvent(state, codelike, offset, target_offset));
}

static inline int
PyMonitoring_FireBranchEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                             PyObject *target_offset)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireBranchEvent(state, codelike, offset, target_offset));
}

static inline int
PyMonitoring_FireCReturnEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                              PyObject *retval)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireCReturnEvent(state, codelike, offset, retval));
}

static inline int
PyMonitoring_FirePyThrowEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                              PyObject *exception)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FirePyThrowEvent(state, codelike, offset, exception));
}

static inline int
PyMonitoring_FireRaiseEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                            PyObject *exception)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireRaiseEvent(state, codelike, offset, exception));
}

static inline int
PyMonitoring_FireReraiseEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                              PyObject *exception)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireReraiseEvent(state, codelike, offset, exception));
}

static inline int
PyMonitoring_FireExceptionHandledEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                       PyObject *exception)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireExceptionHandledEvent(state, codelike, offset, exception));
}

static inline int
PyMonitoring_FireCRaiseEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                             PyObject *exception)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireCRaiseEvent(state, codelike, offset, exception));
}

static inline int
PyMonitoring_FirePyUnwindEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                               PyObject *exception)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FirePyUnwindEvent(state, codelike, offset, exception));
}

static inline int
PyMonitoring_FireStopIterationEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                    PyObject *exception)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireStopIterationEvent(state, codelike, offset, exception));
}

#undef _PYMONITORING_IF_ACTIVE
#endif


#ifdef __cplusplus
}
#endif
#endif /* !Py_MONITORING_H */
