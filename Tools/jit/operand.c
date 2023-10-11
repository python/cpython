#include "Python.h"

#include "pycore_frame.h"

extern _PyInterpreterFrame *_JIT_CONTINUE(_PyInterpreterFrame *frame, PyObject **stack_pointer, PyThreadState *tstate, int32_t oparg, uint64_t operand);
extern void _JIT_OPERAND;

_PyInterpreterFrame *
_JIT_ENTRY(_PyInterpreterFrame *frame, PyObject **stack_pointer, PyThreadState *tstate, int32_t oparg, uint64_t operand)
{
    __attribute__((musttail))
    return _JIT_CONTINUE(frame, stack_pointer, tstate, oparg, (uintptr_t)&_JIT_OPERAND);
}
