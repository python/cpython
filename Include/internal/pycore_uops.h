#ifndef Py_INTERNAL_UOPS_H
#define Py_INTERNAL_UOPS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_frame.h"         // _PyInterpreterFrame

#define _Py_UOP_MAX_TRACE_LENGTH 512

typedef struct {
    uint16_t opcode;
    uint16_t oparg;
    uint32_t target;
    uint64_t operand;  // A cache entry
} _PyUOpInstruction;

typedef struct {
    _PyExecutorObject base;
    _PyUOpInstruction trace[1];
} _PyUOpExecutorObject;

_Py_CODEUNIT *_PyUOpExecute(
    _PyExecutorObject *executor,
    _PyInterpreterFrame *frame,
    PyObject **stack_pointer);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_UOPS_H */
