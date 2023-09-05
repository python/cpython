
#ifndef Py_INTERNAL_INSTRUMENT_H
#define Py_INTERNAL_INSTRUMENT_H


#include "pycore_bitutils.h"      // _Py_popcount32
#include "pycore_frame.h"

#include "cpython/code.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PY_MONITORING_TOOL_IDS 8

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
#define PY_MONITORING_EVENT_BRANCH 8
#define PY_MONITORING_EVENT_STOP_ITERATION 9

#define PY_MONITORING_IS_INSTRUMENTED_EVENT(ev) \
    ((ev) < _PY_MONITORING_LOCAL_EVENTS)

/* Other events, mainly exceptions */

#define PY_MONITORING_EVENT_RAISE 10
#define PY_MONITORING_EVENT_EXCEPTION_HANDLED 11
#define PY_MONITORING_EVENT_PY_UNWIND 12
#define PY_MONITORING_EVENT_PY_THROW 13
#define PY_MONITORING_EVENT_RERAISE 14


/* Ancilliary events */

#define PY_MONITORING_EVENT_C_RETURN 15
#define PY_MONITORING_EVENT_C_RAISE 16


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
