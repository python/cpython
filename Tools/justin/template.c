#include "Python.h"

#include "pycore_abstract.h"
#include "pycore_call.h"
#include "pycore_ceval.h"
#include "pycore_dict.h"
#include "pycore_emscripten_signal.h"
#include "pycore_frame.h"
#include "pycore_intrinsics.h"
#include "pycore_jit.h"
#include "pycore_long.h"
#include "pycore_object.h"
#include "pycore_opcode.h"
#include "pycore_pyerrors.h"
#include "pycore_range.h"
#include "pycore_sliceobject.h"

#include "Python/ceval_macros.h"

#undef DEOPT_IF
#define DEOPT_IF(COND, INSTNAME) \
    do {                         \
        if ((COND)) {            \
            goto _return_deopt;  \
        }                        \
    } while (0)
#undef DISPATCH_GOTO
#define DISPATCH_GOTO() \
    do {                \
        goto _continue; \
    } while (0)
#undef PREDICT
#define PREDICT(OP)
#undef TARGET
#define TARGET(OP) INSTRUCTION_START((OP));

// XXX: Turn off trace recording in here?

// Stuff that will be patched at "JIT time":
extern _PyJITReturnCode _justin_continue(PyThreadState *tstate,
                                         _PyInterpreterFrame *frame,
                                         PyObject **stack_pointer,
                                         _Py_CODEUNIT *next_instr);
extern _Py_CODEUNIT _justin_next_instr;
extern void _justin_oparg;

// XXX
#define cframe (*tstate->cframe)

_PyJITReturnCode
_justin_entry(PyThreadState *tstate, _PyInterpreterFrame *frame,
              PyObject **stack_pointer, _Py_CODEUNIT *next_instr)
{
    // Locals that the instruction implementations expect to exist:
    int oparg = (uintptr_t)&_justin_oparg;
    uint8_t opcode = _JUSTIN_OPCODE;
    // XXX: This temporary solution only works because we don't trace KW_NAMES:
    PyObject *kwnames = NULL;
#ifdef Py_STATS
    int lastopcode = frame->prev_instr->op.code;
#endif
    if (next_instr != &_justin_next_instr) {
        goto _return_ok;
    }
    if (opcode != JUMP_BACKWARD_QUICK && next_instr->op.code != opcode) {
        frame->prev_instr = next_instr;
        goto _return_deopt;
    }
    // Now, the actual instruction definition:
%s
    Py_UNREACHABLE();
    // Labels that the instruction implementations expect to exist:
start_frame:
    if (_Py_EnterRecursivePy(tstate)) {
        goto exit_unwind;
    }
resume_frame:
    SET_LOCALS_FROM_FRAME();
    DISPATCH();
handle_eval_breaker:
    if (_Py_HandlePending(tstate) != 0) {
        goto error;
    }
    DISPATCH();
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
exit_unwind:
    assert(_PyErr_Occurred(tstate));
    _Py_LeaveRecursiveCallPy(tstate);
    // assert(frame != &entry_frame);
    // GH-99729: We need to unlink the frame *before* clearing it:
    _PyInterpreterFrame *dying = frame;
    frame = cframe.current_frame = dying->previous;
    _PyEvalFrameClearAndPop(tstate, dying);
    frame->return_offset = 0;
    // if (frame == &entry_frame) {
    //     /* Restore previous cframe and exit */
    //     tstate->cframe = cframe.previous;
    //     assert(tstate->cframe->current_frame == frame->previous);
    //     _Py_LeaveRecursiveCallTstate(tstate);
    //     return NULL;
    // }
// resume_with_error:
    // SET_LOCALS_FROM_FRAME();
    goto error;
    // Other labels:
_return_ok:
    frame->prev_instr = next_instr;
    _PyFrame_SetStackPointer(frame, stack_pointer);
    return _JUSTIN_RETURN_OK;
_return_deopt:
    _PyFrame_SetStackPointer(frame, stack_pointer);
    return _JUSTIN_RETURN_DEOPT;
_continue:
    ;  // XXX
    // Finally, the continuation:
    __attribute__((musttail))
    return _justin_continue(tstate, frame, stack_pointer, next_instr);
}
