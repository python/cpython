#include "Python.h"

#include "pycore_frame.h"
#include "pycore_jit.h"

// Stuff that will be patched at "JIT time":
extern int _justin_continue(PyThreadState *tstate, _PyInterpreterFrame *frame,
                            PyObject **stack_pointer, _Py_CODEUNIT *next_instr
                            , PyObject *_tos1
                            , PyObject *_tos2
                            , PyObject *_tos3
                            , PyObject *_tos4
                            );

_PyJITReturnCode
_justin_trampoline(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   PyObject **stack_pointer, _Py_CODEUNIT *next_instr)
{
    PyObject *_tos1 = stack_pointer[/* DON'T REPLACE ME */ -1];
    PyObject *_tos2 = stack_pointer[/* DON'T REPLACE ME */ -2];
    PyObject *_tos3 = stack_pointer[/* DON'T REPLACE ME */ -3];
    PyObject *_tos4 = stack_pointer[/* DON'T REPLACE ME */ -4];
    return _justin_continue(tstate, frame, stack_pointer, next_instr
                            , _tos1
                            , _tos2
                            , _tos3
                            , _tos4
                            );
}
