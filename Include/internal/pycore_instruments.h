
#ifndef Py_INTERNAL_INSTRUMENT_H
#define Py_INTERNAL_INSTRUMENT_H


#include "pycore_bitutils.h"      // _Py_popcount32
#include "pycore_frame.h"

#include "cpython/code.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PY_MONITORING_TOOL_IDS 8

/* Require bytecode instrumentation */

#define PY_MONITORING_EVENT_PY_START 0
#define PY_MONITORING_EVENT_PY_RESUME 1
#define PY_MONITORING_EVENT_PY_RETURN 2
#define PY_MONITORING_EVENT_PY_YIELD 3
#define PY_MONITORING_EVENT_CALL 4
#define PY_MONITORING_EVENT_LINE 5
#define PY_MONITORING_EVENT_INSTRUCTION 6
#define PY_MONITORING_EVENT_JUMP 7
#define PY_MONITORING_EVENT_BRANCH 8

#define PY_MONITORING_INSTRUMENTED_EVENTS 9

/* Grouped events */

#define PY_MONITORING_EVENT_C_RETURN 9
#define PY_MONITORING_EVENT_C_RAISE 10

/* Exceptional events */

#define PY_MONITORING_EVENT_PY_THROW 11
#define PY_MONITORING_EVENT_RAISE 12
#define PY_MONITORING_EVENT_EXCEPTION_HANDLED 13
#define PY_MONITORING_EVENT_PY_UNWIND 14

/* #define INSTRUMENT_EVENT_BRANCH_NOT_TAKEN xxx  -- If we can afford this */

/* Temporary and internal events */

// #define PY_INSTRUMENT_PEP_523 50
/* #define PY_INSTRUMENT_JIT_API 17  -- Reserved */

typedef uint32_t _PyMonitoringEventSet;

/* Reserved IDs */

#define PY_INSTRUMENT_PEP_523 5
#define PY_INSTRUMENT_SYS_PROFILE 6
#define PY_INSTRUMENT_SYS_TRACE 7


/* API functions */

PyObject *_PyMonitoring_RegisterCallback(int tool_id, int event_id, PyObject *obj);

void _PyMonitoring_SetEvents(int tool_id, _PyMonitoringEventSet events);

extern int
_Py_call_instrumentation(PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr);

extern int
_Py_call_instrumentation_line(PyThreadState *tstate,
                              PyCodeObject *code, _Py_CODEUNIT *instr);

int
_Py_call_instrumentation_jump(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, _Py_CODEUNIT *target);

extern int
_Py_call_instrumentation_arg(PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, PyObject *arg);

extern void
_Py_call_instrumentation_exc(PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, PyObject *arg);

extern int
_Py_Instrumentation_GetLine(PyCodeObject *code, int index);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_INSTRUMENT_H */
