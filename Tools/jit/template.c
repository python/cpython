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

#include "Python/ceval_macros.h"

#undef DEOPT_IF
#define DEOPT_IF(COND, INSTNAME) \
    if ((COND)) {                \
        goto deoptimize;         \
    }
#undef ENABLE_SPECIALIZATION
#define ENABLE_SPECIALIZATION 0
#undef ASSERT_KWNAMES_IS_NULL
#define ASSERT_KWNAMES_IS_NULL() (void)0

// Stuff that will be patched at "JIT time":
extern _PyInterpreterFrame *_jit_branch(_PyExecutorObject *executor,
                                        _PyInterpreterFrame *frame,
                                        PyObject **stack_pointer,
                                        PyThreadState *tstate);
extern _PyInterpreterFrame *_jit_continue(_PyExecutorObject *executor,
                                          _PyInterpreterFrame *frame,
                                          PyObject **stack_pointer,
                                          PyThreadState *tstate);
extern _PyInterpreterFrame *_jit_loop(_PyExecutorObject *executor,
                                      _PyInterpreterFrame *frame,
                                      PyObject **stack_pointer,
                                      PyThreadState *tstate);
// The address of an extern can't be 0:
extern void _jit_oparg_plus_one;
extern void _jit_operand_plus_one;
extern _Py_CODEUNIT _jit_pc_plus_one;

// XXX
#define ip_offset ((_Py_CODEUNIT *)_PyFrame_GetCode(frame)->co_code_adaptive)

_PyInterpreterFrame *
_jit_entry(_PyExecutorObject *executor, _PyInterpreterFrame *frame,
           PyObject **stack_pointer, PyThreadState *tstate
           )
{
    // Locals that the instruction implementations expect to exist:
    _PyUOpExecutorObject *self = (_PyUOpExecutorObject *)executor;
    uint32_t opcode = _JIT_OPCODE;
    int32_t oparg = (uintptr_t)&_jit_oparg_plus_one - 1;
    uint64_t operand = (uintptr_t)&_jit_operand_plus_one - 1;
    int pc = -1;
    switch (opcode) {
        // Now, the actual instruction definitions (only one will be used):
#include "Python/executor_cases.c.h"
        default:
            Py_UNREACHABLE();
    }
    if (pc != -1) {
        if (opcode == JUMP_TO_TOP) {
            __attribute__((musttail))
            return _jit_loop(executor, frame, stack_pointer, tstate);
        }
        assert(opcode == _POP_JUMP_IF_FALSE || opcode == _POP_JUMP_IF_TRUE);
        __attribute__((musttail))
        return _jit_branch(executor, frame, stack_pointer, tstate);
    }
    // Finally, the continuation:
    __attribute__((musttail))
    return _jit_continue(executor, frame, stack_pointer, tstate);
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
    Py_DECREF(self);
    return NULL;
deoptimize:
    frame->prev_instr--;
    _PyFrame_SetStackPointer(frame, stack_pointer);
    Py_DECREF(self);
    return frame;
}
