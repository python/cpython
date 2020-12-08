#ifndef Py_INTERNAL_ATEXIT_H
#define Py_INTERNAL_ATEXIT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

typedef struct {
    PyObject *func;
    PyObject *args;
    PyObject *kwargs;
} _Py_atexit_callback;

struct _atexit_runtime_state {
    _Py_atexit_callback **atexit_callbacks;
    int ncallbacks;
    int callback_len;
};

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_ATEXIT_H */