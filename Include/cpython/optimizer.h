#ifndef Py_CPYTHON_OPTIMIZERL_H
#  error "this header file must not be included directly"
#endif

typedef struct _PyExecutorObject _PyExecutorObject;
typedef struct  _PyOptimizerObject  _PyOptimizerObject;
typedef struct  _PyExecutorArray  _PyExecutorArray;
typedef uint32_t _PyVMData;
typedef uint32_t _PyOptimizerCapabilities; /* Data used by the VM for executor object management */

struct _PyExecutorObject {
    PyObject_HEAD
    /* WARNING: execute consumes a reference to self. This is necessary to allow executors to tail call into each other. */
    _PyInterpreterFrame *(*execute)(_PyExecutorObject *self, _PyInterpreterFrame *frame, PyObject **stack_pointer);
    _PyVMData vm_data; /* Used by the VM, but opaque to the optimizer */
    /* Data needed by the executor goes here, but is opaque to the VM */
};

struct  _PyExecutorArray  {
    uintptr_t size;
    _PyExecutorObject *executors[1];
};

/* This would be nicer as an enum, but C doesn't define the size of enums */
#define PY_OPTIMIZE_FUNCTION_ENTRY 1
#define PY_OPTIMIZE_RESUME_AFTER_YIELD 2
#define PY_OPTIMIZE_LOOP 4
#define PY_OPTIMIZE_ANYWHERE 8

struct _PyOptimizerObject {
    PyObject_HEAD
    _PyExecutorObject *(*compile)(_PyOptimizerObject* self, PyCodeObject *code, int offset);
    _PyOptimizerCapabilities capabilities;
    float optimization_cost;
    float run_cost;
    /* Data needed by the compiler goes here, but is opaque to the VM */
};

void PyUnstable_Insert_Executor(PyCodeObject *code, int offset, _PyExecutorObject *executor);

int PyUnstable_RegisterOptimizer(_PyOptimizerObject* optimizer);

