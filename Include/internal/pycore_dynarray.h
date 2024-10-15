#ifndef Py_INTERNAL_DYNARRAY_H
#define Py_INTERNAL_DYNARRAY_H
#ifdef __cplusplus
extern "C" {
#endif

#include "Python.h" // Py_ssize_t

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

typedef void (*_PyDynArray_Deallocator)(void *);

typedef struct {
    void **items;
    Py_ssize_t capacity;
    Py_ssize_t length;
    _PyDynArray_Deallocator deallocator;
} _PyDynArray;

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_DYNARRAY_H */
