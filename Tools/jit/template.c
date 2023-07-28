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
extern _PyInterpreterFrame *_jit_continue(_PyExecutorObject *executor,
                                          _PyInterpreterFrame *frame,
                                          PyObject **stack_pointer,
                                          PyThreadState *tstate, int pc
                                          , PyObject *_tos1
                                          , PyObject *_tos2
                                          , PyObject *_tos3
                                          , PyObject *_tos4
                                          );
// The address of an extern can't be 0:
extern void _jit_oparg_plus_one;
extern void _jit_operand_plus_one;
extern _Py_CODEUNIT _jit_pc_plus_one;

// XXX
#define ip_offset ((_Py_CODEUNIT *)_PyFrame_GetCode(frame)->co_code_adaptive)

_PyInterpreterFrame *
_jit_entry(_PyExecutorObject *executor, _PyInterpreterFrame *frame,
           PyObject **stack_pointer, PyThreadState *tstate, int pc
           , PyObject *_tos1
           , PyObject *_tos2
           , PyObject *_tos3
           , PyObject *_tos4
           )
{
    __builtin_assume(_tos1 == stack_pointer[/* DON'T REPLACE ME */ -1]);
    __builtin_assume(_tos2 == stack_pointer[/* DON'T REPLACE ME */ -2]);
    __builtin_assume(_tos3 == stack_pointer[/* DON'T REPLACE ME */ -3]);
    __builtin_assume(_tos4 == stack_pointer[/* DON'T REPLACE ME */ -4]);
    if (pc != (intptr_t)&_jit_pc_plus_one - 1) {
        return _PyUopExecute(executor, frame, stack_pointer, pc);
    }
    // Locals that the instruction implementations expect to exist:
    _PyUOpExecutorObject *self = (_PyUOpExecutorObject *)executor;
    uint32_t opcode = _JIT_OPCODE;
    int32_t oparg = (uintptr_t)&_jit_oparg_plus_one - 1;
    uint64_t operand = (uintptr_t)&_jit_operand_plus_one - 1;
    assert(self->trace[pc].opcode == opcode);
    assert(self->trace[pc].oparg == oparg);
    assert(self->trace[pc].operand == operand);
    pc++;
    switch (opcode) {
        // Now, the actual instruction definitions (only one will be used):
#include "Python/executor_cases.c.h"
        default:
            Py_UNREACHABLE();
    }
    // Finally, the continuation:
    _tos1 = stack_pointer[/* DON'T REPLACE ME */ -1];
    _tos2 = stack_pointer[/* DON'T REPLACE ME */ -2];
    _tos3 = stack_pointer[/* DON'T REPLACE ME */ -3];
    _tos4 = stack_pointer[/* DON'T REPLACE ME */ -4];
    __attribute__((musttail))
    return _jit_continue(executor, frame, stack_pointer, tstate, pc
                         , _tos1
                         , _tos2
                         , _tos3
                         , _tos4
                         );
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
