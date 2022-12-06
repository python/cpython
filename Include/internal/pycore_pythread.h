#ifndef Py_INTERNAL_PYTHREAD_H
#define Py_INTERNAL_PYTHREAD_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


struct _pythread_runtime_state {
    int _not_used;
};


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYTHREAD_H */
