#ifndef Py_INTERNAL_ALLOCATORS_H
#define Py_INTERNAL_ALLOCATORS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stdlib.h>               // malloc()


/*********************************************/
/* the (raw) malloc allocator implementation */

static void *
_PyMem_RawMalloc(void *Py_UNUSED(ctx), size_t size)
{
    /* PyMem_RawMalloc(0) means malloc(1). Some systems would return NULL
       for malloc(0), which would be treated as an error. Some platforms would
       return a pointer with no memory behind it, which would break pymalloc.
       To solve these problems, allocate an extra byte. */
    if (size == 0)
        size = 1;
    return malloc(size);
}

static void *
_PyMem_RawCalloc(void *Py_UNUSED(ctx), size_t nelem, size_t elsize)
{
    /* PyMem_RawCalloc(0, 0) means calloc(1, 1). Some systems would return NULL
       for calloc(0, 0), which would be treated as an error. Some platforms
       would return a pointer with no memory behind it, which would break
       pymalloc.  To solve these problems, allocate an extra byte. */
    if (nelem == 0 || elsize == 0) {
        nelem = 1;
        elsize = 1;
    }
    return calloc(nelem, elsize);
}

static void *
_PyMem_RawRealloc(void *Py_UNUSED(ctx), void *ptr, size_t size)
{
    if (size == 0)
        size = 1;
    return realloc(ptr, size);
}

static void
_PyMem_RawFree(void *Py_UNUSED(ctx), void *ptr)
{
    free(ptr);
}


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_ALLOCATORS_H */
