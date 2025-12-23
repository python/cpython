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

/* To be able to reason about code layout and branches, keep code size below 1 MB */
#define PY_MAX_JIT_CODE_SIZE ((1 << 20)-1)

#ifdef _Py_JIT

typedef _Py_CODEUNIT *(*jit_func)(
    _PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate,
    _PyStackRef _tos_cache0, _PyStackRef _tos_cache1, _PyStackRef _tos_cache2
);

int _PyJIT_Compile(_PyExecutorObject *executor, const _PyUOpInstruction *trace, size_t length);
void _PyJIT_Free(_PyExecutorObject *executor);
void _PyJIT_Fini(void);

#endif  // _Py_JIT

#ifdef __cplusplus
}
#endif

#endif // !Py_INTERNAL_JIT_H
