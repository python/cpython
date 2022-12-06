#ifndef Py_INTERNAL_OS_H
#define Py_INTERNAL_OS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


struct _os_runtime_state {
    int _not_used;
};
# define _OS_RUNTIME_INIT \
    { \
        ._not_used = 0, \
    }


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_OS_H */
