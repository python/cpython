#ifndef Py_INTERNAL_MIMALLOC_STRUCTS_H
#define Py_INTERNAL_MIMALLOC_STRUCTS_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// mimalloc state embedded in the interpreter and thread states. This header
// does not include mimalloc itself; use pycore_mimalloc.h to call into it.

#include "pycore_pymem.h"         // previously exposed via pycore_tstate.h

typedef enum {
    _Py_MIMALLOC_HEAP_MEM = 0,      // PyMem_Malloc() and friends
    _Py_MIMALLOC_HEAP_OBJECT = 1,   // non-GC objects
    _Py_MIMALLOC_HEAP_GC = 2,       // GC objects without pre-header
    _Py_MIMALLOC_HEAP_GC_PRE = 3,   // GC objects with pre-header
    _Py_MIMALLOC_HEAP_COUNT
} _Py_mimalloc_heap_id;

#ifdef WITH_MIMALLOC
// Defined by mimalloc as mi_heap_t and mi_theap_t
struct mi_heap_s;
struct mi_theap_s;

extern void _mi_fork_child(void);
#endif

#ifdef Py_GIL_DISABLED
struct _mimalloc_interp_state {
    struct mi_heap_s *heaps[_Py_MIMALLOC_HEAP_COUNT];
};

struct _mimalloc_thread_state {
    struct mi_theap_s *current_object_heap;
    struct mi_theap_s *heaps[_Py_MIMALLOC_HEAP_COUNT];
    int initialized;
};

extern int _PyMem_InitMimalloc(PyInterpreterState *interp);
extern void _PyMem_FiniMimalloc(PyInterpreterState *interp);
#endif

#endif // Py_INTERNAL_MIMALLOC_STRUCTS_H
