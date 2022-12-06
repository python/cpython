#ifndef Py_INTERNAL_OS_H
#define Py_INTERNAL_OS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


#ifndef MS_WINDOWS
#define _OS_NEED_TICKS_PER_SECOND
# define need_ticks_per_second_STATE \
    long ticks_per_second;
# define need_ticks_per_second_INIT \
    .ticks_per_second = -1,
#else
# define need_ticks_per_second_STATE
# define need_ticks_per_second_INIT
#endif /* MS_WINDOWS */


struct _os_runtime_state {
    int _not_used;
    need_ticks_per_second_STATE
};
# define _OS_RUNTIME_INIT \
    { \
        ._not_used = 0, \
        need_ticks_per_second_INIT \
    }


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_OS_H */
