#include "Python.h"

#include "pycore_call.h"
#include "pycore_ceval.h"
#include "pycore_dict.h"
#include "pycore_emscripten_signal.h"
#include "pycore_intrinsics.h"
#include "pycore_long.h"
#include "pycore_object.h"
#include "pycore_opcode_metadata.h"
#include "pycore_opcode_utils.h"
#include "pycore_pyerrors.h"
#include "pycore_range.h"
#include "pycore_setobject.h"
#include "pycore_sliceobject.h"
#include "pycore_uops.h"

#define TIER_TWO 2
#include "ceval_macros.h"

#include "opcode.h"

#undef DEOPT_IF
#define DEOPT_IF(COND, INSTNAME) \
    if ((COND)) {                \
        goto deoptimize;         \
    }
#undef ENABLE_SPECIALIZATION
#define ENABLE_SPECIALIZATION 0

// Stuff that will be patched at "JIT time":
extern _PyInterpreterFrame *_JIT_CONTINUE(_PyInterpreterFrame *frame,
                                          PyObject **stack_pointer,
                                          PyThreadState *tstate,
                                          int32_t oparg, uint64_t operand);
extern void _JIT_CONTINUE_OPARG;
extern void _JIT_CONTINUE_OPERAND;
extern _PyInterpreterFrame *_JIT_JUMP(_PyInterpreterFrame *frame,
                                      PyObject **stack_pointer,
                                      PyThreadState *tstate,
                                      int32_t oparg, uint64_t operand);
extern void _JIT_JUMP_OPARG;
extern void _JIT_JUMP_OPERAND;

_PyInterpreterFrame *
_JIT_ENTRY(_PyInterpreterFrame *frame, PyObject **stack_pointer,
           PyThreadState *tstate, int32_t oparg, uint64_t operand)
{
    // Locals that the instruction implementations expect to exist:
    _Py_CODEUNIT *ip_offset = _PyCode_CODE(_PyFrame_GetCode(frame));
    uint32_t opcode = _JIT_OPCODE;
    int pc = -1;  // XXX
    switch (opcode) {
        // Now, the actual instruction definitions (only one will be used):
#include "executor_cases.c.h"
        default:
            Py_UNREACHABLE();
    }
    // Finally, the continuations:
    if (pc != -1) {
        assert(pc == oparg);
        assert(opcode == _JUMP_TO_TOP ||
               opcode == _POP_JUMP_IF_FALSE ||
               opcode == _POP_JUMP_IF_TRUE);
        oparg = (uintptr_t)&_JIT_JUMP_OPARG;
        operand = (uintptr_t)&_JIT_JUMP_OPERAND;
        __attribute__((musttail))
        return _JIT_JUMP(frame, stack_pointer, tstate, oparg, operand);
    }
    oparg = (uintptr_t)&_JIT_CONTINUE_OPARG;
    operand = (uintptr_t)&_JIT_CONTINUE_OPERAND;
    __attribute__((musttail))
    return _JIT_CONTINUE(frame, stack_pointer, tstate, oparg, operand);
    // Labels that the instruction implementations expect to exist:
unbound_local_error:
    _PyEval_FormatExcCheckArg(tstate, PyExc_UnboundLocalError,
        UNBOUNDLOCAL_ERROR_MSG,
        PyTuple_GetItem(_PyFrame_GetCode(frame)->co_localsplusnames, oparg)
    );
    goto error;
pop_4_error:
    STACK_SHRINK(1);
pop_3_error:
    STACK_SHRINK(1);
pop_2_error:
    STACK_SHRINK(1);
pop_1_error:
    STACK_SHRINK(1);
error:
    _PyFrame_SetStackPointer(frame, stack_pointer);
    return NULL;
deoptimize:
    frame->prev_instr--;
    _PyFrame_SetStackPointer(frame, stack_pointer);
    return frame;
}
