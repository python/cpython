#ifndef Py_INTERNAL_PYMEM_INIT_H
#define Py_INTERNAL_PYMEM_INIT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_allocators.h"


/***************************************/
/* the object allocator implementation */

#ifdef WITH_PYMALLOC
#  ifdef MS_WINDOWS
#    include <windows.h>
#  elif defined(HAVE_MMAP)
#    include <sys/mman.h>
#    ifdef MAP_ANONYMOUS
#      define ARENAS_USE_MMAP
#    endif
#  endif
#endif

#ifdef MS_WINDOWS
static void *
_PyObject_ArenaVirtualAlloc(void *Py_UNUSED(ctx), size_t size)
{
    return VirtualAlloc(NULL, size,
                        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

static void
_PyObject_ArenaVirtualFree(void *Py_UNUSED(ctx), void *ptr,
    size_t Py_UNUSED(size))
{
    VirtualFree(ptr, 0, MEM_RELEASE);
}

#elif defined(ARENAS_USE_MMAP)
static void *
_PyObject_ArenaMmap(void *Py_UNUSED(ctx), size_t size)
{
    void *ptr;
    ptr = mmap(NULL, size, PROT_READ|PROT_WRITE,
               MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED)
        return NULL;
    assert(ptr != NULL);
    return ptr;
}

static void
_PyObject_ArenaMunmap(void *Py_UNUSED(ctx), void *ptr, size_t size)
{
    munmap(ptr, size);
}

#else
static void *
_PyObject_ArenaMalloc(void *Py_UNUSED(ctx), size_t size)
{
    return malloc(size);
}

static void
_PyObject_ArenaFree(void *Py_UNUSED(ctx), void *ptr, size_t Py_UNUSED(size))
{
    free(ptr);
}
#endif


/********************************/
/* the allocators' initializers */

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
