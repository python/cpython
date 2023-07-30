#ifndef Py_INTERNAL_UOPS_H
#define Py_INTERNAL_UOPS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#define _Py_UOP_MAX_TRACE_LENGTH 32

PyAPI_DATA(PyTypeObject) PyUOpExecutor_Type;

typedef struct {
    uint32_t opcode;
    uint32_t oparg;
    uint64_t operand;  // A cache entry
} _PyUOpInstruction;

typedef struct {
    PyObject_VAR_HEAD
    _PyExecutorObject base;
    _PyUOpInstruction trace[1];
} _PyUOpExecutorObject;

_PyInterpreterFrame *_PyUopExecute(
    _PyExecutorObject *executor,
    _PyInterpreterFrame *frame,
    PyObject **stack_pointer);

/* Cast argument to _PyUOpExecutorObject* type. */
#define _PyUOpExecutor_CAST(op) \
    _Py_CAST(_PyUOpExecutorObject*, (op))

static inline Py_ssize_t PyUOpExecutor_GET_SIZE(PyObject *op) {
    _PyUOpExecutorObject *executor = _PyUOpExecutor_CAST(op);
    return Py_SIZE(executor);
}
#define PyUOpExecutor_GET_SIZE(op) PyUOpExecutor_GET_SIZE(_PyObject_CAST(op))

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_UOPS_H */
