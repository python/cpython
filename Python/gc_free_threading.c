// Cyclic garbage collector implementation for free-threaded build.
#include "Python.h"
#include "pycore_brc.h"           // struct _brc_thread_state
#include "pycore_ceval.h"         // _Py_set_eval_breaker_bit()
#include "pycore_dict.h"          // _PyInlineValuesSize()
#include "pycore_frame.h"         // FRAME_CLEARED
#include "pycore_freelist.h"      // _PyObject_ClearFreeLists()
#include "pycore_genobject.h"     // _PyGen_GetGeneratorFromFrame()
#include "pycore_initconfig.h"    // _PyStatus_NO_MEMORY()
#include "pycore_interp.h"        // PyInterpreterState.gc
#include "pycore_interpframe.h"   // _PyFrame_GetLocalsArray()
#include "pycore_object_alloc.h"  // _PyObject_MallocWithType()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tstate.h"        // _PyThreadStateImpl
#include "pycore_tuple.h"         // _PyTuple_MaybeUntrack()
#include "pycore_weakref.h"       // _PyWeakref_ClearRef()

#include "pydtrace.h"


// enable the "mark alive" pass of GC
#define GC_ENABLE_MARK_ALIVE 1

// if true, enable the use of "prefetch" CPU instructions
#define GC_ENABLE_PREFETCH_INSTRUCTIONS 1

// include additional roots in "mark alive" pass
#define GC_MARK_ALIVE_EXTRA_ROOTS 1

// include Python stacks as set of known roots
#define GC_MARK_ALIVE_STACKS 1


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
    _PyGC_Reason reason;
    // GH-129236: If we see an active frame without a valid stack pointer,
    // we can't collect objects with deferred references because we may not
    // see all references.
    int skip_deferred_objects;
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
        _Py_atomic_store_uintptr_relaxed(&op->ob_tid, 0);
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
gc_has_bit(PyObject *op, uint8_t bit)
{
    return (op->ob_gc_bits & bit) != 0;
}

static inline void
gc_set_bit(PyObject *op, uint8_t bit)
{
    op->ob_gc_bits |= bit;
}

static inline void
gc_clear_bit(PyObject *op, uint8_t bit)
{
    op->ob_gc_bits &= ~bit;
}

static inline int
gc_is_frozen(PyObject *op)
{
    return gc_has_bit(op, _PyGC_BITS_FROZEN);
}

static inline int
gc_is_unreachable(PyObject *op)
{
    return gc_has_bit(op, _PyGC_BITS_UNREACHABLE);
}

static inline void
gc_set_unreachable(PyObject *op)
{
    gc_set_bit(op, _PyGC_BITS_UNREACHABLE);
}

static inline void
gc_clear_unreachable(PyObject *op)
{
    gc_clear_bit(op, _PyGC_BITS_UNREACHABLE);
}

static inline int
gc_is_alive(PyObject *op)
{
    return gc_has_bit(op, _PyGC_BITS_ALIVE);
}

#ifdef GC_ENABLE_MARK_ALIVE
static inline void
gc_set_alive(PyObject *op)
{
    gc_set_bit(op, _PyGC_BITS_ALIVE);
}
#endif

static inline void
gc_clear_alive(PyObject *op)
{
    gc_clear_bit(op, _PyGC_BITS_ALIVE);
}

// Initialize the `ob_tid` field to zero if the object is not already
// initialized as unreachable.
static void
gc_maybe_init_refs(PyObject *op)
{
    if (!gc_is_unreachable(op)) {
        assert(!gc_is_alive(op));
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
    _Py_AddRefTotal(_PyThreadState_GET(), extra);
#endif

    // No atomics necessary; all other threads in this interpreter are paused.
    op->ob_tid = 0;
    op->ob_ref_local = 0;
    op->ob_ref_shared = _Py_REF_SHARED(refcount, _Py_REF_MERGED);
    return refcount;
}

static void
frame_disable_deferred_refcounting(_PyInterpreterFrame *frame)
{
    // Convert locals, variables, and the executable object to strong
    // references from (possibly) deferred references.
    assert(frame->stackpointer != NULL);
    assert(frame->owner == FRAME_OWNED_BY_FRAME_OBJECT ||
           frame->owner == FRAME_OWNED_BY_GENERATOR);

    frame->f_executable = PyStackRef_AsStrongReference(frame->f_executable);

    if (frame->owner == FRAME_OWNED_BY_GENERATOR) {
        PyGenObject *gen = _PyGen_GetGeneratorFromFrame(frame);
        if (gen->gi_frame_state == FRAME_CLEARED) {
            // gh-124068: if the generator is cleared, then most fields other
            // than f_executable are not valid.
            return;
        }
    }

    frame->f_funcobj = PyStackRef_AsStrongReference(frame->f_funcobj);
    for (_PyStackRef *ref = frame->localsplus; ref < frame->stackpointer; ref++) {
        if (!PyStackRef_IsNull(*ref) && PyStackRef_IsDeferred(*ref)) {
            *ref = PyStackRef_AsStrongReference(*ref);
        }
    }
}

static void
disable_deferred_refcounting(PyObject *op)
{
    if (_PyObject_HasDeferredRefcount(op)) {
        op->ob_gc_bits &= ~_PyGC_BITS_DEFERRED;
        op->ob_ref_shared -= _Py_REF_SHARED(_Py_REF_DEFERRED, 0);
        merge_refcount(op, 0);

        // Heap types and code objects also use per-thread refcounting, which
        // should also be disabled when we turn off deferred refcounting.
        _PyObject_DisablePerThreadRefcounting(op);
    }

    // Generators and frame objects may contain deferred references to other
    // objects. If the pointed-to objects are part of cyclic trash, we may
    // have disabled deferred refcounting on them and need to ensure that we
    // use strong references, in case the generator or frame object is
    // resurrected by a finalizer.
    if (PyGen_CheckExact(op) || PyCoro_CheckExact(op) || PyAsyncGen_CheckExact(op)) {
        frame_disable_deferred_refcounting(&((PyGenObject *)op)->gi_iframe);
    }
    else if (PyFrame_Check(op)) {
        frame_disable_deferred_refcounting(((PyFrameObject *)op)->f_frame);
    }
}

static void
gc_restore_tid(PyObject *op)
{
    assert(_PyInterpreterState_GET()->stoptheworld.world_stopped);
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
        assert(!gc_is_alive(op));
        gc_restore_tid(op);
        gc_clear_unreachable(op);
    }
    else {
        gc_clear_alive(op);
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
    if (!include_frozen && gc_is_frozen(op)) {
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
    _Py_FOR_EACH_TSTATE_UNLOCKED(interp, p) {
        struct _mimalloc_thread_state *m = &((_PyThreadStateImpl *)p)->mimalloc;
        if (!_Py_atomic_load_int(&m->initialized)) {
            // The thread may not have called tstate_mimalloc_bind() yet.
            continue;
        }

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

static inline void
gc_visit_stackref(_PyStackRef stackref)
{
    if (PyStackRef_IsDeferred(stackref) && !PyStackRef_IsNull(stackref)) {
        PyObject *obj = PyStackRef_AsPyObjectBorrow(stackref);
        if (_PyObject_GC_IS_TRACKED(obj) && !gc_is_frozen(obj)) {
            gc_add_refs(obj, 1);
        }
    }
}

// Add 1 to the gc_refs for every deferred reference on each thread's stack.
static void
gc_visit_thread_stacks(PyInterpreterState *interp, struct collection_state *state)
{
    _Py_FOR_EACH_TSTATE_BEGIN(interp, p) {
        _PyCStackRef *c_ref = ((_PyThreadStateImpl *)p)->c_stack_refs;
        while (c_ref != NULL) {
            gc_visit_stackref(c_ref->ref);
            c_ref = c_ref->next;
        }

        for (_PyInterpreterFrame *f = p->current_frame; f != NULL; f = f->previous) {
            if (f->owner >= FRAME_OWNED_BY_INTERPRETER) {
                continue;
            }

            _PyStackRef *top = f->stackpointer;
            if (top == NULL) {
                // GH-129236: The stackpointer may be NULL in cases where
                // the GC is run during a PyStackRef_CLOSE() call. Skip this
                // frame and don't collect objects with deferred references.
                state->skip_deferred_objects = 1;
                continue;
            }

            gc_visit_stackref(f->f_executable);
            while (top != f->localsplus) {
                --top;
                gc_visit_stackref(*top);
            }
        }
    }
    _Py_FOR_EACH_TSTATE_END(interp);
}

// Untrack objects that can never create reference cycles.
// Return true if the object was untracked.
static bool
gc_maybe_untrack(PyObject *op)
{
    // Currently we only check for tuples containing only non-GC objects.  In
    // theory we could check other immutable objects that contain references
    // to non-GC objects.
    if (PyTuple_CheckExact(op)) {
        _PyTuple_MaybeUntrack(op);
        if (!_PyObject_GC_IS_TRACKED(op)) {
            return true;
        }
    }
    return false;
}

#ifdef GC_ENABLE_MARK_ALIVE

// prefetch buffer and stack //////////////////////////////////

// The buffer is a circular FIFO queue of PyObject pointers.  We take
// care to not dereference these pointers until they are taken out of
// the buffer.  A prefetch CPU instruction is issued when a pointer is
// put into the buffer.  If all is working as expected, there will be
// enough time between the enqueue and dequeue so that the needed memory
// for the object, most importantly ob_gc_bits and ob_type words, will
// already be in the CPU cache.
#define BUFFER_SIZE 256
#define BUFFER_HI 16
#define BUFFER_LO 8
#define BUFFER_MASK (BUFFER_SIZE - 1)

// the buffer size must be an exact power of two
static_assert(BUFFER_SIZE > 0 && !(BUFFER_SIZE & BUFFER_MASK),
              "Invalid BUFFER_SIZE, must be power of 2");
// the code below assumes these relationships are true
static_assert(BUFFER_HI < BUFFER_SIZE &&
              BUFFER_LO < BUFFER_HI &&
              BUFFER_LO > 0,
              "Invalid prefetch buffer level settings.");

// Prefetch intructions will fetch the line of data from memory that
// contains the byte specified with the source operand to a location in
// the cache hierarchy specified by a locality hint.  The instruction
// is only a hint and the CPU is free to ignore it.  Instructions and
// behaviour are CPU specific but the definitions of locality hints
// below are mostly consistent.
//
// * T0 (temporal data) prefetch data into all levels of the cache hierarchy.
//
// * T1 (temporal data with respect to first level cache) prefetch data into
//   level 2 cache and higher.
//
// * T2 (temporal data with respect to second level cache) prefetch data into
//   level 3 cache and higher, or an implementation-specific choice.
//
// * NTA (non-temporal data with respect to all cache levels) prefetch data into
//   non-temporal cache structure and into a location close to the processor,
//   minimizing cache pollution.

#if defined(__GNUC__) || defined(__clang__)
    #define PREFETCH_T0(ptr)  __builtin_prefetch(ptr, 0, 3)
    #define PREFETCH_T1(ptr)  __builtin_prefetch(ptr, 0, 2)
    #define PREFETCH_T2(ptr)  __builtin_prefetch(ptr, 0, 1)
    #define PREFETCH_NTA(ptr)  __builtin_prefetch(ptr, 0, 0)
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_I86)) && !defined(_M_ARM64EC)
    #include <mmintrin.h>
    #define PREFETCH_T0(ptr)  _mm_prefetch((const char*)(ptr), _MM_HINT_T0)
    #define PREFETCH_T1(ptr)  _mm_prefetch((const char*)(ptr), _MM_HINT_T1)
    #define PREFETCH_T2(ptr)  _mm_prefetch((const char*)(ptr), _MM_HINT_T2)
    #define PREFETCH_NTA(ptr)  _mm_prefetch((const char*)(ptr), _MM_HINT_NTA)
#elif defined (__aarch64__)
    #define PREFETCH_T0(ptr)  \
        do { __asm__ __volatile__("prfm pldl1keep, %0" ::"Q"(*(ptr))); } while (0)
    #define PREFETCH_T1(ptr)  \
        do { __asm__ __volatile__("prfm pldl2keep, %0" ::"Q"(*(ptr))); } while (0)
    #define PREFETCH_T2(ptr)  \
        do { __asm__ __volatile__("prfm pldl3keep, %0" ::"Q"(*(ptr))); } while (0)
    #define PREFETCH_NTA(ptr)  \
        do { __asm__ __volatile__("prfm pldl1strm, %0" ::"Q"(*(ptr))); } while (0)
#else
    #define PREFETCH_T0(ptr) do { (void)(ptr); } while (0)  /* disabled */
    #define PREFETCH_T1(ptr) do { (void)(ptr); } while (0)  /* disabled */
    #define PREFETCH_T2(ptr) do { (void)(ptr); } while (0)  /* disabled */
    #define PREFETCH_NTA(ptr) do { (void)(ptr); } while (0)  /* disabled */
#endif

#ifdef GC_ENABLE_PREFETCH_INSTRUCTIONS
    #define prefetch(ptr) PREFETCH_T1(ptr)
#else
    #define prefetch(ptr)
#endif

// a contigous sequence of PyObject pointers, can contain NULLs
typedef struct {
    PyObject **start;
    PyObject **end;
} gc_span_t;

typedef struct {
    Py_ssize_t size;
    Py_ssize_t capacity;
    gc_span_t *stack;
} gc_span_stack_t;

typedef struct {
    unsigned int in;
    unsigned int out;
    _PyObjectStack stack;
    gc_span_stack_t spans;
    PyObject *buffer[BUFFER_SIZE];
    bool use_prefetch;
} gc_mark_args_t;


// Returns number of entries in buffer
static inline unsigned int
gc_mark_buffer_len(gc_mark_args_t *args)
{
    return args->in - args->out;
}

// Returns number of free entry slots in buffer
#ifndef NDEBUG
static inline unsigned int
gc_mark_buffer_avail(gc_mark_args_t *args)
{
    return BUFFER_SIZE - gc_mark_buffer_len(args);
}
#endif

static inline bool
gc_mark_buffer_is_empty(gc_mark_args_t *args)
{
    return args->in == args->out;
}

static inline bool
gc_mark_buffer_is_full(gc_mark_args_t *args)
{
    return gc_mark_buffer_len(args) == BUFFER_SIZE;
}

static inline PyObject *
gc_mark_buffer_pop(gc_mark_args_t *args)
{
    assert(!gc_mark_buffer_is_empty(args));
    PyObject *op = args->buffer[args->out & BUFFER_MASK];
    args->out++;
    return op;
}

// Called when there is space in the buffer for the object.  Issue the
// prefetch instruction and add it to the end of the buffer.
static inline void
gc_mark_buffer_push(PyObject *op, gc_mark_args_t *args)
{
    assert(!gc_mark_buffer_is_full(args));
    prefetch(op);
    args->buffer[args->in & BUFFER_MASK] = op;
    args->in++;
}

// Called when we run out of space in the buffer or if the prefetching
// is disabled. The object will be pushed on the gc_mark_args.stack.
static int
gc_mark_stack_push(_PyObjectStack *ms, PyObject *op)
{
    if (_PyObjectStack_Push(ms, op) < 0) {
        return -1;
    }
    return 0;
}

static int
gc_mark_span_push(gc_span_stack_t *ss, PyObject **start, PyObject **end)
{
    if (start == end) {
        return 0;
    }
    if (ss->size >= ss->capacity) {
        if (ss->capacity == 0) {
            ss->capacity = 256;
        }
        else {
            ss->capacity *= 2;
        }
        ss->stack = (gc_span_t *)PyMem_Realloc(ss->stack, ss->capacity * sizeof(gc_span_t));
        if (ss->stack == NULL) {
            return -1;
        }
    }
    assert(end > start);
    ss->stack[ss->size].start = start;
    ss->stack[ss->size].end = end;
    ss->size++;
    return 0;
}

static int
gc_mark_enqueue_no_buffer(PyObject *op, gc_mark_args_t *args)
{
    if (op == NULL) {
        return 0;
    }
    if (!gc_has_bit(op,  _PyGC_BITS_TRACKED)) {
        return 0;
    }
    if (gc_is_alive(op)) {
        return 0; // already visited this object
    }
    if (gc_maybe_untrack(op)) {
        return 0; // was untracked, don't visit it
    }

    // Need to call tp_traverse on this object. Add to stack and mark it
    // alive so we don't traverse it a second time.
    gc_set_alive(op);
    if (_PyObjectStack_Push(&args->stack, op) < 0) {
        return -1;
    }
    return 0;
}

static int
gc_mark_enqueue_buffer(PyObject *op, gc_mark_args_t *args)
{
    assert(op != NULL);
    if (!gc_mark_buffer_is_full(args)) {
        gc_mark_buffer_push(op, args);
        return 0;
    }
    else {
        return gc_mark_stack_push(&args->stack, op);
    }
}

// Called when we find an object that needs to be marked alive (either from a
// root or from calling tp_traverse).
static int
gc_mark_enqueue(PyObject *op, gc_mark_args_t *args)
{
    if (args->use_prefetch) {
        return gc_mark_enqueue_buffer(op, args);
    }
    else {
        return gc_mark_enqueue_no_buffer(op, args);
    }
}

// Called when we have a contigous sequence of PyObject pointers, either
// a tuple or list object.  This will add the items to the buffer if there
// is space for them all otherwise push a new "span" on the span stack.  Using
// spans has the advantage of not creating a deep _PyObjectStack stack when
// dealing with long sequences.  Those sequences will be processed in smaller
// chunks by the gc_prime_from_spans() function.
static int
gc_mark_enqueue_span(PyObject **item, Py_ssize_t size, gc_mark_args_t *args)
{
    Py_ssize_t used = gc_mark_buffer_len(args);
    Py_ssize_t free = BUFFER_SIZE - used;
    if (free >= size) {
        for (Py_ssize_t i = 0; i < size; i++) {
            PyObject *op = item[i];
            if (op == NULL) {
                continue;
            }
            gc_mark_buffer_push(op, args);
        }
    }
    else {
        assert(size > 0);
        PyObject **end = &item[size];
        if (gc_mark_span_push(&args->spans, item, end) < 0) {
            return -1;
        }
    }
    return 0;
}

static bool
gc_clear_alive_bits(const mi_heap_t *heap, const mi_heap_area_t *area,
                    void *block, size_t block_size, void *args)
{
    PyObject *op = op_from_block(block, args, false);
    if (op == NULL) {
        return true;
    }
    if (gc_is_alive(op)) {
        gc_clear_alive(op);
    }
    return true;
}

static int
gc_mark_traverse_list(PyObject *self, void *args)
{
    PyListObject *list = (PyListObject *)self;
    if (list->ob_item == NULL) {
        return 0;
    }
    if (gc_mark_enqueue_span(list->ob_item, PyList_GET_SIZE(list), args) < 0) {
        return -1;
    }
    return 0;
}

static int
gc_mark_traverse_tuple(PyObject *self, void *args)
{
    _PyTuple_MaybeUntrack(self);
    if (!gc_has_bit(self,  _PyGC_BITS_TRACKED)) {
        gc_clear_alive(self);
        return 0;
    }
    PyTupleObject *tuple = _PyTuple_CAST(self);
    if (gc_mark_enqueue_span(tuple->ob_item, Py_SIZE(tuple), args) < 0) {
        return -1;
    }
    return 0;
}

static void
gc_abort_mark_alive(PyInterpreterState *interp,
                    struct collection_state *state,
                    gc_mark_args_t *args)
{
    // We failed to allocate memory while doing the "mark alive" phase.
    // In that case, free the memory used for marking state and make
    // sure that no objects have the alive bit set.
    _PyObjectStack_Clear(&args->stack);
    if (args->spans.stack != NULL) {
        PyMem_Free(args->spans.stack);
    }
    gc_visit_heaps(interp, &gc_clear_alive_bits, &state->base);
}

#ifdef GC_MARK_ALIVE_STACKS
static int
gc_visit_stackref_mark_alive(gc_mark_args_t *args, _PyStackRef stackref)
{
    if (!PyStackRef_IsNull(stackref)) {
        PyObject *op = PyStackRef_AsPyObjectBorrow(stackref);
        if (gc_mark_enqueue(op, args) < 0) {
            return -1;
        }
    }
    return 0;
}

static int
gc_visit_thread_stacks_mark_alive(PyInterpreterState *interp, gc_mark_args_t *args)
{
    int err = 0;
    _Py_FOR_EACH_TSTATE_BEGIN(interp, p) {
        for (_PyInterpreterFrame *f = p->current_frame; f != NULL; f = f->previous) {
            if (f->owner >= FRAME_OWNED_BY_INTERPRETER) {
                continue;
            }

            if (f->stackpointer == NULL) {
                // GH-129236: The stackpointer may be NULL in cases where
                // the GC is run during a PyStackRef_CLOSE() call. Skip this
                // frame for now.
                continue;
            }

            _PyStackRef *top = f->stackpointer;
            if (gc_visit_stackref_mark_alive(args, f->f_executable) < 0) {
                err = -1;
                goto exit;
            }
            while (top != f->localsplus) {
                --top;
                if (gc_visit_stackref_mark_alive(args, *top) < 0) {
                    err = -1;
                    goto exit;
                }
            }
        }
    }
exit:
    _Py_FOR_EACH_TSTATE_END(interp);
    return err;
}
#endif // GC_MARK_ALIVE_STACKS
#endif // GC_ENABLE_MARK_ALIVE

static void
queue_untracked_obj_decref(PyObject *op, struct collection_state *state)
{
    if (!_PyObject_GC_IS_TRACKED(op)) {
        // GC objects with zero refcount are handled subsequently by the
        // GC as if they were cyclic trash, but we have to handle dead
        // non-GC objects here. Add one to the refcount so that we can
        // decref and deallocate the object once we start the world again.
        op->ob_ref_shared += (1 << _Py_REF_SHARED_SHIFT);
#ifdef Py_REF_DEBUG
        _Py_IncRefTotal(_PyThreadState_GET());
#endif
        worklist_push(&state->objs_to_decref, op);
    }

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

        if (refcount == 0) {
            queue_untracked_obj_decref(op, state);
        }
    }
}

static void
queue_freed_object(PyObject *obj, void *arg)
{
    queue_untracked_obj_decref(obj, arg);
}

static void
process_delayed_frees(PyInterpreterState *interp, struct collection_state *state)
{
    // While we are in a "stop the world" pause, we can observe the latest
    // write sequence by advancing the write sequence immediately.
    _Py_qsbr_advance(&interp->qsbr);
    _PyThreadStateImpl *current_tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
    _Py_qsbr_quiescent_state(current_tstate->qsbr);

    // Merge the queues from other threads into our own queue so that we can
    // process all of the pending delayed free requests at once.
    _Py_FOR_EACH_TSTATE_BEGIN(interp, p) {
        _PyThreadStateImpl *other = (_PyThreadStateImpl *)p;
        if (other != current_tstate) {
            llist_concat(&current_tstate->mem_free_queue, &other->mem_free_queue);
        }
    }
    _Py_FOR_EACH_TSTATE_END(interp);

    _PyMem_ProcessDelayedNoDealloc((PyThreadState *)current_tstate, queue_freed_object, state);
}

// Subtract an incoming reference from the computed "gc_refs" refcount.
static int
visit_decref(PyObject *op, void *arg)
{
    if (_PyObject_GC_IS_TRACKED(op)
        && !_Py_IsImmortal(op)
        && !gc_is_frozen(op)
        && !gc_is_alive(op))
    {
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

    if (gc_is_alive(op)) {
        return true;
    }

    // Exclude immortal objects from garbage collection
    if (_Py_IsImmortal(op)) {
        op->ob_tid = 0;
        _PyObject_GC_UNTRACK(op);
        gc_clear_unreachable(op);
        return true;
    }

    Py_ssize_t refcount = Py_REFCNT(op);
    if (_PyObject_HasDeferredRefcount(op)) {
        refcount -= _Py_REF_DEFERRED;
    }
    _PyObject_ASSERT(op, refcount >= 0);

    if (refcount > 0 && !_PyObject_HasDeferredRefcount(op)) {
        if (gc_maybe_untrack(op)) {
            gc_restore_refs(op);
            return true;
        }
    }

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
validate_alive_bits(const mi_heap_t *heap, const mi_heap_area_t *area,
                   void *block, size_t block_size, void *args)
{
    PyObject *op = op_from_block(block, args, false);
    if (op == NULL) {
        return true;
    }

    _PyObject_ASSERT_WITH_MSG(op, !gc_is_alive(op),
                              "object should not be marked as alive yet");

    return true;
}

static bool
validate_refcounts(const mi_heap_t *heap, const mi_heap_area_t *area,
                   void *block, size_t block_size, void *args)
{
    PyObject *op = op_from_block(block, args, false);
    if (op == NULL) {
        return true;
    }

    _PyObject_ASSERT_WITH_MSG(op, !gc_is_unreachable(op),
                              "object should not be marked as unreachable yet");

    if (_Py_REF_IS_MERGED(op->ob_ref_shared)) {
        _PyObject_ASSERT_WITH_MSG(op, op->ob_tid == 0,
                                  "merged objects should have ob_tid == 0");
    }
    else if (!_Py_IsImmortal(op)) {
        _PyObject_ASSERT_WITH_MSG(op, op->ob_tid != 0,
                                  "unmerged objects should have ob_tid != 0");
    }

    return true;
}

static bool
validate_gc_objects(const mi_heap_t *heap, const mi_heap_area_t *area,
                    void *block, size_t block_size, void *args)
{
    PyObject *op = op_from_block(block, args, false);
    if (op == NULL) {
        return true;
    }

    if (gc_is_alive(op)) {
        _PyObject_ASSERT(op, !gc_is_unreachable(op));
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

    if (gc_is_alive(op) || !gc_is_unreachable(op)) {
        // Object was already marked as reachable.
        return true;
    }

    _PyObject_ASSERT_WITH_MSG(op, gc_get_refs(op) >= 0,
                                  "refcount is too small");

    // GH-129236: If we've seen an active frame without a valid stack pointer,
    // then we can't collect objects with deferred references because we may
    // have missed some reference to the object on the stack. In that case,
    // treat the object as reachable even if gc_refs is zero.
    struct collection_state *state = (struct collection_state *)args;
    int keep_alive = (state->skip_deferred_objects &&
                      _PyObject_HasDeferredRefcount(op));

    if (gc_get_refs(op) != 0 || keep_alive) {
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

static bool
restore_refs(const mi_heap_t *heap, const mi_heap_area_t *area,
             void *block, size_t block_size, void *args)
{
    PyObject *op = op_from_block(block, args, false);
    if (op == NULL) {
        return true;
    }
    gc_restore_tid(op);
    gc_clear_unreachable(op);
    gc_clear_alive(op);
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
        // Disable deferred refcounting for unreachable objects so that they
        // are collected immediately after finalization.
        disable_deferred_refcounting(op);

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
        return true;
    }

    if (state->reason == _Py_GC_REASON_SHUTDOWN) {
        // Disable deferred refcounting for reachable objects as well during
        // interpreter shutdown. This ensures that these objects are collected
        // immediately when their last reference is removed.
        disable_deferred_refcounting(op);
    }

    // object is reachable, restore `ob_tid`; we're done with these objects
    gc_restore_tid(op);
    gc_clear_alive(op);
    state->long_lived_total++;
    return true;
}

static int
move_legacy_finalizer_reachable(struct collection_state *state);

#ifdef GC_ENABLE_MARK_ALIVE

static void
gc_prime_from_spans(gc_mark_args_t *args)
{
    unsigned int space = BUFFER_HI - gc_mark_buffer_len(args);
    // there should always be at least this amount of space
    assert(space <= gc_mark_buffer_avail(args));
    assert(space <= BUFFER_HI);
    gc_span_t entry = args->spans.stack[--args->spans.size];
    // spans on the stack should always have one or more elements
    assert(entry.start < entry.end);
    do {
        PyObject *op = *entry.start;
        entry.start++;
        if (op != NULL) {
            gc_mark_buffer_push(op, args);
            space--;
            if (space == 0) {
                // buffer is as full as we want and not done with span
                gc_mark_span_push(&args->spans, entry.start, entry.end);
                return;
            }
        }
    } while (entry.start < entry.end);
}

static void
gc_prime_buffer(gc_mark_args_t *args)
{
    if (args->spans.size > 0) {
        gc_prime_from_spans(args);
    }
    else {
        // When priming, don't fill the buffer too full since that would
        // likely cause the stack to be used shortly after when it
        // fills. We want to use the buffer as much as possible and so
        // we only fill to BUFFER_HI, not BUFFER_SIZE.
        Py_ssize_t space = BUFFER_HI - gc_mark_buffer_len(args);
        assert(space > 0);
        do {
            PyObject *op = _PyObjectStack_Pop(&args->stack);
            if (op == NULL) {
                return;
            }
            gc_mark_buffer_push(op, args);
            space--;
        } while (space > 0);
    }
}

static int
gc_propagate_alive_prefetch(gc_mark_args_t *args)
{
    for (;;) {
        Py_ssize_t buf_used = gc_mark_buffer_len(args);
        if (buf_used <= BUFFER_LO) {
            // The mark buffer is getting empty.  If it's too empty
            // then there will not be enough delay between issuing
            // the prefetch and when the object is actually accessed.
            // Prime the buffer with object pointers from the stack or
            // from the spans, if there are any available.
            gc_prime_buffer(args);
            if (gc_mark_buffer_is_empty(args)) {
                return 0;
            }
        }
        PyObject *op = gc_mark_buffer_pop(args);

        if (!gc_has_bit(op, _PyGC_BITS_TRACKED)) {
            continue;
        }

        if (gc_is_alive(op)) {
            continue; // already visited this object
        }

        // Need to call tp_traverse on this object. Mark it alive so we
        // don't traverse it a second time.
        gc_set_alive(op);

        traverseproc traverse = Py_TYPE(op)->tp_traverse;
        if (traverse == PyList_Type.tp_traverse) {
            if (gc_mark_traverse_list(op, args) < 0) {
                return -1;
            }
        }
        else if (traverse == PyTuple_Type.tp_traverse) {
            if (gc_mark_traverse_tuple(op, args) < 0) {
                return -1;
            }
        }
        else if (traverse(op, (visitproc)&gc_mark_enqueue_buffer, args) < 0) {
            return -1;
        }
    }
}

static int
gc_propagate_alive(gc_mark_args_t *args)
{
    if (args->use_prefetch) {
        return gc_propagate_alive_prefetch(args);
    }
    else {
        for (;;) {
            PyObject *op = _PyObjectStack_Pop(&args->stack);
            if (op == NULL) {
                break;
            }
            assert(_PyObject_GC_IS_TRACKED(op));
            assert(gc_is_alive(op));
            traverseproc traverse = Py_TYPE(op)->tp_traverse;
            if (traverse(op, (visitproc)&gc_mark_enqueue_no_buffer, args) < 0) {
                return -1;
            }
        }
        return 0;
    }
}

// Using tp_traverse, mark everything reachable from known root objects
// (which must be non-garbage) as alive (_PyGC_BITS_ALIVE is set).  In
// most programs, this marks nearly all objects that are not actually
// unreachable.
//
// Actually alive objects can be missed in this pass if they are alive
// due to being referenced from an unknown root (e.g. an extension
// module global), some tp_traverse methods are either missing or not
// accurate, or objects that have been untracked.  Objects that are only
// reachable from the aforementioned are also missed.
//
// If gc.freeze() is used, this pass is disabled since it is unlikely to
// help much.  The next stages of cyclic GC will ignore objects with the
// alive bit set.
//
// Returns -1 on failure (out of memory).
static int
gc_mark_alive_from_roots(PyInterpreterState *interp,
                         struct collection_state *state)
{
#ifdef GC_DEBUG
    // Check that all objects don't have alive bit set
    gc_visit_heaps(interp, &validate_alive_bits, &state->base);
#endif
    gc_mark_args_t mark_args = { 0 };

    // Using prefetch instructions is only a win if the set of objects being
    // examined by the GC does not fit into CPU caches.  Otherwise, using the
    // buffer and prefetch instructions is just overhead.  Using the long lived
    // object count seems a good estimate of if things will fit in the cache.
    // On 64-bit platforms, the minimum object size is 32 bytes.  A 4MB L2 cache
    // would hold about 130k objects.
    mark_args.use_prefetch = interp->gc.long_lived_total > 200000;

    #define MARK_ENQUEUE(op) \
        if (op != NULL ) { \
            if (gc_mark_enqueue(op, &mark_args) < 0) { \
                gc_abort_mark_alive(interp, state, &mark_args); \
                return -1; \
            } \
        }
    MARK_ENQUEUE(interp->sysdict);
#ifdef GC_MARK_ALIVE_EXTRA_ROOTS
    MARK_ENQUEUE(interp->builtins);
    MARK_ENQUEUE(interp->dict);
    struct types_state *types = &interp->types;
    for (int i = 0; i < _Py_MAX_MANAGED_STATIC_BUILTIN_TYPES; i++) {
        MARK_ENQUEUE(types->builtins.initialized[i].tp_dict);
        MARK_ENQUEUE(types->builtins.initialized[i].tp_subclasses);
    }
    for (int i = 0; i < _Py_MAX_MANAGED_STATIC_EXT_TYPES; i++) {
        MARK_ENQUEUE(types->for_extensions.initialized[i].tp_dict);
        MARK_ENQUEUE(types->for_extensions.initialized[i].tp_subclasses);
    }
#endif
#ifdef GC_MARK_ALIVE_STACKS
    if (gc_visit_thread_stacks_mark_alive(interp, &mark_args) < 0) {
        gc_abort_mark_alive(interp, state, &mark_args);
        return -1;
    }
#endif
    #undef MARK_ENQUEUE

    // Use tp_traverse to find everything reachable from roots.
    if (gc_propagate_alive(&mark_args) < 0) {
        gc_abort_mark_alive(interp, state, &mark_args);
        return -1;
    }

    assert(mark_args.spans.size == 0);
    if (mark_args.spans.stack != NULL) {
        PyMem_Free(mark_args.spans.stack);
    }
    assert(mark_args.stack.head == NULL);

    return 0;
}
#endif // GC_ENABLE_MARK_ALIVE


static int
deduce_unreachable_heap(PyInterpreterState *interp,
                        struct collection_state *state)
{

#ifdef GC_DEBUG
    // Check that all objects are marked as unreachable and that the computed
    // reference count difference (stored in `ob_tid`) is non-negative.
    gc_visit_heaps(interp, &validate_refcounts, &state->base);
#endif

    // Identify objects that are directly reachable from outside the GC heap
    // by computing the difference between the refcount and the number of
    // incoming references.
    gc_visit_heaps(interp, &update_refs, &state->base);

#ifdef GC_DEBUG
    // Check that all objects are marked as unreachable and that the computed
    // reference count difference (stored in `ob_tid`) is non-negative.
    gc_visit_heaps(interp, &validate_gc_objects, &state->base);
#endif

    // Visit the thread stacks to account for any deferred references.
    gc_visit_thread_stacks(interp, state);

    // Transitively mark reachable objects by clearing the
    // _PyGC_BITS_UNREACHABLE flag.
    if (gc_visit_heaps(interp, &mark_heap_visitor, &state->base) < 0) {
        // On out-of-memory, restore the refcounts and bail out.
        gc_visit_heaps(interp, &restore_refs, &state->base);
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
            PyErr_FormatUnraisable("Exception ignored while "
                                   "calling weakref callback %R", callback);
        }
        else {
            Py_DECREF(temp);
        }

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
    gcstate->young.threshold = 2000;
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

int
_PyGC_VisitStackRef(_PyStackRef *ref, visitproc visit, void *arg)
{
    // This is a bit tricky! We want to ignore deferred references when
    // computing the incoming references, but otherwise treat them like
    // regular references.
    if (!PyStackRef_IsDeferred(*ref) ||
        (visit != visit_decref && visit != visit_decref_unreachable))
    {
        Py_VISIT(PyStackRef_AsPyObjectBorrow(*ref));
    }
    return 0;
}

int
_PyGC_VisitFrameStack(_PyInterpreterFrame *frame, visitproc visit, void *arg)
{
    _PyStackRef *ref = _PyFrame_GetLocalsArray(frame);
    /* locals and stack */
    for (; ref < frame->stackpointer; ref++) {
        _Py_VISIT_STACKREF(*ref);
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
            PyErr_FormatUnraisable("Exception ignored while "
                                   "invoking gc callbacks");
            return;
        }
    }

    PyObject *phase_obj = PyUnicode_FromString(phase);
    if (phase_obj == NULL) {
        Py_XDECREF(info);
        PyErr_FormatUnraisable("Exception ignored while "
                               "invoking gc callbacks");
        return;
    }

    PyObject *stack[] = {phase_obj, info};
    for (Py_ssize_t i=0; i<PyList_GET_SIZE(gcstate->callbacks); i++) {
        PyObject *r, *cb = PyList_GET_ITEM(gcstate->callbacks, i);
        Py_INCREF(cb); /* make sure cb doesn't go away */
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

static void
cleanup_worklist(struct worklist *worklist)
{
    PyObject *op;
    while ((op = worklist_pop(worklist)) != NULL) {
        gc_clear_unreachable(op);
        Py_DECREF(op);
    }
}

static bool
gc_should_collect(GCState *gcstate)
{
    int count = _Py_atomic_load_int_relaxed(&gcstate->young.count);
    int threshold = gcstate->young.threshold;
    int gc_enabled = _Py_atomic_load_int_relaxed(&gcstate->enabled);
    if (count <= threshold || threshold == 0 || !gc_enabled) {
        return false;
    }
    // Avoid quadratic behavior by scaling threshold to the number of live
    // objects. A few tests rely on immediate scheduling of the GC so we ignore
    // the scaled threshold if generations[1].threshold is set to zero.
    return (count > gcstate->long_lived_total / 4 ||
            gcstate->old[0].threshold == 0);
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
        _Py_atomic_add_int(&gcstate->young.count, (int)gc->alloc_count);
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
        _Py_atomic_add_int(&gcstate->young.count, (int)gc->alloc_count);
        gc->alloc_count = 0;
    }
}

static void
gc_collect_internal(PyInterpreterState *interp, struct collection_state *state, int generation)
{
    _PyEval_StopTheWorld(interp);

    // update collection and allocation counters
    if (generation+1 < NUM_GENERATIONS) {
        state->gcstate->old[generation].count += 1;
    }

    state->gcstate->young.count = 0;
    for (int i = 1; i <= generation; ++i) {
        state->gcstate->old[i-1].count = 0;
    }

    _Py_FOR_EACH_TSTATE_BEGIN(interp, p) {
        _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)p;

        // merge per-thread refcount for types into the type's actual refcount
        _PyObject_MergePerThreadRefcounts(tstate);

        // merge refcounts for all queued objects
        merge_queued_objects(tstate, state);
    }
    _Py_FOR_EACH_TSTATE_END(interp);

    process_delayed_frees(interp, state);

    #ifdef GC_ENABLE_MARK_ALIVE
    // If gc.freeze() was used, it seems likely that doing this "mark alive"
    // pass will not be a performance win.  Typically the majority of alive
    // objects will be marked as frozen and will be skipped anyhow, without
    // doing this extra work.  Doing this pass also defeats one of the
    // purposes of using freeze: avoiding writes to objects that are frozen.
    // So, we just skip this if gc.freeze() was used.
    if (!state->gcstate->freeze_active) {
        // Mark objects reachable from known roots as "alive".  These will
        // be ignored for rest of the GC pass.
        int err = gc_mark_alive_from_roots(interp, state);
        if (err < 0) {
            _PyEval_StartTheWorld(interp);
            PyErr_NoMemory();
            return;
        }
    }
    #endif

    // Find unreachable objects
    int err = deduce_unreachable_heap(interp, state);
    if (err < 0) {
        _PyEval_StartTheWorld(interp);
        PyErr_NoMemory();
        return;
    }

#ifdef GC_DEBUG
    // At this point, no object should have the alive bit set
    gc_visit_heaps(interp, &validate_alive_bits, &state->base);
#endif

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
        cleanup_worklist(&state->unreachable);
        cleanup_worklist(&state->legacy_finalizers);
        cleanup_worklist(&state->wrcb_to_call);
        cleanup_worklist(&state->objs_to_decref);
        PyErr_NoMemory();
        return;
    }

    // Call tp_clear on objects in the unreachable set. This will cause
    // the reference cycles to be broken. It may also cause some objects
    // to be freed.
    delete_garbage(state);

    // Append objects with legacy finalizers to the "gc.garbage" list.
    handle_legacy_finalizers(state);
}

/* This is the main function.  Read this to understand how the
 * collection process works. */
static Py_ssize_t
gc_collect_main(PyThreadState *tstate, int generation, _PyGC_Reason reason)
{
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
        // ignore error: don't interrupt the GC if reading the clock fails
        (void)PyTime_PerfCounterRaw(&t1);
    }

    if (PyDTrace_GC_START_ENABLED()) {
        PyDTrace_GC_START(generation);
    }

    PyInterpreterState *interp = tstate->interp;

    struct collection_state state = {
        .interp = interp,
        .gcstate = gcstate,
        .reason = reason,
    };

    gc_collect_internal(interp, &state, generation);

    m = state.collected;
    n = state.uncollectable;

    if (gcstate->debug & _PyGC_DEBUG_STATS) {
        PyTime_t t2;
        (void)PyTime_PerfCounterRaw(&t2);
        double d = PyTime_AsSecondsDouble(t2 - t1);
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

static PyObject *
list_from_object_stack(_PyObjectStack *stack)
{
    PyObject *list = PyList_New(_PyObjectStack_Size(stack));
    if (list == NULL) {
        PyObject *op;
        while ((op = _PyObjectStack_Pop(stack)) != NULL) {
            Py_DECREF(op);
        }
        return NULL;
    }

    PyObject *op;
    Py_ssize_t idx = 0;
    while ((op = _PyObjectStack_Pop(stack)) != NULL) {
        assert(idx < PyList_GET_SIZE(list));
        PyList_SET_ITEM(list, idx++, op);
    }
    assert(idx == PyList_GET_SIZE(list));
    return list;
}

struct get_referrers_args {
    struct visitor_args base;
    PyObject *objs;
    _PyObjectStack results;
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
    if (op->ob_gc_bits & (_PyGC_BITS_UNREACHABLE | _PyGC_BITS_FROZEN)) {
        // Exclude unreachable objects (in-progress GC) and frozen
        // objects from gc.get_objects() to match the default build.
        return true;
    }

    struct get_referrers_args *arg = (struct get_referrers_args *)args;
    if (op == arg->objs) {
        // Don't include the tuple itself in the referrers list.
        return true;
    }
    if (Py_TYPE(op)->tp_traverse(op, referrersvisit, arg->objs)) {
        if (_PyObjectStack_Push(&arg->results, Py_NewRef(op)) < 0) {
            return false;
        }
    }

    return true;
}

PyObject *
_PyGC_GetReferrers(PyInterpreterState *interp, PyObject *objs)
{
    // NOTE: We can't append to the PyListObject during gc_visit_heaps()
    // because PyList_Append() may reclaim an abandoned mimalloc segments
    // while we are traversing them.
    struct get_referrers_args args = { .objs = objs };
    _PyEval_StopTheWorld(interp);
    int err = gc_visit_heaps(interp, &visit_get_referrers, &args.base);
    _PyEval_StartTheWorld(interp);

    PyObject *list = list_from_object_stack(&args.results);
    if (err < 0) {
        PyErr_NoMemory();
        Py_CLEAR(list);
    }
    return list;
}

struct get_objects_args {
    struct visitor_args base;
    _PyObjectStack objects;
};

static bool
visit_get_objects(const mi_heap_t *heap, const mi_heap_area_t *area,
                  void *block, size_t block_size, void *args)
{
    PyObject *op = op_from_block(block, args, true);
    if (op == NULL) {
        return true;
    }
    if (op->ob_gc_bits & (_PyGC_BITS_UNREACHABLE | _PyGC_BITS_FROZEN)) {
        // Exclude unreachable objects (in-progress GC) and frozen
        // objects from gc.get_objects() to match the default build.
        return true;
    }

    struct get_objects_args *arg = (struct get_objects_args *)args;
    if (_PyObjectStack_Push(&arg->objects, Py_NewRef(op)) < 0) {
        return false;
    }
    return true;
}

PyObject *
_PyGC_GetObjects(PyInterpreterState *interp, int generation)
{
    // NOTE: We can't append to the PyListObject during gc_visit_heaps()
    // because PyList_Append() may reclaim an abandoned mimalloc segments
    // while we are traversing them.
    struct get_objects_args args = { 0 };
    _PyEval_StopTheWorld(interp);
    int err = gc_visit_heaps(interp, &visit_get_objects, &args.base);
    _PyEval_StartTheWorld(interp);

    PyObject *list = list_from_object_stack(&args.objects);
    if (err < 0) {
        PyErr_NoMemory();
        Py_CLEAR(list);
    }
    return list;
}

static bool
visit_freeze(const mi_heap_t *heap, const mi_heap_area_t *area,
             void *block, size_t block_size, void *args)
{
    PyObject *op = op_from_block(block, args, true);
    if (op != NULL && !gc_is_unreachable(op)) {
        op->ob_gc_bits |= _PyGC_BITS_FROZEN;
    }
    return true;
}

void
_PyGC_Freeze(PyInterpreterState *interp)
{
    struct visitor_args args;
    _PyEval_StopTheWorld(interp);
    GCState *gcstate = get_gc_state();
    gcstate->freeze_active = true;
    gc_visit_heaps(interp, &visit_freeze, &args);
    _PyEval_StartTheWorld(interp);
}

static bool
visit_unfreeze(const mi_heap_t *heap, const mi_heap_area_t *area,
               void *block, size_t block_size, void *args)
{
    PyObject *op = op_from_block(block, args, true);
    if (op != NULL) {
        gc_clear_bit(op, _PyGC_BITS_FROZEN);
    }
    return true;
}

void
_PyGC_Unfreeze(PyInterpreterState *interp)
{
    struct visitor_args args;
    _PyEval_StopTheWorld(interp);
    GCState *gcstate = get_gc_state();
    gcstate->freeze_active = false;
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
    if (op != NULL && gc_is_frozen(op)) {
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
    return _Py_atomic_exchange_int(&gcstate->enabled, 1);
}

int
PyGC_Disable(void)
{
    GCState *gcstate = get_gc_state();
    return _Py_atomic_exchange_int(&gcstate->enabled, 0);
}

int
PyGC_IsEnabled(void)
{
    GCState *gcstate = get_gc_state();
    return _Py_atomic_load_int_relaxed(&gcstate->enabled);
}

/* Public API to invoke gc.collect() from C */
Py_ssize_t
PyGC_Collect(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    GCState *gcstate = &tstate->interp->gc;

    if (!_Py_atomic_load_int_relaxed(&gcstate->enabled)) {
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

void
_PyGC_CollectNoFail(PyThreadState *tstate)
{
    /* Ideally, this function is only called on interpreter shutdown,
       and therefore not recursively.  Unfortunately, when there are daemon
       threads, a daemon thread can start a cyclic garbage collection
       during interpreter shutdown (and then never finish it).
       See http://bugs.python.org/issue8713#msg195178 for an example.
       */
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
        /* PyErr_WarnFormat does too many things and we are at shutdown,
           the warnings module's dependencies (e.g. linecache) may be gone
           already. */
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
    if (!PyGC_IsEnabled()) {
        return;
    }
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

    record_deallocation(_PyThreadState_GET());
    PyObject_Free(((char *)op)-presize);
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
_PyGC_VisitObjectsWorldStopped(PyInterpreterState *interp,
                               gcvisitobjects_t callback, void *arg)
{
    struct custom_visitor_args wrapper = {
        .callback = callback,
        .arg = arg,
    };
    gc_visit_heaps(interp, &custom_visitor_wrapper, &wrapper.base);
}

void
PyUnstable_GC_VisitObjects(gcvisitobjects_t callback, void *arg)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyEval_StopTheWorld(interp);
    _PyGC_VisitObjectsWorldStopped(interp, callback, arg);
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
    _Py_FOR_EACH_TSTATE_BEGIN(interp, p) {
        _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)p;
        _PyObject_ClearFreeLists(&tstate->freelists, 0);
    }
    _Py_FOR_EACH_TSTATE_END(interp);
}

#endif  // Py_GIL_DISABLED
