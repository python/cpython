#include "Python.h"

#include "pycore_frame.h"

// Stuff that will be patched at "JIT time":
extern _PyInterpreterFrame *_jit_continue(_PyInterpreterFrame *frame,
                                          PyObject **stack_pointer,
                                          PyThreadState *tstate,
                                          _Py_CODEUNIT *ip_offset);

_PyInterpreterFrame *
_jit_trampoline(_PyExecutorObject *executor, _PyInterpreterFrame *frame,
                PyObject **stack_pointer)
{
    PyThreadState *tstate = PyThreadState_Get();
    _Py_CODEUNIT *ip_offset = (_Py_CODEUNIT *)_PyFrame_GetCode(frame)->co_code_adaptive;
    frame = _jit_continue(frame, stack_pointer, tstate, ip_offset);
    Py_DECREF(executor);
    return frame;
}
