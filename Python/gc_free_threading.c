// Cyclic garbage collector implementation for free-threaded build.
#include "Python.h"
#include "pycore_brc.h"           // struct _brc_thread_state
#include "pycore_ceval.h"         // _Py_set_eval_breaker_bit()
#include "pycore_context.h"
#include "pycore_dict.h"          // _PyDict_MaybeUntrack()
#include "pycore_initconfig.h"
#include "pycore_interp.h"        // PyInterpreterState.gc
#include "pycore_object.h"
#include "pycore_object_alloc.h"  // _PyObject_MallocWithType()
#include "pycore_object_stack.h"
#include "pycore_pyerrors.h"
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_time.h"          // _PyTime_GetPerfCounter()
#include "pycore_tstate.h"        // _PyThreadStateImpl
#include "pycore_weakref.h"       // _PyWeakref_ClearRef()
#include "pydtrace.h"

#ifdef Py_GIL_DISABLED

typedef struct _gc_runtime_state GCState;

#ifdef Py_DEBUG
#  define GC_DEBUG
#endif

// Each thread buffers the count of allocated objects in a thread-local
// variable up to +/- this amount to reduce the overhead of updating
// the global count.
#define LOCAL_ALLOC_COUNT_THRESHOLD 512

// Automatically choose the generation that needs collecting.
#define GENERATION_AUTO (-1)

// A linked list of objects using the `ob_tid` field as the next pointer.
// The linked list pointers are distinct from any real thread ids, because the
// thread ids returned by _Py_ThreadId() are also pointers to distinct objects.
// No thread will confuse its own id with a linked list pointer.
struct worklist {
    uintptr_t head;
};

struct worklist_iter {
    uintptr_t *ptr;   // pointer to current object
    uintptr_t *next;  // next value of ptr
};

struct visitor_args {
    size_t offset;  // offset of PyObject from start of block
};

// Per-collection state
struct collection_state {
    struct visitor_args base;
    PyInterpreterState *interp;
    GCState *gcstate;
    Py_ssize_t collected;
    Py_ssize_t uncollectable;
    Py_ssize_t long_lived_total;
    struct worklist unreachable;
    struct worklist legacy_finalizers;
    struct worklist wrcb_to_call;
    struct worklist objs_to_decref;
};

// iterate over a worklist
#define WORKSTACK_FOR_EACH(stack, op) \
    for ((op) = (PyObject *)(stack)->head; (op) != NULL; (op) = (PyObject *)(op)->ob_tid)

// iterate over a worklist with support for removing the current object
#define WORKSTACK_FOR_EACH_ITER(stack, iter, op) \
    for (worklist_iter_init((iter), &(stack)->head), (op) = (PyObject *)(*(iter)->ptr); \
         (op) != NULL; \
         worklist_iter_init((iter), (iter)->next), (op) = (PyObject *)(*(iter)->ptr))

static void
worklist_push(struct worklist *worklist, PyObject *op)
{
    assert(op->ob_tid == 0);
    op->ob_tid = worklist->head;
    worklist->head = (uintptr_t)op;
}

static PyObject *
worklist_pop(struct worklist *worklist)
{
    PyObject *op = (PyObject *)worklist->head;
    if (op != NULL) {
        worklist->head = op->ob_tid;
        op->ob_tid = 0;
    }
    return op;
}

static void
worklist_iter_init(struct worklist_iter *iter, uintptr_t *next)
{
    iter->ptr = next;
    PyObject *op = (PyObject *)*(iter->ptr);
    if (op) {
        iter->next = &op->ob_tid;
    }
}

static void
worklist_remove(struct worklist_iter *iter)
{
    PyObject *op = (PyObject *)*(iter->ptr);
    *(iter->ptr) = op->ob_tid;
    op->ob_tid = 0;
    iter->next = iter->ptr;
}

static inline int
gc_is_unreachable(PyObject *op)
{
    return (op->ob_gc_bits & _PyGC_BITS_UNREACHABLE) != 0;
}

static void
gc_set_unreachable(PyObject *op)
{
    op->ob_gc_bits |= _PyGC_BITS_UNREACHABLE;
}

static void
gc_clear_unreachable(PyObject *op)
{
    op->ob_gc_bits &= ~_PyGC_BITS_UNREACHABLE;
}

// Initialize the `ob_tid` field to zero if the object is not already
// initialized as unreachable.
static void
gc_maybe_init_refs(PyObject *op)
{
    if (!gc_is_unreachable(op)) {
        gc_set_unreachable(op);
        op->ob_tid = 0;
    }
}

static inline Py_ssize_t
gc_get_refs(PyObject *op)
{
    return (Py_ssize_t)op->ob_tid;
}

static inline void
gc_add_refs(PyObject *op, Py_ssize_t refs)
{
    assert(_PyObject_GC_IS_TRACKED(op));
    op->ob_tid += refs;
}

static inline void
gc_decref(PyObject *op)
{
    op->ob_tid -= 1;
}

static Py_ssize_t
merge_refcount(PyObject *op, Py_ssize_t extra)
{
    assert(_PyInterpreterState_GET()->stoptheworld.world_stopped);

    Py_ssize_t refcount = Py_REFCNT(op);
    refcount += extra;

#ifdef Py_REF_DEBUG
    _Py_AddRefTotal(_PyInterpreterState_GET(), extra);
#endif

    // No atomics necessary; all other threads in this interpreter are paused.
    op->ob_tid = 0;
    op->ob_ref_local = 0;
    op->ob_ref_shared = _Py_REF_SHARED(refcount, _Py_REF_MERGED);
    return refcount;
}

static void
gc_restore_tid(PyObject *op)
{
    mi_segment_t *segment = _mi_ptr_segment(op);
    if (_Py_REF_IS_MERGED(op->ob_ref_shared)) {
        op->ob_tid = 0;
    }
    else {
        // NOTE: may change ob_tid if the object was re-initialized by
        // a different thread or its segment was abandoned and reclaimed.
        // The segment thread id might be zero, in which case we should
        // ensure the refcounts are now merged.
        op->ob_tid = segment->thread_id;
        if (op->ob_tid == 0) {
            merge_refcount(op, 0);
        }
    }
}

static void
gc_restore_refs(PyObject *op)
{
    if (gc_is_unreachable(op)) {
        gc_restore_tid(op);
        gc_clear_unreachable(op);
    }
}

// Given a mimalloc memory block return the PyObject stored in it or NULL if
// the block is not allocated or the object is not tracked or is immortal.
static PyObject *
op_from_block(void *block, void *arg, bool include_frozen)
{
    struct visitor_args *a = arg;
    if (block == NULL) {
        return NULL;
    }
    PyObject *op = (PyObject *)((char*)block + a->offset);
    assert(PyObject_IS_GC(op));
    if (!_PyObject_GC_IS_TRACKED(op)) {
        return NULL;
    }
    if (!include_frozen && (op->ob_gc_bits & _PyGC_BITS_FROZEN) != 0) {
        return NULL;
    }
    return op;
}

static int
gc_visit_heaps_lock_held(PyInterpreterState *interp, mi_block_visit_fun *visitor,
                         struct visitor_args *arg)
{
    // Offset of PyObject header from start of memory block.
    Py_ssize_t offset_base = 0;
    if (_PyMem_DebugEnabled()) {
        // The debug allocator adds two words at the beginning of each block.
        offset_base += 2 * sizeof(size_t);
    }

    // Objects with Py_TPFLAGS_PREHEADER have two extra fields
    Py_ssize_t offset_pre = offset_base + 2 * sizeof(PyObject*);

    // visit each thread's heaps for GC objects
    for (PyThreadState *p = interp->threads.head; p != NULL; p = p->next) {
        struct _mimalloc_thread_state *m = &((_PyThreadStateImpl *)p)->mimalloc;

        arg->offset = offset_base;
        if (!mi_heap_visit_blocks(&m->heaps[_Py_MIMALLOC_HEAP_GC], true,
                                  visitor, arg)) {
            return -1;
        }
        arg->offset = offset_pre;
        if (!mi_heap_visit_blocks(&m->heaps[_Py_MIMALLOC_HEAP_GC_PRE], true,
                                  visitor, arg)) {
            return -1;
        }
    }

    // visit blocks in the per-interpreter abandoned pool (from dead threads)
    mi_abandoned_pool_t *pool = &interp->mimalloc.abandoned_pool;
    arg->offset = offset_base;
    if (!_mi_abandoned_pool_visit_blocks(pool, _Py_MIMALLOC_HEAP_GC, true,
                                         visitor, arg)) {
        return -1;
    }
    arg->offset = offset_pre;
    if (!_mi_abandoned_pool_visit_blocks(pool, _Py_MIMALLOC_HEAP_GC_PRE, true,
                                         visitor, arg)) {
        return -1;
    }
    return 0;
}

// Visits all GC objects in the interpreter's heaps.
// NOTE: It is not safe to allocate or free any mimalloc managed memory while
// this function is running.
static int
gc_visit_heaps(PyInterpreterState *interp, mi_block_visit_fun *visitor,
               struct visitor_args *arg)
{
    // Other threads in the interpreter must be paused so that we can safely
    // traverse their heaps.
    assert(interp->stoptheworld.world_stopped);

    int err;
    HEAD_LOCK(&_PyRuntime);
    err = gc_visit_heaps_lock_held(interp, visitor, arg);
    HEAD_UNLOCK(&_PyRuntime);
    return err;
}

static void
merge_queued_objects(_PyThreadStateImpl *tstate, struct collection_state *state)
{
    struct _brc_thread_state *brc = &tstate->brc;
    _PyObjectStack_Merge(&brc->local_objects_to_merge, &brc->objects_to_merge);

    PyObject *op;
    while ((op = _PyObjectStack_Pop(&brc->local_objects_to_merge)) != NULL) {
        // Subtract one when merging because the queue had a reference.
        Py_ssize_t refcount = merge_refcount(op, -1);

        if (!_PyObject_GC_IS_TRACKED(op) && refcount == 0) {
            // GC objects with zero refcount are handled subsequently by the
            // GC as if they were cyclic trash, but we have to handle dead
            // non-GC objects here. Add one to the refcount so that we can
            // decref and deallocate the object once we start the world again.
            op->ob_ref_shared += (1 << _Py_REF_SHARED_SHIFT);
#ifdef Py_REF_DEBUG
            _Py_IncRefTotal(_PyInterpreterState_GET());
#endif
            worklist_push(&state->objs_to_decref, op);
        }
    }
}

static void
merge_all_queued_objects(PyInterpreterState *interp, struct collection_state *state)
{
    HEAD_LOCK(&_PyRuntime);
    for (PyThreadState *p = interp->threads.head; p != NULL; p = p->next) {
        merge_queued_objects((_PyThreadStateImpl *)p, state);
    }
    HEAD_UNLOCK(&_PyRuntime);
}

static void
process_delayed_frees(PyInterpreterState *interp)
{
    // In STW status, we can observe the latest write sequence by
    // advancing the write sequence immediately.
    _Py_qsbr_advance(&interp->qsbr);
    _PyThreadStateImpl *current_tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
    _Py_qsbr_quiescent_state(current_tstate->qsbr);
    HEAD_LOCK(&_PyRuntime);
    PyThreadState *tstate = interp->threads.head;
    while (tstate != NULL) {
        _PyMem_ProcessDelayed(tstate);
        tstate = (PyThreadState *)tstate->next;
    }
    HEAD_UNLOCK(&_PyRuntime);
}

// Subtract an incoming reference from the computed "gc_refs" refcount.
static int
visit_decref(PyObject *op, void *arg)
{
    if (_PyObject_GC_IS_TRACKED(op) && !_Py_IsImmortal(op)) {
        // If update_refs hasn't reached this object yet, mark it
        // as (tentatively) unreachable and initialize ob_tid to zero.
        gc_maybe_init_refs(op);
        gc_decref(op);
    }
    return 0;
}

// Compute the number of external references to objects in the heap
// by subtracting internal references from the refcount. The difference is
// computed in the ob_tid field (we restore it later).
static bool
update_refs(const mi_heap_t *heap, const mi_heap_area_t *area,
            void *block, size_t block_size, void *args)
{
    PyObject *op = op_from_block(block, args, false);
    if (op == NULL) {
        return true;
    }

    // Exclude immortal objects from garbage collection
    if (_Py_IsImmortal(op)) {
        op->ob_tid = 0;
        _PyObject_GC_UNTRACK(op);
        gc_clear_unreachable(op);
        return true;
    }

    // Untrack tuples and dicts as necessary in this pass.
    if (PyTuple_CheckExact(op)) {
        _PyTuple_MaybeUntrack(op);
        if (!_PyObject_GC_IS_TRACKED(op)) {
            gc_restore_refs(op);
            return true;
        }
    }
    else if (PyDict_CheckExact(op)) {
        _PyDict_MaybeUntrack(op);
        if (!_PyObject_GC_IS_TRACKED(op)) {
            gc_restore_refs(op);
            return true;
        }
    }

    Py_ssize_t refcount = Py_REFCNT(op);
    _PyObject_ASSERT(op, refcount >= 0);

    // We repurpose ob_tid to compute "gc_refs", the number of external
    // references to the object (i.e., from outside the GC heaps). This means
    // that ob_tid is no longer a valid thread id until it is restored by
    // scan_heap_visitor(). Until then, we cannot use the standard reference
    // counting functions or allow other threads to run Python code.
    gc_maybe_init_refs(op);

    // Add the actual refcount to ob_tid.
    gc_add_refs(op, refcount);

    // Subtract internal references from ob_tid. Objects with ob_tid > 0
    // are directly reachable from outside containers, and so can't be
    // collected.
    Py_TYPE(op)->tp_traverse(op, visit_decref, NULL);
    return true;
}

static int
visit_clear_unreachable(PyObject *op, _PyObjectStack *stack)
{
    if (gc_is_unreachable(op)) {
        _PyObject_ASSERT(op, _PyObject_GC_IS_TRACKED(op));
        gc_clear_unreachable(op);
        return _PyObjectStack_Push(stack, op);
    }
    return 0;
}

// Transitively clear the unreachable bit on all objects reachable from op.
static int
mark_reachable(PyObject *op)
{
    _PyObjectStack stack = { NULL };
    do {
        traverseproc traverse = Py_TYPE(op)->tp_traverse;
        if (traverse(op, (visitproc)&visit_clear_unreachable, &stack) < 0) {
            _PyObjectStack_Clear(&stack);
            return -1;
        }
        op = _PyObjectStack_Pop(&stack);
    } while (op != NULL);
    return 0;
}

#ifdef GC_DEBUG
static bool
validate_gc_objects(const mi_heap_t *heap, const mi_heap_area_t *area,
                    void *block, size_t block_size, void *args)
{
    PyObject *op = op_from_block(block, args, false);
    if (op == NULL) {
        return true;
    }

    _PyObject_ASSERT(op, gc_is_unreachable(op));
    _PyObject_ASSERT_WITH_MSG(op, gc_get_refs(op) >= 0,
                                  "refcount is too small");
    return true;
}
#endif

static bool
mark_heap_visitor(const mi_heap_t *heap, const mi_heap_area_t *area,
                  void *block, size_t block_size, void *args)
{
    PyObject *op = op_from_block(block, args, false);
    if (op == NULL) {
        return true;
    }

    if (gc_is_unreachable(op) && gc_get_refs(op) != 0) {
        // Object is reachable but currently marked as unreachable.
        // Mark it as reachable and traverse its pointers to find
        // any other object that may be directly reachable from it.
        gc_clear_unreachable(op);

        // Transitively mark reachable objects by clearing the unreachable flag.
        if (mark_reachable(op) < 0) {
            return false;
        }
    }

    return true;
}

/* Return true if object has a pre-PEP 442 finalization method. */
static int
has_legacy_finalizer(PyObject *op)
{
    return Py_TYPE(op)->tp_del != NULL;
}

static bool
scan_heap_visitor(const mi_heap_t *heap, const mi_heap_area_t *area,
                  void *block, size_t block_size, void *args)
{
    PyObject *op = op_from_block(block, args, false);
    if (op == NULL) {
        return true;
    }

    struct collection_state *state = (struct collection_state *)args;
    if (gc_is_unreachable(op)) {
        // Merge and add one to the refcount to prevent deallocation while we
        // are holding on to it in a worklist.
        merge_refcount(op, 1);

        if (has_legacy_finalizer(op)) {
            // would be unreachable, but has legacy finalizer
            gc_clear_unreachable(op);
            worklist_push(&state->legacy_finalizers, op);
        }
        else {
            worklist_push(&state->unreachable, op);
        }
    }
    else {
        // object is reachable, restore `ob_tid`; we're done with these objects
        gc_restore_tid(op);
        state->long_lived_total++;
    }

    return true;
}

static int
move_legacy_finalizer_reachable(struct collection_state *state);

static int
deduce_unreachable_heap(PyInterpreterState *interp,
                        struct collection_state *state)
{
    // Identify objects that are directly reachable from outside the GC heap
    // by computing the difference between the refcount and the number of
    // incoming references.
    gc_visit_heaps(interp, &update_refs, &state->base);

#ifdef GC_DEBUG
    // Check that all objects are marked as unreachable and that the computed
    // reference count difference (stored in `ob_tid`) is non-negative.
    gc_visit_heaps(interp, &validate_gc_objects, &state->base);
#endif

    // Transitively mark reachable objects by clearing the
    // _PyGC_BITS_UNREACHABLE flag.
    if (gc_visit_heaps(interp, &mark_heap_visitor, &state->base) < 0) {
        return -1;
    }

    // Identify remaining unreachable objects and push them onto a stack.
    // Restores ob_tid for reachable objects.
    gc_visit_heaps(interp, &scan_heap_visitor, &state->base);

    if (state->legacy_finalizers.head) {
        // There may be objects reachable from legacy finalizers that are in
        // the unreachable set. We need to mark them as reachable.
        if (move_legacy_finalizer_reachable(state) < 0) {
            return -1;
        }
    }

    return 0;
}

static int
move_legacy_finalizer_reachable(struct collection_state *state)
{
    // Clear the reachable bit on all objects transitively reachable
    // from the objects with legacy finalizers.
    PyObject *op;
    WORKSTACK_FOR_EACH(&state->legacy_finalizers, op) {
        if (mark_reachable(op) < 0) {
            return -1;
        }
    }

    // Move the reachable objects from the unreachable worklist to the legacy
    // finalizer worklist.
    struct worklist_iter iter;
    WORKSTACK_FOR_EACH_ITER(&state->unreachable, &iter, op) {
        if (!gc_is_unreachable(op)) {
            worklist_remove(&iter);
            worklist_push(&state->legacy_finalizers, op);
        }
    }

    return 0;
}

// Clear all weakrefs to unreachable objects. Weakrefs with callbacks are
// enqueued in `wrcb_to_call`, but not invoked yet.
static void
clear_weakrefs(struct collection_state *state)
{
    PyObject *op;
    WORKSTACK_FOR_EACH(&state->unreachable, op) {
        if (PyWeakref_Check(op)) {
            // Clear weakrefs that are themselves unreachable to ensure their
            // callbacks will not be executed later from a `tp_clear()`
            // inside delete_garbage(). That would be unsafe: it could
            // resurrect a dead object or access a an already cleared object.
            // See bpo-38006 for one example.
            _PyWeakref_ClearRef((PyWeakReference *)op);
        }

        if (!_PyType_SUPPORTS_WEAKREFS(Py_TYPE(op))) {
            continue;
        }

        // NOTE: This is never triggered for static types so we can avoid the
        // (slightly) more costly _PyObject_GET_WEAKREFS_LISTPTR().
        PyWeakReference **wrlist = _PyObject_GET_WEAKREFS_LISTPTR_FROM_OFFSET(op);

        // `op` may have some weakrefs.  March over the list, clear
        // all the weakrefs, and enqueue the weakrefs with callbacks
        // that must be called into wrcb_to_call.
        for (PyWeakReference *wr = *wrlist; wr != NULL; wr = *wrlist) {
            // _PyWeakref_ClearRef clears the weakref but leaves
            // the callback pointer intact.  Obscure: it also
            // changes *wrlist.
            _PyObject_ASSERT((PyObject *)wr, wr->wr_object == op);
            _PyWeakref_ClearRef(wr);
            _PyObject_ASSERT((PyObject *)wr, wr->wr_object == Py_None);

            // We do not invoke callbacks for weakrefs that are themselves
            // unreachable. This is partly for historical reasons: weakrefs
            // predate safe object finalization, and a weakref that is itself
            // unreachable may have a callback that resurrects other
            // unreachable objects.
            if (wr->wr_callback == NULL || gc_is_unreachable((PyObject *)wr)) {
                continue;
            }

            // Create a new reference so that wr can't go away before we can
            // process it again.
            merge_refcount((PyObject *)wr, 1);

            // Enqueue weakref to be called later.
            worklist_push(&state->wrcb_to_call, (PyObject *)wr);
        }
    }
}

static void
call_weakref_callbacks(struct collection_state *state)
{
    // Invoke the callbacks we decided to honor.
    PyObject *op;
    while ((op = worklist_pop(&state->wrcb_to_call)) != NULL) {
        _PyObject_ASSERT(op, PyWeakref_Check(op));

        PyWeakReference *wr = (PyWeakReference *)op;
        PyObject *callback = wr->wr_callback;
        _PyObject_ASSERT(op, callback != NULL);

        /* copy-paste of weakrefobject.c's handle_callback() */
        PyObject *temp = PyObject_CallOneArg(callback, (PyObject *)wr);
        if (temp == NULL) {
            PyErr_WriteUnraisable(callback);
        }
        else {
            Py_DECREF(temp);
        }

        gc_restore_tid(op);
        Py_DECREF(op);  // drop worklist reference
    }
}


static GCState *
get_gc_state(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return &interp->gc;
}


void
_PyGC_InitState(GCState *gcstate)
{
    // TODO: move to pycore_runtime_init.h once the incremental GC lands.
    gcstate->generations[0].threshold = 2000;
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

static void
debug_cycle(const char *msg, PyObject *op)
{
    PySys_FormatStderr("gc: %s <%s %p>\n",
                       msg, Py_TYPE(op)->tp_name, op);
}

/* Run first-time finalizers (if any) on all the objects in collectable.
 * Note that this may remove some (or even all) of the objects from the
 * list, due to refcounts falling to 0.
 */
static void
finalize_garbage(struct collection_state *state)
{
    // NOTE: the unreachable worklist holds a strong reference to the object
    // to prevent it from being deallocated while we are holding on to it.
    PyObject *op;
    WORKSTACK_FOR_EACH(&state->unreachable, op) {
        if (!_PyGC_FINALIZED(op)) {
            destructor finalize = Py_TYPE(op)->tp_finalize;
            if (finalize != NULL) {
                _PyGC_SET_FINALIZED(op);
                finalize(op);
                assert(!_PyErr_Occurred(_PyThreadState_GET()));
            }
        }
    }
}

// Break reference cycles by clearing the containers involved.
static void
delete_garbage(struct collection_state *state)
{
    PyThreadState *tstate = _PyThreadState_GET();
    GCState *gcstate = state->gcstate;

    assert(!_PyErr_Occurred(tstate));

    PyObject *op;
    while ((op = worklist_pop(&state->objs_to_decref)) != NULL) {
        Py_DECREF(op);
    }

    while ((op = worklist_pop(&state->unreachable)) != NULL) {
        _PyObject_ASSERT(op, gc_is_unreachable(op));

        // Clear the unreachable flag.
        gc_clear_unreachable(op);

        if (!_PyObject_GC_IS_TRACKED(op)) {
            // Object might have been untracked by some other tp_clear() call.
            Py_DECREF(op);  // drop the reference from the worklist
            continue;
        }

        state->collected++;

        if (gcstate->debug & _PyGC_DEBUG_SAVEALL) {
            assert(gcstate->garbage != NULL);
            if (PyList_Append(gcstate->garbage, op) < 0) {
                _PyErr_Clear(tstate);
            }
        }
        else {
            inquiry clear = Py_TYPE(op)->tp_clear;
            if (clear != NULL) {
                (void) clear(op);
                if (_PyErr_Occurred(tstate)) {
                    PyErr_FormatUnraisable("Exception ignored in tp_clear of %s",
                                           Py_TYPE(op)->tp_name);
                }
            }
        }

        Py_DECREF(op);  // drop the reference from the worklist
    }
}

static void
handle_legacy_finalizers(struct collection_state *state)
{
    GCState *gcstate = state->gcstate;
    assert(gcstate->garbage != NULL);

    PyObject *op;
    while ((op = worklist_pop(&state->legacy_finalizers)) != NULL) {
        state->uncollectable++;

        if (gcstate->debug & _PyGC_DEBUG_UNCOLLECTABLE) {
            debug_cycle("uncollectable", op);
        }

        if ((gcstate->debug & _PyGC_DEBUG_SAVEALL) || has_legacy_finalizer(op)) {
            if (PyList_Append(gcstate->garbage, op) < 0) {
                PyErr_Clear();
            }
        }
        Py_DECREF(op);  // drop worklist reference
    }
}

// Show stats for objects in each generations
static void
show_stats_each_generations(GCState *gcstate)
{
    // TODO
}

// Traversal callback for handle_resurrected_objects.
static int
visit_decref_unreachable(PyObject *op, void *data)
{
    if (gc_is_unreachable(op) && _PyObject_GC_IS_TRACKED(op)) {
        op->ob_ref_local -= 1;
    }
    return 0;
}

// Handle objects that may have resurrected after a call to 'finalize_garbage'.
static int
handle_resurrected_objects(struct collection_state *state)
{
    // First, find externally reachable objects by computing the reference
    // count difference in ob_ref_local. We can't use ob_tid here because
    // that's already used to store the unreachable worklist.
    PyObject *op;
    struct worklist_iter iter;
    WORKSTACK_FOR_EACH_ITER(&state->unreachable, &iter, op) {
        assert(gc_is_unreachable(op));
        assert(_Py_REF_IS_MERGED(op->ob_ref_shared));

        if (!_PyObject_GC_IS_TRACKED(op)) {
            // Object was untracked by a finalizer. Schedule it for a Py_DECREF
            // after we finish with the stop-the-world pause.
            gc_clear_unreachable(op);
            worklist_remove(&iter);
            worklist_push(&state->objs_to_decref, op);
            continue;
        }

        Py_ssize_t refcount = (op->ob_ref_shared >> _Py_REF_SHARED_SHIFT);
        if (refcount > INT32_MAX) {
            // The refcount is too big to fit in `ob_ref_local`. Mark the
            // object as immortal and bail out.
            gc_clear_unreachable(op);
            worklist_remove(&iter);
            _Py_SetImmortal(op);
            continue;
        }

        op->ob_ref_local += (uint32_t)refcount;

        // Subtract one to account for the reference from the worklist.
        op->ob_ref_local -= 1;

        traverseproc traverse = Py_TYPE(op)->tp_traverse;
        (void) traverse(op,
            (visitproc)visit_decref_unreachable,
            NULL);
    }

    // Find resurrected objects
    bool any_resurrected = false;
    WORKSTACK_FOR_EACH(&state->unreachable, op) {
        int32_t gc_refs = (int32_t)op->ob_ref_local;
        op->ob_ref_local = 0;  // restore ob_ref_local

        _PyObject_ASSERT(op, gc_refs >= 0);

        if (gc_is_unreachable(op) && gc_refs > 0) {
            // Clear the unreachable flag on any transitively reachable objects
            // from this one.
            any_resurrected = true;
            gc_clear_unreachable(op);
            if (mark_reachable(op) < 0) {
                return -1;
            }
        }
    }

    if (any_resurrected) {
        // Remove resurrected objects from the unreachable list.
        WORKSTACK_FOR_EACH_ITER(&state->unreachable, &iter, op) {
            if (!gc_is_unreachable(op)) {
                _PyObject_ASSERT(op, Py_REFCNT(op) > 1);
                worklist_remove(&iter);
                merge_refcount(op, -1);  // remove worklist reference
            }
        }
    }

#ifdef GC_DEBUG
    WORKSTACK_FOR_EACH(&state->unreachable, op) {
        _PyObject_ASSERT(op, gc_is_unreachable(op));
        _PyObject_ASSERT(op, _PyObject_GC_IS_TRACKED(op));
        _PyObject_ASSERT(op, op->ob_ref_local == 0);
        _PyObject_ASSERT(op, _Py_REF_IS_MERGED(op->ob_ref_shared));
    }
#endif

    return 0;
}


/* Invoke progress callbacks to notify clients that garbage collection
 * is starting or stopping
 */
static void
invoke_gc_callback(PyThreadState *tstate, const char *phase,
                   int generation, Py_ssize_t collected,
                   Py_ssize_t uncollectable)
{
    assert(!_PyErr_Occurred(tstate));

    /* we may get called very early */
    GCState *gcstate = &tstate->interp->gc;
    if (gcstate->callbacks == NULL) {
        return;
    }

    /* The local variable cannot be rebound, check it for sanity */
    assert(PyList_CheckExact(gcstate->callbacks));
    PyObject *info = NULL;
    if (PyList_GET_SIZE(gcstate->callbacks) != 0) {
        info = Py_BuildValue("{sisnsn}",
            "generation", generation,
            "collected", collected,
            "uncollectable", uncollectable);
        if (info == NULL) {
            PyErr_FormatUnraisable("Exception ignored on invoking gc callbacks");
            return;
        }
    }

    PyObject *phase_obj = PyUnicode_FromString(phase);
    if (phase_obj == NULL) {
        Py_XDECREF(info);
        PyErr_FormatUnraisable("Exception ignored on invoking gc callbacks");
        return;
    }

    PyObject *stack[] = {phase_obj, info};
    for (Py_ssize_t i=0; i<PyList_GET_SIZE(gcstate->callbacks); i++) {
        PyObject *r, *cb = PyList_GET_ITEM(gcstate->callbacks, i);
        Py_INCREF(cb); /* make sure cb doesn't go away */
        r = PyObject_Vectorcall(cb, stack, 2, NULL);
        if (r == NULL) {
            PyErr_WriteUnraisable(cb);
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

static void
cleanup_worklist(struct worklist *worklist)
{
    PyObject *op;
    while ((op = worklist_pop(worklist)) != NULL) {
        gc_restore_tid(op);
        gc_clear_unreachable(op);
        Py_DECREF(op);
    }
}

static bool
gc_should_collect(GCState *gcstate)
{
    int count = _Py_atomic_load_int_relaxed(&gcstate->generations[0].count);
    int threshold = gcstate->generations[0].threshold;
    if (count <= threshold || threshold == 0 || !gcstate->enabled) {
        return false;
    }
    // Avoid quadratic behavior by scaling threshold to the number of live
    // objects. A few tests rely on immediate scheduling of the GC so we ignore
    // the scaled threshold if generations[1].threshold is set to zero.
    return (count > gcstate->long_lived_total / 4 ||
            gcstate->generations[1].threshold == 0);
}

static void
record_allocation(PyThreadState *tstate)
{
    struct _gc_thread_state *gc = &((_PyThreadStateImpl *)tstate)->gc;

    // We buffer the allocation count to avoid the overhead of atomic
    // operations for every allocation.
    gc->alloc_count++;
    if (gc->alloc_count >= LOCAL_ALLOC_COUNT_THRESHOLD) {
        // TODO: Use Py_ssize_t for the generation count.
        GCState *gcstate = &tstate->interp->gc;
        _Py_atomic_add_int(&gcstate->generations[0].count, (int)gc->alloc_count);
        gc->alloc_count = 0;

        if (gc_should_collect(gcstate) &&
            !_Py_atomic_load_int_relaxed(&gcstate->collecting))
        {
            _Py_ScheduleGC(tstate);
        }
    }
}

static void
record_deallocation(PyThreadState *tstate)
{
    struct _gc_thread_state *gc = &((_PyThreadStateImpl *)tstate)->gc;

    gc->alloc_count--;
    if (gc->alloc_count <= -LOCAL_ALLOC_COUNT_THRESHOLD) {
        GCState *gcstate = &tstate->interp->gc;
        _Py_atomic_add_int(&gcstate->generations[0].count, (int)gc->alloc_count);
        gc->alloc_count = 0;
    }
}

static void
gc_collect_internal(PyInterpreterState *interp, struct collection_state *state)
{
    _PyEval_StopTheWorld(interp);
    // merge refcounts for all queued objects
    merge_all_queued_objects(interp, state);
    process_delayed_frees(interp);

    // Find unreachable objects
    int err = deduce_unreachable_heap(interp, state);
    if (err < 0) {
        _PyEval_StartTheWorld(interp);
        goto error;
    }

    // Print debugging information.
    if (interp->gc.debug & _PyGC_DEBUG_COLLECTABLE) {
        PyObject *op;
        WORKSTACK_FOR_EACH(&state->unreachable, op) {
            debug_cycle("collectable", op);
        }
    }

    // Record the number of live GC objects
    interp->gc.long_lived_total = state->long_lived_total;

    // Clear weakrefs and enqueue callbacks (but do not call them).
    clear_weakrefs(state);
    _PyEval_StartTheWorld(interp);

    // Deallocate any object from the refcount merge step
    cleanup_worklist(&state->objs_to_decref);

    // Call weakref callbacks and finalizers after unpausing other threads to
    // avoid potential deadlocks.
    call_weakref_callbacks(state);
    finalize_garbage(state);

    // Handle any objects that may have resurrected after the finalization.
    _PyEval_StopTheWorld(interp);
    err = handle_resurrected_objects(state);
    // Clear free lists in all threads
    _PyGC_ClearAllFreeLists(interp);
    _PyEval_StartTheWorld(interp);

    if (err < 0) {
        goto error;
    }

    // Call tp_clear on objects in the unreachable set. This will cause
    // the reference cycles to be broken. It may also cause some objects
    // to be freed.
    delete_garbage(state);

    // Append objects with legacy finalizers to the "gc.garbage" list.
    handle_legacy_finalizers(state);
    return;

error:
    cleanup_worklist(&state->unreachable);
    cleanup_worklist(&state->legacy_finalizers);
    cleanup_worklist(&state->wrcb_to_call);
    cleanup_worklist(&state->objs_to_decref);
    PyErr_NoMemory();
    PyErr_FormatUnraisable("Out of memory during garbage collection");
}

/* This is the main function.  Read this to understand how the
 * collection process works. */
static Py_ssize_t
gc_collect_main(PyThreadState *tstate, int generation, _PyGC_Reason reason)
{
    int i;
    Py_ssize_t m = 0; /* # objects collected */
    Py_ssize_t n = 0; /* # unreachable objects that couldn't be collected */
    PyTime_t t1 = 0;   /* initialize to prevent a compiler warning */
    GCState *gcstate = &tstate->interp->gc;

    // gc_collect_main() must not be called before _PyGC_Init
    // or after _PyGC_Fini()
    assert(gcstate->garbage != NULL);
    assert(!_PyErr_Occurred(tstate));

    int expected = 0;
    if (!_Py_atomic_compare_exchange_int(&gcstate->collecting, &expected, 1)) {
        // Don't start a garbage collection if one is already in progress.
        return 0;
    }

    if (reason == _Py_GC_REASON_HEAP && !gc_should_collect(gcstate)) {
        // Don't collect if the threshold is not exceeded.
        _Py_atomic_store_int(&gcstate->collecting, 0);
        return 0;
    }

    assert(generation >= 0 && generation < NUM_GENERATIONS);

#ifdef Py_STATS
    if (_Py_stats) {
        _Py_stats->object_stats.object_visits = 0;
    }
#endif
    GC_STAT_ADD(generation, collections, 1);

    if (reason != _Py_GC_REASON_SHUTDOWN) {
        invoke_gc_callback(tstate, "start", generation, 0, 0);
    }

    if (gcstate->debug & _PyGC_DEBUG_STATS) {
        PySys_WriteStderr("gc: collecting generation %d...\n", generation);
        show_stats_each_generations(gcstate);
        t1 = _PyTime_PerfCounterUnchecked();
    }

    if (PyDTrace_GC_START_ENABLED()) {
        PyDTrace_GC_START(generation);
    }

    /* update collection and allocation counters */
    if (generation+1 < NUM_GENERATIONS) {
        gcstate->generations[generation+1].count += 1;
    }
    for (i = 0; i <= generation; i++) {
        gcstate->generations[i].count = 0;
    }

    PyInterpreterState *interp = tstate->interp;

    struct collection_state state = {
        .interp = interp,
        .gcstate = gcstate,
    };

    gc_collect_internal(interp, &state);

    m = state.collected;
    n = state.uncollectable;

    if (gcstate->debug & _PyGC_DEBUG_STATS) {
        double d = PyTime_AsSecondsDouble(_PyTime_PerfCounterUnchecked() - t1);
        PySys_WriteStderr(
            "gc: done, %zd unreachable, %zd uncollectable, %.4fs elapsed\n",
            n+m, n, d);
    }

    // Clear the current thread's free-list again.
    _PyThreadStateImpl *tstate_impl = (_PyThreadStateImpl *)tstate;
    _PyObject_ClearFreeLists(&tstate_impl->freelists, 0);

    if (_PyErr_Occurred(tstate)) {
        if (reason == _Py_GC_REASON_SHUTDOWN) {
            _PyErr_Clear(tstate);
        }
        else {
            PyErr_FormatUnraisable("Exception ignored in garbage collection");
        }
    }

    /* Update stats */
    struct gc_generation_stats *stats = &gcstate->generation_stats[generation];
    stats->collections++;
    stats->collected += m;
    stats->uncollectable += n;

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
        invoke_gc_callback(tstate, "stop", generation, m, n);
    }

    assert(!_PyErr_Occurred(tstate));
    _Py_atomic_store_int(&gcstate->collecting, 0);
    return n + m;
}

struct get_referrers_args {
    struct visitor_args base;
    PyObject *objs;
    struct worklist results;
};

static int
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

static bool
visit_get_referrers(const mi_heap_t *heap, const mi_heap_area_t *area,
                    void *block, size_t block_size, void *args)
{
    PyObject *op = op_from_block(block, args, true);
    if (op == NULL) {
        return true;
    }

    struct get_referrers_args *arg = (struct get_referrers_args *)args;
    if (Py_TYPE(op)->tp_traverse(op, referrersvisit, arg->objs)) {
        op->ob_tid = 0;  // we will restore the refcount later
        worklist_push(&arg->results, op);
    }

    return true;
}

PyObject *
_PyGC_GetReferrers(PyInterpreterState *interp, PyObject *objs)
{
    PyObject *result = PyList_New(0);
    if (!result) {
        return NULL;
    }

    _PyEval_StopTheWorld(interp);

    // Append all objects to a worklist. This abuses ob_tid. We will restore
    // it later. NOTE: We can't append to the PyListObject during
    // gc_visit_heaps() because PyList_Append() may reclaim an abandoned
    // mimalloc segments while we are traversing them.
    struct get_referrers_args args = { .objs = objs };
    gc_visit_heaps(interp, &visit_get_referrers, &args.base);

    bool error = false;
    PyObject *op;
    while ((op = worklist_pop(&args.results)) != NULL) {
        gc_restore_tid(op);
        if (op != objs && PyList_Append(result, op) < 0) {
            error = true;
            break;
        }
    }

    // In case of error, clear the remaining worklist
    while ((op = worklist_pop(&args.results)) != NULL) {
        gc_restore_tid(op);
    }

    _PyEval_StartTheWorld(interp);

    if (error) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

struct get_objects_args {
    struct visitor_args base;
    struct worklist objects;
};

static bool
visit_get_objects(const mi_heap_t *heap, const mi_heap_area_t *area,
                  void *block, size_t block_size, void *args)
{
    PyObject *op = op_from_block(block, args, true);
    if (op == NULL) {
        return true;
    }

    struct get_objects_args *arg = (struct get_objects_args *)args;
    op->ob_tid = 0;  // we will restore the refcount later
    worklist_push(&arg->objects, op);

    return true;
}

PyObject *
_PyGC_GetObjects(PyInterpreterState *interp, Py_ssize_t generation)
{
    PyObject *result = PyList_New(0);
    if (!result) {
        return NULL;
    }

    _PyEval_StopTheWorld(interp);

    // Append all objects to a worklist. This abuses ob_tid. We will restore
    // it later. NOTE: We can't append to the list during gc_visit_heaps()
    // because PyList_Append() may reclaim an abandoned mimalloc segment
    // while we are traversing it.
    struct get_objects_args args = { 0 };
    gc_visit_heaps(interp, &visit_get_objects, &args.base);

    bool error = false;
    PyObject *op;
    while ((op = worklist_pop(&args.objects)) != NULL) {
        gc_restore_tid(op);
        if (op != result && PyList_Append(result, op) < 0) {
            error = true;
            break;
        }
    }

    // In case of error, clear the remaining worklist
    while ((op = worklist_pop(&args.objects)) != NULL) {
        gc_restore_tid(op);
    }

    _PyEval_StartTheWorld(interp);

    if (error) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

static bool
visit_freeze(const mi_heap_t *heap, const mi_heap_area_t *area,
             void *block, size_t block_size, void *args)
{
    PyObject *op = op_from_block(block, args, true);
    if (op != NULL) {
        op->ob_gc_bits |= _PyGC_BITS_FROZEN;
    }
    return true;
}

void
_PyGC_Freeze(PyInterpreterState *interp)
{
    struct visitor_args args;
    _PyEval_StopTheWorld(interp);
    gc_visit_heaps(interp, &visit_freeze, &args);
    _PyEval_StartTheWorld(interp);
}

static bool
visit_unfreeze(const mi_heap_t *heap, const mi_heap_area_t *area,
               void *block, size_t block_size, void *args)
{
    PyObject *op = op_from_block(block, args, true);
    if (op != NULL) {
        op->ob_gc_bits &= ~_PyGC_BITS_FROZEN;
    }
    return true;
}

void
_PyGC_Unfreeze(PyInterpreterState *interp)
{
    struct visitor_args args;
    _PyEval_StopTheWorld(interp);
    gc_visit_heaps(interp, &visit_unfreeze, &args);
    _PyEval_StartTheWorld(interp);
}

struct count_frozen_args {
    struct visitor_args base;
    Py_ssize_t count;
};

static bool
visit_count_frozen(const mi_heap_t *heap, const mi_heap_area_t *area,
                   void *block, size_t block_size, void *args)
{
    PyObject *op = op_from_block(block, args, true);
    if (op != NULL && (op->ob_gc_bits & _PyGC_BITS_FROZEN) != 0) {
        struct count_frozen_args *arg = (struct count_frozen_args *)args;
        arg->count++;
    }
    return true;
}

Py_ssize_t
_PyGC_GetFreezeCount(PyInterpreterState *interp)
{
    struct count_frozen_args args = { .count = 0 };
    _PyEval_StopTheWorld(interp);
    gc_visit_heaps(interp, &visit_count_frozen, &args.base);
    _PyEval_StartTheWorld(interp);
    return args.count;
}

/* C API for controlling the state of the garbage collector */
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

/* Public API to invoke gc.collect() from C */
Py_ssize_t
PyGC_Collect(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    GCState *gcstate = &tstate->interp->gc;

    if (!gcstate->enabled) {
        return 0;
    }

    Py_ssize_t n;
    PyObject *exc = _PyErr_GetRaisedException(tstate);
    n = gc_collect_main(tstate, NUM_GENERATIONS - 1, _Py_GC_REASON_MANUAL);
    _PyErr_SetRaisedException(tstate, exc);

    return n;
}

Py_ssize_t
_PyGC_Collect(PyThreadState *tstate, int generation, _PyGC_Reason reason)
{
    return gc_collect_main(tstate, generation, reason);
}

Py_ssize_t
_PyGC_CollectNoFail(PyThreadState *tstate)
{
    /* Ideally, this function is only called on interpreter shutdown,
       and therefore not recursively.  Unfortunately, when there are daemon
       threads, a daemon thread can start a cyclic garbage collection
       during interpreter shutdown (and then never finish it).
       See http://bugs.python.org/issue8713#msg195178 for an example.
       */
    return gc_collect_main(tstate, NUM_GENERATIONS - 1, _Py_GC_REASON_SHUTDOWN);
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
        /* PyErr_WarnFormat does too many things and we are at shutdown,
           the warnings module's dependencies (e.g. linecache) may be gone
           already. */
        if (PyErr_WarnExplicitFormat(PyExc_ResourceWarning, "gc", 0,
                                     "gc", NULL, message,
                                     PyList_GET_SIZE(gcstate->garbage)))
        {
            PyErr_WriteUnraisable(NULL);
        }
        if (gcstate->debug & _PyGC_DEBUG_UNCOLLECTABLE) {
            PyObject *repr = NULL, *bytes = NULL;
            repr = PyObject_Repr(gcstate->garbage);
            if (!repr || !(bytes = PyUnicode_EncodeFSDefault(repr))) {
                PyErr_WriteUnraisable(gcstate->garbage);
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

    /* We expect that none of this interpreters objects are shared
       with other interpreters.
       See https://github.com/python/cpython/issues/90228. */
}

/* for debugging */

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
    /* Obscure:  the Py_TRASHCAN mechanism requires that we be able to
     * call PyObject_GC_UnTrack twice on an object.
     */
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
_PyObject_GC_Link(PyObject *op)
{
    record_allocation(_PyThreadState_GET());
}

void
_Py_RunGC(PyThreadState *tstate)
{
    gc_collect_main(tstate, 0, _Py_GC_REASON_HEAP);
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
    if (presize) {
        ((PyObject **)mem)[0] = NULL;
        ((PyObject **)mem)[1] = NULL;
    }
    PyObject *op = (PyObject *)(mem + presize);
    record_allocation(tstate);
    return op;
}

PyObject *
_PyObject_GC_New(PyTypeObject *tp)
{
    size_t presize = _PyType_PreHeaderSize(tp);
    PyObject *op = gc_alloc(tp, _PyObject_SIZE(tp), presize);
    if (op == NULL) {
        return NULL;
    }
    _PyObject_Init(op, tp);
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
    PyObject *op = gc_alloc(tp, _PyObject_SIZE(tp) + extra_size, presize);
    if (op == NULL) {
        return NULL;
    }
    memset(op, 0, _PyObject_SIZE(tp) + extra_size);
    _PyObject_Init(op, tp);
    return op;
}

PyVarObject *
_PyObject_GC_Resize(PyVarObject *op, Py_ssize_t nitems)
{
    const size_t basicsize = _PyObject_VAR_SIZE(Py_TYPE(op), nitems);
    const size_t presize = _PyType_PreHeaderSize(((PyObject *)op)->ob_type);
    _PyObject_ASSERT((PyObject *)op, !_PyObject_GC_IS_TRACKED(op));
    if (basicsize > (size_t)PY_SSIZE_T_MAX - presize) {
        return (PyVarObject *)PyErr_NoMemory();
    }
    char *mem = (char *)op - presize;
    mem = (char *)_PyObject_ReallocWithType(Py_TYPE(op), mem,  presize + basicsize);
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
    size_t presize = _PyType_PreHeaderSize(((PyObject *)op)->ob_type);
    if (_PyObject_GC_IS_TRACKED(op)) {
        _PyObject_GC_UNTRACK(op);
#ifdef Py_DEBUG
        PyObject *exc = PyErr_GetRaisedException();
        if (PyErr_WarnExplicitFormat(PyExc_ResourceWarning, "gc", 0,
                                     "gc", NULL, "Object of type %s is not untracked before destruction",
                                     ((PyObject*)op)->ob_type->tp_name)) {
            PyErr_WriteUnraisable(NULL);
        }
        PyErr_SetRaisedException(exc);
#endif
    }

    record_deallocation(_PyThreadState_GET());
    PyObject *self = (PyObject *)op;
    if (_PyObject_GC_IS_SHARED_INLINE(self)) {
        _PyObject_FreeDelayed(((char *)op)-presize);
    }
    else {
        PyObject_Free(((char *)op)-presize);
    }
}

int
PyObject_GC_IsTracked(PyObject* obj)
{
    return _PyObject_GC_IS_TRACKED(obj);
}

int
PyObject_GC_IsFinalized(PyObject *obj)
{
    return _PyGC_FINALIZED(obj);
}

struct custom_visitor_args {
    struct visitor_args base;
    gcvisitobjects_t callback;
    void *arg;
};

static bool
custom_visitor_wrapper(const mi_heap_t *heap, const mi_heap_area_t *area,
                       void *block, size_t block_size, void *args)
{
    PyObject *op = op_from_block(block, args, false);
    if (op == NULL) {
        return true;
    }

    struct custom_visitor_args *wrapper = (struct custom_visitor_args *)args;
    if (!wrapper->callback(op, wrapper->arg)) {
        return false;
    }

    return true;
}

void
PyUnstable_GC_VisitObjects(gcvisitobjects_t callback, void *arg)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    struct custom_visitor_args wrapper = {
        .callback = callback,
        .arg = arg,
    };

    _PyEval_StopTheWorld(interp);
    gc_visit_heaps(interp, &custom_visitor_wrapper, &wrapper.base);
    _PyEval_StartTheWorld(interp);
}

/* Clear all free lists
 * All free lists are cleared during the collection of the highest generation.
 * Allocated items in the free list may keep a pymalloc arena occupied.
 * Clearing the free lists may give back memory to the OS earlier.
 * Free-threading version: Since freelists are managed per thread,
 * GC should clear all freelists by traversing all threads.
 */
void
_PyGC_ClearAllFreeLists(PyInterpreterState *interp)
{
    HEAD_LOCK(&_PyRuntime);
    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)interp->threads.head;
    while (tstate != NULL) {
        _PyObject_ClearFreeLists(&tstate->freelists, 0);
        tstate = (_PyThreadStateImpl *)tstate->base.next;
    }
    HEAD_UNLOCK(&_PyRuntime);
}

#endif  // Py_GIL_DISABLED
