#include "Python.h"

#include "pycore_ceval.h"
#include "pycore_frame.h"
#include "pycore_jit.h"

_Py_CODEUNIT *
_ENTRY(_PyInterpreterFrame *frame, PyObject **stack_pointer, PyThreadState *tstate)
{
    PyAPI_DATA(void) _JIT_EXECUTOR;
    PyObject *executor = (PyObject *)(uintptr_t)&_JIT_EXECUTOR;
    Py_INCREF(executor);
    PyAPI_DATA(void) _JIT_CONTINUE;
    _Py_CODEUNIT *target = ((jit_func)&_JIT_CONTINUE)(frame, stack_pointer, tstate);
    Py_SETREF(tstate->previous_executor, executor);
    return target;
}
