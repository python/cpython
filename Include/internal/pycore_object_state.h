#ifndef Py_INTERNAL_OBJECT_STATE_H
#define Py_INTERNAL_OBJECT_STATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

struct _py_object_runtime_state {
#ifdef Py_REF_DEBUG
    Py_ssize_t interpreter_leaks;
#else
    int _not_used;
#endif
};

struct _py_object_state {
#ifdef Py_REF_DEBUG
    Py_ssize_t reftotal;
#else
    int _not_used;
#endif
};


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_OBJECT_STATE_H */
