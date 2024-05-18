
#ifndef Py_LIMITED_API
#ifndef Py_OPTIMIZER_H
#define Py_OPTIMIZER_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PyExecutorLinkListNode {
    struct _PyExecutorObject *next;
    struct _PyExecutorObject *previous;
} _PyExecutorLinkListNode;


/* Bloom filter with m = 256
 * https://en.wikipedia.org/wiki/Bloom_filter */
#define _Py_BLOOM_FILTER_WORDS 8

typedef struct {
    uint32_t bits[_Py_BLOOM_FILTER_WORDS];
} _PyBloomFilter;

typedef struct {
    uint8_t opcode;
    uint8_t oparg;
    uint8_t valid;
    uint8_t linked;
    int index;           // Index of ENTER_EXECUTOR (if code isn't NULL, below).
    _PyBloomFilter bloom;
    _PyExecutorLinkListNode links;
    PyCodeObject *code;  // Weak (NULL if no corresponding ENTER_EXECUTOR).
} _PyVMData;

/* Depending on the format,
 * the 32 bits between the oparg and operand are:
 * UOP_FORMAT_TARGET:
 *    uint32_t target;
 * UOP_FORMAT_EXIT
 *    uint16_t exit_index;
 *    uint16_t error_target;
 * UOP_FORMAT_JUMP
 *    uint16_t jump_target;
 *    uint16_t error_target;
 */
typedef struct {
    uint16_t opcode:14;
    uint16_t format:2;
    uint16_t oparg;
    union {
        uint32_t target;
        struct {
            union {
                uint16_t exit_index;
                uint16_t jump_target;
            };
            uint16_t error_target;
        };
    };
    uint64_t operand;  // A cache entry
} _PyUOpInstruction;

typedef struct {
    uint32_t target;
    _Py_BackoffCounter temperature;
    const struct _PyExecutorObject *executor;
} _PyExitData;

typedef struct _PyExecutorObject {
    PyObject_VAR_HEAD
    const _PyUOpInstruction *trace;
    _PyVMData vm_data; /* Used by the VM, but opaque to the optimizer */
    uint32_t exit_count;
    uint32_t code_size;
    size_t jit_size;
    void *jit_code;
    void *jit_side_entry;
    _PyExitData exits[1];
} _PyExecutorObject;

typedef struct _PyOptimizerObject _PyOptimizerObject;

/* Should return > 0 if a new executor is created. O if no executor is produced and < 0 if an error occurred. */
typedef int (*_Py_optimize_func)(
    _PyOptimizerObject* self, struct _PyInterpreterFrame *frame,
    _Py_CODEUNIT *instr, _PyExecutorObject **exec_ptr,
    int curr_stackentries);

struct _PyOptimizerObject {
    PyObject_HEAD
    _Py_optimize_func optimize;
    /* Data needed by the optimizer goes here, but is opaque to the VM */
};

/** Test support **/
typedef struct {
    _PyOptimizerObject base;
    int64_t count;
} _PyCounterOptimizerObject;

PyAPI_FUNC(int) PyUnstable_Replace_Executor(PyCodeObject *code, _Py_CODEUNIT *instr, _PyExecutorObject *executor);

_PyOptimizerObject *_Py_SetOptimizer(PyInterpreterState *interp, _PyOptimizerObject* optimizer);

PyAPI_FUNC(int) PyUnstable_SetOptimizer(_PyOptimizerObject* optimizer);

PyAPI_FUNC(_PyOptimizerObject *) PyUnstable_GetOptimizer(void);

PyAPI_FUNC(_PyExecutorObject *) PyUnstable_GetExecutor(PyCodeObject *code, int offset);

void _Py_ExecutorInit(_PyExecutorObject *, const _PyBloomFilter *);
void _Py_ExecutorDetach(_PyExecutorObject *);
void _Py_BloomFilter_Init(_PyBloomFilter *);
void _Py_BloomFilter_Add(_PyBloomFilter *bloom, void *obj);
PyAPI_FUNC(void) _Py_Executor_DependsOn(_PyExecutorObject *executor, void *obj);
/* For testing */
PyAPI_FUNC(PyObject *)PyUnstable_Optimizer_NewCounter(void);
PyAPI_FUNC(PyObject *)PyUnstable_Optimizer_NewUOpOptimizer(void);

#define _Py_MAX_ALLOWED_BUILTINS_MODIFICATIONS 3
#define _Py_MAX_ALLOWED_GLOBALS_MODIFICATIONS 6

#ifdef _Py_TIER2
PyAPI_FUNC(void) _Py_Executors_InvalidateDependency(PyInterpreterState *interp, void *obj, int is_invalidation);
PyAPI_FUNC(void) _Py_Executors_InvalidateAll(PyInterpreterState *interp, int is_invalidation);
#else
#  define _Py_Executors_InvalidateDependency(A, B, C) ((void)0)
#  define _Py_Executors_InvalidateAll(A, B) ((void)0)
#endif


#ifdef __cplusplus
}
#endif
#endif /* !Py_OPTIMIZER_H */
#endif /* Py_LIMITED_API */
