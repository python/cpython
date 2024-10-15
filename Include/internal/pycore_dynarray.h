#ifndef Py_INTERNAL_DYNARRAY_H
#define Py_INTERNAL_DYNARRAY_H
#ifdef __cplusplus
extern "C" {
#endif

#include "Python.h" // Py_ssize_t

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#define _PyDynArray_DEFAULT_SIZE 16

typedef void (*_PyDynArray_Deallocator)(void *);

typedef struct {
    void **items;
    Py_ssize_t capacity;
    Py_ssize_t length;
    _PyDynArray_Deallocator deallocator;
} _PyDynArray;

PyAPI_FUNC(int)
_PyDynArray_InitWithSize(_PyDynArray *array,
                         _PyDynArray_Deallocator deallocator,
                         Py_ssize_t initial);

PyAPI_FUNC(int) _PyDynArray_Append(_PyDynArray *array, void *item);

PyAPI_FUNC(void) _PyDynArray_Clear(_PyDynArray *array);

static inline void
_PyDynArray_Free(_PyDynArray *array)
{
    _PyDynArray_Clear(array);
    PyMem_RawFree(array);
}

static inline int
_PyDynArray_Init(_PyDynArray *array, _PyDynArray_Deallocator deallocator)
{
    return _PyDynArray_InitWithSize(array, deallocator, _PyDynArray_DEFAULT_SIZE);
}

static inline _PyDynArray *
_PyDynArray_NewWithSize(_PyDynArray_Deallocator deallocator, Py_ssize_t initial)
{
    _PyDynArray *array = PyMem_RawMalloc(sizeof(_PyDynArray));
    if (array == NULL)
    {
        return NULL;
    }

    if (_PyDynArray_InitWithSize(array, deallocator, initial) < 0)
    {
        PyMem_RawFree(array);
        return NULL;
    }

    return array;
}

static inline _PyDynArray *
_PyDynArray_New(_PyDynArray_Deallocator deallocator)
{
    return _PyDynArray_NewWithSize(deallocator, _PyDynArray_DEFAULT_SIZE);
}

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_DYNARRAY_H */
