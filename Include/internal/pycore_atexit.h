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
#define NEXITFUNCS 32
    atexit_callbackfunc callbacks[NEXITFUNCS];
    int ncallbacks;
};


//###################
// interpreter atexit

typedef void (*atexit_datacallbackfunc)(void *);

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
    atexit_py_callback **callbacks;
    int ncallbacks;
    int callback_len;

    atexit_callback *ll_callbacks;
    atexit_callback *last_ll_callback;
};

PyAPI_FUNC(int) _Py_AtExit(
        PyInterpreterState *, atexit_datacallbackfunc, void *);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_ATEXIT_H */
