#ifndef Py_INTERNAL_UOPS_H
#define Py_INTERNAL_UOPS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#define _Py_UOP_MAX_TRACE_LENGTH 16

typedef struct {
    int opcode;
    uint64_t operand;  // Sometimes oparg, sometimes a cache entry
} _PyUOpInstruction;

typedef struct {
    _PyExecutorObject base;
    _PyUOpInstruction trace[_Py_UOP_MAX_TRACE_LENGTH];  // TODO: variable length
} _PyUOpExecutorObject;

_PyInterpreterFrame *_PyUopExecute(
    _PyExecutorObject *executor,
    _PyInterpreterFrame *frame,
    PyObject **stack_pointer);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_UOPS_H */
