#ifndef Py_INTERNAL_CEVAL_STATE_H
#define Py_INTERNAL_CEVAL_STATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_lock.h"            // PyMutex
#include "pycore_gil.h"             // struct _gil_runtime_state


typedef int (*_Py_pending_call_func)(void *);

struct _pending_calls {
    int busy;
    PyMutex mutex;
    /* Request for running pending calls. */
    int32_t calls_to_do;
#define NPENDINGCALLS 32
    struct _pending_call {
        _Py_pending_call_func func;
        void *arg;
        int flags;
    } calls[NPENDINGCALLS];
    int first;
    int last;
};

typedef enum {
    PERF_STATUS_FAILED = -1,  // Perf trampoline is in an invalid state
    PERF_STATUS_NO_INIT = 0,  // Perf trampoline is not initialized
    PERF_STATUS_OK = 1,       // Perf trampoline is ready to be executed
} perf_status_t;


#ifdef PY_HAVE_PERF_TRAMPOLINE
struct code_arena_st;

struct trampoline_api_st {
    void* (*init_state)(void);
    void (*write_state)(void* state, const void *code_addr,
                        unsigned int code_size, PyCodeObject* code);
    int (*free_state)(void* state);
    void *state;
};
#endif

struct _ceval_runtime_state {
    struct {
#ifdef PY_HAVE_PERF_TRAMPOLINE
        perf_status_t status;
        Py_ssize_t extra_code_index;
        struct code_arena_st *code_arena;
        struct trampoline_api_st trampoline_api;
        FILE *map_file;
        Py_ssize_t persist_after_fork;
#else
        int _not_used;
#endif
    } perf;
    /* Pending calls to be made only on the main thread. */
    struct _pending_calls pending_mainthread;
};

#ifdef PY_HAVE_PERF_TRAMPOLINE
# define _PyEval_RUNTIME_PERF_INIT \
    { \
        .status = PERF_STATUS_NO_INIT, \
        .extra_code_index = -1, \
        .persist_after_fork = 0, \
    }
#else
# define _PyEval_RUNTIME_PERF_INIT {0}
#endif


struct _ceval_state {
    /* This single variable consolidates all requests to break out of
     * the fast path in the eval loop.
     * It is by far the hottest field in this struct and
     * should be placed at the beginning. */
    uintptr_t eval_breaker;
    /* Avoid false sharing */
    int64_t padding[7];
    int recursion_limit;
    struct _gil_runtime_state *gil;
    int own_gil;
    struct _pending_calls pending;
};


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CEVAL_STATE_H */
