#include "Python.h"

#include "pycore_backoff.h"
#include "pycore_call.h"
#include "pycore_cell.h"
#include "pycore_ceval.h"
#include "pycore_code.h"
#include "pycore_descrobject.h"
#include "pycore_dict.h"
#include "pycore_emscripten_signal.h"
#include "pycore_floatobject.h"
#include "pycore_frame.h"
#include "pycore_function.h"
#include "pycore_genobject.h"
#include "pycore_interpframe.h"
#include "pycore_interpolation.h"
#include "pycore_intrinsics.h"
#include "pycore_jit.h"
#include "pycore_list.h"
#include "pycore_long.h"
#include "pycore_opcode_metadata.h"
#include "pycore_opcode_utils.h"
#include "pycore_optimizer.h"
#include "pycore_pyatomic_ft_wrappers.h"
#include "pycore_range.h"
#include "pycore_setobject.h"
#include "pycore_sliceobject.h"
#include "pycore_stackref.h"
#include "pycore_template.h"
#include "pycore_tuple.h"
#include "pycore_unicodeobject.h"

#include "ceval_macros.h"

#include "jit.h"

#undef CURRENT_OPARG
#define CURRENT_OPARG() (_oparg)

#undef CURRENT_OPERAND0
#define CURRENT_OPERAND0() (_operand0)

#undef CURRENT_OPERAND1
#define CURRENT_OPERAND1() (_operand1)

#undef CURRENT_TARGET
#define CURRENT_TARGET() (_target)

#undef TIER2_TO_TIER2
#define TIER2_TO_TIER2(EXECUTOR)                                           \
do {                                                                       \
    OPT_STAT_INC(traces_executed);                                         \
    _PyExecutorObject *_executor = (EXECUTOR);                             \
    jit_func_preserve_none jitted = _executor->jit_code;                   \
    __attribute__((musttail)) return jitted(frame, stack_pointer, tstate, _tos_cache0, _tos_cache1, _tos_cache2); \
} while (0)

#undef GOTO_TIER_ONE
#define GOTO_TIER_ONE(TARGET)                       \
do {                                                \
    tstate->current_executor = NULL;                \
    _PyFrame_SetStackPointer(frame, stack_pointer); \
    return TARGET;                                  \
} while (0)

#undef LOAD_IP
#define LOAD_IP(UNUSED) \
    do {                \
    } while (0)

#undef LLTRACE_RESUME_FRAME
#define LLTRACE_RESUME_FRAME() \
    do {                       \
    } while (0)

#define PATCH_JUMP(ALIAS)                                                 \
do {                                                                      \
    DECLARE_TARGET(ALIAS);                                                \
    __attribute__((musttail)) return ALIAS(frame, stack_pointer, tstate, _tos_cache0, _tos_cache1, _tos_cache2); \
} while (0)

#undef JUMP_TO_JUMP_TARGET
#define JUMP_TO_JUMP_TARGET() PATCH_JUMP(_JIT_JUMP_TARGET)

#undef JUMP_TO_ERROR
#define JUMP_TO_ERROR() PATCH_JUMP(_JIT_ERROR_TARGET)

#define TIER_TWO 2

__attribute__((preserve_none)) _Py_CODEUNIT *
_JIT_ENTRY(
    _PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate,
    _PyStackRef _tos_cache0, _PyStackRef _tos_cache1, _PyStackRef _tos_cache2
) {
    // Locals that the instruction implementations expect to exist:
    PATCH_VALUE(_PyExecutorObject *, current_executor, _JIT_EXECUTOR)
    int oparg;
    int uopcode = _JIT_OPCODE;
    _Py_CODEUNIT *next_instr;
    // Other stuff we need handy:
    PATCH_VALUE(uint16_t, _oparg, _JIT_OPARG)
#if SIZEOF_VOID_P == 8
    PATCH_VALUE(uint64_t, _operand0, _JIT_OPERAND0)
    PATCH_VALUE(uint64_t, _operand1, _JIT_OPERAND1)
#else
    assert(SIZEOF_VOID_P == 4);
    PATCH_VALUE(uint32_t, _operand0_hi, _JIT_OPERAND0_HI)
    PATCH_VALUE(uint32_t, _operand0_lo, _JIT_OPERAND0_LO)
    uint64_t _operand0 = ((uint64_t)_operand0_hi << 32) | _operand0_lo;
    PATCH_VALUE(uint32_t, _operand1_hi, _JIT_OPERAND1_HI)
    PATCH_VALUE(uint32_t, _operand1_lo, _JIT_OPERAND1_LO)
    uint64_t _operand1 = ((uint64_t)_operand1_hi << 32) | _operand1_lo;
#endif
    PATCH_VALUE(uint32_t, _target, _JIT_TARGET)
    OPT_STAT_INC(uops_executed);
    UOP_STAT_INC(uopcode, execution_count);
    switch (uopcode) {
        // The actual instruction definition gets inserted here:
        CASE
        default:
            Py_UNREACHABLE();
    }
    PATCH_JUMP(_JIT_CONTINUE);
}
