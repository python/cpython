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
    _PyRuntime.static_objects.NAME
#define _Py_SINGLETON(NAME) \
    _Py_GLOBAL_OBJECT(singletons.NAME)


#define _Py_INTERP_CACHED_OBJECT(interp, NAME) \
    (interp)->cached_objects.NAME


#define _Py_INTERP_STATIC_OBJECT(interp, NAME) \
    (interp)->static_objects.NAME
#define _Py_INTERP_SINGLETON(interp, NAME) \
    _Py_INTERP_STATIC_OBJECT(interp, singletons.NAME)


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GLOBAL_OBJECTS_H */
