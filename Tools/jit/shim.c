#include "Python.h"

#include "pycore_ceval.h"
#include "pycore_frame.h"
#include "pycore_jit.h"

#include "jit.h"

_Py_CODEUNIT *
_JIT_ENTRY(_PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate)
{
    // Note that this is *not* a tail call:
    DECLARE_TARGET(_JIT_CONTINUE);
    return _JIT_CONTINUE(frame, stack_pointer, tstate);
}
