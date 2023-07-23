#ifndef Py_INTERNAL_CEVAL_STATE_H
#define Py_INTERNAL_CEVAL_STATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


#include "pycore_atomic.h"          /* _Py_atomic_address */
#include "pycore_gil.h"             // struct _gil_runtime_state


struct _pending_calls {
    int busy;
    PyThread_type_lock lock;
    /* Request for running pending calls. */
    _Py_atomic_int calls_to_do;
    /* Request for looking at the `async_exc` field of the current
       thread state.
       Guarded by the GIL. */
    int async_exc;
#define NPENDINGCALLS 32
    struct _pending_call {
        int (*func)(void *);
        void *arg;
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
#else
        int _not_used;
#endif
    } perf;
    /* Request for checking signals. It is shared by all interpreters (see
       bpo-40513). Any thread of any interpreter can receive a signal, but only
       the main thread of the main interpreter can handle signals: see
       _Py_ThreadCanHandleSignals(). */
    _Py_atomic_int signals_pending;
    /* Pending calls to be made only on the main thread. */
    struct _pending_calls pending_mainthread;
};

#ifdef PY_HAVE_PERF_TRAMPOLINE
# define _PyEval_RUNTIME_PERF_INIT \
    { \
        .status = PERF_STATUS_NO_INIT, \
        .extra_code_index = -1, \
    }
#else
# define _PyEval_RUNTIME_PERF_INIT {0}
#endif


struct _ceval_state {
    /* This single variable consolidates all requests to break out of
       the fast path in the eval loop. */
    _Py_atomic_int eval_breaker;
    /* Request for dropping the GIL */
    _Py_atomic_int gil_drop_request;
    int recursion_limit;
    struct _gil_runtime_state *gil;
    int own_gil;
    /* The GC is ready to be executed */
    _Py_atomic_int gc_scheduled;
    struct _pending_calls pending;
};


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CEVAL_STATE_H */
