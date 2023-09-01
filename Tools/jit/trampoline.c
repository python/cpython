#include "Python.h"

#include "pycore_frame.h"

// Stuff that will be patched at "JIT time":
extern _PyInterpreterFrame *_jit_continue(_PyInterpreterFrame *frame,
                                          PyObject **stack_pointer,
                                          PyThreadState *tstate);

_PyInterpreterFrame *
_jit_trampoline(_PyExecutorObject *executor, _PyInterpreterFrame *frame,
                PyObject **stack_pointer)
{
    PyThreadState *tstate = PyThreadState_Get();
    frame = _jit_continue(frame, stack_pointer, tstate);
    Py_DECREF(executor);
    return frame;
}
