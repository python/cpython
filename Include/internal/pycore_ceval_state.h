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

typedef struct _pending_call {
    _Py_pending_call_func func;
    void *arg;
    int flags;
    struct _pending_call *next;
} _pending_call;

/* For interpreter-level pending calls, we want to avoid spending too
   much time on pending calls in any one thread, so we apply a limit. */
#define MAXPENDINGCALLSLOOP 100

/* For the main thread, we want to make sure all pending calls are
   run at once, for the sake of prompt signal handling.  This is
   unlikely to cause any problems since there should be very few
   pending calls for the main thread. */
#define MAXPENDINGCALLSLOOP_MAIN INT32_MAX

struct _pending_calls {
    PyThreadState *handling_thread;
    PyMutex mutex;
    /* The number of pending calls. */
    int32_t npending;
    /* We don't want a flood of pending calls to interrupt any one thread
       for too long, so we keep a limit on the number handled per pass.
       A value of 0 means there is no limit (other than the maximum
       size of the list of pending calls). */
    int32_t maxloop;
    _pending_call *head;

#define _Py_PENDING_CALLS_FREELIST_SIZE 100
    _pending_call *freelist;
    size_t freelist_num;
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
    Py_ssize_t code_padding;
};
#endif


struct _ceval_runtime_state {
    struct {
#ifdef PY_HAVE_PERF_TRAMPOLINE
        perf_status_t status;
        int perf_trampoline_type;
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
    // The signal machinery falls back on this
    // so it must be especially stable and efficient.
    struct _pending_calls pending_mainthread;
    PyMutex sys_trace_profile_mutex;
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
    /* This variable holds the global instrumentation version. When a thread is
       running, this value is overlaid onto PyThreadState.eval_breaker so that
       changes in the instrumentation version will trigger the eval breaker. */
    uintptr_t instrumentation_version;
    int recursion_limit;
    struct _gil_runtime_state *gil;
    int own_gil;
    struct _pending_calls pending;
};


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CEVAL_STATE_H */
