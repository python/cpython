#include "Python.h"

#include "pycore_frame.h"

// Stuff that will be patched at "JIT time":
extern _PyInterpreterFrame *_jit_continue(_PyExecutorObject *executor,
                                          _PyInterpreterFrame *frame,
                                          PyObject **stack_pointer,
                                          PyThreadState *tstate
                                          , PyObject *_tos1
                                          , PyObject *_tos2
                                          , PyObject *_tos3
                                          , PyObject *_tos4
                                          );

_PyInterpreterFrame *
_jit_trampoline(_PyExecutorObject *executor, _PyInterpreterFrame *frame,
                PyObject **stack_pointer)
{
    PyObject *_tos1 = stack_pointer[/* DON'T REPLACE ME */ -1];
    PyObject *_tos2 = stack_pointer[/* DON'T REPLACE ME */ -2];
    PyObject *_tos3 = stack_pointer[/* DON'T REPLACE ME */ -3];
    PyObject *_tos4 = stack_pointer[/* DON'T REPLACE ME */ -4];
    PyThreadState *tstate = PyThreadState_Get();
    return _jit_continue(executor, frame, stack_pointer, tstate
                         , _tos1
                         , _tos2
                         , _tos3
                         , _tos4
                         );
}
