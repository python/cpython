
#ifndef Py_LIMITED_API
#ifndef Py_OPTIMIZER_H
#define Py_OPTIMIZER_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t opcode;
    uint8_t oparg;
} _PyVMData;

typedef struct _PyExecutorObject {
    PyObject_HEAD
    /* WARNING: execute consumes a reference to self. This is necessary to allow executors to tail call into each other. */
    struct _PyInterpreterFrame *(*execute)(struct _PyExecutorObject *self, struct _PyInterpreterFrame *frame, PyObject **stack_pointer);
    _PyVMData vm_data; /* Used by the VM, but opaque to the optimizer */
    /* Data needed by the executor goes here, but is opaque to the VM */
} _PyExecutorObject;

typedef struct _PyOptimizerObject _PyOptimizerObject;

/* Should return > 0 if a new executor is created. O if no executor is produced and < 0 if an error occurred. */
typedef int (*optimize_func)(_PyOptimizerObject* self, PyCodeObject *code, _Py_CODEUNIT *instr, _PyExecutorObject **);

typedef struct _PyOptimizerObject {
    PyObject_HEAD
    optimize_func optimize;
    uint16_t resume_threshold;
    uint16_t backedge_threshold;
    /* Data needed by the optimizer goes here, but is opaque to the VM */
} _PyOptimizerObject;

PyAPI_FUNC(int) PyUnstable_Replace_Executor(PyCodeObject *code, _Py_CODEUNIT *instr, _PyExecutorObject *executor);

PyAPI_FUNC(void) PyUnstable_SetOptimizer(_PyOptimizerObject* optimizer);

PyAPI_FUNC(_PyOptimizerObject *) PyUnstable_GetOptimizer(void);

struct _PyInterpreterFrame *
_PyOptimizer_BackEdge(struct _PyInterpreterFrame *frame, _Py_CODEUNIT *src, _Py_CODEUNIT *dest, PyObject **stack_pointer);

extern _PyOptimizerObject _PyOptimizer_Default;

/* For testing */
PyAPI_FUNC(PyObject *)PyUnstable_Optimizer_NewCounter(void);
PyAPI_FUNC(PyObject *)PyUnstable_Optimizer_NewUOpOptimizer(void);

#define OPTIMIZER_BITS_IN_COUNTER 4

#ifdef __cplusplus
}
#endif
#endif /* !Py_OPTIMIZER_H */
#endif /* Py_LIMITED_API */
