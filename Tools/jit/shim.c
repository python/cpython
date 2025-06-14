#include "Python.h"

#include "pycore_ceval.h"
#include "pycore_frame.h"
#include "pycore_jit.h"

#include "jit.h"

_Py_CODEUNIT *
_JIT_ENTRY(
    _PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate,
    _PyStackRef _tos_cache0, _PyStackRef _tos_cache1, _PyStackRef _tos_cache2
) {
    // Note that this is *not* a tail call:
    PATCH_VALUE(jit_func_preserve_none, call, _JIT_CONTINUE);
    return call(frame, stack_pointer, tstate, _tos_cache0, _tos_cache1, _tos_cache2);
}
