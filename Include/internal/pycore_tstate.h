#ifndef Py_INTERNAL_TSTATE_H
#define Py_INTERNAL_TSTATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_brc.h"             // struct _brc_thread_state
#include "pycore_freelist_state.h"  // struct _Py_freelists
#include "pycore_mimalloc.h"        // struct _mimalloc_thread_state
#include "pycore_qsbr.h"            // struct qsbr
#include "pycore_uop.h"             // struct _PyUOpInstruction
#include "pycore_structs.h"

#ifdef Py_GIL_DISABLED
struct _gc_thread_state {
    /* Thread-local allocation count. */
    Py_ssize_t alloc_count;
};
#endif

#if _Py_TIER2
typedef struct _PyJitTracerInitialState {
    int stack_depth;
    int chain_depth;
    struct _PyExitData *exit;
    PyCodeObject *code; // Strong
    PyFunctionObject *func; // Strong
    _Py_CODEUNIT *start_instr;
    _Py_CODEUNIT *close_loop_instr;
    _Py_CODEUNIT *jump_backward_instr;
} _PyJitTracerInitialState;

typedef struct _PyJitTracerPreviousState {
    bool dependencies_still_valid;
    bool instr_is_super;
    int code_max_size;
    int code_curr_size;
    int instr_oparg;
    int instr_stacklevel;
    _Py_CODEUNIT *instr;
    PyCodeObject *instr_code; // Strong
    struct _PyInterpreterFrame *instr_frame;
    _PyBloomFilter dependencies;
} _PyJitTracerPreviousState;

typedef struct _PyJitTracerState {
    _PyUOpInstruction *code_buffer;
    _PyJitTracerInitialState initial_state;
    _PyJitTracerPreviousState prev_state;
} _PyJitTracerState;
#endif

// Every PyThreadState is actually allocated as a _PyThreadStateImpl. The
// PyThreadState fields are exposed as part of the C API, although most fields
// are intended to be private. The _PyThreadStateImpl fields not exposed.
typedef struct _PyThreadStateImpl {
    // semi-public fields are in PyThreadState.
    PyThreadState base;

    // The reference count field is used to synchronize deallocation of the
    // thread state during runtime finalization.
    Py_ssize_t refcount;

    // These are addresses, but we need to convert to ints to avoid UB.
    uintptr_t c_stack_top;
    uintptr_t c_stack_soft_limit;
    uintptr_t c_stack_hard_limit;

    // PyUnstable_ThreadState_ResetStackProtection() values
    uintptr_t c_stack_init_base;
    uintptr_t c_stack_init_top;

    PyObject *asyncio_running_loop; // Strong reference
    PyObject *asyncio_running_task; // Strong reference

    /* Head of circular linked-list of all tasks which are instances of `asyncio.Task`
       or subclasses of it used in `asyncio.all_tasks`.
    */
    struct llist_node asyncio_tasks_head;
    struct _qsbr_thread_state *qsbr;  // only used by free-threaded build
    struct llist_node mem_free_queue; // delayed free queue

#ifdef Py_GIL_DISABLED
    // Stack references for the current thread that exist on the C stack
    struct _PyCStackRef *c_stack_refs;
    struct _gc_thread_state gc;
    struct _mimalloc_thread_state mimalloc;
    struct _Py_freelists freelists;
    struct _brc_thread_state brc;
    struct {
        // The per-thread refcounts
        Py_ssize_t *values;

        // Size of the refcounts array.
        Py_ssize_t size;

        // If set, don't use per-thread refcounts
        int is_finalized;
    } refcounts;

    // Index to use to retrieve thread-local bytecode for this thread
    int32_t tlbc_index;

    // When >1, code objects do not immortalize their non-string constants.
    int suppress_co_const_immortalization;

#ifdef Py_STATS
     // per-thread stats, will be merged into interp->pystats_struct
     PyStats *pystats_struct; // allocated by _PyStats_ThreadInit()
#endif

#endif // Py_GIL_DISABLED

#if defined(Py_REF_DEBUG) && defined(Py_GIL_DISABLED)
    Py_ssize_t reftotal;  // this thread's total refcount operations
#endif
#if _Py_TIER2
    _PyJitTracerState jit_tracer_state;
#endif
} _PyThreadStateImpl;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TSTATE_H */
