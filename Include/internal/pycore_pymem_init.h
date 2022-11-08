#ifndef Py_INTERNAL_PYMEM_INIT_H
#define Py_INTERNAL_PYMEM_INIT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_allocators.h"


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
