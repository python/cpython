#ifndef Py_INTERNAL_GLOBAL_OBJECTS_H
#define Py_INTERNAL_GLOBAL_OBJECTS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


#define _PY_NSMALLPOSINTS           257
#define _PY_NSMALLNEGINTS           5

// _PyLong_GetZero() and _PyLong_GetOne() must always be available
#if _PY_NSMALLPOSINTS < 2
#  error "_PY_NSMALLPOSINTS must be greater than 1"
#endif


// Only immutable objects should be considered runtime-global.
// All others must be per-interpreter.

#define _Py_GLOBAL_OBJECT(NAME) \
    _PyRuntime.global_objects.NAME
#define _Py_SINGLETON(NAME) \
    _Py_GLOBAL_OBJECT(singletons.NAME)

struct _Py_global_objects {
    struct {
        /* Small integers are preallocated in this array so that they
         * can be shared.
         * The integers that are preallocated are those in the range
         *-_PY_NSMALLNEGINTS (inclusive) to _PY_NSMALLPOSINTS (not inclusive).
         */
        PyLongObject small_ints[_PY_NSMALLNEGINTS + _PY_NSMALLPOSINTS];
    } singletons;
};

#define _Py_global_objects_INIT { \
    .singletons = { \
    }, \
}

static inline void
_Py_global_objects_reset(struct _Py_global_objects *objects)
{
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GLOBAL_OBJECTS_H */
