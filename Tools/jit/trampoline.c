#include "Python.h"

#include "pycore_frame.h"

// Stuff that will be patched at "JIT time":
extern _PyInterpreterFrame *_jit_continue(_PyExecutorObject *executor,
                                          _PyInterpreterFrame *frame,
                                          PyObject **stack_pointer,
                                          PyThreadState *tstate);

_PyInterpreterFrame *
_jit_trampoline(_PyExecutorObject *executor, _PyInterpreterFrame *frame,
                PyObject **stack_pointer)
{
    PyThreadState *tstate = PyThreadState_Get();
    return _jit_continue(executor, frame, stack_pointer, tstate);
}
