#ifndef Py_INTERNAL_DYNARRAY_H
#define Py_INTERNAL_DYNARRAY_H
#ifdef __cplusplus
extern "C" {
#endif

#include "Python.h" // Py_ssize_t

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

typedef void (*_Py_dynarray_deallocator)(void *);

typedef struct {
    _Py_dynarray_deallocator deallocator;
    void *contents;
} _Py_dynarray_item;

typedef struct {
    _Py_dynarray_item **items;
    Py_ssize_t capacity;
    Py_ssize_t length;
} _Py_dynarray_t;

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_DYNARRAY_H */
