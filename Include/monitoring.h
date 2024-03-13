#ifndef Py_MONITORING_H
#define Py_MONITORING_H
#ifdef __cplusplus
extern "C" {

#endif

typedef struct _PyMonitoringState {
    uint8_t active;
    uint8_t opaque;
} PyMonitoringState;


#ifndef Py_LIMITED_API
#  define Py_CPYTHON_MONITORING_H
#  include "cpython/monitoring.h"
#  undef Py_CPYTHON_MONITORING_H

#define IF_ACTIVE(X)  \
    if (state->active) { \
        return (X); \
    } \
    else { \
        return 0; \
    }

#endif

#ifndef Py_LIMITED_API
static inline void
PyMonitoringScopeBegin(PyMonitoringState *state_array, uint64_t *version,
                       uint8_t *event_types, uint32_t length)
{
    _PyMonitoringScopeBegin(state_array, version, event_types, length);
}
#else
extern void
PyMonitoringScopeBegin(PyMonitoringState *state_array, uint64_t *version,
                       uint8_t *event_types, uint32_t length);
#endif


#ifndef Py_LIMITED_API
static inline int
PyMonitoring_FirePyStartEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset)
{
    IF_ACTIVE(_PyMonitoring_FirePyStartEvent(state, codelike, offset));
}
#else
extern int
PyMonitoring_FirePyStartEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset);
#endif


#ifndef Py_LIMITED_API
static inline int
PyMonitoring_FirePyResumeEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset)
{
    IF_ACTIVE(_PyMonitoring_FirePyResumeEvent(state, codelike, offset));
}
#else
extern int
PyMonitoring_FirePyResumeEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset);
#endif

#ifndef Py_LIMITED_API
static inline int
PyMonitoring_FirePyReturnEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                               PyObject *retval)
{
    IF_ACTIVE(_PyMonitoring_FirePyReturnEvent(state, codelike, offset, retval));
}
#else
extern int
PyMonitoring_FirePyReturnEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                               PyObject *retval);
#endif

#ifndef Py_LIMITED_API
static inline int
PyMonitoring_FirePyYieldEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                              PyObject *retval)
{
    IF_ACTIVE(_PyMonitoring_FirePyYieldEvent(state, codelike, offset, retval));
}
#else
extern int
PyMonitoring_FirePyYieldEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                              PyObject *retval);
#endif

#ifndef Py_LIMITED_API
static inline int
PyMonitoring_FirePyCallEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                              PyObject* callable, PyObject *arg0)
{
    IF_ACTIVE(_PyMonitoring_FirePyCallEvent(state, codelike, offset, callable, arg0));
}
#else
extern int
PyMonitoring_FirePyCallEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                             PyObject* callable, PyObject *arg0);
#endif

#ifndef Py_LIMITED_API
static inline int
PyMonitoring_FireCallEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                           PyObject* callable, PyObject *arg0)
{
    IF_ACTIVE(_PyMonitoring_FireCallEvent(state, codelike, offset, callable, arg0));
}
#else
extern int
PyMonitoring_FireCallEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                           PyObject* callable, PyObject *arg0);
#endif

#ifndef Py_LIMITED_API
static inline int
PyMonitoring_FireLineEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                           PyObject *lineno)
{
    IF_ACTIVE(_PyMonitoring_FireLineEvent(state, codelike, offset, lineno));
}
#else
extern int
PyMonitoring_FireLineEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                           PyObject *lineno);
#endif

#ifndef Py_LIMITED_API
static inline int
PyMonitoring_FireInstructionEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset)
{
    IF_ACTIVE(_PyMonitoring_FireInstructionEvent(state, codelike, offset));
}
#else
extern int
PyMonitoring_FireInstructionEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset);
#endif

#ifndef Py_LIMITED_API
static inline int
PyMonitoring_FireJumpEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                           PyObject *target_offset)
{
    IF_ACTIVE(_PyMonitoring_FireJumpEvent(state, codelike, offset, target_offset));
}
#else
extern int
PyMonitoring_FireJumpEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                           PyObject *target_offset);
#endif

#ifndef Py_LIMITED_API
static inline int
PyMonitoring_FireBranchEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                             PyObject *target_offset)
{
    IF_ACTIVE(_PyMonitoring_FireBranchEvent(state, codelike, offset, target_offset));
}
#else
extern int
PyMonitoring_FireBranchEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                             PyObject *target_offset);
#endif

#ifndef Py_LIMITED_API
static inline int
PyMonitoring_FireCReturnEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                              PyObject *callable, PyObject *arg0)
{
    IF_ACTIVE(_PyMonitoring_FireCReturnEvent(state, codelike, offset, callable, arg0));
}
#else
extern int
PyMonitoring_FireCReturnEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                              PyObject *callable, PyObject *arg0);
#endif

#ifndef Py_LIMITED_API
static inline int
PyMonitoring_FirePyThrowEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                              PyObject *exception)
{
    IF_ACTIVE(_PyMonitoring_FirePyThrowEvent(state, codelike, offset, exception));
}
#else
extern int
PyMonitoring_FirePyThrowEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                              PyObject *exception);
#endif

#ifndef Py_LIMITED_API
static inline int
PyMonitoring_FireRaiseEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                            PyObject *exception)
{
    IF_ACTIVE(_PyMonitoring_FireRaiseEvent(state, codelike, offset, exception));
}
#else
extern int
PyMonitoring_FireRaiseEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                            PyObject *exception);
#endif

#ifndef Py_LIMITED_API
static inline int
PyMonitoring_FireReraiseEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                              PyObject *exception)
{
    IF_ACTIVE(_PyMonitoring_FireReraiseEvent(state, codelike, offset, exception));
}
#else
extern int
PyMonitoring_FireReraiseEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                              PyObject *exception);
#endif

#ifndef Py_LIMITED_API
static inline int
PyMonitoring_FireExceptionHandledEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                                       PyObject *exception)
{
    IF_ACTIVE(_PyMonitoring_FireExceptionHandledEvent(state, codelike, offset, exception));
}
#else
extern int
PyMonitoring_FireExceptionHandledEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                                       PyObject *exception);
#endif

#ifndef Py_LIMITED_API
static inline int
PyMonitoring_FireCRaiseEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                             PyObject *callable, PyObject *arg0)
{
    IF_ACTIVE(_PyMonitoring_FireCRaiseEvent(state,  codelike, offset, callable, arg0));
}
#else
extern int
PyMonitoring_FireCRaiseEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                             PyObject *callable, PyObject *arg0);
#endif

#ifndef Py_LIMITED_API
static inline int
PyMonitoring_FirePyUnwindEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                               PyObject *exception)
{
    IF_ACTIVE(_PyMonitoring_FirePyUnwindEvent(state, codelike, offset, exception));
}
#else
extern int
PyMonitoring_FirePyUnwindEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                               PyObject *exception);
#endif

#ifndef Py_LIMITED_API
static inline int
PyMonitoring_FireStopIterationEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                                    PyObject *exception)
{
    IF_ACTIVE(_PyMonitoring_FireStopIterationEvent(state, codelike, offset, exception));
}
#else
extern int
PyMonitoring_FireStopIterationEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                                    PyObject *exception);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_MONITORING_H */
