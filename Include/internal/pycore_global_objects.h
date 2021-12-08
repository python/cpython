#ifndef Py_INTERNAL_GLOBAL_OBJECTS_H
#define Py_INTERNAL_GLOBAL_OBJECTS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


// Only immutable objects should be considered runtime-global.
// All others must be per-interpreter.

#define _Py_GLOBAL_OBJECT(NAME) \
    _PyRuntime.global_objects.NAME

struct _Py_global_objects {
};


#define _Py_global_objects_INIT { \
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GLOBAL_OBJECTS_H */
