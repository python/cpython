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

#define _PyMonitoring_IF_ACTIVE(STATE, X)  \
    if ((STATE)->active) { \
        return (X); \
    } \
    else { \
        return 0; \
    }


#ifndef Py_LIMITED_API
static inline void
PyUnstable_Monitoring_EnterScope(PyMonitoringState *state_array, uint64_t *version,
                                 const uint8_t *event_types, uint32_t length)
{
    _PyMonitoring_EnterScope(state_array, version, event_types, length);
}
#endif

extern void
PyMonitoring_EnterScope(PyMonitoringState *state_array, uint64_t *version,
                        const uint8_t *event_types, uint32_t length);

#ifndef Py_LIMITED_API
static inline void
PyUnstable_Monitoring_ExitScope(void)
{
    _PyMonitoring_ExitScope();
}
#endif

extern void
PyMonitoring_ExitScope(void);


#ifndef Py_LIMITED_API
static inline int
PyUnstable_Monitoring_FirePyStartEvent(PyMonitoringState *state, PyObject *codelike, int offset)
{
    _PyMonitoring_IF_ACTIVE(
        state,
        _PyMonitoring_FirePyStartEvent(state, codelike, offset));
}
#endif

extern int
PyMonitoring_FirePyStartEvent(PyMonitoringState *state, PyObject *codelike, int offset);


#ifndef Py_LIMITED_API
static inline int
PyUnstable_Monitoring_FirePyResumeEvent(PyMonitoringState *state, PyObject *codelike, int offset)
{
    _PyMonitoring_IF_ACTIVE(
        state,
        _PyMonitoring_FirePyResumeEvent(state, codelike, offset));
}
#endif

extern int
PyMonitoring_FirePyResumeEvent(PyMonitoringState *state, PyObject *codelike, int offset);

#ifndef Py_LIMITED_API
static inline int
PyUnstable_Monitoring_FirePyReturnEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                       PyObject *retval)
{
    _PyMonitoring_IF_ACTIVE(
        state,
        _PyMonitoring_FirePyReturnEvent(state, codelike, offset, retval));
}
#endif

extern int
PyMonitoring_FirePyReturnEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                               PyObject *retval);

#ifndef Py_LIMITED_API
static inline int
PyUnstable_Monitoring_FirePyYieldEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                      PyObject *retval)
{
    _PyMonitoring_IF_ACTIVE(
        state,
        _PyMonitoring_FirePyYieldEvent(state, codelike, offset, retval));
}
#endif

extern int
PyMonitoring_FirePyYieldEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                              PyObject *retval);

#ifndef Py_LIMITED_API
static inline int
PyUnstable_Monitoring_FireCallEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                   PyObject* callable, PyObject *arg0)
{
    _PyMonitoring_IF_ACTIVE(
        state,
        _PyMonitoring_FireCallEvent(state, codelike, offset, callable, arg0));
}
#endif

extern int
PyMonitoring_FireCallEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                           PyObject* callable, PyObject *arg0);

#ifndef Py_LIMITED_API
static inline int
PyUnstable_Monitoring_FireLineEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                   int lineno)
{
    _PyMonitoring_IF_ACTIVE(
        state,
        _PyMonitoring_FireLineEvent(state, codelike, offset, lineno));
}
#endif

extern int
PyMonitoring_FireLineEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                           int lineno);

#ifndef Py_LIMITED_API
static inline int
PyUnstable_Monitoring_FireInstructionEvent(PyMonitoringState *state, PyObject *codelike, int offset)
{
    _PyMonitoring_IF_ACTIVE(
        state,
        _PyMonitoring_FireInstructionEvent(state, codelike, offset));
}
#endif

extern int
PyMonitoring_FireInstructionEvent(PyMonitoringState *state, PyObject *codelike, int offset);

#ifndef Py_LIMITED_API
static inline int
PyUnstable_Monitoring_FireJumpEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                   PyObject *target_offset)
{
    _PyMonitoring_IF_ACTIVE(
        state,
        _PyMonitoring_FireJumpEvent(state, codelike, offset, target_offset));
}
#endif

extern int
PyMonitoring_FireJumpEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                           PyObject *target_offset);

#ifndef Py_LIMITED_API
static inline int
PyUnstable_Monitoring_FireBranchEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                     PyObject *target_offset)
{
    _PyMonitoring_IF_ACTIVE(
        state,
        _PyMonitoring_FireBranchEvent(state, codelike, offset, target_offset));
}
#endif

extern int
PyMonitoring_FireBranchEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                             PyObject *target_offset);

#ifndef Py_LIMITED_API
static inline int
PyUnstable_Monitoring_FireCReturnEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                      PyObject *retval)
{
    _PyMonitoring_IF_ACTIVE(
        state,
        _PyMonitoring_FireCReturnEvent(state, codelike, offset, retval));
}
#endif

extern int
PyMonitoring_FireCReturnEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                              PyObject *retval);

#ifndef Py_LIMITED_API
static inline int
PyUnstable_Monitoring_FirePyThrowEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                      PyObject *exception)
{
    _PyMonitoring_IF_ACTIVE(
        state,
        _PyMonitoring_FirePyThrowEvent(state, codelike, offset, exception));
}
#endif

extern int
PyMonitoring_FirePyThrowEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                              PyObject *exception);

#ifndef Py_LIMITED_API
static inline int
PyUnstable_Monitoring_FireRaiseEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                    PyObject *exception)
{
    _PyMonitoring_IF_ACTIVE(
        state,
        _PyMonitoring_FireRaiseEvent(state, codelike, offset, exception));
}
#endif

extern int
PyMonitoring_FireRaiseEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                            PyObject *exception);

#ifndef Py_LIMITED_API
static inline int
PyUnstable_Monitoring_FireReraiseEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                      PyObject *exception)
{
    _PyMonitoring_IF_ACTIVE(
        state,
        _PyMonitoring_FireReraiseEvent(state, codelike, offset, exception));
}
#endif

extern int
PyMonitoring_FireReraiseEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                              PyObject *exception);

#ifndef Py_LIMITED_API
static inline int
PyUnstable_Monitoring_FireExceptionHandledEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                               PyObject *exception)
{
    _PyMonitoring_IF_ACTIVE(
        state,
        _PyMonitoring_FireExceptionHandledEvent(state, codelike, offset, exception));
}
#endif

extern int
PyMonitoring_FireExceptionHandledEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                       PyObject *exception);

#ifndef Py_LIMITED_API
static inline int
PyUnstable_Monitoring_FireCRaiseEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                     PyObject *exception)
{
    _PyMonitoring_IF_ACTIVE(
        state,
        _PyMonitoring_FireCRaiseEvent(state, codelike, offset, exception));
}
#endif

extern int
PyMonitoring_FireCRaiseEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                             PyObject *exception);

#ifndef Py_LIMITED_API
static inline int
PyUnstable_Monitoring_FirePyUnwindEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                       PyObject *exception)
{
    _PyMonitoring_IF_ACTIVE(
        state,
        _PyMonitoring_FirePyUnwindEvent(state, codelike, offset, exception));
}
#endif

extern int
PyMonitoring_FirePyUnwindEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                               PyObject *exception);

#ifndef Py_LIMITED_API
static inline int
PyUnstable_Monitoring_FireStopIterationEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                            PyObject *exception)
{
    _PyMonitoring_IF_ACTIVE(
        state,
        _PyMonitoring_FireStopIterationEvent(state, codelike, offset, exception));
}
#endif

extern int
PyMonitoring_FireStopIterationEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                    PyObject *exception);

#undef _PyMonitoring_IF_ACTIVE

#ifdef __cplusplus
}
#endif
#endif /* !Py_MONITORING_H */
