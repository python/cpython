#ifndef Py_INTERNAL_INSTRUMENT_H
#define Py_INTERNAL_INSTRUMENT_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_frame.h"         // _PyInterpreterFrame

#ifdef __cplusplus
extern "C" {
#endif

#define PY_MONITORING_TOOL_IDS 8

typedef uint32_t _PyMonitoringEventSet;

/* Tool IDs */

/* These are defined in PEP 669 for convenience to avoid clashes */
#define PY_MONITORING_DEBUGGER_ID 0
#define PY_MONITORING_COVERAGE_ID 1
#define PY_MONITORING_PROFILER_ID 2
#define PY_MONITORING_OPTIMIZER_ID 5

/* Internal IDs used to suuport sys.setprofile() and sys.settrace() */
#define PY_MONITORING_SYS_PROFILE_ID 6
#define PY_MONITORING_SYS_TRACE_ID 7


PyObject *_PyMonitoring_RegisterCallback(int tool_id, int event_id, PyObject *obj);

int _PyMonitoring_SetEvents(int tool_id, _PyMonitoringEventSet events);
int _PyMonitoring_SetLocalEvents(PyCodeObject *code, int tool_id, _PyMonitoringEventSet events);
int _PyMonitoring_GetLocalEvents(PyCodeObject *code, int tool_id, _PyMonitoringEventSet *events);

extern int
_Py_call_instrumentation(PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr);

extern int
_Py_call_instrumentation_line(PyThreadState *tstate, _PyInterpreterFrame* frame,
                              _Py_CODEUNIT *instr, _Py_CODEUNIT *prev);

extern int
_Py_call_instrumentation_instruction(
    PyThreadState *tstate, _PyInterpreterFrame* frame, _Py_CODEUNIT *instr);

_Py_CODEUNIT *
_Py_call_instrumentation_jump(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, _Py_CODEUNIT *target);

extern int
_Py_call_instrumentation_arg(PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, PyObject *arg);

extern int
_Py_call_instrumentation_2args(PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, PyObject *arg0, PyObject *arg1);

extern void
_Py_call_instrumentation_exc2(PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, PyObject *arg0, PyObject *arg1);

extern int
_Py_Instrumentation_GetLine(PyCodeObject *code, int index);

extern PyObject _PyInstrumentation_MISSING;
extern PyObject _PyInstrumentation_DISABLE;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_INSTRUMENT_H */
