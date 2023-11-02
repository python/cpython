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

extern void _JIT_OPARG;
extern void _JIT_OPERAND;

#undef DEOPT_IF
#define DEOPT_IF(COND, INSTNAME) \
    if ((COND)) {                \
        goto deoptimize;         \
    }
#undef ENABLE_SPECIALIZATION
#define ENABLE_SPECIALIZATION 0
#undef GOTO_ERROR
#define GOTO_ERROR(LABEL) goto LABEL ## _tier_two
#undef LOAD_IP
#define LOAD_IP(UNUSED) ((void)0)
#undef JUMP_TO_OPARG
#define JUMP_TO_OPARG()     \
    do {                    \
        _jump_taken = true; \
    } while (0)

#define TAIL_CALL(WHERE)                                                \
    do {                                                                \
        extern _PyInterpreterFrame *(WHERE)(_PyInterpreterFrame *frame, \
                                            PyObject **stack_pointer,   \
                                            PyThreadState *tstate);     \
        __attribute__((musttail))                                       \
        return (WHERE)(frame, stack_pointer, tstate);                   \
    } while (0)

_PyInterpreterFrame *
_JIT_ENTRY(_PyInterpreterFrame *frame, PyObject **stack_pointer,
           PyThreadState *tstate)
{
    // Locals that the instruction implementations expect to exist:
    uint32_t opcode = _JIT_OPCODE;
    int32_t oparg = (uintptr_t)&_JIT_OPARG;
    uint64_t operand = (uintptr_t)&_JIT_OPERAND;
    // Pretend to modify the values to keep clang from being clever and
    // optimizing them based on valid extern addresses, which must be in
    // range(1, 2**31 - 2**24):
    asm("" : "+r" (oparg));
    asm("" : "+r" (operand));
    bool _jump_taken = false;
    switch (opcode) {
        // Now, the actual instruction definitions (only one will be used):
#include "executor_cases.c.h"
        default:
            Py_UNREACHABLE();
    }
    if (_jump_taken) {
        assert(opcode == _JUMP_TO_TOP || opcode == _POP_JUMP_IF_FALSE || opcode == _POP_JUMP_IF_TRUE);
        TAIL_CALL(_JIT_JUMP);
    }
    TAIL_CALL(_JIT_CONTINUE);
    // Labels that the instruction implementations expect to exist:
unbound_local_error_tier_two:
    _PyEval_FormatExcCheckArg(tstate, PyExc_UnboundLocalError, UNBOUNDLOCAL_ERROR_MSG, PyTuple_GetItem(_PyFrame_GetCode(frame)->co_localsplusnames, oparg));
    goto error_tier_two;
pop_4_error_tier_two:
    STACK_SHRINK(1);
pop_3_error_tier_two:
    STACK_SHRINK(1);
pop_2_error_tier_two:
    STACK_SHRINK(1);
pop_1_error_tier_two:
    STACK_SHRINK(1);
error_tier_two:
    TAIL_CALL(_JIT_ERROR);
deoptimize:
exit_trace:
    TAIL_CALL(_JIT_DEOPTIMIZE);
}
