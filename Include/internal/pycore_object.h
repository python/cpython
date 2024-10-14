#ifndef Py_INTERNAL_OBJECT_H
#define Py_INTERNAL_OBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stdbool.h>
#include "pycore_gc.h"            // _PyObject_GC_IS_TRACKED()
#include "pycore_emscripten_trampoline.h" // _PyCFunction_TrampolineCall()
#include "pycore_interp.h"        // PyInterpreterState.gc
#include "pycore_pyatomic_ft_wrappers.h"  // FT_ATOMIC_STORE_PTR_RELAXED
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_uniqueid.h"      // _PyType_IncrefSlow

// This value is added to `ob_ref_shared` for objects that use deferred
// reference counting so that they are not immediately deallocated when the
// non-deferred reference count drops to zero.
//
// The value is half the maximum shared refcount because the low two bits of
// `ob_ref_shared` are used for flags.
#define _Py_REF_DEFERRED (PY_SSIZE_T_MAX / 8)

/* For backwards compatibility -- Do not use this */
#define _Py_IsImmortalLoose(op) _Py_IsImmortal


/* Check if an object is consistent. For example, ensure that the reference
   counter is greater than or equal to 1, and ensure that ob_type is not NULL.

   Call _PyObject_AssertFailed() if the object is inconsistent.

   If check_content is zero, only check header fields: reduce the overhead.

   The function always return 1. The return value is just here to be able to
   write:

   assert(_PyObject_CheckConsistency(obj, 1)); */
extern int _PyObject_CheckConsistency(PyObject *op, int check_content);

extern void _PyDebugAllocatorStats(FILE *out, const char *block_name,
                                   int num_blocks, size_t sizeof_block);

extern void _PyObject_DebugTypeStats(FILE *out);

#ifdef Py_TRACE_REFS
// Forget a reference registered by _Py_NewReference(). Function called by
// _Py_Dealloc().
//
// On a free list, the function can be used before modifying an object to
// remove the object from traced objects. Then _Py_NewReference() or
// _Py_NewReferenceNoTotal() should be called again on the object to trace
// it again.
extern void _Py_ForgetReference(PyObject *);
#endif

// Export for shared _testinternalcapi extension
PyAPI_FUNC(int) _PyObject_IsFreed(PyObject *);

/* We need to maintain an internal copy of Py{Var}Object_HEAD_INIT to avoid
   designated initializer conflicts in C++20. If we use the deinition in
   object.h, we will be mixing designated and non-designated initializers in
   pycore objects which is forbiddent in C++20. However, if we then use
   designated initializers in object.h then Extensions without designated break.
   Furthermore, we can't use designated initializers in Extensions since these
   are not supported pre-C++20. Thus, keeping an internal copy here is the most
   backwards compatible solution */
#if defined(Py_GIL_DISABLED)
#define _PyObject_HEAD_INIT(type)                   \
    {                                               \
        .ob_ref_local = _Py_IMMORTAL_REFCNT_LOCAL,  \
        .ob_type = (type)                           \
    }
#else
#define _PyObject_HEAD_INIT(type)         \
    {                                     \
        .ob_refcnt = _Py_IMMORTAL_INITIAL_REFCNT, \
        .ob_type = (type)                 \
    }
#endif
#define _PyVarObject_HEAD_INIT(type, size)    \
    {                                         \
        .ob_base = _PyObject_HEAD_INIT(type), \
        .ob_size = size                       \
    }

PyAPI_FUNC(void) _Py_NO_RETURN _Py_FatalRefcountErrorFunc(
    const char *func,
    const char *message);

#define _Py_FatalRefcountError(message) \
    _Py_FatalRefcountErrorFunc(__func__, (message))


#ifdef Py_REF_DEBUG
/* The symbol is only exposed in the API for the sake of extensions
   built against the pre-3.12 stable ABI. */
PyAPI_DATA(Py_ssize_t) _Py_RefTotal;

extern void _Py_AddRefTotal(PyThreadState *, Py_ssize_t);
extern void _Py_IncRefTotal(PyThreadState *);
extern void _Py_DecRefTotal(PyThreadState *);

#  define _Py_DEC_REFTOTAL(interp) \
    interp->object_state.reftotal--
#endif

// Increment reference count by n
static inline void _Py_RefcntAdd(PyObject* op, Py_ssize_t n)
{
    if (_Py_IsImmortal(op)) {
        return;
    }
#ifdef Py_REF_DEBUG
    _Py_AddRefTotal(_PyThreadState_GET(), n);
#endif
#if !defined(Py_GIL_DISABLED)
    op->ob_refcnt += n;
#else
    if (_Py_IsOwnedByCurrentThread(op)) {
        uint32_t local = op->ob_ref_local;
        Py_ssize_t refcnt = (Py_ssize_t)local + n;
#  if PY_SSIZE_T_MAX > UINT32_MAX
        if (refcnt > (Py_ssize_t)UINT32_MAX) {
            // Make the object immortal if the 32-bit local reference count
            // would overflow.
            refcnt = _Py_IMMORTAL_REFCNT_LOCAL;
        }
#  endif
        _Py_atomic_store_uint32_relaxed(&op->ob_ref_local, (uint32_t)refcnt);
    }
    else {
        _Py_atomic_add_ssize(&op->ob_ref_shared, (n << _Py_REF_SHARED_SHIFT));
    }
#endif
}
#define _Py_RefcntAdd(op, n) _Py_RefcntAdd(_PyObject_CAST(op), n)

// Checks if an object has a single, unique reference. If the caller holds a
// unique reference, it may be able to safely modify the object in-place.
static inline int
_PyObject_IsUniquelyReferenced(PyObject *ob)
{
#if !defined(Py_GIL_DISABLED)
    return Py_REFCNT(ob) == 1;
#else
    // NOTE: the entire ob_ref_shared field must be zero, including flags, to
    // ensure that other threads cannot concurrently create new references to
    // this object.
    return (_Py_IsOwnedByCurrentThread(ob) &&
            _Py_atomic_load_uint32_relaxed(&ob->ob_ref_local) == 1 &&
            _Py_atomic_load_ssize_relaxed(&ob->ob_ref_shared) == 0);
#endif
}

PyAPI_FUNC(void) _Py_SetImmortal(PyObject *op);
PyAPI_FUNC(void) _Py_SetImmortalUntracked(PyObject *op);

// Makes an immortal object mortal again with the specified refcnt. Should only
// be used during runtime finalization.
static inline void _Py_SetMortal(PyObject *op, Py_ssize_t refcnt)
{
    if (op) {
        assert(_Py_IsImmortal(op));
#ifdef Py_GIL_DISABLED
        op->ob_tid = _Py_UNOWNED_TID;
        op->ob_ref_local = 0;
        op->ob_ref_shared = _Py_REF_SHARED(refcnt, _Py_REF_MERGED);
#else
        op->ob_refcnt = refcnt;
#endif
    }
}

/* _Py_ClearImmortal() should only be used during runtime finalization. */
static inline void _Py_ClearImmortal(PyObject *op)
{
    if (op) {
        _Py_SetMortal(op, 1);
        Py_DECREF(op);
    }
}
#define _Py_ClearImmortal(op) \
    do { \
        _Py_ClearImmortal(_PyObject_CAST(op)); \
        op = NULL; \
    } while (0)

#if !defined(Py_GIL_DISABLED)
static inline void
_Py_DECREF_SPECIALIZED(PyObject *op, const destructor destruct)
{
    if (_Py_IsImmortal(op)) {
        _Py_DECREF_IMMORTAL_STAT_INC();
        return;
    }
    _Py_DECREF_STAT_INC();
#ifdef Py_REF_DEBUG
    _Py_DEC_REFTOTAL(PyInterpreterState_Get());
#endif
    if (--op->ob_refcnt != 0) {
        assert(op->ob_refcnt > 0);
    }
    else {
#ifdef Py_TRACE_REFS
        _Py_ForgetReference(op);
#endif
        destruct(op);
    }
}

static inline void
_Py_DECREF_NO_DEALLOC(PyObject *op)
{
    if (_Py_IsImmortal(op)) {
        _Py_DECREF_IMMORTAL_STAT_INC();
        return;
    }
    _Py_DECREF_STAT_INC();
#ifdef Py_REF_DEBUG
    _Py_DEC_REFTOTAL(PyInterpreterState_Get());
#endif
    op->ob_refcnt--;
#ifdef Py_DEBUG
    if (op->ob_refcnt <= 0) {
        _Py_FatalRefcountError("Expected a positive remaining refcount");
    }
#endif
}

#else
// TODO: implement Py_DECREF specializations for Py_GIL_DISABLED build
static inline void
_Py_DECREF_SPECIALIZED(PyObject *op, const destructor destruct)
{
    Py_DECREF(op);
}

static inline void
_Py_DECREF_NO_DEALLOC(PyObject *op)
{
    Py_DECREF(op);
}

static inline int
_Py_REF_IS_MERGED(Py_ssize_t ob_ref_shared)
{
    return (ob_ref_shared & _Py_REF_SHARED_FLAG_MASK) == _Py_REF_MERGED;
}

static inline int
_Py_REF_IS_QUEUED(Py_ssize_t ob_ref_shared)
{
    return (ob_ref_shared & _Py_REF_SHARED_FLAG_MASK) == _Py_REF_QUEUED;
}

// Merge the local and shared reference count fields and add `extra` to the
// refcount when merging.
Py_ssize_t _Py_ExplicitMergeRefcount(PyObject *op, Py_ssize_t extra);
#endif // !defined(Py_GIL_DISABLED)

#ifdef Py_REF_DEBUG
#  undef _Py_DEC_REFTOTAL
#endif


extern int _PyType_CheckConsistency(PyTypeObject *type);
extern int _PyDict_CheckConsistency(PyObject *mp, int check_content);

/* Update the Python traceback of an object. This function must be called
   when a memory block is reused from a free list.

   Internal function called by _Py_NewReference(). */
extern int _PyTraceMalloc_TraceRef(PyObject *op, PyRefTracerEvent event, void*);

// Fast inlined version of PyType_HasFeature()
static inline int
_PyType_HasFeature(PyTypeObject *type, unsigned long feature) {
    return ((FT_ATOMIC_LOAD_ULONG_RELAXED(type->tp_flags) & feature) != 0);
}

extern void _PyType_InitCache(PyInterpreterState *interp);

extern PyStatus _PyObject_InitState(PyInterpreterState *interp);
extern void _PyObject_FiniState(PyInterpreterState *interp);
extern bool _PyRefchain_IsTraced(PyInterpreterState *interp, PyObject *obj);

#ifndef Py_GIL_DISABLED
#  define _Py_INCREF_TYPE Py_INCREF
#  define _Py_DECREF_TYPE Py_DECREF
#else
static inline void
_Py_INCREF_TYPE(PyTypeObject *type)
{
    if (!_PyType_HasFeature(type, Py_TPFLAGS_HEAPTYPE)) {
        assert(_Py_IsImmortal(type));
        _Py_INCREF_IMMORTAL_STAT_INC();
        return;
    }

    // gh-122974: GCC 11 warns about the access to PyHeapTypeObject fields when
    // _Py_INCREF_TYPE() is called on a statically allocated type.  The
    // _PyType_HasFeature check above ensures that the type is a heap type.
#if defined(__GNUC__) && __GNUC__ >= 11
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Warray-bounds"
#endif

    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
    PyHeapTypeObject *ht = (PyHeapTypeObject *)type;

    // Unsigned comparison so that `unique_id=-1`, which indicates that
    // per-thread refcounting has been disabled on this type, is handled by
    // the "else".
    if ((size_t)ht->unique_id < (size_t)tstate->refcounts.size) {
#  ifdef Py_REF_DEBUG
        _Py_INCREF_IncRefTotal();
#  endif
        _Py_INCREF_STAT_INC();
        tstate->refcounts.values[ht->unique_id]++;
    }
    else {
        // The slow path resizes the thread-local refcount array if necessary.
        // It handles the unique_id=-1 case to keep the inlinable function smaller.
        _PyType_IncrefSlow(ht);
    }

#if defined(__GNUC__) && __GNUC__ >= 11
#  pragma GCC diagnostic pop
#endif
}

static inline void
_Py_DECREF_TYPE(PyTypeObject *type)
{
    if (!_PyType_HasFeature(type, Py_TPFLAGS_HEAPTYPE)) {
        assert(_Py_IsImmortal(type));
        _Py_DECREF_IMMORTAL_STAT_INC();
        return;
    }

    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
    PyHeapTypeObject *ht = (PyHeapTypeObject *)type;

    // Unsigned comparison so that `unique_id=-1`, which indicates that
    // per-thread refcounting has been disabled on this type, is handled by
    // the "else".
    if ((size_t)ht->unique_id < (size_t)tstate->refcounts.size) {
#  ifdef Py_REF_DEBUG
        _Py_DECREF_DecRefTotal();
#  endif
        _Py_DECREF_STAT_INC();
        tstate->refcounts.values[ht->unique_id]--;
    }
    else {
        // Directly decref the type if the type id is not assigned or if
        // per-thread refcounting has been disabled on this type.
        Py_DECREF(type);
    }
}
#endif

/* Inline functions trading binary compatibility for speed:
   _PyObject_Init() is the fast version of PyObject_Init(), and
   _PyObject_InitVar() is the fast version of PyObject_InitVar().

   These inline functions must not be called with op=NULL. */
static inline void
_PyObject_Init(PyObject *op, PyTypeObject *typeobj)
{
    assert(op != NULL);
    Py_SET_TYPE(op, typeobj);
    assert(_PyType_HasFeature(typeobj, Py_TPFLAGS_HEAPTYPE) || _Py_IsImmortal(typeobj));
    _Py_INCREF_TYPE(typeobj);
    _Py_NewReference(op);
}

static inline void
_PyObject_InitVar(PyVarObject *op, PyTypeObject *typeobj, Py_ssize_t size)
{
    assert(op != NULL);
    assert(typeobj != &PyLong_Type);
    _PyObject_Init((PyObject *)op, typeobj);
    Py_SET_SIZE(op, size);
}


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

    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyGC_Head *generation0 = &interp->gc.young.head;
    PyGC_Head *last = (PyGC_Head*)(generation0->_gc_prev);
    _PyGCHead_SET_NEXT(last, gc);
    _PyGCHead_SET_PREV(gc, last);
    /* Young objects will be moved into the visited space during GC, so set the bit here */
    gc->_gc_next = ((uintptr_t)generation0) | (uintptr_t)interp->gc.visited_space;
    generation0->_gc_prev = (uintptr_t)gc;
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
#endif
}

// Macros to accept any type for the parameter, and to automatically pass
// the filename and the filename (if NDEBUG is not defined) where the macro
// is called.
#ifdef NDEBUG
#  define _PyObject_GC_TRACK(op) \
        _PyObject_GC_TRACK(_PyObject_CAST(op))
#  define _PyObject_GC_UNTRACK(op) \
        _PyObject_GC_UNTRACK(_PyObject_CAST(op))
#else
#  define _PyObject_GC_TRACK(op) \
        _PyObject_GC_TRACK(__FILE__, __LINE__, _PyObject_CAST(op))
#  define _PyObject_GC_UNTRACK(op) \
        _PyObject_GC_UNTRACK(__FILE__, __LINE__, _PyObject_CAST(op))
#endif

#ifdef Py_GIL_DISABLED

/* Tries to increment an object's reference count
 *
 * This is a specialized version of _Py_TryIncref that only succeeds if the
 * object is immortal or local to this thread. It does not handle the case
 * where the  reference count modification requires an atomic operation. This
 * allows call sites to specialize for the immortal/local case.
 */
static inline int
_Py_TryIncrefFast(PyObject *op) {
    uint32_t local = _Py_atomic_load_uint32_relaxed(&op->ob_ref_local);
    local += 1;
    if (local == 0) {
        // immortal
        _Py_INCREF_IMMORTAL_STAT_INC();
        return 1;
    }
    if (_Py_IsOwnedByCurrentThread(op)) {
        _Py_INCREF_STAT_INC();
        _Py_atomic_store_uint32_relaxed(&op->ob_ref_local, local);
#ifdef Py_REF_DEBUG
        _Py_IncRefTotal(_PyThreadState_GET());
#endif
        return 1;
    }
    return 0;
}

static inline int
_Py_TryIncRefShared(PyObject *op)
{
    Py_ssize_t shared = _Py_atomic_load_ssize_relaxed(&op->ob_ref_shared);
    for (;;) {
        // If the shared refcount is zero and the object is either merged
        // or may not have weak references, then we cannot incref it.
        if (shared == 0 || shared == _Py_REF_MERGED) {
            return 0;
        }

        if (_Py_atomic_compare_exchange_ssize(
                &op->ob_ref_shared,
                &shared,
                shared + (1 << _Py_REF_SHARED_SHIFT))) {
#ifdef Py_REF_DEBUG
            _Py_IncRefTotal(_PyThreadState_GET());
#endif
            _Py_INCREF_STAT_INC();
            return 1;
        }
    }
}

/* Tries to incref the object op and ensures that *src still points to it. */
static inline int
_Py_TryIncrefCompare(PyObject **src, PyObject *op)
{
    if (_Py_TryIncrefFast(op)) {
        return 1;
    }
    if (!_Py_TryIncRefShared(op)) {
        return 0;
    }
    if (op != _Py_atomic_load_ptr(src)) {
        Py_DECREF(op);
        return 0;
    }
    return 1;
}

/* Loads and increfs an object from ptr, which may contain a NULL value.
   Safe with concurrent (atomic) updates to ptr.
   NOTE: The writer must set maybe-weakref on the stored object! */
static inline PyObject *
_Py_XGetRef(PyObject **ptr)
{
    for (;;) {
        PyObject *value = _Py_atomic_load_ptr(ptr);
        if (value == NULL) {
            return value;
        }
        if (_Py_TryIncrefCompare(ptr, value)) {
            return value;
        }
    }
}

/* Attempts to loads and increfs an object from ptr. Returns NULL
   on failure, which may be due to a NULL value or a concurrent update. */
static inline PyObject *
_Py_TryXGetRef(PyObject **ptr)
{
    PyObject *value = _Py_atomic_load_ptr(ptr);
    if (value == NULL) {
        return value;
    }
    if (_Py_TryIncrefCompare(ptr, value)) {
        return value;
    }
    return NULL;
}

/* Like Py_NewRef but also optimistically sets _Py_REF_MAYBE_WEAKREF
   on objects owned by a different thread. */
static inline PyObject *
_Py_NewRefWithLock(PyObject *op)
{
    if (_Py_TryIncrefFast(op)) {
        return op;
    }
#ifdef Py_REF_DEBUG
    _Py_IncRefTotal(_PyThreadState_GET());
#endif
    _Py_INCREF_STAT_INC();
    for (;;) {
        Py_ssize_t shared = _Py_atomic_load_ssize_relaxed(&op->ob_ref_shared);
        Py_ssize_t new_shared = shared + (1 << _Py_REF_SHARED_SHIFT);
        if ((shared & _Py_REF_SHARED_FLAG_MASK) == 0) {
            new_shared |= _Py_REF_MAYBE_WEAKREF;
        }
        if (_Py_atomic_compare_exchange_ssize(
                &op->ob_ref_shared,
                &shared,
                new_shared)) {
            return op;
        }
    }
}

static inline PyObject *
_Py_XNewRefWithLock(PyObject *obj)
{
    if (obj == NULL) {
        return NULL;
    }
    return _Py_NewRefWithLock(obj);
}

static inline void
_PyObject_SetMaybeWeakref(PyObject *op)
{
    if (_Py_IsImmortal(op)) {
        return;
    }
    for (;;) {
        Py_ssize_t shared = _Py_atomic_load_ssize_relaxed(&op->ob_ref_shared);
        if ((shared & _Py_REF_SHARED_FLAG_MASK) != 0) {
            // Nothing to do if it's in WEAKREFS, QUEUED, or MERGED states.
            return;
        }
        if (_Py_atomic_compare_exchange_ssize(
                &op->ob_ref_shared, &shared, shared | _Py_REF_MAYBE_WEAKREF)) {
            return;
        }
    }
}

#endif

/* Tries to incref op and returns 1 if successful or 0 otherwise. */
static inline int
_Py_TryIncref(PyObject *op)
{
#ifdef Py_GIL_DISABLED
    return _Py_TryIncrefFast(op) || _Py_TryIncRefShared(op);
#else
    if (Py_REFCNT(op) > 0) {
        Py_INCREF(op);
        return 1;
    }
    return 0;
#endif
}

#ifdef Py_REF_DEBUG
extern void _PyInterpreterState_FinalizeRefTotal(PyInterpreterState *);
extern void _Py_FinalizeRefTotal(_PyRuntimeState *);
extern void _PyDebug_PrintTotalRefs(void);
#endif

#ifdef Py_TRACE_REFS
extern void _Py_AddToAllObjects(PyObject *op);
extern void _Py_PrintReferences(PyInterpreterState *, FILE *);
extern void _Py_PrintReferenceAddresses(PyInterpreterState *, FILE *);
#endif


/* Return the *address* of the object's weaklist.  The address may be
 * dereferenced to get the current head of the weaklist.  This is useful
 * for iterating over the linked list of weakrefs, especially when the
 * list is being modified externally (e.g. refs getting removed).
 *
 * The returned pointer should not be used to change the head of the list
 * nor should it be used to add, remove, or swap any refs in the list.
 * That is the sole responsibility of the code in weakrefobject.c.
 */
static inline PyObject **
_PyObject_GET_WEAKREFS_LISTPTR(PyObject *op)
{
    if (PyType_Check(op) &&
            ((PyTypeObject *)op)->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        managed_static_type_state *state = _PyStaticType_GetState(
                                                interp, (PyTypeObject *)op);
        return _PyStaticType_GET_WEAKREFS_LISTPTR(state);
    }
    // Essentially _PyObject_GET_WEAKREFS_LISTPTR_FROM_OFFSET():
    Py_ssize_t offset = Py_TYPE(op)->tp_weaklistoffset;
    return (PyObject **)((char *)op + offset);
}

/* This is a special case of _PyObject_GET_WEAKREFS_LISTPTR().
 * Only the most fundamental lookup path is used.
 * Consequently, static types should not be used.
 *
 * For static builtin types the returned pointer will always point
 * to a NULL tp_weaklist.  This is fine for any deallocation cases,
 * since static types are never deallocated and static builtin types
 * are only finalized at the end of runtime finalization.
 *
 * If the weaklist for static types is actually needed then use
 * _PyObject_GET_WEAKREFS_LISTPTR().
 */
static inline PyWeakReference **
_PyObject_GET_WEAKREFS_LISTPTR_FROM_OFFSET(PyObject *op)
{
    assert(!PyType_Check(op) ||
            ((PyTypeObject *)op)->tp_flags & Py_TPFLAGS_HEAPTYPE);
    Py_ssize_t offset = Py_TYPE(op)->tp_weaklistoffset;
    return (PyWeakReference **)((char *)op + offset);
}

// Fast inlined version of PyType_IS_GC()
#define _PyType_IS_GC(t) _PyType_HasFeature((t), Py_TPFLAGS_HAVE_GC)

// Fast inlined version of PyObject_IS_GC()
static inline int
_PyObject_IS_GC(PyObject *obj)
{
    PyTypeObject *type = Py_TYPE(obj);
    return (_PyType_IS_GC(type)
            && (type->tp_is_gc == NULL || type->tp_is_gc(obj)));
}

// Fast inlined version of PyObject_Hash()
static inline Py_hash_t
_PyObject_HashFast(PyObject *op)
{
    if (PyUnicode_CheckExact(op)) {
        Py_hash_t hash = FT_ATOMIC_LOAD_SSIZE_RELAXED(
                             _PyASCIIObject_CAST(op)->hash);
        if (hash != -1) {
            return hash;
        }
    }
    return PyObject_Hash(op);
}

static inline size_t
_PyType_PreHeaderSize(PyTypeObject *tp)
{
    return (
#ifndef Py_GIL_DISABLED
        (size_t)_PyType_IS_GC(tp) * sizeof(PyGC_Head) +
#endif
        (size_t)_PyType_HasFeature(tp, Py_TPFLAGS_PREHEADER) * 2 * sizeof(PyObject *)
    );
}

void _PyObject_GC_Link(PyObject *op);

// Usage: assert(_Py_CheckSlotResult(obj, "__getitem__", result != NULL));
extern int _Py_CheckSlotResult(
    PyObject *obj,
    const char *slot_name,
    int success);

// Test if a type supports weak references
static inline int _PyType_SUPPORTS_WEAKREFS(PyTypeObject *type) {
    return (type->tp_weaklistoffset != 0);
}

extern PyObject* _PyType_AllocNoTrack(PyTypeObject *type, Py_ssize_t nitems);
PyAPI_FUNC(PyObject *) _PyType_NewManagedObject(PyTypeObject *type);

extern PyTypeObject* _PyType_CalculateMetaclass(PyTypeObject *, PyObject *);
extern PyObject* _PyType_GetDocFromInternalDoc(const char *, const char *);
extern PyObject* _PyType_GetTextSignatureFromInternalDoc(const char *, const char *, int);
extern int _PyObject_SetAttributeErrorContext(PyObject *v, PyObject* name);

void _PyObject_InitInlineValues(PyObject *obj, PyTypeObject *tp);
extern int _PyObject_StoreInstanceAttribute(PyObject *obj,
                                            PyObject *name, PyObject *value);
extern bool _PyObject_TryGetInstanceAttribute(PyObject *obj, PyObject *name,
                                              PyObject **attr);

#ifdef Py_GIL_DISABLED
#  define MANAGED_DICT_OFFSET    (((Py_ssize_t)sizeof(PyObject *))*-1)
#  define MANAGED_WEAKREF_OFFSET (((Py_ssize_t)sizeof(PyObject *))*-2)
#else
#  define MANAGED_DICT_OFFSET    (((Py_ssize_t)sizeof(PyObject *))*-3)
#  define MANAGED_WEAKREF_OFFSET (((Py_ssize_t)sizeof(PyObject *))*-4)
#endif

typedef union {
    PyDictObject *dict;
} PyManagedDictPointer;

static inline PyManagedDictPointer *
_PyObject_ManagedDictPointer(PyObject *obj)
{
    assert(Py_TYPE(obj)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
    return (PyManagedDictPointer *)((char *)obj + MANAGED_DICT_OFFSET);
}

static inline PyDictObject *
_PyObject_GetManagedDict(PyObject *obj)
{
    PyManagedDictPointer *dorv = _PyObject_ManagedDictPointer(obj);
    return (PyDictObject *)FT_ATOMIC_LOAD_PTR_ACQUIRE(dorv->dict);
}

static inline PyDictValues *
_PyObject_InlineValues(PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    assert(tp->tp_basicsize > 0 && (size_t)tp->tp_basicsize % sizeof(PyObject *) == 0);
    assert(Py_TYPE(obj)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
    assert(Py_TYPE(obj)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
    return (PyDictValues *)((char *)obj + tp->tp_basicsize);
}

extern PyObject ** _PyObject_ComputedDictPointer(PyObject *);
extern int _PyObject_IsInstanceDictEmpty(PyObject *);

// Export for 'math' shared extension
PyAPI_FUNC(PyObject*) _PyObject_LookupSpecial(PyObject *, PyObject *);
PyAPI_FUNC(PyObject*) _PyObject_LookupSpecialMethod(PyObject *self, PyObject *attr, PyObject **self_or_null);

extern int _PyObject_IsAbstract(PyObject *);

PyAPI_FUNC(int) _PyObject_GetMethod(PyObject *obj, PyObject *name, PyObject **method);
extern PyObject* _PyObject_NextNotImplemented(PyObject *);

// Pickle support.
// Export for '_datetime' shared extension
PyAPI_FUNC(PyObject*) _PyObject_GetState(PyObject *);

/* C function call trampolines to mitigate bad function pointer casts.
 *
 * Typical native ABIs ignore additional arguments or fill in missing
 * values with 0/NULL in function pointer cast. Compilers do not show
 * warnings when a function pointer is explicitly casted to an
 * incompatible type.
 *
 * Bad fpcasts are an issue in WebAssembly. WASM's indirect_call has strict
 * function signature checks. Argument count, types, and return type must
 * match.
 *
 * Third party code unintentionally rely on problematic fpcasts. The call
 * trampoline mitigates common occurrences of bad fpcasts on Emscripten.
 */
#if !(defined(__EMSCRIPTEN__) && defined(PY_CALL_TRAMPOLINE))
#define _PyCFunction_TrampolineCall(meth, self, args) \
    (meth)((self), (args))
#define _PyCFunctionWithKeywords_TrampolineCall(meth, self, args, kw) \
    (meth)((self), (args), (kw))
#endif // __EMSCRIPTEN__ && PY_CALL_TRAMPOLINE

// Export these 2 symbols for '_pickle' shared extension
PyAPI_DATA(PyTypeObject) _PyNone_Type;
PyAPI_DATA(PyTypeObject) _PyNotImplemented_Type;

// Maps Py_LT to Py_GT, ..., Py_GE to Py_LE.
// Export for the stable ABI.
PyAPI_DATA(int) _Py_SwappedOp[];

extern void _Py_GetConstant_Init(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_OBJECT_H */
