
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
#define BLOOM_FILTER_WORDS 8

typedef struct _bloom_filter {
    uint32_t bits[BLOOM_FILTER_WORDS];
} _PyBloomFilter;

typedef struct {
    uint8_t opcode;
    uint8_t oparg;
    uint8_t valid;
    uint8_t linked;
    _PyBloomFilter bloom;
    _PyExecutorLinkListNode links;
} _PyVMData;

typedef struct _PyExecutorObject {
    PyObject_VAR_HEAD
    /* WARNING: execute consumes a reference to self. This is necessary to allow executors to tail call into each other. */
    _Py_CODEUNIT *(*execute)(struct _PyExecutorObject *self, struct _PyInterpreterFrame *frame, PyObject **stack_pointer);
    _PyVMData vm_data; /* Used by the VM, but opaque to the optimizer */
    /* Data needed by the executor goes here, but is opaque to the VM */
} _PyExecutorObject;

typedef struct _PyOptimizerObject _PyOptimizerObject;

/* Should return > 0 if a new executor is created. O if no executor is produced and < 0 if an error occurred. */
typedef int (*optimize_func)(_PyOptimizerObject* self, PyCodeObject *code, _Py_CODEUNIT *instr, _PyExecutorObject **, int curr_stackentries);

typedef struct _PyOptimizerObject {
    PyObject_HEAD
    optimize_func optimize;
    /* These thresholds are treated as signed so do not exceed INT16_MAX
     * Use INT16_MAX to indicate that the optimizer should never be called */
    uint16_t resume_threshold;
    uint16_t backedge_threshold;
    /* Data needed by the optimizer goes here, but is opaque to the VM */
} _PyOptimizerObject;

PyAPI_FUNC(int) PyUnstable_Replace_Executor(PyCodeObject *code, _Py_CODEUNIT *instr, _PyExecutorObject *executor);

PyAPI_FUNC(void) PyUnstable_SetOptimizer(_PyOptimizerObject* optimizer);

PyAPI_FUNC(_PyOptimizerObject *) PyUnstable_GetOptimizer(void);

PyAPI_FUNC(_PyExecutorObject *) PyUnstable_GetExecutor(PyCodeObject *code, int offset);

int
_PyOptimizer_BackEdge(struct _PyInterpreterFrame *frame, _Py_CODEUNIT *src, _Py_CODEUNIT *dest, PyObject **stack_pointer);

extern _PyOptimizerObject _PyOptimizer_Default;

void _Py_ExecutorInit(_PyExecutorObject *, _PyBloomFilter *);
void _Py_ExecutorClear(_PyExecutorObject *);
void _Py_BloomFilter_Init(_PyBloomFilter *);
void _Py_BloomFilter_Add(_PyBloomFilter *bloom, void *obj);
PyAPI_FUNC(void) _Py_Executor_DependsOn(_PyExecutorObject *executor, void *obj);
PyAPI_FUNC(void) _Py_Executors_InvalidateDependency(PyInterpreterState *interp, void *obj);
extern void _Py_Executors_InvalidateAll(PyInterpreterState *interp);

/* For testing */
PyAPI_FUNC(PyObject *)PyUnstable_Optimizer_NewCounter(void);
PyAPI_FUNC(PyObject *)PyUnstable_Optimizer_NewUOpOptimizer(void);

#define OPTIMIZER_BITS_IN_COUNTER 4
/* Minimum of 16 additional executions before retry */
#define MINIMUM_TIER2_BACKOFF 4

#ifdef __cplusplus
}
#endif
#endif /* !Py_OPTIMIZER_H */
#endif /* Py_LIMITED_API */
