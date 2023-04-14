#include "Python.h"

#include "pycore_abstract.h"
#include "pycore_call.h"
#include "pycore_dict.h"
#include "pycore_emscripten_signal.h"
#include "pycore_frame.h"
#include "pycore_intrinsics.h"
#include "pycore_long.h"
#include "pycore_object.h"
#include "pycore_opcode.h"
#include "pycore_pyerrors.h"
#include "pycore_range.h"
#include "pycore_sliceobject.h"

#include "Python/ceval_macros.h"

#define _JUSTIN_RETURN_OK                        0
#define _JUSTIN_RETURN_DEOPT                    -1
#define _JUSTIN_RETURN_GOTO_ERROR               -2
#define _JUSTIN_RETURN_GOTO_HANDLE_EVAL_BREAKER -3

// We obviously don't want compiled code specializing itself:
#undef ENABLE_SPECIALIZATION
#define ENABLE_SPECIALIZATION 0
#undef DEOPT_IF
#define DEOPT_IF(COND, INSTNAME) \
    do {                         \
        if ((COND)) {            \
            goto _return_deopt;  \
        }                        \
    } while (0)
#undef PREDICT
#define PREDICT(OP)
#undef TARGET
#define TARGET(OP) INSTRUCTION_START((OP));

// Stuff that will be patched at "JIT time":
extern int _justin_continue(PyThreadState *tstate, _PyInterpreterFrame *frame,
                            PyObject **stack_pointer);
extern _Py_CODEUNIT _justin_next_instr;
extern _Py_CODEUNIT _justin_next_trace;
extern void _justin_oparg;


// Get dispatches and staying on trace working for multiple instructions:
#undef DISPATCH
#define DISPATCH()                                        \
    do {                                                  \
        if (_JUSTIN_CHECK && next_instr != _next_trace) { \
            goto _return_ok;                              \
        }                                                 \
        goto _continue;                                   \
    } while (0)

int
_justin_entry(PyThreadState *tstate, _PyInterpreterFrame *frame,
              PyObject **stack_pointer)
{
    // Locals that the instruction implementations expect to exist:
    _Py_atomic_int *const eval_breaker = &tstate->interp->ceval.eval_breaker;
    _Py_CODEUNIT *next_instr = &_justin_next_instr;
    int oparg = (uintptr_t)&_justin_oparg;
    uint8_t opcode = _JUSTIN_OPCODE;
    // Stuff to make Justin work:
    _Py_CODEUNIT *_next_trace = &_justin_next_trace;
    // Now, the actual instruction definition:
%s
    Py_UNREACHABLE();
_continue:;
    // Finally, the continuation:
    __attribute__((musttail))
    return _justin_continue(tstate, frame, stack_pointer);
    // Labels that the instruction implementations expect to exist:
pop_4_error:
    STACK_SHRINK(1);
pop_3_error:
    STACK_SHRINK(1);
pop_2_error:
    STACK_SHRINK(1);
pop_1_error:
    STACK_SHRINK(1);
error:
    frame->prev_instr = next_instr;
    _PyFrame_SetStackPointer(frame, stack_pointer);
    return _JUSTIN_RETURN_GOTO_ERROR;
handle_eval_breaker:
    frame->prev_instr = next_instr;
    _PyFrame_SetStackPointer(frame, stack_pointer);
    return _JUSTIN_RETURN_GOTO_HANDLE_EVAL_BREAKER;
    // Other labels:
_return_ok:
    frame->prev_instr = next_instr;
    _PyFrame_SetStackPointer(frame, stack_pointer);
    return _JUSTIN_RETURN_OK;
_return_deopt:
    _PyFrame_SetStackPointer(frame, stack_pointer);
    return _JUSTIN_RETURN_DEOPT;
}
