#include "Python.h"

#include "pycore_abstract.h"
#include "pycore_call.h"
#include "pycore_ceval.h"
#include "pycore_dict.h"
#include "pycore_emscripten_signal.h"
#include "pycore_intrinsics.h"
#include "pycore_long.h"
#include "pycore_object.h"
#include "pycore_opcode.h"
#include "pycore_opcode_metadata.h"
#include "pycore_pyerrors.h"
#include "pycore_range.h"
#include "pycore_sliceobject.h"
#include "pycore_uops.h"
#include "pycore_jit.h"

#include "Python/ceval_macros.h"

#undef DEOPT_IF
#define DEOPT_IF(COND, INSTNAME) \
    if ((COND)) {                \
        goto deoptimize;         \
    }
#undef ENABLE_SPECIALIZATION
#define ENABLE_SPECIALIZATION 0

// Stuff that will be patched at "JIT time":
extern _PyInterpreterFrame *_justin_continue(_PyExecutorObject *executor,
                                             _PyInterpreterFrame *frame,
                                             PyObject **stack_pointer,
                                             PyThreadState *tstate, int pc
                                             , PyObject *_tos1
                                             , PyObject *_tos2
                                             , PyObject *_tos3
                                             , PyObject *_tos4
                                             );
extern _Py_CODEUNIT _justin_pc_plus_one;
extern void _justin_operand_plus_one;

// XXX
#define ip_offset ((_Py_CODEUNIT *)_PyFrame_GetCode(frame)->co_code_adaptive)

_PyInterpreterFrame *
_justin_entry(_PyExecutorObject *executor, _PyInterpreterFrame *frame,
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
    _PyUOpExecutorObject *self = (_PyUOpExecutorObject *)executor;
    int opcode = _JUSTIN_OPCODE;
    // Locals that the instruction implementations expect to exist:
    // The address of an extern can't be 0:
    uint64_t operand = (uintptr_t)&_justin_operand_plus_one - 1;
    int oparg = (int)operand;
    if (pc != (intptr_t)&_justin_pc_plus_one - 1) {
        return _PyUopExecute(executor, frame, stack_pointer, pc);
    }
    assert(self->trace[pc].opcode == opcode);
    assert(self->trace[pc].operand == operand);
    pc++;
    switch (opcode) {
    // Now, the actual instruction definition:
%s
        default:
            Py_UNREACHABLE();
    }
    // Finally, the continuation:
    _tos1 = stack_pointer[/* DON'T REPLACE ME */ -1];
    _tos2 = stack_pointer[/* DON'T REPLACE ME */ -2];
    _tos3 = stack_pointer[/* DON'T REPLACE ME */ -3];
    _tos4 = stack_pointer[/* DON'T REPLACE ME */ -4];
    __attribute__((musttail))
    return _justin_continue(executor, frame, stack_pointer, tstate, pc
                            , _tos1
                            , _tos2
                            , _tos3
                            , _tos4
                            );
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
    _PyFrame_SetStackPointer(frame, stack_pointer);
    Py_DECREF(self);
    return NULL;
deoptimize:
    frame->prev_instr--;  // Back up to just before destination
    _PyFrame_SetStackPointer(frame, stack_pointer);
    Py_DECREF(self);
    return frame;
}
