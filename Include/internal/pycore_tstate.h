#ifndef Py_INTERNAL_TSTATE_H
#define Py_INTERNAL_TSTATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_brc.h"           // struct _brc_thread_state
#include "pycore_freelist.h"      // struct _Py_freelist_state
#include "pycore_mimalloc.h"      // struct _mimalloc_thread_state
#include "pycore_qsbr.h"          // struct qsbr


// Every PyThreadState is actually allocated as a _PyThreadStateImpl. The
// PyThreadState fields are exposed as part of the C API, although most fields
// are intended to be private. The _PyThreadStateImpl fields not exposed.
typedef struct _PyThreadStateImpl {
    // semi-public fields are in PyThreadState.
    PyThreadState base;

    PyObject *asyncio_running_loop; // Strong reference

    struct _qsbr_thread_state *qsbr;  // only used by free-threaded build
    struct llist_node mem_free_queue; // delayed free queue

#ifdef Py_GIL_DISABLED
    struct _gc_thread_state gc;
    struct _mimalloc_thread_state mimalloc;
    struct _Py_object_freelists freelists;
    struct _brc_thread_state brc;
#endif

#if defined(Py_REF_DEBUG) && defined(Py_GIL_DISABLED)
    Py_ssize_t reftotal;  // this thread's total refcount operations
#endif

} _PyThreadStateImpl;


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TSTATE_H */
