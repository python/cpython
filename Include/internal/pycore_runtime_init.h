#ifndef Py_INTERNAL_RUNTIME_INIT_H
#define Py_INTERNAL_RUNTIME_INIT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


#define _PyThreadState_INIT \
    { \
        ._static = 1, \
    }
#define _PyInterpreterState_INIT \
    { \
        ._static = 1, \
        ._initial_thread = _PyThreadState_INIT, \
    }
#define _PyRuntimeState_INIT \
    { \
        .global_objects = _Py_global_objects_INIT, \
        ._main_interpreter = _PyInterpreterState_INIT, \
    }


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_RUNTIME_INIT_H */
