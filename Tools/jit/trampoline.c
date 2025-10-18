#include "Python.h"

#include "pycore_ceval.h"
#include "pycore_frame.h"
#include "pycore_jit.h"

#include "jit.h"

_Py_CODEUNIT *
_JIT_ENTRY(
    _PyExecutorObject *exec, _PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate
) {
    typedef _Py_CODEUNIT *__attribute__((preserve_none)) (*jit_func)(_PyInterpreterFrame *, _PyStackRef *, PyThreadState *);
    jit_func jitted = (jit_func)exec->jit_code;
    return jitted(frame, stack_pointer, tstate);
}
