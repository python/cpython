
#ifndef Py_LIMITED_API
#ifndef Py_OPTIMIZER_H
#define Py_OPTIMIZER_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t opcode;
    uint8_t oparg;
    uint16_t _;
} _PyVMData;

typedef uint32_t _PyOptimizerCapabilities; /* Data used by the VM for executor object management */

typedef struct _PyExecutorObject {
    PyObject_HEAD
    /* WARNING: execute consumes a reference to self. This is necessary to allow executors to tail call into each other. */
    struct _PyInterpreterFrame *(*execute)(struct _PyExecutorObject *self, struct _PyInterpreterFrame *frame, PyObject **stack_pointer);
    _PyVMData vm_data; /* Used by the VM, but opaque to the optimizer */
    /* Data needed by the executor goes here, but is opaque to the VM */
} _PyExecutorObject;

typedef struct _PyOptimizerObject _PyOptimizerObject;

/* This would be nicer as an enum, but C doesn't define the size of enums */
#define PY_OPTIMIZE_FUNCTION_ENTRY 1
#define PY_OPTIMIZE_RESUME_AFTER_YIELD 2
#define PY_OPTIMIZE_LOOP 4
#define PY_OPTIMIZE_ANYWHERE 8

typedef struct _PyInterpreterFrame *(*optimize_func)(_PyOptimizerObject* self, struct _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, PyObject **stack_pointer);

typedef struct _PyOptimizerObject {
    PyObject_HEAD
    optimize_func optimize;
    _PyOptimizerCapabilities capabilities;
    /* Data needed by the compiler goes here, but is opaque to the VM */
} _PyOptimizerObject;

int PyUnstable_Insert_Executor(PyCodeObject *code, int offset, _PyExecutorObject *executor);

int PyUnstable_RegisterOptimizer(_PyOptimizerObject* optimizer);

extern _PyOptimizerObject _PyOptimizer_Default;

#define OPTIMIZER_BITS_IN_COUNTER 4

#ifdef __cplusplus
}
#endif
#endif /* !Py_OPTIMIZER_H */
#endif /* Py_LIMITED_API */
