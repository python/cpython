#ifndef Py_MONITORING_H
#define Py_MONITORING_H
#ifndef Py_LIMITED_API
#ifdef __cplusplus
extern "C" {
#endif

// There is currently no limited API for monitoring


/* Local events.
 * These require bytecode instrumentation */

#define PY_MONITORING_EVENT_PY_START 0
#define PY_MONITORING_EVENT_PY_RESUME 1
#define PY_MONITORING_EVENT_PY_RETURN 2
#define PY_MONITORING_EVENT_PY_YIELD 3
#define PY_MONITORING_EVENT_CALL 4
#define PY_MONITORING_EVENT_LINE 5
#define PY_MONITORING_EVENT_INSTRUCTION 6
#define PY_MONITORING_EVENT_JUMP 7
#define PY_MONITORING_EVENT_BRANCH_LEFT 8
#define PY_MONITORING_EVENT_BRANCH_RIGHT 9
#define PY_MONITORING_EVENT_STOP_ITERATION 10

#define PY_MONITORING_IS_INSTRUMENTED_EVENT(ev) \
    ((ev) < _PY_MONITORING_LOCAL_EVENTS)

/* Other events, mainly exceptions */

#define PY_MONITORING_EVENT_RAISE 11
#define PY_MONITORING_EVENT_EXCEPTION_HANDLED 12
#define PY_MONITORING_EVENT_PY_UNWIND 13
#define PY_MONITORING_EVENT_PY_THROW 14
#define PY_MONITORING_EVENT_RERAISE 15


/* Ancillary events */

#define PY_MONITORING_EVENT_C_RETURN 16
#define PY_MONITORING_EVENT_C_RAISE 17
#define PY_MONITORING_EVENT_BRANCH 18


typedef struct _PyMonitoringState {
    uint8_t active;
    uint8_t opaque;
} PyMonitoringState;


PyAPI_FUNC(int)
PyMonitoring_EnterScope(PyMonitoringState *state_array, uint64_t *version,
                         const uint8_t *event_types, Py_ssize_t length);

PyAPI_FUNC(int)
PyMonitoring_ExitScope(void);


PyAPI_FUNC(int)
_PyMonitoring_FirePyStartEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset);

PyAPI_FUNC(int)
_PyMonitoring_FirePyResumeEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset);

PyAPI_FUNC(int)
_PyMonitoring_FirePyReturnEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                                PyObject *retval);

PyAPI_FUNC(int)
_PyMonitoring_FirePyYieldEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                               PyObject *retval);

PyAPI_FUNC(int)
_PyMonitoring_FireCallEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                            PyObject* callable, PyObject *arg0);

PyAPI_FUNC(int)
_PyMonitoring_FireLineEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                            int lineno);

PyAPI_FUNC(int)
_PyMonitoring_FireJumpEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                            PyObject *target_offset);

Py_DEPRECATED(3.14) PyAPI_FUNC(int)
_PyMonitoring_FireBranchEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                              PyObject *target_offset);

PyAPI_FUNC(int)
_PyMonitoring_FireBranchRightEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                              PyObject *target_offset);

PyAPI_FUNC(int)
_PyMonitoring_FireBranchLeftEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                              PyObject *target_offset);

PyAPI_FUNC(int)
_PyMonitoring_FireCReturnEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                               PyObject *retval);

PyAPI_FUNC(int)
_PyMonitoring_FirePyThrowEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset);

PyAPI_FUNC(int)
_PyMonitoring_FireRaiseEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset);

PyAPI_FUNC(int)
_PyMonitoring_FireReraiseEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset);

PyAPI_FUNC(int)
_PyMonitoring_FireExceptionHandledEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset);

PyAPI_FUNC(int)
_PyMonitoring_FireCRaiseEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset);

PyAPI_FUNC(int)
_PyMonitoring_FirePyUnwindEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset);

PyAPI_FUNC(int)
_PyMonitoring_FireStopIterationEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject *value);


#define _PYMONITORING_IF_ACTIVE(STATE, X)  \
    if ((STATE)->active) { \
        return (X); \
    } \
    else { \
        return 0; \
    }

static inline int
PyMonitoring_FirePyStartEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FirePyStartEvent(state, codelike, offset));
}

static inline int
PyMonitoring_FirePyResumeEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FirePyResumeEvent(state, codelike, offset));
}

static inline int
PyMonitoring_FirePyReturnEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                               PyObject *retval)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FirePyReturnEvent(state, codelike, offset, retval));
}

static inline int
PyMonitoring_FirePyYieldEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                              PyObject *retval)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FirePyYieldEvent(state, codelike, offset, retval));
}

static inline int
PyMonitoring_FireCallEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                           PyObject* callable, PyObject *arg0)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireCallEvent(state, codelike, offset, callable, arg0));
}

static inline int
PyMonitoring_FireLineEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                           int lineno)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireLineEvent(state, codelike, offset, lineno));
}

static inline int
PyMonitoring_FireJumpEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                           PyObject *target_offset)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireJumpEvent(state, codelike, offset, target_offset));
}

static inline int
PyMonitoring_FireBranchRightEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                             PyObject *target_offset)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireBranchRightEvent(state, codelike, offset, target_offset));
}

static inline int
PyMonitoring_FireBranchLeftEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                             PyObject *target_offset)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireBranchLeftEvent(state, codelike, offset, target_offset));
}

static inline int
PyMonitoring_FireCReturnEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                              PyObject *retval)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireCReturnEvent(state, codelike, offset, retval));
}

static inline int
PyMonitoring_FirePyThrowEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FirePyThrowEvent(state, codelike, offset));
}

static inline int
PyMonitoring_FireRaiseEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireRaiseEvent(state, codelike, offset));
}

static inline int
PyMonitoring_FireReraiseEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireReraiseEvent(state, codelike, offset));
}

static inline int
PyMonitoring_FireExceptionHandledEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireExceptionHandledEvent(state, codelike, offset));
}

static inline int
PyMonitoring_FireCRaiseEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireCRaiseEvent(state, codelike, offset));
}

static inline int
PyMonitoring_FirePyUnwindEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FirePyUnwindEvent(state, codelike, offset));
}

static inline int
PyMonitoring_FireStopIterationEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject *value)
{
    _PYMONITORING_IF_ACTIVE(
        state,
        _PyMonitoring_FireStopIterationEvent(state, codelike, offset, value));
}

#undef _PYMONITORING_IF_ACTIVE

#ifdef __cplusplus
}
#endif
#endif  // !Py_LIMITED_API
#endif  // !Py_MONITORING_H
