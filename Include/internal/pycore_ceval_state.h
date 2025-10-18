#ifndef Py_INTERNAL_CEVAL_STATE_H
#define Py_INTERNAL_CEVAL_STATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_gil.h"             // struct _gil_runtime_state


#define MAXPENDINGCALLS PENDINGCALLSARRAYSIZE
/* For interpreter-level pending calls, we want to avoid spending too
   much time on pending calls in any one thread, so we apply a limit. */
#if MAXPENDINGCALLS > 100
#  define MAXPENDINGCALLSLOOP 100
#else
#  define MAXPENDINGCALLSLOOP MAXPENDINGCALLS
#endif

/* We keep the number small to preserve as much compatibility
   as possible with earlier versions. */
#define MAXPENDINGCALLS_MAIN 32
/* For the main thread, we want to make sure all pending calls are
   run at once, for the sake of prompt signal handling.  This is
   unlikely to cause any problems since there should be very few
   pending calls for the main thread. */
#define MAXPENDINGCALLSLOOP_MAIN 0


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


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CEVAL_STATE_H */
