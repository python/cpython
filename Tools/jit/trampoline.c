#include "Python.h"

#include "pycore_frame.h"

extern _PyInterpreterFrame *_JIT_CONTINUE(_PyInterpreterFrame *frame, PyObject **stack_pointer, PyThreadState *tstate);

_PyInterpreterFrame *
_JIT_TRAMPOLINE(_PyExecutorObject *executor, _PyInterpreterFrame *frame, PyObject **stack_pointer)
{
    frame = _JIT_CONTINUE(frame, stack_pointer, PyThreadState_Get());
    Py_DECREF(executor);
    return frame;
}
