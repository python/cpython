#include "Python.h"

#include "pycore_frame.h"

// Stuff that will be patched at "JIT time":
extern int _justin_continue(PyThreadState *tstate, _PyInterpreterFrame *frame,
                            PyObject **stack_pointer);

int
_justin_trampoline(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   PyObject **stack_pointer)
{
    return _justin_continue(tstate, frame, stack_pointer);
}
