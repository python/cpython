#ifndef Py_INTERNAL_ATEXIT_H
#define Py_INTERNAL_ATEXIT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


//###############
// runtime atexit

typedef void (*atexit_callbackfunc)(void);

struct _atexit_runtime_state {
    PyThread_type_lock mutex;
#define NEXITFUNCS 32
    atexit_callbackfunc callbacks[NEXITFUNCS];
    int ncallbacks;
};


//###################
// interpreter atexit

struct atexit_callback;
typedef struct atexit_callback {
    atexit_datacallbackfunc func;
    void *data;
    struct atexit_callback *next;
} atexit_callback;

typedef struct {
    PyObject *func;
    PyObject *args;
    PyObject *kwargs;
} atexit_py_callback;

struct atexit_state {
    atexit_callback *ll_callbacks;
    atexit_callback *last_ll_callback;

    // XXX The rest of the state could be moved to the atexit module state
    // and a low-level callback added for it during module exec.
    // For the moment we leave it here.
    atexit_py_callback **callbacks;
    int ncallbacks;
    int callback_len;
};


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_ATEXIT_H */
