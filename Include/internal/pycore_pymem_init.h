#ifndef Py_INTERNAL_PYMEM_INIT_H
#define Py_INTERNAL_PYMEM_INIT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_allocators.h"

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

#ifdef Py_DEBUG
#define _pymem_allocators_standard_INIT \
    { \
        PYDBGRAW_ALLOC, \
        PYDBGMEM_ALLOC, \
        PYDBGOBJ_ALLOC, \
    }
#else
#define _pymem_allocators_standard_INIT \
    { \
        PYRAW_ALLOC, \
        PYMEM_ALLOC, \
        PYOBJ_ALLOC, \
    }
#endif

#define _pymem_allocators_debug_INIT \
   { \
       {'r', PYRAW_ALLOC}, \
       {'m', PYMEM_ALLOC}, \
       {'o', PYOBJ_ALLOC}, \
   }

#ifdef MS_WINDOWS
#  define _pymem_allocators_obj_arena_INIT \
    { NULL, _PyObject_ArenaVirtualAlloc, _PyObject_ArenaVirtualFree }
#elif defined(ARENAS_USE_MMAP)
#  define _pymem_allocators_obj_arena_INIT \
    { NULL, _PyObject_ArenaMmap, _PyObject_ArenaMunmap }
#else
#  define _pymem_allocators_obj_arena_INIT \
    { NULL, _PyObject_ArenaMalloc, _PyObject_ArenaFree }
#endif


#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_PYMEM_INIT_H
