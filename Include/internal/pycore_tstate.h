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


#ifdef Py_GIL_DISABLED
struct _gc_thread_state {
    /* Thread-local allocation count. */
    Py_ssize_t alloc_count;
};
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
#endif

#if defined(Py_REF_DEBUG) && defined(Py_GIL_DISABLED)
    Py_ssize_t reftotal;  // this thread's total refcount operations
#endif

} _PyThreadStateImpl;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TSTATE_H */
