#include "Python.h"

#include "pycore_frame.h"

// Stuff that will be patched at "JIT time":
extern int _justin_continue(PyThreadState *tstate, _PyInterpreterFrame *frame,
                            PyObject **stack_pointer, PyObject *next_instr);

int
_justin_trampoline(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   PyObject **stack_pointer, PyObject *next_instr)
{
    return _justin_continue(tstate, frame, stack_pointer, next_instr);
}
