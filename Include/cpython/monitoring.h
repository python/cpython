#ifndef Py_CPYTHON_MONITORING_H
#  error "this header file must not be included directly"
#endif

#ifndef Py_LIMITED_API

PyAPI_FUNC(void)
PyMonitoring_EnterScope(PyMonitoringState *state_array, uint64_t *version,
                         const uint8_t *event_types, uint32_t length);

PyAPI_FUNC(void)
PyMonitoring_ExitScope(void);

PyAPI_FUNC(int)
_PyMonitoring_FirePyStartEvent(PyMonitoringState *state, PyObject *codelike, int offset);

PyAPI_FUNC(int)
_PyMonitoring_FirePyResumeEvent(PyMonitoringState *state, PyObject *codelike, int offset);

PyAPI_FUNC(int)
_PyMonitoring_FirePyReturnEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                PyObject *retval);

PyAPI_FUNC(int)
_PyMonitoring_FirePyYieldEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                               PyObject *retval);

PyAPI_FUNC(int)
_PyMonitoring_FireCallEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                            PyObject* callable, PyObject *arg0);

PyAPI_FUNC(int)
_PyMonitoring_FireLineEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                            int lineno);

PyAPI_FUNC(int)
_PyMonitoring_FireJumpEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                            PyObject *target_offset);

PyAPI_FUNC(int)
_PyMonitoring_FireBranchEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                              PyObject *target_offset);

PyAPI_FUNC(int)
_PyMonitoring_FireCReturnEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                               PyObject *retval);

PyAPI_FUNC(int)
_PyMonitoring_FirePyThrowEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                               PyObject *exception);

PyAPI_FUNC(int)
_PyMonitoring_FireRaiseEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                             PyObject *exception);

PyAPI_FUNC(int)
_PyMonitoring_FireReraiseEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                               PyObject *exception);

PyAPI_FUNC(int)
_PyMonitoring_FireExceptionHandledEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                        PyObject *exception);

PyAPI_FUNC(int)
_PyMonitoring_FireCRaiseEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                              PyObject *exception);

PyAPI_FUNC(int)
_PyMonitoring_FirePyUnwindEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                PyObject *exception);

PyAPI_FUNC(int)
_PyMonitoring_FireStopIterationEvent(PyMonitoringState *state, PyObject *codelike, int offset,
                                    PyObject *exception);

#endif
