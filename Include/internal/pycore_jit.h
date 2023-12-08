#ifndef Py_INTERNAL_JIT_H
#define Py_INTERNAL_JIT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_uops.h"

typedef _PyInterpreterFrame *(*_PyJITContinueFunction)(
    _PyInterpreterFrame *frame, PyObject **stack_pointer, PyThreadState *tstate);

#ifdef _Py_JIT

int _PyJIT_Compile(_PyUOpExecutorObject *executor);
void _PyJIT_Free(_PyUOpExecutorObject *executor);

#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_JIT_H */
