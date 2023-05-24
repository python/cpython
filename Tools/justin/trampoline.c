#include "Python.h"

#include "pycore_frame.h"
#include "pycore_jit.h"

// Stuff that will be patched at "JIT time":
extern int _justin_continue(PyThreadState *tstate, _PyInterpreterFrame *frame,
                            PyObject **stack_pointer, _Py_CODEUNIT *next_instr);

_PyJITReturnCode
_justin_trampoline(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   PyObject **stack_pointer, _Py_CODEUNIT *next_instr)
{
    return _justin_continue(tstate, frame, stack_pointer, next_instr);
}
