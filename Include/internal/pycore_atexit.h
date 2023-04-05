#ifndef Py_INTERNAL_ATEXIT_H
#define Py_INTERNAL_ATEXIT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


typedef void (*atexit_callbackfunc)(void);


typedef struct {
    PyObject *func;
    PyObject *args;
    PyObject *kwargs;
} atexit_py_callback;

struct atexit_state {
    atexit_py_callback **callbacks;
    int ncallbacks;
    int callback_len;
};



struct _atexit_runtime_state {
#define NEXITFUNCS 32
    atexit_callbackfunc callbacks[NEXITFUNCS];
    int ncallbacks;
};


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_ATEXIT_H */
