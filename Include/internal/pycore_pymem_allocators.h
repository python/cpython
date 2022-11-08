#ifndef Py_INTERNAL_PYMEM_ALLOCATORS_H
#define Py_INTERNAL_PYMEM_ALLOCATORS_H
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

#ifdef WITH_PYMALLOC
void* _PyObject_Malloc(void *ctx, size_t size);
void* _PyObject_Calloc(void *ctx, size_t nelem, size_t elsize);
void _PyObject_Free(void *ctx, void *p);
void* _PyObject_Realloc(void *ctx, void *ptr, size_t size);
#endif

#define MALLOC_ALLOC {NULL, _PyMem_RawMalloc, _PyMem_RawCalloc, _PyMem_RawRealloc, _PyMem_RawFree}
#ifdef WITH_PYMALLOC
#  define PYMALLOC_ALLOC {NULL, _PyObject_Malloc, _PyObject_Calloc, _PyObject_Realloc, _PyObject_Free}
#endif

#define PYRAW_ALLOC MALLOC_ALLOC
#ifdef WITH_PYMALLOC
#  define PYOBJ_ALLOC PYMALLOC_ALLOC
#else
#  define PYOBJ_ALLOC MALLOC_ALLOC
#endif
#define PYMEM_ALLOC PYOBJ_ALLOC

void* _PyMem_DebugRawMalloc(void *ctx, size_t size);
void* _PyMem_DebugRawCalloc(void *ctx, size_t nelem, size_t elsize);
void* _PyMem_DebugRawRealloc(void *ctx, void *ptr, size_t size);
void _PyMem_DebugRawFree(void *ctx, void *ptr);

void* _PyMem_DebugMalloc(void *ctx, size_t size);
void* _PyMem_DebugCalloc(void *ctx, size_t nelem, size_t elsize);
void* _PyMem_DebugRealloc(void *ctx, void *ptr, size_t size);
void _PyMem_DebugFree(void *ctx, void *p);

#define PYDBGRAW_ALLOC \
    {&_PyRuntime.allocators.debug.raw, _PyMem_DebugRawMalloc, _PyMem_DebugRawCalloc, _PyMem_DebugRawRealloc, _PyMem_DebugRawFree}
#define PYDBGMEM_ALLOC \
    {&_PyRuntime.allocators.debug.mem, _PyMem_DebugMalloc, _PyMem_DebugCalloc, _PyMem_DebugRealloc, _PyMem_DebugFree}
#define PYDBGOBJ_ALLOC \
    {&_PyRuntime.allocators.debug.obj, _PyMem_DebugMalloc, _PyMem_DebugCalloc, _PyMem_DebugRealloc, _PyMem_DebugFree}


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYMEM_ALLOCATORS_H */
