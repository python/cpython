#ifndef Py_INTERNAL_FAULTHANDLER_H
#define Py_INTERNAL_FAULTHANDLER_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#ifdef HAVE_SIGACTION
#  include <signal.h>
#endif


#ifndef MS_WINDOWS
   /* register() is useless on Windows, because only SIGSEGV, SIGABRT and
      SIGILL can be handled by the process, and these signals can only be used
      with enable(), not using register() */
#  define FAULTHANDLER_USER
#endif


#ifdef HAVE_SIGACTION
/* Using an alternative stack requires sigaltstack()
   and sigaction() SA_ONSTACK */
#  ifdef HAVE_SIGALTSTACK
#    define FAULTHANDLER_USE_ALT_STACK
#  endif
typedef struct sigaction _Py_sighandler_t;
#else
typedef PyOS_sighandler_t _Py_sighandler_t;
#endif  // HAVE_SIGACTION


#ifdef FAULTHANDLER_USER
struct faulthandler_user_signal {
    int enabled;
    PyObject *file;
    int fd;
    int all_threads;
    int chain;
    _Py_sighandler_t previous;
    PyInterpreterState *interp;
};
#endif /* FAULTHANDLER_USER */


struct _faulthandler_runtime_state {
    struct {
        int enabled;
        PyObject *file;
        int fd;
        int all_threads;
        PyInterpreterState *interp;
#ifdef MS_WINDOWS
        void *exc_handler;
#endif
    } fatal_error;

    struct {
        PyObject *file;
        int fd;
        PY_TIMEOUT_T timeout_us;   /* timeout in microseconds */
        int repeat;
        PyInterpreterState *interp;
        int exit;
        char *header;
        size_t header_len;
        /* The main thread always holds this lock. It is only released when
           faulthandler_thread() is interrupted before this thread exits, or at
           Python exit. */
        PyThread_type_lock cancel_event;
        /* released by child thread when joined */
        PyThread_type_lock running;
    } thread;

#ifdef FAULTHANDLER_USER
    struct faulthandler_user_signal *user_signals;
#endif

#ifdef FAULTHANDLER_USE_ALT_STACK
    stack_t stack;
    stack_t old_stack;
#endif
};

#define _faulthandler_runtime_state_INIT \
    { \
        .fatal_error = { \
            .fd = -1, \
        }, \
    }


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FAULTHANDLER_H */
