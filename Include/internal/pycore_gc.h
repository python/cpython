#ifndef Py_INTERNAL_GC_H
#define Py_INTERNAL_GC_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_interp_structs.h" // PyGC_Head
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_typedefs.h"      // _PyInterpreterFrame


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

extern void _Py_ScheduleGC(PyThreadState *tstate);

#ifndef Py_GIL_DISABLED
extern void _Py_TriggerGC(struct _gc_runtime_state *gcstate);
#endif


/* Tell the GC to track this object.
 *
 * The object must not be tracked by the GC.
 *
 * NB: While the object is tracked by the collector, it must be safe to call the
 * ob_traverse method.
 *
 * Internal note: interp->gc.generation0->_gc_prev doesn't have any bit flags
 * because it's not object header.  So we don't use _PyGCHead_PREV() and
 * _PyGCHead_SET_PREV() for it to avoid unnecessary bitwise operations.
 *
 * See also the public PyObject_GC_Track() function.
 */
static inline void _PyObject_GC_TRACK(
// The preprocessor removes _PyObject_ASSERT_FROM() calls if NDEBUG is defined
#ifndef NDEBUG
    const char *filename, int lineno,
#endif
    PyObject *op)
{
    _PyObject_ASSERT_FROM(op, !_PyObject_GC_IS_TRACKED(op),
                          "object already tracked by the garbage collector",
                          filename, lineno, __func__);
#ifdef Py_GIL_DISABLED
    _PyObject_SET_GC_BITS(op, _PyGC_BITS_TRACKED);
#else
    PyGC_Head *gc = _Py_AS_GC(op);
    _PyObject_ASSERT_FROM(op,
                          (gc->_gc_prev & _PyGC_PREV_MASK_COLLECTING) == 0,
                          "object is in generation which is garbage collected",
                          filename, lineno, __func__);

    struct _gc_runtime_state *gcstate = &_PyInterpreterState_GET()->gc;
    PyGC_Head *generation0 = &gcstate->young.head;
    PyGC_Head *last = (PyGC_Head*)(generation0->_gc_prev);
    _PyGCHead_SET_NEXT(last, gc);
    _PyGCHead_SET_PREV(gc, last);
    uintptr_t not_visited = 1 ^ gcstate->visited_space;
    gc->_gc_next = ((uintptr_t)generation0) | not_visited;
    generation0->_gc_prev = (uintptr_t)gc;
    gcstate->young.count++; /* number of tracked GC objects */
    gcstate->heap_size++;
    if (gcstate->young.count > gcstate->young.threshold) {
        _Py_TriggerGC(gcstate);
    }
#endif
}

/* Tell the GC to stop tracking this object.
 *
 * Internal note: This may be called while GC. So _PyGC_PREV_MASK_COLLECTING
 * must be cleared. But _PyGC_PREV_MASK_FINALIZED bit is kept.
 *
 * The object must be tracked by the GC.
 *
 * See also the public PyObject_GC_UnTrack() which accept an object which is
 * not tracked.
 */
static inline void _PyObject_GC_UNTRACK(
// The preprocessor removes _PyObject_ASSERT_FROM() calls if NDEBUG is defined
#ifndef NDEBUG
    const char *filename, int lineno,
#endif
    PyObject *op)
{
    _PyObject_ASSERT_FROM(op, _PyObject_GC_IS_TRACKED(op),
                          "object not tracked by the garbage collector",
                          filename, lineno, __func__);

#ifdef Py_GIL_DISABLED
    _PyObject_CLEAR_GC_BITS(op, _PyGC_BITS_TRACKED);
#else
    PyGC_Head *gc = _Py_AS_GC(op);
    PyGC_Head *prev = _PyGCHead_PREV(gc);
    PyGC_Head *next = _PyGCHead_NEXT(gc);
    _PyGCHead_SET_NEXT(prev, next);
    _PyGCHead_SET_PREV(next, prev);
    gc->_gc_next = 0;
    gc->_gc_prev &= _PyGC_PREV_MASK_FINALIZED;
    struct _gc_runtime_state *gcstate = &_PyInterpreterState_GET()->gc;
    if (gcstate->young.count > 0) {
        gcstate->young.count--;
    }
    gcstate->heap_size--;
#endif
}



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
extern void _Py_RunGC(PyThreadState *tstate);

union _PyStackRef;

// GC visit callback for tracked interpreter frames
extern int _PyGC_VisitFrameStack(_PyInterpreterFrame *frame, visitproc visit, void *arg);
extern int _PyGC_VisitStackRef(union _PyStackRef *ref, visitproc visit, void *arg);

#ifdef Py_GIL_DISABLED
extern void _PyGC_VisitObjectsWorldStopped(PyInterpreterState *interp,
                                           gcvisitobjects_t callback, void *arg);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GC_H */
