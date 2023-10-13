#include "Python.h"

#include "pycore_frame.h"

_PyInterpreterFrame *
_JIT_ENTRY(_PyInterpreterFrame *frame, PyObject **stack_pointer, PyThreadState *tstate, int32_t oparg, uint64_t operand)
{
    _PyFrame_SetStackPointer(frame, stack_pointer);
    return NULL;
}
