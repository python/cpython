#include "Python.h"

#include "pycore_frame.h"

// Stuff that will be patched at "JIT time":
extern _PyInterpreterFrame *_JIT_CONTINUE(_PyInterpreterFrame *frame,
                                          PyObject **stack_pointer,
                                          PyThreadState *tstate, int32_t oparg,
                                          uint64_t operand);
extern void _JIT_CONTINUE_OPARG;
extern void _JIT_CONTINUE_OPERAND;

_PyInterpreterFrame *
_JIT_TRAMPOLINE(_PyExecutorObject *executor, _PyInterpreterFrame *frame,
                PyObject **stack_pointer)
{
    PyThreadState *tstate = PyThreadState_Get();
    int32_t oparg = (uintptr_t)&_JIT_CONTINUE_OPARG;
    uint64_t operand = (uintptr_t)&_JIT_CONTINUE_OPERAND;
    frame = _JIT_CONTINUE(frame, stack_pointer, tstate, oparg, operand);
    Py_DECREF(executor);
    return frame;
}
