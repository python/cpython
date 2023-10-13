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

#define TAIL_CALL(WHERE)                                                      \
    do {                                                                      \
        extern _PyInterpreterFrame *(WHERE)(_PyInterpreterFrame *frame,       \
                                            PyObject **stack_pointer,         \
                                            PyThreadState *tstate,            \
                                            int32_t oparg, uint64_t operand); \
        /* Free up these registers (since we don't care what's passed in): */ \
        asm inline ("" : "=r"(oparg), "=r"(operand));                         \
        __attribute__((musttail))                                             \
        return (WHERE)(frame, stack_pointer, tstate, oparg, operand);         \
    } while (0)

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
    if (pc != -1) {
        assert(pc == oparg);
        assert(opcode == _JUMP_TO_TOP || opcode == _POP_JUMP_IF_FALSE || opcode == _POP_JUMP_IF_TRUE);
        TAIL_CALL(_JIT_JUMP);
    }
    TAIL_CALL(_JIT_CONTINUE);
    // Labels that the instruction implementations expect to exist:
unbound_local_error:
    _PyEval_FormatExcCheckArg(tstate, PyExc_UnboundLocalError, UNBOUNDLOCAL_ERROR_MSG, PyTuple_GetItem(_PyFrame_GetCode(frame)->co_localsplusnames, oparg));
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
    TAIL_CALL(_JIT_ERROR);
deoptimize:
    TAIL_CALL(_JIT_DEOPTIMIZE);
}
