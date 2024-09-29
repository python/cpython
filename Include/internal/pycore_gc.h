#ifndef Py_INTERNAL_GC_H
#define Py_INTERNAL_GC_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_freelist.h"   // _PyFreeListState

/* GC information is stored BEFORE the object structure. */
typedef struct {
    // Pointer to next object in the list.
    // 0 means the object is not tracked
    uintptr_t _gc_next;

    // Pointer to previous object in the list.
    // Lowest two bits are used for flags documented later.
    uintptr_t _gc_prev;
} PyGC_Head;

#define _PyGC_Head_UNUSED PyGC_Head


/* Get an object's GC head */
static inline PyGC_Head* _Py_AS_GC(PyObject *op) {
    char *gc = ((char*)op) - sizeof(PyGC_Head);
    return (PyGC_Head*)gc;
}

/* Get the object given the GC head */
static inline PyObject* _Py_FROM_GC(PyGC_Head *gc) {
    char *op = ((char *)gc) + sizeof(PyGC_Head);
    return (PyObject *)op;
}


/* Bit flags for ob_gc_bits (in Py_GIL_DISABLED builds)
 *
 * Setting the bits requires a relaxed store. The per-object lock must also be
 * held, except when the object is only visible to a single thread (e.g. during
 * object initialization or destruction).
 *
 * Reading the bits requires using a relaxed load, but does not require holding
 * the per-object lock.
 */
#ifdef Py_GIL_DISABLED
#  define _PyGC_BITS_TRACKED        (1)     // Tracked by the GC
#  define _PyGC_BITS_FINALIZED      (2)     // tp_finalize was called
#  define _PyGC_BITS_UNREACHABLE    (4)
#  define _PyGC_BITS_FROZEN         (8)
#  define _PyGC_BITS_SHARED         (16)
#  define _PyGC_BITS_SHARED_INLINE  (32)
#  define _PyGC_BITS_DEFERRED       (64)    // Use deferred reference counting
#endif

#ifdef Py_GIL_DISABLED

static inline void
_PyObject_SET_GC_BITS(PyObject *op, uint8_t new_bits)
{
    uint8_t bits = _Py_atomic_load_uint8_relaxed(&op->ob_gc_bits);
    _Py_atomic_store_uint8_relaxed(&op->ob_gc_bits, bits | new_bits);
}

static inline int
_PyObject_HAS_GC_BITS(PyObject *op, uint8_t bits)
{
    return (_Py_atomic_load_uint8_relaxed(&op->ob_gc_bits) & bits) != 0;
}

static inline void
_PyObject_CLEAR_GC_BITS(PyObject *op, uint8_t bits_to_clear)
{
    uint8_t bits = _Py_atomic_load_uint8_relaxed(&op->ob_gc_bits);
    _Py_atomic_store_uint8_relaxed(&op->ob_gc_bits, bits & ~bits_to_clear);
}

#endif

/* True if the object is currently tracked by the GC. */
static inline int _PyObject_GC_IS_TRACKED(PyObject *op) {
#ifdef Py_GIL_DISABLED
    return _PyObject_HAS_GC_BITS(op, _PyGC_BITS_TRACKED);
#else
    PyGC_Head *gc = _Py_AS_GC(op);
    return (gc->_gc_next != 0);
#endif
}
#define _PyObject_GC_IS_TRACKED(op) _PyObject_GC_IS_TRACKED(_Py_CAST(PyObject*, op))

/* True if the object may be tracked by the GC in the future, or already is.
   This can be useful to implement some optimizations. */
static inline int _PyObject_GC_MAY_BE_TRACKED(PyObject *obj) {
    if (!PyObject_IS_GC(obj)) {
        return 0;
    }
    if (PyTuple_CheckExact(obj)) {
        return _PyObject_GC_IS_TRACKED(obj);
    }
    return 1;
}

#ifdef Py_GIL_DISABLED

/* True if memory the object references is shared between
 * multiple threads and needs special purpose when freeing
 * those references due to the possibility of in-flight
 * lock-free reads occurring.  The object is responsible
 * for calling _PyMem_FreeDelayed on the referenced
 * memory. */
static inline int _PyObject_GC_IS_SHARED(PyObject *op) {
    return _PyObject_HAS_GC_BITS(op, _PyGC_BITS_SHARED);
}
#define _PyObject_GC_IS_SHARED(op) _PyObject_GC_IS_SHARED(_Py_CAST(PyObject*, op))

static inline void _PyObject_GC_SET_SHARED(PyObject *op) {
    _PyObject_SET_GC_BITS(op, _PyGC_BITS_SHARED);
}
#define _PyObject_GC_SET_SHARED(op) _PyObject_GC_SET_SHARED(_Py_CAST(PyObject*, op))

/* True if the memory of the object is shared between multiple
 * threads and needs special purpose when freeing due to
 * the possibility of in-flight lock-free reads occurring.
 * Objects with this bit that are GC objects will automatically
 * delay-freed by PyObject_GC_Del. */
static inline int _PyObject_GC_IS_SHARED_INLINE(PyObject *op) {
    return _PyObject_HAS_GC_BITS(op, _PyGC_BITS_SHARED_INLINE);
}
#define _PyObject_GC_IS_SHARED_INLINE(op) \
    _PyObject_GC_IS_SHARED_INLINE(_Py_CAST(PyObject*, op))

static inline void _PyObject_GC_SET_SHARED_INLINE(PyObject *op) {
    _PyObject_SET_GC_BITS(op, _PyGC_BITS_SHARED_INLINE);
}
#define _PyObject_GC_SET_SHARED_INLINE(op) \
    _PyObject_GC_SET_SHARED_INLINE(_Py_CAST(PyObject*, op))

#endif

/* Bit flags for _gc_prev */
/* Bit 0 is set when tp_finalize is called */
#define _PyGC_PREV_MASK_FINALIZED  (1)
/* Bit 1 is set when the object is in generation which is GCed currently. */
#define _PyGC_PREV_MASK_COLLECTING (2)
/* The (N-2) most significant bits contain the real address. */
#define _PyGC_PREV_SHIFT           (2)
#define _PyGC_PREV_MASK            (((uintptr_t) -1) << _PyGC_PREV_SHIFT)

/* set for debugging information */
#define _PyGC_DEBUG_STATS             (1<<0) /* print collection statistics */
#define _PyGC_DEBUG_COLLECTABLE       (1<<1) /* print collectable objects */
#define _PyGC_DEBUG_UNCOLLECTABLE     (1<<2) /* print uncollectable objects */
#define _PyGC_DEBUG_SAVEALL           (1<<5) /* save all garbage in gc.garbage */
#define _PyGC_DEBUG_LEAK              _PyGC_DEBUG_COLLECTABLE | \
                                      _PyGC_DEBUG_UNCOLLECTABLE | \
                                      _PyGC_DEBUG_SAVEALL

typedef enum {
    // GC was triggered by heap allocation
    _Py_GC_REASON_HEAP,

    // GC was called during shutdown
    _Py_GC_REASON_SHUTDOWN,

    // GC was called by gc.collect() or PyGC_Collect()
    _Py_GC_REASON_MANUAL
} _PyGC_Reason;

// Lowest bit of _gc_next is used for flags only in GC.
// But it is always 0 for normal code.
static inline PyGC_Head* _PyGCHead_NEXT(PyGC_Head *gc) {
    uintptr_t next = gc->_gc_next;
    return (PyGC_Head*)next;
}
static inline void _PyGCHead_SET_NEXT(PyGC_Head *gc, PyGC_Head *next) {
    gc->_gc_next = (uintptr_t)next;
}

// Lowest two bits of _gc_prev is used for _PyGC_PREV_MASK_* flags.
static inline PyGC_Head* _PyGCHead_PREV(PyGC_Head *gc) {
    uintptr_t prev = (gc->_gc_prev & _PyGC_PREV_MASK);
    return (PyGC_Head*)prev;
}
static inline void _PyGCHead_SET_PREV(PyGC_Head *gc, PyGC_Head *prev) {
    uintptr_t uprev = (uintptr_t)prev;
    assert((uprev & ~_PyGC_PREV_MASK) == 0);
    gc->_gc_prev = ((gc->_gc_prev & ~_PyGC_PREV_MASK) | uprev);
}

static inline int _PyGC_FINALIZED(PyObject *op) {
#ifdef Py_GIL_DISABLED
    return _PyObject_HAS_GC_BITS(op, _PyGC_BITS_FINALIZED);
#else
    PyGC_Head *gc = _Py_AS_GC(op);
    return ((gc->_gc_prev & _PyGC_PREV_MASK_FINALIZED) != 0);
#endif
}
static inline void _PyGC_SET_FINALIZED(PyObject *op) {
#ifdef Py_GIL_DISABLED
    _PyObject_SET_GC_BITS(op, _PyGC_BITS_FINALIZED);
#else
    PyGC_Head *gc = _Py_AS_GC(op);
    gc->_gc_prev |= _PyGC_PREV_MASK_FINALIZED;
#endif
}
static inline void _PyGC_CLEAR_FINALIZED(PyObject *op) {
#ifdef Py_GIL_DISABLED
    _PyObject_CLEAR_GC_BITS(op, _PyGC_BITS_FINALIZED);
#else
    PyGC_Head *gc = _Py_AS_GC(op);
    gc->_gc_prev &= ~_PyGC_PREV_MASK_FINALIZED;
#endif
}


/* GC runtime state */

/* If we change this, we need to change the default value in the
   signature of gc.collect. */
#define NUM_GENERATIONS 3
/*
   NOTE: about untracking of mutable objects.

   Certain types of container cannot participate in a reference cycle, and
   so do not need to be tracked by the garbage collector. Untracking these
   objects reduces the cost of garbage collections. However, determining
   which objects may be untracked is not free, and the costs must be
   weighed against the benefits for garbage collection.

   There are two possible strategies for when to untrack a container:

   i) When the container is created.
   ii) When the container is examined by the garbage collector.

   Tuples containing only immutable objects (integers, strings etc, and
   recursively, tuples of immutable objects) do not need to be tracked.
   The interpreter creates a large number of tuples, many of which will
   not survive until garbage collection. It is therefore not worthwhile
   to untrack eligible tuples at creation time.

   Instead, all tuples except the empty tuple are tracked when created.
   During garbage collection it is determined whether any surviving tuples
   can be untracked. A tuple can be untracked if all of its contents are
   already not tracked. Tuples are examined for untracking in all garbage
   collection cycles. It may take more than one cycle to untrack a tuple.

   Dictionaries containing only immutable objects also do not need to be
   tracked. Dictionaries are untracked when created. If a tracked item is
   inserted into a dictionary (either as a key or value), the dictionary
   becomes tracked. During a full garbage collection (all generations),
   the collector will untrack any dictionaries whose contents are not
   tracked.

   The module provides the python function is_tracked(obj), which returns
   the CURRENT tracking status of the object. Subsequent garbage
   collections may change the tracking status of the object.

   Untracking of certain containers was introduced in issue #4688, and
   the algorithm was refined in response to issue #14775.
*/

struct gc_generation {
    PyGC_Head head;
    int threshold; /* collection threshold */
    int count; /* count of allocations or collections of younger
                  generations */
};

/* Running stats per generation */
struct gc_generation_stats {
    /* total number of collections */
    Py_ssize_t collections;
    /* total number of collected objects */
    Py_ssize_t collected;
    /* total number of uncollectable objects (put into gc.garbage) */
    Py_ssize_t uncollectable;
};

struct _gc_runtime_state {
    /* List of objects that still need to be cleaned up, singly linked
     * via their gc headers' gc_prev pointers.  */
    PyObject *trash_delete_later;
    /* Current call-stack depth of tp_dealloc calls. */
    int trash_delete_nesting;

    /* Is automatic collection enabled? */
    int enabled;
    int debug;
    /* linked lists of container objects */
    struct gc_generation generations[NUM_GENERATIONS];
    PyGC_Head *generation0;
    /* a permanent generation which won't be collected */
    struct gc_generation permanent_generation;
    struct gc_generation_stats generation_stats[NUM_GENERATIONS];
    /* true if we are currently running the collector */
    int collecting;
    /* list of uncollectable objects */
    PyObject *garbage;
    /* a list of callbacks to be invoked when collection is performed */
    PyObject *callbacks;

    /* This is the number of objects that survived the last full
       collection. It approximates the number of long lived objects
       tracked by the GC.

       (by "full collection", we mean a collection of the oldest
       generation). */
    Py_ssize_t long_lived_total;
    /* This is the number of objects that survived all "non-full"
       collections, and are awaiting to undergo a full collection for
       the first time. */
    Py_ssize_t long_lived_pending;

#ifdef Py_GIL_DISABLED
    /* gh-117783: Deferred reference counting is not fully implemented yet, so
       as a temporary measure we treat objects using deferred reference
       counting as immortal. The value may be zero, one, or a negative number:
        0: immortalize deferred RC objects once the first thread is created
        1: immortalize all deferred RC objects immediately
        <0: suppressed; don't immortalize objects */
    int immortalize;
#endif
};

#ifdef Py_GIL_DISABLED
struct _gc_thread_state {
    /* Thread-local allocation count. */
    Py_ssize_t alloc_count;
};
#endif


extern void _PyGC_InitState(struct _gc_runtime_state *);

extern Py_ssize_t _PyGC_Collect(PyThreadState *tstate, int generation,
                                _PyGC_Reason reason);
extern void _PyGC_CollectNoFail(PyThreadState *tstate);

/* Freeze objects tracked by the GC and ignore them in future collections. */
extern void _PyGC_Freeze(PyInterpreterState *interp);
/* Unfreezes objects placing them in the oldest generation */
extern void _PyGC_Unfreeze(PyInterpreterState *interp);
/* Number of frozen objects */
extern Py_ssize_t _PyGC_GetFreezeCount(PyInterpreterState *interp);

extern PyObject *_PyGC_GetObjects(PyInterpreterState *interp, int generation);
extern PyObject *_PyGC_GetReferrers(PyInterpreterState *interp, PyObject *objs);

// Functions to clear types free lists
extern void _PyGC_ClearAllFreeLists(PyInterpreterState *interp);
extern void _Py_ScheduleGC(PyThreadState *tstate);
extern void _Py_RunGC(PyThreadState *tstate);

#ifdef Py_GIL_DISABLED
// gh-117783: Immortalize objects that use deferred reference counting
extern void _PyGC_ImmortalizeDeferredObjects(PyInterpreterState *interp);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GC_H */
