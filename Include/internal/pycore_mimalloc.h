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
#  ifdef Py_GIL_DISABLED
#    define MI_PRIM_THREAD_ID   _Py_ThreadId
#  endif
#  define MI_DEBUG_UNINIT     PYMEM_CLEANBYTE
#  define MI_DEBUG_FREED      PYMEM_DEADBYTE
#  define MI_DEBUG_PADDING    PYMEM_FORBIDDENBYTE
#ifdef Py_DEBUG
#  define MI_DEBUG 2
#else
#  define MI_DEBUG 0
#endif

#ifdef _Py_THREAD_SANITIZER
#  define MI_TSAN 1
#endif

#ifdef __cplusplus
extern "C++" {
#endif

#include "mimalloc/mimalloc.h"
#include "mimalloc/mimalloc/types.h"
#include "mimalloc/mimalloc/internal.h"

#ifdef __cplusplus
}
#endif

#endif

#ifdef Py_GIL_DISABLED
struct _mimalloc_interp_state {
    // When exiting, threads abandon any segments with live blocks. The
    // abandoned segments are tracked per mimalloc sub-process so that other
    // threads of the same interpreter can claim and reuse them, and so that
    // the GC can visit them.
    mi_subproc_t *subproc;
};

struct _mimalloc_thread_state {
    mi_heap_t *current_object_heap;
    mi_heap_t heaps[_Py_MIMALLOC_HEAP_COUNT];
    mi_tld_t tld;
    int initialized;
    struct llist_node page_list;
};

// Visit the blocks in the abandoned segments (from exited threads) of a
// mimalloc sub-process. Same as mi_abandoned_visit_blocks() but without
// requiring mi_option_visit_abandoned: the caller must guarantee that no
// other thread abandons or reclaims segments concurrently (STW).
static inline bool
_PyMem_mi_visit_abandoned_blocks(mi_subproc_t *subproc, int heap_tag,
                                 bool visit_blocks,
                                 mi_block_visit_fun *visitor, void *arg)
{
    mi_arena_field_cursor_t current;
    _mi_arena_field_cursor_init(NULL, subproc, true /* visit all */, &current);
    mi_segment_t *segment;
    bool ok = true;
    while (ok && (segment = _mi_arena_segment_clear_abandoned_next(&current)) != NULL) {
        ok = _mi_segment_visit_blocks(segment, heap_tag, visit_blocks, visitor, arg);
        _mi_arena_segment_mark_abandoned(segment);
    }
    _mi_arena_field_cursor_done(&current);
    return ok;
}
#endif

#endif // Py_INTERNAL_MIMALLOC_H
