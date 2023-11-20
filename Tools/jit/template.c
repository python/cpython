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

extern _PyUOpExecutorObject _JIT_CURRENT_EXECUTOR;
extern void _JIT_OPARG;
extern void _JIT_OPERAND;
extern void _JIT_TARGET;

#undef CURRENT_OPARG
#define CURRENT_OPARG() (_oparg)
#undef CURRENT_OPERAND
#define CURRENT_OPERAND() (_operand)
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
#undef JUMP_TO_TOP
#define JUMP_TO_TOP() ((void)0)

#define TAIL_CALL(WHERE)                                         \
    do {                                                         \
        _PyInterpreterFrame *(WHERE)(_PyInterpreterFrame *frame, \
                                     PyObject **stack_pointer,   \
                                     PyThreadState *tstate);     \
        __attribute__((musttail))                                \
        return (WHERE)(frame, stack_pointer, tstate);            \
    } while (0)

_PyInterpreterFrame *
_JIT_ENTRY(_PyInterpreterFrame *frame, PyObject **stack_pointer,
           PyThreadState *tstate)
{
    // Locals that the instruction implementations expect to exist:
    _PyUOpExecutorObject *current_executor = &_JIT_CURRENT_EXECUTOR;
    uint32_t opcode = _JIT_OPCODE;
    int32_t oparg;
    int32_t _oparg = (uintptr_t)&_JIT_OPARG;
    uint64_t _operand = (uintptr_t)&_JIT_OPERAND;
    uint32_t _target = (uintptr_t)&_JIT_TARGET;
    // Pretend to modify the burned-in values to keep clang from being clever
    // and optimizing them based on valid extern addresses, which must be in
    // range(1, 2**31 - 2**24):
    asm("" : "+r" (current_executor));
    asm("" : "+r" (_oparg));
    asm("" : "+r" (_operand));
    asm("" : "+r" (_target));
    // Now, the actual instruction definitions (only one will be used):
    if (opcode == _JUMP_TO_TOP) {
        CHECK_EVAL_BREAKER();
        TAIL_CALL(_JIT_TOP);
    }
    switch (opcode) {
#include "executor_cases.c.h"
        default:
            Py_UNREACHABLE();
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
    frame->instr_ptr = _PyCode_CODE(_PyFrame_GetCode(frame)) + _target;
    TAIL_CALL(_JIT_DEOPTIMIZE);
}
