#include "Python.h"

#include "pycore_ceval.h"
#include "pycore_frame.h"
#include "pycore_jit.h"

#include "jit.h"

_Py_CODEUNIT *
_JIT_ENTRY(
    _PyExecutorObject *exec, _PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate
) {
    typedef DECLARE_TARGET((*jit_func));
    jit_func jitted = (jit_func)exec->jit_code;
    if (--tstate->interp->trace_run_counter == 0) {
        _Py_set_eval_breaker_bit(tstate, _PY_EVAL_JIT_INVALIDATE_COLD_BIT);
    }
    return jitted(frame, stack_pointer, tstate);
}
