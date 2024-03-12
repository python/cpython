#ifndef Py_CPYTHON_MONITORING_H
#  error "this header file must not be included directly"
#endif

void
_PyMonitoringScopeBegin(PyMonitoringState *state_array, uint64_t *version,
                        uint8_t *event_types, uint32_t length);

int
_PyMonitoring_FirePyStartEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset);

int
_PyMonitoring_FirePyResumeEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset);

int
_PyMonitoring_FirePyReturnEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                                PyObject *retval);

int
_PyMonitoring_FirePyYieldEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                               PyObject *retval);

int
_PyMonitoring_FirePyCallEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                              PyObject* callable, PyObject *arg0);

int
_PyMonitoring_FireCallEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                            PyObject* callable, PyObject *arg0);


int
_PyMonitoring_FireLineEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                            PyObject *lineno);

int
_PyMonitoring_FireInstructionEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset);

int
_PyMonitoring_FireJumpEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                            PyObject *target_offset);

int
_PyMonitoring_FireJumpEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                            PyObject *target_offset);

int
_PyMonitoring_FireBranchEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                              PyObject *target_offset);

int
_PyMonitoring_FireCReturnEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                               PyObject *callable, PyObject *arg0);

int
_PyMonitoring_FirePyThrowEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                               PyObject *exception);

int
_PyMonitoring_FireRaiseEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                             PyObject *exception);

int
_PyMonitoring_FireReraiseEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                               PyObject *exception);

int
_PyMonitoring_FireExceptionHandledEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                                        PyObject *exception);

int
_PyMonitoring_FireCRaiseEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                              PyObject *callable, PyObject *arg0);

int
_PyMonitoring_FirePyUnwindEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                                PyObject *exception);

int
_PyMonitoring_FireStopIterationEvent(PyMonitoringState *state, PyObject *codelike, uint32_t offset,
                                    PyObject *exception);

