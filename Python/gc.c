// Generational GC specific code, forward-ported from CPython 3.13.

#include "Python.h"

#if !defined(Py_GIL_DISABLED)

#include "pycore_ceval.h"         // _Py_set_eval_breaker_bit()
#include "pycore_dict.h"          // _PyInlineValuesSize()
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_context.h"
#include "pycore_interp.h"        // PyInterpreterState.gc
#include "pycore_interpframe.h"   // _PyFrame_GetLocalsArray()
#include "pycore_object.h"
#include "pycore_object_alloc.h"  // _PyObject_MallocWithType()
#include "pycore_pyerrors.h"
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tuple.h"         // _PyTuple_MaybeUntrack()
#include "pycore_weakref.h"       // _PyWeakref_ClearRef()

#include "pydtrace.h"


typedef struct _gc_runtime_state GCState;

#ifdef Py_DEBUG
#  define GC_DEBUG
#endif



#define GC_NEXT _PyGCHead_NEXT
#define GC_PREV _PyGCHead_PREV

// update_refs() set this bit for all objects in current generation.
// subtract_refs() and move_unreachable() uses this to distinguish
// visited object is in GCing or not.
//
// move_unreachable() removes this flag from reachable objects.
// Only unreachable objects have this flag.
//
// No objects in interpreter have this flag after GC ends.
#define PREV_MASK_COLLECTING   _PyGC_PREV_MASK_COLLECTING

// Lowest bit of _gc_next is used for UNREACHABLE flag.
//
// This flag represents the object is in unreachable list in move_unreachable()
//
// Although this flag is used only in move_unreachable(), move_unreachable()
// doesn't clear this flag to skip unnecessary iteration.
// move_legacy_finalizers() removes this flag instead.
// Between them, unreachable list is not normal list and we can not use
// most gc_list_* functions for it.
/* NEXT_MASK_UNREACHABLE is defined locally in each GC variant */

#define AS_GC(op) _Py_AS_GC(op)
#define FROM_GC(gc) _Py_FROM_GC(gc)

// Automatically choose the generation that needs collecting.
#define GENERATION_AUTO (-1)

static inline int
gc_is_collecting(PyGC_Head *g)
{
    return (g->_gc_prev & PREV_MASK_COLLECTING) != 0;
}

static inline void
gc_clear_collecting(PyGC_Head *g)
{
    g->_gc_prev &= ~PREV_MASK_COLLECTING;
}

static inline Py_ssize_t
gc_get_refs(PyGC_Head *g)
{
    return (Py_ssize_t)(g->_gc_prev >> _PyGC_PREV_SHIFT);
}

static inline void
gc_set_refs(PyGC_Head *g, Py_ssize_t refs)
{
    g->_gc_prev = (g->_gc_prev & ~_PyGC_PREV_MASK)
        | ((uintptr_t)(refs) << _PyGC_PREV_SHIFT);
}

static inline void
gc_reset_refs(PyGC_Head *g, Py_ssize_t refs)
{
    g->_gc_prev = (g->_gc_prev & _PyGC_PREV_MASK_FINALIZED)
        | PREV_MASK_COLLECTING
        | ((uintptr_t)(refs) << _PyGC_PREV_SHIFT);
}

static inline void
gc_decref(PyGC_Head *g)
{
    _PyObject_ASSERT_WITH_MSG(FROM_GC(g),
                              gc_get_refs(g) > 0,
                              "refcount is too small");
    g->_gc_prev -= 1 << _PyGC_PREV_SHIFT;
}


static inline GCState *
get_gc_state(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return &interp->gc;
}



/*
_gc_prev values
---------------

Between collections, _gc_prev is used for doubly linked list.

Lowest two bits of _gc_prev are used for flags.
PREV_MASK_COLLECTING is used only while collecting and cleared before GC ends
or _PyObject_GC_UNTRACK() is called.

During a collection, _gc_prev is temporary used for gc_refs, and the gc list
is singly linked until _gc_prev is restored.

gc_refs
    At the start of a collection, update_refs() copies the true refcount
    to gc_refs, for each object in the generation being collected.
    subtract_refs() then adjusts gc_refs so that it equals the number of
    times an object is referenced directly from outside the generation
    being collected.

PREV_MASK_COLLECTING
    Objects in generation being collected are marked PREV_MASK_COLLECTING in
    update_refs().


_gc_next values
---------------

_gc_next takes these values:

0
    The object is not tracked

!= 0
    Pointer to the next object in the GC list.
    Additionally, lowest bit is used temporary for
    NEXT_MASK_UNREACHABLE flag described below.

NEXT_MASK_UNREACHABLE
    move_unreachable() then moves objects not reachable (whether directly or
    indirectly) from outside the generation into an "unreachable" set and
    set this flag.

    Objects that are found to be reachable have gc_refs set to 1.
    When this flag is set for the reachable object, the object must be in
    "unreachable" set.
    The flag is unset and the object is moved back to "reachable" set.

    move_legacy_finalizers() will remove this flag from "unreachable" set.
*/

/*** list functions ***/

static inline void
gc_list_init(PyGC_Head *list)
{
    // List header must not have flags.
    // We can assign pointer by simple cast.
    list->_gc_prev = (uintptr_t)list;
    list->_gc_next = (uintptr_t)list;
}

static inline int
gc_list_is_empty(PyGC_Head *list)
{
    return (list->_gc_next == (uintptr_t)list);
}

/* Append `node` to `list`. */
static inline void
gc_list_append(PyGC_Head *node, PyGC_Head *list)
{
    assert((list->_gc_prev & ~_PyGC_PREV_MASK) == 0);
    PyGC_Head *last = (PyGC_Head *)list->_gc_prev;

    // last <-> node
    _PyGCHead_SET_PREV(node, last);
    _PyGCHead_SET_NEXT(last, node);

    // node <-> list
    _PyGCHead_SET_NEXT(node, list);
    list->_gc_prev = (uintptr_t)node;
}

/* Remove `node` from the gc list it's currently in. */
static inline void
gc_list_remove(PyGC_Head *node)
{
    PyGC_Head *prev = GC_PREV(node);
    PyGC_Head *next = GC_NEXT(node);

    _PyGCHead_SET_NEXT(prev, next);
    _PyGCHead_SET_PREV(next, prev);

    node->_gc_next = 0; /* object is not currently tracked */
}

/* Move `node` from the gc list it's currently in (which is not explicitly
 * named here) to the end of `list`.  This is semantically the same as
 * gc_list_remove(node) followed by gc_list_append(node, list).
 */
static inline void
gc_list_move(PyGC_Head *node, PyGC_Head *list)
{
    /* Unlink from current list. */
    PyGC_Head *from_prev = GC_PREV(node);
    PyGC_Head *from_next = GC_NEXT(node);
    _PyGCHead_SET_NEXT(from_prev, from_next);
    _PyGCHead_SET_PREV(from_next, from_prev);

    /* Relink at end of new list. */
    // list must not have flags.  So we can skip macros.
    PyGC_Head *to_prev = (PyGC_Head*)list->_gc_prev;
    _PyGCHead_SET_PREV(node, to_prev);
    _PyGCHead_SET_NEXT(to_prev, node);
    list->_gc_prev = (uintptr_t)node;
    _PyGCHead_SET_NEXT(node, list);
}

/* append list `from` onto list `to`; `from` becomes an empty list */
static inline void
gc_list_merge(PyGC_Head *from, PyGC_Head *to)
{
    assert(from != to);
    if (!gc_list_is_empty(from)) {
        PyGC_Head *to_tail = GC_PREV(to);
        PyGC_Head *from_head = GC_NEXT(from);
        PyGC_Head *from_tail = GC_PREV(from);
        assert(from_head != from);
        assert(from_tail != from);

        _PyGCHead_SET_NEXT(to_tail, from_head);
        _PyGCHead_SET_PREV(from_head, to_tail);

        _PyGCHead_SET_NEXT(from_tail, to);
        _PyGCHead_SET_PREV(to, from_tail);
    }
    gc_list_init(from);
}

static inline Py_ssize_t
gc_list_size(PyGC_Head *list)
{
    PyGC_Head *gc;
    Py_ssize_t n = 0;
    for (gc = GC_NEXT(list); gc != list; gc = GC_NEXT(gc)) {
        n++;
    }
    return n;
}

/* Walk the list and mark all objects as non-collecting */
static inline void
gc_list_clear_collecting(PyGC_Head *collectable)
{
    PyGC_Head *gc;
    for (gc = GC_NEXT(collectable); gc != collectable; gc = GC_NEXT(gc)) {
        gc_clear_collecting(gc);
    }
}

/* Append objects in a GC list to a Python list.
 * Return 0 if all OK, < 0 if error (out of memory for list)
 */
static inline int
append_objects(PyObject *py_list, PyGC_Head *gc_list)
{
    PyGC_Head *gc;
    for (gc = GC_NEXT(gc_list); gc != gc_list; gc = GC_NEXT(gc)) {
        PyObject *op = FROM_GC(gc);
        if (op != py_list) {
            if (PyList_Append(py_list, op)) {
                return -1; /* exception */
            }
        }
    }
    return 0;
}

// Constants for validate_list's flags argument.
enum flagstates {collecting_clear_unreachable_clear,
                 collecting_clear_unreachable_set,
                 collecting_set_unreachable_clear,
                 collecting_set_unreachable_set};


/*** end of list stuff ***/


/* A traversal callback for subtract_refs.
 *
 * This function must have external linkage (not static) because
 * _PyGC_VisitStackRef compares function pointers against it.
 * If it were static in a shared header, each compilation unit would
 * get a different copy with a different address, breaking the comparison.
 */
int
_PyGC_VisitDecref(PyObject *op, void *parent);

/* Subtract internal references from gc_refs.  After this, gc_refs is >= 0
 * for all objects in containers, and is GC_REACHABLE for all tracked gc
 * objects not in containers.  The ones with gc_refs > 0 are directly
 * reachable from outside containers, and so can't be collected.
 */
static inline void
subtract_refs(PyGC_Head *containers)
{
    traverseproc traverse;
    PyGC_Head *gc = GC_NEXT(containers);
    for (; gc != containers; gc = GC_NEXT(gc)) {
        PyObject *op = FROM_GC(gc);
        traverse = Py_TYPE(op)->tp_traverse;
        (void) traverse(op,
                        _PyGC_VisitDecref,
                        op);
    }
}



/* In theory, all tuples should be younger than the
* objects they refer to, as tuples are immortal.
* Therefore, untracking tuples in oldest-first order in the
* young generation before promoting them should have tracked
* all the tuples that can be untracked.
*
* Unfortunately, the C API allows tuples to be created
* and then filled in. So this won't untrack all tuples
* that can be untracked. It should untrack most of them
* and is much faster than a more complex approach that
* would untrack all relevant tuples.
*/
static inline void
untrack_tuples(PyGC_Head *head)
{
    PyGC_Head *gc = GC_NEXT(head);
    while (gc != head) {
        PyObject *op = FROM_GC(gc);
        PyGC_Head *next = GC_NEXT(gc);
        if (PyTuple_CheckExact(op)) {
            _PyTuple_MaybeUntrack(op);
        }
        gc = next;
    }
}

/* Return true if object has a pre-PEP 442 finalization method. */
static inline int
has_legacy_finalizer(PyObject *op)
{
    return Py_TYPE(op)->tp_del != NULL;
}


/* A traversal callback for move_legacy_finalizer_reachable. */
static inline int
visit_move(PyObject *op, void *arg)
{
    PyGC_Head *tolist = arg;
    OBJECT_STAT_INC(object_visits);
    if (_PyObject_IS_GC(op)) {
        PyGC_Head *gc = AS_GC(op);
        if (gc_is_collecting(gc)) {
            gc_list_move(gc, tolist);
            gc_clear_collecting(gc);
        }
    }
    return 0;
}

/* Move objects that are reachable from finalizers, from the unreachable set
 * into finalizers set.
 */
static inline void
move_legacy_finalizer_reachable(PyGC_Head *finalizers)
{
    traverseproc traverse;
    PyGC_Head *gc = GC_NEXT(finalizers);
    for (; gc != finalizers; gc = GC_NEXT(gc)) {
        /* Note that the finalizers list may grow during this. */
        traverse = Py_TYPE(FROM_GC(gc))->tp_traverse;
        (void) traverse(FROM_GC(gc),
                        visit_move,
                        (void *)finalizers);
    }
}

static inline void
debug_cycle(const char *msg, PyObject *op)
{
    PySys_FormatStderr("gc: %s <%s %p>\n",
                       msg, Py_TYPE(op)->tp_name, op);
}

/* Handle uncollectable garbage (cycles with tp_del slots, and stuff reachable
 * only from such cycles).
 * If _PyGC_DEBUG_SAVEALL, all objects in finalizers are appended to the module
 * garbage list (a Python list), else only the objects in finalizers with
 * __del__ methods are appended to garbage.  All objects in finalizers are
 * merged into the old list regardless.
 */
static inline void
handle_legacy_finalizers(PyThreadState *tstate,
                         GCState *gcstate,
                         PyGC_Head *finalizers, PyGC_Head *old)
{
    assert(!_PyErr_Occurred(tstate));
    assert(gcstate->garbage != NULL);

    PyGC_Head *gc = GC_NEXT(finalizers);
    for (; gc != finalizers; gc = GC_NEXT(gc)) {
        PyObject *op = FROM_GC(gc);

        if ((gcstate->debug & _PyGC_DEBUG_SAVEALL) || has_legacy_finalizer(op)) {
            if (PyList_Append(gcstate->garbage, op) < 0) {
                _PyErr_Clear(tstate);
                break;
            }
        }
    }

    gc_list_merge(finalizers, old);
}

/* Run first-time finalizers (if any) on all the objects in collectable.
 * Note that this may remove some (or even all) of the objects from the
 * list, due to refcounts falling to 0.
 */
static inline void
finalize_garbage(PyThreadState *tstate, PyGC_Head *collectable)
{
    destructor finalize;
    PyGC_Head seen;

    /* While we're going through the loop, `finalize(op)` may cause op, or
     * other objects, to be reclaimed via refcounts falling to zero.  So
     * there's little we can rely on about the structure of the input
     * `collectable` list across iterations.  For safety, we always take the
     * first object in that list and move it to a temporary `seen` list.
     * If objects vanish from the `collectable` and `seen` lists we don't
     * care.
     */
    gc_list_init(&seen);

    while (!gc_list_is_empty(collectable)) {
        PyGC_Head *gc = GC_NEXT(collectable);
        PyObject *op = FROM_GC(gc);
        gc_list_move(gc, &seen);
        if (!_PyGC_FINALIZED(op) &&
            (finalize = Py_TYPE(op)->tp_finalize) != NULL)
        {
            _PyGC_SET_FINALIZED(op);
            Py_INCREF(op);
            finalize(op);
            assert(!_PyErr_Occurred(tstate));
            Py_DECREF(op);
        }
    }
    gc_list_merge(&seen, collectable);
}

/* Break reference cycles by clearing the containers involved.  This is
 * tricky business as the lists can be changing and we don't know which
 * objects may be freed.  It is possible I screwed something up here.
 */
static inline void
delete_garbage(PyThreadState *tstate, GCState *gcstate,
               PyGC_Head *collectable, PyGC_Head *old)
{
    assert(!_PyErr_Occurred(tstate));

    while (!gc_list_is_empty(collectable)) {
        PyGC_Head *gc = GC_NEXT(collectable);
        PyObject *op = FROM_GC(gc);

        _PyObject_ASSERT_WITH_MSG(op, Py_REFCNT(op) > 0,
                                  "refcount is too small");

        if (gcstate->debug & _PyGC_DEBUG_SAVEALL) {
            assert(gcstate->garbage != NULL);
            if (PyList_Append(gcstate->garbage, op) < 0) {
                _PyErr_Clear(tstate);
            }
        }
        else {
            inquiry clear;
            if ((clear = Py_TYPE(op)->tp_clear) != NULL) {
                Py_INCREF(op);
                (void) clear(op);
                if (_PyErr_Occurred(tstate)) {
                    PyErr_FormatUnraisable("Exception ignored in tp_clear of %s",
                                           Py_TYPE(op)->tp_name);
                }
                Py_DECREF(op);
            }
        }
        if (GC_NEXT(collectable) == gc) {
            /* object is still alive, move it, it may die later */
            gc_clear_collecting(gc);
            gc_list_move(gc, old);
        }
    }
}



static inline int
referrersvisit(PyObject* obj, void *arg)
{
    PyObject *objs = arg;
    Py_ssize_t i;
    for (i = 0; i < PyTuple_GET_SIZE(objs); i++) {
        if (PyTuple_GET_ITEM(objs, i) == obj) {
            return 1;
        }
    }
    return 0;
}

static inline int
gc_referrers_for(PyObject *objs, PyGC_Head *list, PyObject *resultlist)
{
    PyGC_Head *gc;
    PyObject *obj;
    traverseproc traverse;
    for (gc = GC_NEXT(list); gc != list; gc = GC_NEXT(gc)) {
        obj = FROM_GC(gc);
        traverse = Py_TYPE(obj)->tp_traverse;
        if (obj == objs || obj == resultlist) {
            continue;
        }
        if (traverse(obj, referrersvisit, objs)) {
            if (PyList_Append(resultlist, obj) < 0) {
                return 0; /* error */
            }
        }
    }
    return 1; /* no error */
}


static inline void
finalize_unlink_gc_head(PyGC_Head *gc) {
    PyGC_Head *prev = GC_PREV(gc);
    PyGC_Head *next = GC_NEXT(gc);
    _PyGCHead_SET_NEXT(prev, next);
    _PyGCHead_SET_PREV(next, prev);
}


static inline int
visit_generation(gcvisitobjects_t callback, void *arg, struct gc_generation *gen)
{
    PyGC_Head *gc_list, *gc;
    gc_list = &gen->head;
    for (gc = GC_NEXT(gc_list); gc != gc_list; gc = GC_NEXT(gc)) {
        PyObject *op = FROM_GC(gc);
        Py_INCREF(op);
        int res = callback(op, arg);
        Py_DECREF(op);
        if (!res) {
            return -1;
        }
    }
    return 0;
}

#define NEXT_MASK_UNREACHABLE  (1)

#define GEN_HEAD(gcstate, n) (&(gcstate)->generations[n].head)


void
_PyGC_InitState(GCState *gcstate)
{
#define INIT_HEAD(GEN) \
    do { \
        GEN.head._gc_next = (uintptr_t)&GEN.head; \
        GEN.head._gc_prev = (uintptr_t)&GEN.head; \
    } while (0)

    for (int i = 0; i < NUM_GENERATIONS; i++) {
        assert(gcstate->generations[i].count == 0);
        INIT_HEAD(gcstate->generations[i]);
        memset(&gcstate->generation_stats_gen[i], 0,
               sizeof(gcstate->generation_stats_gen[i]));
    };
    gcstate->generation0 = GEN_HEAD(gcstate, 0);
    gcstate->long_lived_total = 0;
    gcstate->long_lived_pending = 0;
    gcstate->frame = NULL;
    INIT_HEAD(gcstate->permanent_generation);

#undef INIT_HEAD
}


PyStatus
_PyGC_Init(PyInterpreterState *interp)
{
    GCState *gcstate = &interp->gc;

    gcstate->garbage = PyList_New(0);
    if (gcstate->garbage == NULL) {
        return _PyStatus_NO_MEMORY();
    }

    gcstate->callbacks = PyList_New(0);
    if (gcstate->callbacks == NULL) {
        return _PyStatus_NO_MEMORY();
    }

    return _PyStatus_OK();
}



#ifdef GC_DEBUG
static void
validate_list(PyGC_Head *head, enum flagstates flags)
{
    assert((head->_gc_prev & ~_PyGC_PREV_MASK) == 0);
    assert((head->_gc_next & ~_PyGC_PREV_MASK) == 0);
    uintptr_t prev_value = 0, next_value = 0;
    switch (flags) {
        case collecting_clear_unreachable_clear:
            break;
        case collecting_set_unreachable_clear:
            prev_value = PREV_MASK_COLLECTING;
            break;
        case collecting_clear_unreachable_set:
            next_value = NEXT_MASK_UNREACHABLE;
            break;
        case collecting_set_unreachable_set:
            prev_value = PREV_MASK_COLLECTING;
            next_value = NEXT_MASK_UNREACHABLE;
            break;
        default:
            assert(! "bad internal flags argument");
    }
    PyGC_Head *prev = head;
    PyGC_Head *gc = GC_NEXT(head);
    while (gc != head) {
        PyGC_Head *trueprev = GC_PREV(gc);
        PyGC_Head *truenext = (PyGC_Head *)(gc->_gc_next  & ~NEXT_MASK_UNREACHABLE);
        assert(truenext != NULL);
        assert(trueprev == prev);
        assert((gc->_gc_prev & PREV_MASK_COLLECTING) == prev_value);
        assert((gc->_gc_next & NEXT_MASK_UNREACHABLE) == next_value);
        prev = gc;
        gc = truenext;
    }
    assert(prev == GC_PREV(head));
}
#else
#define validate_list(x, y) do{}while(0)
#endif

/*** end of list stuff ***/


/* Set all gc_refs = ob_refcnt.  After this, gc_refs is > 0 and
 * PREV_MASK_COLLECTING bit is set for all objects in containers.
 */
static Py_ssize_t
update_refs(PyGC_Head *containers)
{
    PyGC_Head *next;
    PyGC_Head *gc = GC_NEXT(containers);
    Py_ssize_t candidates = 0;

    while (gc != containers) {
        next = GC_NEXT(gc);
        PyObject *op = FROM_GC(gc);
        if (_Py_IsImmortal(op)) {
            assert(!_Py_IsStaticImmortal(op));
            _PyObject_GC_UNTRACK(op);
           gc = next;
           continue;
        }
        gc_reset_refs(gc, Py_REFCNT(op));
        _PyObject_ASSERT(op, gc_get_refs(gc) != 0);
        gc = next;
        candidates++;
    }
    return candidates;
}

int
_PyGC_VisitDecref(PyObject *op, void *parent)
{
    OBJECT_STAT_INC(object_visits);
    _PyObject_ASSERT(parent, !_PyObject_IsFreed(op));

    if (_PyObject_IS_GC(op)) {
        PyGC_Head *gc = AS_GC(op);
        if (gc_is_collecting(gc)) {
            gc_decref(gc);
        }
    }
    return 0;
}

int
_PyGC_VisitStackRef(_PyStackRef *ref, visitproc visit, void *arg)
{
    assert(!PyStackRef_IsTaggedInt(*ref));
    if (!PyStackRef_RefcountOnObject(*ref) && (visit == _PyGC_VisitDecref)) {
        return 0;
    }
    Py_VISIT(PyStackRef_AsPyObjectBorrow(*ref));
    return 0;
}

int
_PyGC_VisitFrameStack(_PyInterpreterFrame *frame, visitproc visit, void *arg)
{
    _PyStackRef *ref = _PyFrame_GetLocalsArray(frame);
    for (; ref < frame->stackpointer; ref++) {
        if (!PyStackRef_IsTaggedInt(*ref)) {
            _Py_VISIT_STACKREF(*ref);
        }
    }
    return 0;
}

/* A traversal callback for subtract_refs. */
static int
visit_reachable(PyObject *op, void *arg)
{
    PyGC_Head *reachable = arg;
    OBJECT_STAT_INC(object_visits);
    if (!_PyObject_IS_GC(op)) {
        return 0;
    }

    PyGC_Head *gc = AS_GC(op);
    const Py_ssize_t gc_refs = gc_get_refs(gc);

    if (! gc_is_collecting(gc)) {
        return 0;
    }
    _PyObject_ASSERT(op, gc->_gc_next != 0);

    if (gc->_gc_next & NEXT_MASK_UNREACHABLE) {
        PyGC_Head *prev = GC_PREV(gc);
        PyGC_Head *next = (PyGC_Head*)(gc->_gc_next & ~NEXT_MASK_UNREACHABLE);
        _PyObject_ASSERT(FROM_GC(prev),
                         prev->_gc_next & NEXT_MASK_UNREACHABLE);
        _PyObject_ASSERT(FROM_GC(next),
                         next->_gc_next & NEXT_MASK_UNREACHABLE);
        prev->_gc_next = gc->_gc_next;  // copy NEXT_MASK_UNREACHABLE
        gc->_gc_next &= ~NEXT_MASK_UNREACHABLE;
        _PyGCHead_SET_PREV(next, prev);

        gc_list_append(gc, reachable);
        gc_set_refs(gc, 1);
    }
    else if (gc_refs == 0) {
        gc_set_refs(gc, 1);
    }
    else {
        _PyObject_ASSERT_WITH_MSG(op, gc_refs > 0, "refcount is too small");
    }
    return 0;
}

static void
move_unreachable(PyGC_Head *young, PyGC_Head *unreachable)
{
    PyGC_Head *prev = young;
    PyGC_Head *gc = GC_NEXT(young);

    while (gc != young) {
        if (gc_get_refs(gc)) {
            PyObject *op = FROM_GC(gc);
            traverseproc traverse = Py_TYPE(op)->tp_traverse;
            _PyObject_ASSERT_WITH_MSG(op, gc_get_refs(gc) > 0,
                                      "refcount is too small");
            (void) traverse(op,
                    visit_reachable,
                    (void *)young);
            _PyGCHead_SET_PREV(gc, prev);
            gc_clear_collecting(gc);
            prev = gc;
        }
        else {
            prev->_gc_next = gc->_gc_next;

            PyGC_Head *last = GC_PREV(unreachable);
            last->_gc_next = (NEXT_MASK_UNREACHABLE | (uintptr_t)gc);
            _PyGCHead_SET_PREV(gc, last);
            gc->_gc_next = (NEXT_MASK_UNREACHABLE | (uintptr_t)unreachable);
            unreachable->_gc_prev = (uintptr_t)gc;
        }
        gc = (PyGC_Head*)prev->_gc_next;
    }
    young->_gc_prev = (uintptr_t)prev;
    unreachable->_gc_next &= ~NEXT_MASK_UNREACHABLE;
}

static void
move_legacy_finalizers(PyGC_Head *unreachable, PyGC_Head *finalizers)
{
    PyGC_Head *gc, *next;
    _PyObject_ASSERT(
        FROM_GC(unreachable),
        (unreachable->_gc_next & NEXT_MASK_UNREACHABLE) == 0);

    for (gc = GC_NEXT(unreachable); gc != unreachable; gc = next) {
        PyObject *op = FROM_GC(gc);

        _PyObject_ASSERT(op, gc->_gc_next & NEXT_MASK_UNREACHABLE);
        gc->_gc_next &= ~NEXT_MASK_UNREACHABLE;
        next = (PyGC_Head*)gc->_gc_next;

        if (has_legacy_finalizer(op)) {
            gc_clear_collecting(gc);
            gc_list_move(gc, finalizers);
        }
    }
}

static inline void
clear_unreachable_mask(PyGC_Head *unreachable)
{
    _PyObject_ASSERT(
        FROM_GC(unreachable),
        ((uintptr_t)unreachable & NEXT_MASK_UNREACHABLE) == 0);
    _PyObject_ASSERT(
        FROM_GC(unreachable),
        (unreachable->_gc_next & NEXT_MASK_UNREACHABLE) == 0);

    PyGC_Head *gc, *next;
    for (gc = GC_NEXT(unreachable); gc != unreachable; gc = next) {
        _PyObject_ASSERT((PyObject*)FROM_GC(gc), gc->_gc_next & NEXT_MASK_UNREACHABLE);
        gc->_gc_next &= ~NEXT_MASK_UNREACHABLE;
        next = (PyGC_Head*)gc->_gc_next;
    }
    validate_list(unreachable, collecting_set_unreachable_clear);
}

/* Handle weakref callbacks.  Weakrefs without callbacks are cleared later,
 * after finalizers have run but before tp_clear() executes on the remaining
 * unreachable objects.
 */
static int
handle_weakref_callbacks(PyGC_Head *unreachable, PyGC_Head *old)
{
    PyGC_Head *gc;
    PyGC_Head wrcb_to_call;
    PyGC_Head *next;
    int num_freed = 0;

    gc_list_init(&wrcb_to_call);

    for (gc = GC_NEXT(unreachable); gc != unreachable; gc = next) {
        PyWeakReference **wrlist;

        PyObject *op = FROM_GC(gc);
        next = GC_NEXT(gc);

        if (! _PyType_SUPPORTS_WEAKREFS(Py_TYPE(op))) {
            continue;
        }

        wrlist = _PyObject_GET_WEAKREFS_LISTPTR_FROM_OFFSET(op);

        PyWeakReference *next_wr;
        for (PyWeakReference *wr = *wrlist; wr != NULL; wr = next_wr) {
            next_wr = wr->wr_next;

            if (wr->wr_callback == NULL) {
                continue;
            }

            _PyObject_ASSERT((PyObject *)wr, wr->wr_object == op);
            _PyWeakref_ClearRef(wr);
            _PyObject_ASSERT((PyObject *)wr, wr->wr_object == Py_None);

            if (gc_is_collecting(AS_GC((PyObject *)wr))) {
                continue;
            }

            Py_INCREF(wr);

            PyGC_Head *wrasgc = AS_GC((PyObject *)wr);
            _PyObject_ASSERT((PyObject *)wr, wrasgc != next);
            gc_list_move(wrasgc, &wrcb_to_call);
        }
    }

    while (!gc_list_is_empty(&wrcb_to_call)) {
        PyObject *temp;
        PyObject *callback;

        gc = (PyGC_Head *)wrcb_to_call._gc_next;
        PyObject *op = FROM_GC(gc);
        _PyObject_ASSERT(op, PyWeakref_Check(op));
        PyWeakReference *wr = (PyWeakReference *)op;
        callback = wr->wr_callback;
        _PyObject_ASSERT(op, callback != NULL);

        temp = PyObject_CallOneArg(callback, (PyObject *)wr);
        if (temp == NULL) {
            PyErr_FormatUnraisable("Exception ignored on "
                                   "calling weakref callback %R", callback);
        }
        else {
            Py_DECREF(temp);
        }

        Py_DECREF(op);
        if (wrcb_to_call._gc_next == (uintptr_t)gc) {
            gc_list_move(gc, old);
        }
        else {
            ++num_freed;
        }
    }

    return num_freed;
}

/* Clear all weakrefs to unreachable objects. */
static void
clear_weakrefs(PyGC_Head *unreachable)
{
    PyGC_Head *gc;
    PyGC_Head *next;

    for (gc = GC_NEXT(unreachable); gc != unreachable; gc = next) {
        PyWeakReference **wrlist;

        PyObject *op = FROM_GC(gc);
        next = GC_NEXT(gc);

        if (PyWeakref_Check(op)) {
            _PyWeakref_ClearRef((PyWeakReference *)op);
        }

        if (! _PyType_SUPPORTS_WEAKREFS(Py_TYPE(op))) {
            continue;
        }

        wrlist = _PyObject_GET_WEAKREFS_LISTPTR_FROM_OFFSET(op);

        for (PyWeakReference *wr = *wrlist; wr != NULL; wr = *wrlist) {
            _PyObject_ASSERT((PyObject *)wr, wr->wr_object == op);
            _PyWeakref_ClearRef(wr);
            _PyObject_ASSERT((PyObject *)wr, wr->wr_object == Py_None);
        }
    }
}

static void
show_stats_each_generations(GCState *gcstate)
{
    char buf[100];
    size_t pos = 0;

    for (int i = 0; i < NUM_GENERATIONS && pos < sizeof(buf); i++) {
        pos += PyOS_snprintf(buf+pos, sizeof(buf)-pos,
                             " %zd",
                             gc_list_size(GEN_HEAD(gcstate, i)));
    }

    PySys_FormatStderr(
        "gc: objects in each generation:%s\n"
        "gc: objects in permanent generation: %zd\n",
        buf, gc_list_size(&gcstate->permanent_generation.head));
}

static inline Py_ssize_t
deduce_unreachable(PyGC_Head *base, PyGC_Head *unreachable) {
    validate_list(base, collecting_clear_unreachable_clear);
    Py_ssize_t candidates = update_refs(base);  // gc_prev is used for gc_refs
    subtract_refs(base);
    gc_list_init(unreachable);
    move_unreachable(base, unreachable);  // gc_prev is pointer again
    validate_list(base, collecting_clear_unreachable_clear);
    validate_list(unreachable, collecting_set_unreachable_set);
    return candidates;
}

static inline void
handle_resurrected_objects(PyGC_Head *unreachable, PyGC_Head* still_unreachable,
                           PyGC_Head *old_generation)
{
    gc_list_clear_collecting(unreachable);

    PyGC_Head* resurrected = unreachable;
    deduce_unreachable(resurrected, still_unreachable);
    clear_unreachable_mask(still_unreachable);

    gc_list_merge(resurrected, old_generation);
}


static void
invoke_gc_callback(PyThreadState *tstate, const char *phase,
                   int generation, struct gc_generation_stats *stats)
{
    assert(!_PyErr_Occurred(tstate));

    GCState *gcstate = &tstate->interp->gc;
    if (gcstate->callbacks == NULL) {
        return;
    }

    assert(PyList_CheckExact(gcstate->callbacks));
    PyObject *info = NULL;
    if (PyList_GET_SIZE(gcstate->callbacks) != 0) {
        info = Py_BuildValue("{sisnsnsnsd}",
            "generation", generation,
            "collected", stats->collected,
            "uncollectable", stats->uncollectable,
            "candidates", stats->candidates,
            "duration", stats->duration);
        if (info == NULL) {
            PyErr_FormatUnraisable("Exception ignored while invoking gc callbacks");
            return;
        }
    }

    PyObject *phase_obj = PyUnicode_FromString(phase);
    if (phase_obj == NULL) {
        Py_XDECREF(info);
        PyErr_FormatUnraisable("Exception ignored while invoking gc callbacks");
        return;
    }

    PyObject *stack[] = {phase_obj, info};
    for (Py_ssize_t i=0; i<PyList_GET_SIZE(gcstate->callbacks); i++) {
        PyObject *r, *cb = PyList_GET_ITEM(gcstate->callbacks, i);
        Py_INCREF(cb);
        r = PyObject_Vectorcall(cb, stack, 2, NULL);
        if (r == NULL) {
            PyErr_FormatUnraisable("Exception ignored while "
                                   "calling GC callback %R", cb);
        }
        else {
            Py_DECREF(r);
        }
        Py_DECREF(cb);
    }
    Py_DECREF(phase_obj);
    Py_XDECREF(info);
    assert(!_PyErr_Occurred(tstate));
}


/* Find the oldest generation where the count exceeds the threshold. */
static int
gc_select_generation(GCState *gcstate)
{
    for (int i = NUM_GENERATIONS-1; i >= 0; i--) {
        if (gcstate->generations[i].count > gcstate->generations[i].threshold) {
            if (i == NUM_GENERATIONS - 1
                && gcstate->long_lived_pending < gcstate->long_lived_total / 4)
            {
                continue;
            }
            return i;
        }
    }
    return -1;
}


/* This is the main function.  Read this to understand how the
 * collection process works. */
static Py_ssize_t
gc_collect_main(PyThreadState *tstate, int generation, _PyGC_Reason reason)
{
    int i;
    Py_ssize_t m = 0; /* # objects collected */
    Py_ssize_t n = 0; /* # unreachable objects that couldn't be collected */
    PyGC_Head *young; /* the generation we are examining */
    PyGC_Head *old; /* next older generation */
    PyGC_Head unreachable; /* non-problematic unreachable trash */
    PyGC_Head finalizers;  /* objects with, & reachable from, __del__ */
    PyGC_Head *gc;
    struct gc_generation_stats stats = {0};
    GCState *gcstate = &tstate->interp->gc;

    // gc_collect_main() must not be called before _PyGC_Init
    // or after _PyGC_Fini()
    assert(gcstate->garbage != NULL);
    assert(!_PyErr_Occurred(tstate));
    assert(tstate->current_frame == NULL || tstate->current_frame->stackpointer != NULL);

    int expected = 0;
    if (!_Py_atomic_compare_exchange_int(&gcstate->collecting, &expected, 1)) {
        return 0;
    }
    gcstate->frame = tstate->current_frame;

    if (generation == GENERATION_AUTO) {
        generation = gc_select_generation(gcstate);
        if (generation < 0) {
            gcstate->frame = NULL;
            _Py_atomic_store_int(&gcstate->collecting, 0);
            return 0;
        }
    }

    assert(generation >= 0 && generation < NUM_GENERATIONS);

#ifdef Py_STATS
    if (_Py_stats) {
        _Py_stats->object_stats.object_visits = 0;
    }
#endif
    GC_STAT_ADD(generation, collections, 1);

    if (reason != _Py_GC_REASON_SHUTDOWN) {
        invoke_gc_callback(tstate, "start", generation, &stats);
    }

    if (gcstate->debug & _PyGC_DEBUG_STATS) {
        PySys_WriteStderr("gc: collecting generation %d...\n", generation);
        show_stats_each_generations(gcstate);
    }

    if (PyDTrace_GC_START_ENABLED()) {
        PyDTrace_GC_START(generation);
    }
    (void)PyTime_PerfCounterRaw(&stats.ts_start);

    /* update collection and allocation counters */
    if (generation+1 < NUM_GENERATIONS) {
        gcstate->generations[generation+1].count += 1;
    }
    for (i = 0; i <= generation; i++) {
        gcstate->generations[i].count = 0;
    }

    /* merge younger generations with one we are currently collecting */
    for (i = 0; i < generation; i++) {
        gc_list_merge(GEN_HEAD(gcstate, i), GEN_HEAD(gcstate, generation));
    }

    /* handy references */
    young = GEN_HEAD(gcstate, generation);
    if (generation < NUM_GENERATIONS-1) {
        old = GEN_HEAD(gcstate, generation+1);
    }
    else {
        old = young;
    }
    validate_list(old, collecting_clear_unreachable_clear);

    stats.candidates = deduce_unreachable(young, &unreachable);

    untrack_tuples(young);
    /* Move reachable objects to next generation. */
    if (young != old) {
        if (generation == NUM_GENERATIONS - 2) {
            gcstate->long_lived_pending += gc_list_size(young);
        }
        gc_list_merge(young, old);
    }
    else {
        /* We only un-track dicts in full collections, to avoid quadratic
           dict build-up. See issue #14775.
           Note: _PyDict_MaybeUntrack was removed in 3.14, so dict
           untracking during GC is no longer done. */
        gcstate->long_lived_pending = 0;
        gcstate->long_lived_total = gc_list_size(young);
    }

    /* All objects in unreachable are trash, but objects reachable from
     * legacy finalizers (e.g. tp_del) can't safely be deleted.
     */
    gc_list_init(&finalizers);
    // NEXT_MASK_UNREACHABLE is cleared here.
    move_legacy_finalizers(&unreachable, &finalizers);
    move_legacy_finalizer_reachable(&finalizers);

    validate_list(&finalizers, collecting_clear_unreachable_clear);
    validate_list(&unreachable, collecting_set_unreachable_clear);

    /* Print debugging information. */
    if (gcstate->debug & _PyGC_DEBUG_COLLECTABLE) {
        for (gc = GC_NEXT(&unreachable); gc != &unreachable; gc = GC_NEXT(gc)) {
            debug_cycle("collectable", FROM_GC(gc));
        }
    }

    /* Invoke weakref callbacks as necessary. */
    m += handle_weakref_callbacks(&unreachable, old);

    validate_list(old, collecting_clear_unreachable_clear);
    validate_list(&unreachable, collecting_set_unreachable_clear);

    /* Call tp_finalize on objects which have one. */
    finalize_garbage(tstate, &unreachable);

    /* Handle any objects that may have resurrected after the call
     * to 'finalize_garbage' and continue the collection with the
     * objects that are still unreachable */
    PyGC_Head final_unreachable;
    handle_resurrected_objects(&unreachable, &final_unreachable, old);

    /* Clear weakrefs to objects in the remaining unreachable set before
     * tp_clear() can expose them.
     */
    clear_weakrefs(&final_unreachable);

    m += gc_list_size(&final_unreachable);
    delete_garbage(tstate, gcstate, &final_unreachable, old);

    /* Collect statistics on uncollectable objects found and print
     * debugging information. */
    for (gc = GC_NEXT(&finalizers); gc != &finalizers; gc = GC_NEXT(gc)) {
        n++;
        if (gcstate->debug & _PyGC_DEBUG_UNCOLLECTABLE)
            debug_cycle("uncollectable", FROM_GC(gc));
    }
    (void)PyTime_PerfCounterRaw(&stats.ts_stop);
    stats.collected = m;
    stats.uncollectable = n;
    stats.duration = PyTime_AsSecondsDouble(stats.ts_stop - stats.ts_start);
    if (gcstate->debug & _PyGC_DEBUG_STATS) {
        PySys_WriteStderr(
            "gc: done, %zd unreachable, %zd uncollectable, %.4fs elapsed\n",
            n+m, n, stats.duration);
    }

    handle_legacy_finalizers(tstate, gcstate, &finalizers, old);
    validate_list(old, collecting_clear_unreachable_clear);

    /* Clear free list only during the collection of the highest
     * generation */
    if (generation == NUM_GENERATIONS-1) {
        _PyGC_ClearAllFreeLists(tstate->interp);
    }

    if (_PyErr_Occurred(tstate)) {
        if (reason == _Py_GC_REASON_SHUTDOWN) {
            _PyErr_Clear(tstate);
        }
        else {
            PyErr_FormatUnraisable("Exception ignored in garbage collection");
        }
    }

    /* Update stats */
    struct gc_generation_stats *total_stats = &gcstate->generation_stats_gen[generation];
    total_stats->collections++;
    total_stats->collected += m;
    total_stats->uncollectable += n;
    total_stats->candidates += stats.candidates;
    total_stats->duration += stats.duration;

    GC_STAT_ADD(generation, objects_collected, m);
#ifdef Py_STATS
    if (_Py_stats) {
        GC_STAT_ADD(generation, object_visits,
            _Py_stats->object_stats.object_visits);
        _Py_stats->object_stats.object_visits = 0;
    }
#endif

    if (PyDTrace_GC_DONE_ENABLED()) {
        PyDTrace_GC_DONE(n + m);
    }

    if (reason != _Py_GC_REASON_SHUTDOWN) {
        invoke_gc_callback(tstate, "stop", generation, &stats);
    }

    assert(!_PyErr_Occurred(tstate));
    gcstate->frame = NULL;
    _Py_atomic_store_int(&gcstate->collecting, 0);
    return n + m;
}

PyObject *
_PyGC_GetReferrers(PyInterpreterState *interp, PyObject *objs)
{
    PyObject *result = PyList_New(0);
    if (!result) {
        return NULL;
    }

    GCState *gcstate = &interp->gc;
    for (int i = 0; i < NUM_GENERATIONS; i++) {
        if (!(gc_referrers_for(objs, GEN_HEAD(gcstate, i), result))) {
            Py_DECREF(result);
            return NULL;
        }
    }
    return result;
}

PyObject *
_PyGC_GetObjects(PyInterpreterState *interp, int generation)
{
    assert(generation >= -1 && generation < NUM_GENERATIONS);
    GCState *gcstate = &interp->gc;

    PyObject *result = PyList_New(0);
    if (result == NULL) {
        return NULL;
    }

    if (generation == -1) {
        for (int i = 0; i < NUM_GENERATIONS; i++) {
            if (append_objects(result, GEN_HEAD(gcstate, i))) {
                goto error;
            }
        }
    }
    else {
        if (append_objects(result, GEN_HEAD(gcstate, generation))) {
            goto error;
        }
    }

    return result;
error:
    Py_DECREF(result);
    return NULL;
}

void
_PyGC_Freeze(PyInterpreterState *interp)
{
    GCState *gcstate = &interp->gc;
    for (int i = 0; i < NUM_GENERATIONS; ++i) {
        gc_list_merge(GEN_HEAD(gcstate, i), &gcstate->permanent_generation.head);
        gcstate->generations[i].count = 0;
    }
}

void
_PyGC_Unfreeze(PyInterpreterState *interp)
{
    GCState *gcstate = &interp->gc;
    gc_list_merge(&gcstate->permanent_generation.head,
                  GEN_HEAD(gcstate, NUM_GENERATIONS-1));
}

Py_ssize_t
_PyGC_GetFreezeCount(PyInterpreterState *interp)
{
    GCState *gcstate = &interp->gc;
    return gc_list_size(&gcstate->permanent_generation.head);
}

int
PyGC_Enable(void)
{
    GCState *gcstate = get_gc_state();
    int old_state = gcstate->enabled;
    gcstate->enabled = 1;
    return old_state;
}

int
PyGC_Disable(void)
{
    GCState *gcstate = get_gc_state();
    int old_state = gcstate->enabled;
    gcstate->enabled = 0;
    return old_state;
}

int
PyGC_IsEnabled(void)
{
    GCState *gcstate = get_gc_state();
    return gcstate->enabled;
}

Py_ssize_t
PyGC_Collect(void)
{
    return _PyGC_Collect(_PyThreadState_GET(), NUM_GENERATIONS - 1,
                         _Py_GC_REASON_MANUAL);
}

Py_ssize_t
_PyGC_Collect(PyThreadState *tstate, int generation, _PyGC_Reason reason)
{
    Py_ssize_t n;
    PyObject *exc = _PyErr_GetRaisedException(tstate);
    n = gc_collect_main(tstate, generation, reason);
    _PyErr_SetRaisedException(tstate, exc);
    return n;
}

void
_PyGC_CollectNoFail(PyThreadState *tstate)
{
    gc_collect_main(tstate, NUM_GENERATIONS - 1, _Py_GC_REASON_SHUTDOWN);
}

void
_PyGC_DumpShutdownStats(PyInterpreterState *interp)
{
    GCState *gcstate = &interp->gc;
    if (!(gcstate->debug & _PyGC_DEBUG_SAVEALL)
        && gcstate->garbage != NULL && PyList_GET_SIZE(gcstate->garbage) > 0) {
        const char *message;
        if (gcstate->debug & _PyGC_DEBUG_UNCOLLECTABLE) {
            message = "gc: %zd uncollectable objects at shutdown";
        }
        else {
            message = "gc: %zd uncollectable objects at shutdown; " \
                "use gc.set_debug(gc.DEBUG_UNCOLLECTABLE) to list them";
        }
        if (PyErr_WarnExplicitFormat(PyExc_ResourceWarning, "gc", 0,
                                     "gc", NULL, message,
                                     PyList_GET_SIZE(gcstate->garbage)))
        {
            PyErr_FormatUnraisable("Exception ignored in GC shutdown");
        }
        if (gcstate->debug & _PyGC_DEBUG_UNCOLLECTABLE) {
            PyObject *repr = NULL, *bytes = NULL;
            repr = PyObject_Repr(gcstate->garbage);
            if (!repr || !(bytes = PyUnicode_EncodeFSDefault(repr))) {
                PyErr_FormatUnraisable("Exception ignored in GC shutdown "
                                       "while formatting garbage");
            }
            else {
                PySys_WriteStderr(
                    "      %s\n",
                    PyBytes_AS_STRING(bytes)
                    );
            }
            Py_XDECREF(repr);
            Py_XDECREF(bytes);
        }
    }
}

void
_PyGC_Fini(PyInterpreterState *interp)
{
    GCState *gcstate = &interp->gc;
    Py_CLEAR(gcstate->garbage);
    Py_CLEAR(gcstate->callbacks);

    /* Prevent a subtle bug that affects sub-interpreters that use basic
     * single-phase init extensions (m_size == -1).  Those extensions cause objects
     * to be shared between interpreters, via the PyDict_Update(mdict, m_copy) call
     * in import_find_extension().
     *
     * If they are GC objects, their GC head next or prev links could refer to
     * the interpreter _gc_runtime_state PyGC_Head nodes.  Those nodes go away
     * when the interpreter structure is freed and so pointers to them become
     * invalid.  If those objects are still used by another interpreter and
     * UNTRACK is called on them, a crash will happen.  We untrack the nodes
     * here to avoid that.
     *
     * This bug was originally fixed when reported as gh-90228.  The bug was
     * re-introduced in gh-94673.
     */
    for (int i = 0; i < NUM_GENERATIONS; i++) {
        finalize_unlink_gc_head(&gcstate->generations[i].head);
    }
    finalize_unlink_gc_head(&gcstate->permanent_generation.head);
}

#ifdef Py_DEBUG
static int
visit_validate(PyObject *op, void *parent_raw)
{
    PyObject *parent = _PyObject_CAST(parent_raw);
    if (_PyObject_IsFreed(op)) {
        _PyObject_ASSERT_FAILED_MSG(parent,
                                    "PyObject_GC_Track() object is not valid");
    }
    return 0;
}
#endif


/* extension modules might be compiled with GC support so these
   functions must always be available */

void
PyObject_GC_Track(void *op_raw)
{
    PyObject *op = _PyObject_CAST(op_raw);
    if (_PyObject_GC_IS_TRACKED(op)) {
        _PyObject_ASSERT_FAILED_MSG(op,
                                    "object already tracked "
                                    "by the garbage collector");
    }
    _PyObject_GC_TRACK(op);

#ifdef Py_DEBUG
    /* Check that the object is valid: validate objects traversed
       by tp_traverse() */
    traverseproc traverse = Py_TYPE(op)->tp_traverse;
    (void)traverse(op, visit_validate, op);
#endif
}

void
PyObject_GC_UnTrack(void *op_raw)
{
    PyObject *op = _PyObject_CAST(op_raw);
    /* The code for some objects, such as tuples, is a bit
     * sloppy about when the object is tracked and untracked. */
    if (_PyObject_GC_IS_TRACKED(op)) {
        _PyObject_GC_UNTRACK(op);
    }
}

int
PyObject_IS_GC(PyObject *obj)
{
    return _PyObject_IS_GC(obj);
}

void
_Py_ScheduleGC(PyThreadState *tstate)
{
    if (!_Py_eval_breaker_bit_is_set(tstate, _PY_GC_SCHEDULED_BIT))
    {
        _Py_set_eval_breaker_bit(tstate, _PY_GC_SCHEDULED_BIT);
    }
}

void
_Py_TriggerGC(struct _gc_runtime_state *gcstate)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (gcstate->enabled &&
        gcstate->generations[0].threshold != 0 &&
        !_Py_atomic_load_int_relaxed(&gcstate->collecting) &&
        !_PyErr_Occurred(tstate))
    {
        _Py_ScheduleGC(tstate);
    }
}

/* for debugging */
void
_PyObject_GC_Link(PyObject *op)
{
    PyGC_Head *gc = AS_GC(op);
    // gc must be correctly aligned
    _PyObject_ASSERT(op, ((uintptr_t)gc & (sizeof(uintptr_t)-1)) == 0);

    gc->_gc_next = 0;
    gc->_gc_prev = 0;
}

void
_Py_RunGC(PyThreadState *tstate)
{
    GCState *gcstate = &tstate->interp->gc;
    if (!gcstate->enabled) {
        return;
    }
    gc_collect_main(tstate, GENERATION_AUTO, _Py_GC_REASON_HEAP);
}

static PyObject *
gc_alloc(PyTypeObject *tp, size_t basicsize, size_t presize)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (basicsize > PY_SSIZE_T_MAX - presize) {
        return _PyErr_NoMemory(tstate);
    }
    size_t size = presize + basicsize;
    char *mem = _PyObject_MallocWithType(tp, size);
    if (mem == NULL) {
        return _PyErr_NoMemory(tstate);
    }
    ((PyObject **)mem)[0] = NULL;
    ((PyObject **)mem)[1] = NULL;
    PyObject *op = (PyObject *)(mem + presize);
    _PyObject_GC_Link(op);
    return op;
}

PyObject *
_PyObject_GC_New(PyTypeObject *tp)
{
    size_t presize = _PyType_PreHeaderSize(tp);
    size_t size = _PyObject_SIZE(tp);
    if (_PyType_HasFeature(tp, Py_TPFLAGS_INLINE_VALUES)) {
        size += _PyInlineValuesSize(tp);
    }
    PyObject *op = gc_alloc(tp, size, presize);
    if (op == NULL) {
        return NULL;
    }
    _PyObject_Init(op, tp);
    if (tp->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        _PyObject_InitInlineValues(op, tp);
    }
    return op;
}

PyVarObject *
_PyObject_GC_NewVar(PyTypeObject *tp, Py_ssize_t nitems)
{
    PyVarObject *op;

    if (nitems < 0) {
        PyErr_BadInternalCall();
        return NULL;
    }
    size_t presize = _PyType_PreHeaderSize(tp);
    size_t size = _PyObject_VAR_SIZE(tp, nitems);
    op = (PyVarObject *)gc_alloc(tp, size, presize);
    if (op == NULL) {
        return NULL;
    }
    _PyObject_InitVar(op, tp, nitems);
    return op;
}

PyObject *
PyUnstable_Object_GC_NewWithExtraData(PyTypeObject *tp, size_t extra_size)
{
    size_t presize = _PyType_PreHeaderSize(tp);
    size_t size = _PyObject_SIZE(tp) + extra_size;
    PyObject *op = gc_alloc(tp, size, presize);
    if (op == NULL) {
        return NULL;
    }
    memset((char *)op + sizeof(PyObject), 0, size - sizeof(PyObject));
    _PyObject_Init(op, tp);
    return op;
}

PyVarObject *
_PyObject_GC_Resize(PyVarObject *op, Py_ssize_t nitems)
{
    const size_t basicsize = _PyObject_VAR_SIZE(Py_TYPE(op), nitems);
    const size_t presize = _PyType_PreHeaderSize(Py_TYPE(op));
    _PyObject_ASSERT((PyObject *)op, !_PyObject_GC_IS_TRACKED(op));
    if (basicsize > (size_t)PY_SSIZE_T_MAX - presize) {
        return (PyVarObject *)PyErr_NoMemory();
    }
    char *mem = (char *)op - presize;
    mem = (char *)_PyObject_ReallocWithType(Py_TYPE(op), mem, presize + basicsize);
    if (mem == NULL) {
        return (PyVarObject *)PyErr_NoMemory();
    }
    op = (PyVarObject *) (mem + presize);
    Py_SET_SIZE(op, nitems);
    return op;
}

void
PyObject_GC_Del(void *op)
{
    size_t presize = _PyType_PreHeaderSize(Py_TYPE(op));
    PyGC_Head *g = AS_GC(op);
    if (_PyObject_GC_IS_TRACKED(op)) {
        gc_list_remove(g);
        GCState *gcstate = get_gc_state();
        if (gcstate->generations[0].count > 0) {
            gcstate->generations[0].count--;
        }
#ifdef Py_DEBUG
        PyObject *exc = PyErr_GetRaisedException();
        if (PyErr_WarnExplicitFormat(PyExc_ResourceWarning, "gc", 0,
                                     "gc", NULL,
                                     "Object of type %s is not untracked "
                                     "before destruction",
                                     Py_TYPE(op)->tp_name))
        {
            PyErr_FormatUnraisable("Exception ignored on object deallocation");
        }
        PyErr_SetRaisedException(exc);
#endif
    }
    PyObject_Free(((char *)op)-presize);
}

int
PyObject_GC_IsTracked(PyObject *obj)
{
    if (_PyObject_IS_GC(obj) && _PyObject_GC_IS_TRACKED(obj)) {
        return 1;
    }
    return 0;
}

int
PyObject_GC_IsFinalized(PyObject *obj)
{
    if (_PyObject_IS_GC(obj) && _PyGC_FINALIZED(obj)) {
         return 1;
    }
    return 0;
}

void
PyUnstable_GC_VisitObjects(gcvisitobjects_t callback, void *arg)
{
    GCState *gcstate = get_gc_state();
    int original_state = gcstate->enabled;
    gcstate->enabled = 0;
    for (size_t i = 0; i < NUM_GENERATIONS; i++) {
        if (visit_generation(callback, arg, &gcstate->generations[i]) < 0) {
            goto done;
        }
    }
    visit_generation(callback, arg, &gcstate->permanent_generation);
done:
    gcstate->enabled = original_state;
}

#undef NEXT_MASK_UNREACHABLE
#undef GEN_HEAD

#endif  // !Py_GIL_DISABLED
