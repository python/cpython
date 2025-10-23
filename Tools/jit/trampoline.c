#include "Python.h"

#include "pycore_ceval.h"
#include "pycore_frame.h"
#include "pycore_jit.h"

#include "jit.h"

_Py_CODEUNIT *
_JIT_ENTRY(
    _PyExecutorObject *exec, _PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate
) {
    tstate->jit_exit = NULL;
    jit_func_preserve_none jitted = (jit_func_preserve_none)exec->jit_code;
    return jitted(frame, stack_pointer, tstate, PyStackRef_ZERO_BITS, PyStackRef_ZERO_BITS, PyStackRef_ZERO_BITS);
}
