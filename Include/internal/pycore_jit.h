#ifndef Py_INTERNAL_JIT_H
#define Py_INTERNAL_JIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pycore_interp.h"
#include "pycore_optimizer.h"
#include "pycore_stackref.h"

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#ifdef _Py_JIT

typedef _Py_CODEUNIT *(*jit_func)(_PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate);

// JIT code has no function type metadata for UBSan to check.
static inline _Py_CODEUNIT* _Py_NO_SANITIZE_UNDEFINED
_PyJIT_Entry(jit_func jitted, _PyInterpreterFrame *frame,
            _PyStackRef *stack_pointer, PyThreadState *tstate)
{
    return jitted(frame, stack_pointer, tstate);
}

int _PyJIT_Compile(_PyExecutorObject *executor, const _PyUOpInstruction *trace, size_t length);
void _PyJIT_Free(_PyExecutorObject *executor);

#endif  // _Py_JIT

#ifdef __cplusplus
}
#endif

#endif // !Py_INTERNAL_JIT_H
