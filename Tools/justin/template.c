#define Py_BUILD_CORE

#include "Python.h"

#include "pycore_emscripten_signal.h"
#include "pycore_frame.h"
#include "pycore_object.h"
#include "pycore_opcode.h"

#include "Python/ceval_macros.h"

#define _JUSTIN_RETURN_OK                        0
#define _JUSTIN_RETURN_DEOPT                    -1
#define _JUSTIN_RETURN_GOTO_ERROR               -2
#define _JUSTIN_RETURN_GOTO_HANDLE_EVAL_BREAKER -3

// We obviously don't want compiled code specializing itself:
#undef ENABLE_SPECIALIZATION
#define ENABLE_SPECIALIZATION 0
#undef DEOPT_IF
#define DEOPT_IF(COND, INSTNAME)                                  \
    do {                                                          \
        if ((COND)) {                                             \
            /* This is only a single return on release builds! */ \
            UPDATE_MISS_STATS((INSTNAME));                        \
            assert(_PyOpcode_Deopt[opcode] == (INSTNAME));        \
            _res = _JUSTIN_RETURN_DEOPT;                          \
            next_instr = frame->prev_instr;                       \
            goto _bail;                                           \
        }                                                         \
    } while (0)

// Stuff that will be patched at "JIT time":
extern int _justin_continue(PyThreadState *tstate, _PyInterpreterFrame *frame, PyObject **stack_pointer);
extern _Py_CODEUNIT _justin_next_instr;
extern int _justin_oparg;


// Get dispatches and staying on trace working for multiple instructions:
#undef DISPATCH
#define DISPATCH()                                 \
    do {                                           \
        if (_check && next_instr != _next_trace) { \
            _res = _JUSTIN_RETURN_OK;              \
            goto _bail;                            \
        }                                          \
        goto _JUSTIN_CONTINUE;                     \
    } while (0)
#undef TARGET
#define TARGET(OP) INSTRUCTION_START(OP);
#define _JUSTIN_PART(N)                             \
    do {                                            \
        extern _Py_CODEUNIT _justin_next_trace_##N; \
        extern _Py_CODEUNIT _justin_oparg_##N;      \
        _check = _JUSTIN_CHECK_##N;                 \
        _next_trace = &_justin_next_trace_##N;      \
        oparg = (uintptr_t)&_justin_oparg_##N;      \
        opcode = _JUSTIN_OPCODE_##N;                \
    } while (0)
#undef PREDICTED
#define PREDICTED(OP)

int
_justin_target(PyThreadState *tstate, _PyInterpreterFrame *frame, PyObject **stack_pointer)
{
    int _res;
    // Locals that the instruction implementations expect to exist:
    _Py_atomic_int *const eval_breaker = &tstate->interp->ceval.eval_breaker;
    _Py_CODEUNIT *next_instr = &_justin_next_instr;
    int oparg;
    uint8_t opcode;
    // Labels that the instruction implementations expect to exist:
    if (false) {
    error:
        _res = _JUSTIN_RETURN_GOTO_ERROR;
        goto _bail;
    handle_eval_breaker:
        _res = _JUSTIN_RETURN_GOTO_HANDLE_EVAL_BREAKER;
        goto _bail;
    }
    // Stuff to make Justin work:
    _Py_CODEUNIT *_next_trace;
    int _check;
    // Now, the actual instruction definition:
%s
    // Finally, the continuation:
    __attribute__((musttail))
    return _justin_continue(tstate, frame, stack_pointer);
_bail:
    _PyFrame_SetStackPointer(frame, stack_pointer);
    frame->prev_instr = next_instr;
    return _res;
}
