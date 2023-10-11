#include "Python.h"

#include "pycore_frame.h"

extern _PyInterpreterFrame *_JIT_CONTINUE(_PyInterpreterFrame *frame, PyObject **stack_pointer, PyThreadState *tstate, int32_t oparg, uint64_t operand);

_PyInterpreterFrame *
_JIT_TRAMPOLINE(_PyExecutorObject *executor, _PyInterpreterFrame *frame, PyObject **stack_pointer)
{
    frame = _JIT_CONTINUE(frame, stack_pointer, PyThreadState_Get(), 0, 0);
    Py_DECREF(executor);
    return frame;
}
