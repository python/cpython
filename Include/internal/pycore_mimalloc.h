#ifndef Py_INTERNAL_MIMALLOC_H
#define Py_INTERNAL_MIMALLOC_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#if defined(MIMALLOC_H) || defined(MIMALLOC_TYPES_H)
#  error "pycore_mimalloc.h must be included before mimalloc.h"
#endif

typedef enum {
    _Py_MIMALLOC_HEAP_MEM = 0,      // PyMem_Malloc() and friends
    _Py_MIMALLOC_HEAP_OBJECT = 1,   // non-GC objects
    _Py_MIMALLOC_HEAP_GC = 2,       // GC objects without pre-header
    _Py_MIMALLOC_HEAP_GC_PRE = 3,   // GC objects with pre-header
    _Py_MIMALLOC_HEAP_COUNT
} _Py_mimalloc_heap_id;

#include "pycore_pymem.h"

#ifdef WITH_MIMALLOC
#define MI_DEBUG_UNINIT     PYMEM_CLEANBYTE
#define MI_DEBUG_FREED      PYMEM_DEADBYTE
#define MI_DEBUG_PADDING    PYMEM_FORBIDDENBYTE
#ifdef Py_DEBUG
#  define MI_DEBUG 1
#else
#  define MI_DEBUG 0
#endif

#include "mimalloc.h"
#include "mimalloc/types.h"
#include "mimalloc/internal.h"
#endif

#ifdef Py_GIL_DISABLED
struct _mimalloc_interp_state {
    // When exiting, threads place any segments with live blocks in this
    // shared pool for other threads to claim and reuse.
    mi_abandoned_pool_t abandoned_pool;
};

struct _mimalloc_thread_state {
    mi_heap_t *current_object_heap;
    mi_heap_t heaps[_Py_MIMALLOC_HEAP_COUNT];
    mi_tld_t tld;
};
#endif

#endif // Py_INTERNAL_MIMALLOC_H
