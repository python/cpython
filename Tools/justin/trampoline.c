#define Py_BUILD_CORE

#include "Python.h"

#include "pycore_frame.h"

// Stuff that will be patched at "JIT time":
extern int _justin_continue(PyThreadState *tstate, _PyInterpreterFrame *frame, PyObject **stack_pointer);

int
_justin_trampoline(void)
{
    PyThreadState *tstate = PyThreadState_GET();
    _PyInterpreterFrame *frame = tstate->cframe->current_frame->previous->previous;
    PyObject **stack_pointer = _PyFrame_GetStackPointer(frame);
    return _justin_continue(tstate, frame, stack_pointer);
}
