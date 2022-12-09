#ifndef Py_INTERNAL_FAULTHANDLER_H
#define Py_INTERNAL_FAULTHANDLER_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


struct _faulthandler_runtime_state {
    int _not_used;
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
