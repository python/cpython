#ifndef Py_INTERNAL_FAULTHANDLER_H
#define Py_INTERNAL_FAULTHANDLER_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


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
