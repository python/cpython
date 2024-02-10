#ifndef Py_INTERNAL_PYMEM_INIT_H
#define Py_INTERNAL_PYMEM_INIT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/********************************/
/* the allocators' initializers */

extern void * _PyMem_RawMalloc(void *, size_t);
extern void * _PyMem_RawCalloc(void *, size_t, size_t);
extern void * _PyMem_RawRealloc(void *, void *, size_t);
extern void _PyMem_RawFree(void *, void *);
#define PYRAW_ALLOC {NULL, _PyMem_RawMalloc, _PyMem_RawCalloc, _PyMem_RawRealloc, _PyMem_RawFree}

#ifdef Py_GIL_DISABLED
// Py_GIL_DISABLED requires mimalloc
extern void* _PyObject_MiMalloc(void *, size_t);
extern void* _PyObject_MiCalloc(void *, size_t, size_t);
extern void _PyObject_MiFree(void *, void *);
extern void* _PyObject_MiRealloc(void *, void *, size_t);
#  define PYOBJ_ALLOC {NULL, _PyObject_MiMalloc, _PyObject_MiCalloc, _PyObject_MiRealloc, _PyObject_MiFree}
extern void* _PyMem_MiMalloc(void *, size_t);
extern void* _PyMem_MiCalloc(void *, size_t, size_t);
extern void _PyMem_MiFree(void *, void *);
extern void* _PyMem_MiRealloc(void *, void *, size_t);
#  define PYMEM_ALLOC {NULL, _PyMem_MiMalloc, _PyMem_MiCalloc, _PyMem_MiRealloc, _PyMem_MiFree}
#elif defined(WITH_PYMALLOC)
extern void* _PyObject_Malloc(void *, size_t);
extern void* _PyObject_Calloc(void *, size_t, size_t);
extern void _PyObject_Free(void *, void *);
extern void* _PyObject_Realloc(void *, void *, size_t);
#  define PYOBJ_ALLOC {NULL, _PyObject_Malloc, _PyObject_Calloc, _PyObject_Realloc, _PyObject_Free}
#  define PYMEM_ALLOC PYOBJ_ALLOC
#else
# define PYOBJ_ALLOC PYRAW_ALLOC
# define PYMEM_ALLOC PYOBJ_ALLOC
#endif  // WITH_PYMALLOC


extern void* _PyMem_DebugRawMalloc(void *, size_t);
extern void* _PyMem_DebugRawCalloc(void *, size_t, size_t);
extern void* _PyMem_DebugRawRealloc(void *, void *, size_t);
extern void _PyMem_DebugRawFree(void *, void *);

extern void* _PyMem_DebugMalloc(void *, size_t);
extern void* _PyMem_DebugCalloc(void *, size_t, size_t);
extern void* _PyMem_DebugRealloc(void *, void *, size_t);
extern void _PyMem_DebugFree(void *, void *);

#define PYDBGRAW_ALLOC(runtime) \
    {&(runtime).allocators.debug.raw, _PyMem_DebugRawMalloc, _PyMem_DebugRawCalloc, _PyMem_DebugRawRealloc, _PyMem_DebugRawFree}
#define PYDBGMEM_ALLOC(runtime) \
    {&(runtime).allocators.debug.mem, _PyMem_DebugMalloc, _PyMem_DebugCalloc, _PyMem_DebugRealloc, _PyMem_DebugFree}
#define PYDBGOBJ_ALLOC(runtime) \
    {&(runtime).allocators.debug.obj, _PyMem_DebugMalloc, _PyMem_DebugCalloc, _PyMem_DebugRealloc, _PyMem_DebugFree}

extern void * _PyMem_ArenaAlloc(void *, size_t);
extern void _PyMem_ArenaFree(void *, void *, size_t);

#ifdef Py_DEBUG
# define _pymem_allocators_standard_INIT(runtime) \
    { \
        PYDBGRAW_ALLOC(runtime), \
        PYDBGMEM_ALLOC(runtime), \
        PYDBGOBJ_ALLOC(runtime), \
    }
#else
# define _pymem_allocators_standard_INIT(runtime) \
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

#  define _pymem_allocators_obj_arena_INIT \
    { NULL, _PyMem_ArenaAlloc, _PyMem_ArenaFree }


#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_PYMEM_INIT_H
