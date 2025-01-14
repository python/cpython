#ifndef Py_INTERNAL_GC_H
#define Py_INTERNAL_GC_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* GC information is stored BEFORE the object structure. */
typedef struct {
    // Tagged pointer to next object in the list.
    // 0 means the object is not tracked
    uintptr_t _gc_next;

    // Tagged pointer to previous object in the list.
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
#  define _PyGC_BITS_TRACKED        (1<<0)     // Tracked by the GC
#  define _PyGC_BITS_FINALIZED      (1<<1)     // tp_finalize was called
#  define _PyGC_BITS_UNREACHABLE    (1<<2)
#  define _PyGC_BITS_FROZEN         (1<<3)
#  define _PyGC_BITS_SHARED         (1<<4)
#  define _PyGC_BITS_ALIVE          (1<<5)    // Reachable from a known root.
#  define _PyGC_BITS_DEFERRED       (1<<6)    // Use deferred reference counting
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

#endif

/* Bit flags for _gc_prev */
/* Bit 0 is set when tp_finalize is called */
#define _PyGC_PREV_MASK_FINALIZED  ((uintptr_t)1)
/* Bit 1 is set when the object is in generation which is GCed currently. */
#define _PyGC_PREV_MASK_COLLECTING ((uintptr_t)2)

/* Bit 0 in _gc_next is the old space bit.
 * It is set as follows:
 * Young: gcstate->visited_space
 * old[0]: 0
 * old[1]: 1
 * permanent: 0
 *
 * During a collection all objects handled should have the bit set to
 * gcstate->visited_space, as objects are moved from the young gen
 * and the increment into old[gcstate->visited_space].
 * When object are moved from the pending space, old[gcstate->visited_space^1]
 * into the increment, the old space bit is flipped.
*/
#define _PyGC_NEXT_MASK_OLD_SPACE_1    1

#define _PyGC_PREV_SHIFT           2
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
    uintptr_t next = gc->_gc_next & _PyGC_PREV_MASK;
    return (PyGC_Head*)next;
}
static inline void _PyGCHead_SET_NEXT(PyGC_Head *gc, PyGC_Head *next) {
    uintptr_t unext = (uintptr_t)next;
    assert((unext & ~_PyGC_PREV_MASK) == 0);
    gc->_gc_next = (gc->_gc_next & ~_PyGC_PREV_MASK) | unext;
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

struct gc_collection_stats {
    /* number of collected objects */
    Py_ssize_t collected;
    /* total number of uncollectable objects (put into gc.garbage) */
    Py_ssize_t uncollectable;
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

enum _GCPhase {
    GC_PHASE_MARK = 0,
    GC_PHASE_COLLECT = 1
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
    struct gc_generation young;
    struct gc_generation old[2];
    /* a permanent generation which won't be collected */
    struct gc_generation permanent_generation;
    struct gc_generation_stats generation_stats[NUM_GENERATIONS];
    /* true if we are currently running the collector */
    int collecting;
    /* list of uncollectable objects */
    PyObject *garbage;
    /* a list of callbacks to be invoked when collection is performed */
    PyObject *callbacks;

    Py_ssize_t heap_size;
    Py_ssize_t work_to_do;
    /* Which of the old spaces is the visited space */
    int visited_space;
    int phase;

#ifdef Py_GIL_DISABLED
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

    /* True if gc.freeze() has been used. */
    int freeze_active;
#endif
};

#ifdef Py_GIL_DISABLED
struct _gc_thread_state {
    /* Thread-local allocation count. */
    Py_ssize_t alloc_count;
};
#endif


extern void _PyGC_InitState(struct _gc_runtime_state *);

extern Py_ssize_t _PyGC_Collect(PyThreadState *tstate, int generation, _PyGC_Reason reason);
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

union _PyStackRef;

// GC visit callback for tracked interpreter frames
extern int _PyGC_VisitFrameStack(struct _PyInterpreterFrame *frame, visitproc visit, void *arg);
extern int _PyGC_VisitStackRef(union _PyStackRef *ref, visitproc visit, void *arg);

// Like Py_VISIT but for _PyStackRef fields
#define _Py_VISIT_STACKREF(ref)                                         \
    do {                                                                \
        if (!PyStackRef_IsNull(ref)) {                                  \
            int vret = _PyGC_VisitStackRef(&(ref), visit, arg);         \
            if (vret)                                                   \
                return vret;                                            \
        }                                                               \
    } while (0)

#ifdef Py_GIL_DISABLED
extern void _PyGC_VisitObjectsWorldStopped(PyInterpreterState *interp,
                                           gcvisitobjects_t callback, void *arg);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GC_H */
