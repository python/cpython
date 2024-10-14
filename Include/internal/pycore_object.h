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
#include "pycore_interp.h"        // PyInterpreterState.gc
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_runtime.h"       // _PyRuntime

/* We need to maintain an internal copy of Py{Var}Object_HEAD_INIT to avoid
   designated initializer conflicts in C++20. If we use the deinition in
   object.h, we will be mixing designated and non-designated initializers in
   pycore objects which is forbiddent in C++20. However, if we then use
   designated initializers in object.h then Extensions without designated break.
   Furthermore, we can't use designated initializers in Extensions since these
   are not supported pre-C++20. Thus, keeping an internal copy here is the most
   backwards compatible solution */
#define _PyObject_HEAD_INIT(type)         \
    {                                     \
        _PyObject_EXTRA_INIT              \
        .ob_refcnt = _Py_IMMORTAL_REFCNT, \
        .ob_type = (type)                 \
    },
#define _PyVarObject_HEAD_INIT(type, size)    \
    {                                         \
        .ob_base = _PyObject_HEAD_INIT(type)  \
        .ob_size = size                       \
    },

PyAPI_FUNC(void) _Py_NO_RETURN _Py_FatalRefcountErrorFunc(
    const char *func,
    const char *message);

#define _Py_FatalRefcountError(message) \
    _Py_FatalRefcountErrorFunc(__func__, (message))


#ifdef Py_REF_DEBUG
/* The symbol is only exposed in the API for the sake of extensions
   built against the pre-3.12 stable ABI. */
PyAPI_DATA(Py_ssize_t) _Py_RefTotal;

extern void _Py_AddRefTotal(PyInterpreterState *, Py_ssize_t);
extern void _Py_IncRefTotal(PyInterpreterState *);
extern void _Py_DecRefTotal(PyInterpreterState *);

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
    _Py_AddRefTotal(_PyInterpreterState_GET(), n);
#endif
    op->ob_refcnt += n;
}
#define _Py_RefcntAdd(op, n) _Py_RefcntAdd(_PyObject_CAST(op), n)

static inline void _Py_SetImmortal(PyObject *op)
{
#ifdef Py_DEBUG
    // For strings, use _PyUnicode_InternImmortal instead.
    if (PyUnicode_CheckExact(op)) {
        assert(PyUnicode_CHECK_INTERNED(op) == SSTATE_INTERNED_IMMORTAL
            || PyUnicode_CHECK_INTERNED(op) == SSTATE_INTERNED_IMMORTAL_STATIC);
    }
#endif
    if (op) {
        op->ob_refcnt = _Py_IMMORTAL_REFCNT;
    }
}
#define _Py_SetImmortal(op) _Py_SetImmortal(_PyObject_CAST(op))

/* _Py_ClearImmortal() should only be used during runtime finalization. */
static inline void _Py_ClearImmortal(PyObject *op)
{
    if (op) {
        assert(_Py_IsImmortal(op));
        op->ob_refcnt = 1;
        Py_DECREF(op);
    }
}
#define _Py_ClearImmortal(op) \
    do { \
        _Py_ClearImmortal(_PyObject_CAST(op)); \
        op = NULL; \
    } while (0)

static inline void
_Py_DECREF_SPECIALIZED(PyObject *op, const destructor destruct)
{
    if (_Py_IsImmortal(op)) {
        return;
    }
    _Py_DECREF_STAT_INC();
#ifdef Py_REF_DEBUG
    _Py_DEC_REFTOTAL(_PyInterpreterState_GET());
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
        return;
    }
    _Py_DECREF_STAT_INC();
#ifdef Py_REF_DEBUG
    _Py_DEC_REFTOTAL(_PyInterpreterState_GET());
#endif
    op->ob_refcnt--;
#ifdef Py_DEBUG
    if (op->ob_refcnt <= 0) {
        _Py_FatalRefcountError("Expected a positive remaining refcount");
    }
#endif
}

#ifdef Py_REF_DEBUG
#  undef _Py_DEC_REFTOTAL
#endif


PyAPI_FUNC(int) _PyType_CheckConsistency(PyTypeObject *type);
PyAPI_FUNC(int) _PyDict_CheckConsistency(PyObject *mp, int check_content);

/* Update the Python traceback of an object. This function must be called
   when a memory block is reused from a free list.

   Internal function called by _Py_NewReference(). */
extern int _PyTraceMalloc_NewReference(PyObject *op);

// Fast inlined version of PyType_HasFeature()
static inline int
_PyType_HasFeature(PyTypeObject *type, unsigned long feature) {
    return ((type->tp_flags & feature) != 0);
}

extern void _PyType_InitCache(PyInterpreterState *interp);

extern void _PyObject_InitState(PyInterpreterState *interp);

/* Inline functions trading binary compatibility for speed:
   _PyObject_Init() is the fast version of PyObject_Init(), and
   _PyObject_InitVar() is the fast version of PyObject_InitVar().

   These inline functions must not be called with op=NULL. */
static inline void
_PyObject_Init(PyObject *op, PyTypeObject *typeobj)
{
    assert(op != NULL);
    Py_SET_TYPE(op, typeobj);
    if (_PyType_HasFeature(typeobj, Py_TPFLAGS_HEAPTYPE)) {
        Py_INCREF(typeobj);
    }
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

    PyGC_Head *gc = _Py_AS_GC(op);
    _PyObject_ASSERT_FROM(op,
                          (gc->_gc_prev & _PyGC_PREV_MASK_COLLECTING) == 0,
                          "object is in generation which is garbage collected",
                          filename, lineno, __func__);

    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyGC_Head *generation0 = interp->gc.generation0;
    PyGC_Head *last = (PyGC_Head*)(generation0->_gc_prev);
    _PyGCHead_SET_NEXT(last, gc);
    _PyGCHead_SET_PREV(gc, last);
    _PyGCHead_SET_NEXT(gc, generation0);
    generation0->_gc_prev = (uintptr_t)gc;
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

    PyGC_Head *gc = _Py_AS_GC(op);
    PyGC_Head *prev = _PyGCHead_PREV(gc);
    PyGC_Head *next = _PyGCHead_NEXT(gc);
    _PyGCHead_SET_NEXT(prev, next);
    _PyGCHead_SET_PREV(next, prev);
    gc->_gc_next = 0;
    gc->_gc_prev &= _PyGC_PREV_MASK_FINALIZED;
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

#ifdef Py_REF_DEBUG
extern void _PyInterpreterState_FinalizeRefTotal(PyInterpreterState *);
extern void _Py_FinalizeRefTotal(_PyRuntimeState *);
extern void _PyDebug_PrintTotalRefs(void);
#endif

#ifdef Py_TRACE_REFS
extern void _Py_AddToAllObjects(PyObject *op, int force);
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
        static_builtin_state *state = _PyStaticType_GetState(
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


// Fast inlined version of PyObject_IS_GC()
static inline int
_PyObject_IS_GC(PyObject *obj)
{
    return (PyType_IS_GC(Py_TYPE(obj))
            && (Py_TYPE(obj)->tp_is_gc == NULL
                || Py_TYPE(obj)->tp_is_gc(obj)));
}

// Fast inlined version of PyType_IS_GC()
#define _PyType_IS_GC(t) _PyType_HasFeature((t), Py_TPFLAGS_HAVE_GC)

static inline size_t
_PyType_PreHeaderSize(PyTypeObject *tp)
{
    return _PyType_IS_GC(tp) * sizeof(PyGC_Head) +
        _PyType_HasFeature(tp, Py_TPFLAGS_PREHEADER) * 2 * sizeof(PyObject *);
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

extern int _PyObject_InitializeDict(PyObject *obj);
extern int _PyObject_StoreInstanceAttribute(PyObject *obj, PyDictValues *values,
                                          PyObject *name, PyObject *value);
PyObject * _PyObject_GetInstanceAttribute(PyObject *obj, PyDictValues *values,
                                        PyObject *name);

typedef union {
    PyObject *dict;
    /* Use a char* to generate a warning if directly assigning a PyDictValues */
    char *values;
} PyDictOrValues;

static inline PyDictOrValues *
_PyObject_DictOrValuesPointer(PyObject *obj)
{
    assert(Py_TYPE(obj)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
    return ((PyDictOrValues *)obj)-3;
}

static inline int
_PyDictOrValues_IsValues(PyDictOrValues dorv)
{
    return ((uintptr_t)dorv.values) & 1;
}

static inline PyDictValues *
_PyDictOrValues_GetValues(PyDictOrValues dorv)
{
    assert(_PyDictOrValues_IsValues(dorv));
    return (PyDictValues *)(dorv.values + 1);
}

static inline PyObject *
_PyDictOrValues_GetDict(PyDictOrValues dorv)
{
    assert(!_PyDictOrValues_IsValues(dorv));
    return dorv.dict;
}

static inline void
_PyDictOrValues_SetValues(PyDictOrValues *ptr, PyDictValues *values)
{
    ptr->values = ((char *)values) - 1;
}

#define MANAGED_WEAKREF_OFFSET (((Py_ssize_t)sizeof(PyObject *))*-4)

extern PyObject ** _PyObject_ComputedDictPointer(PyObject *);
extern void _PyObject_FreeInstanceAttributes(PyObject *obj);
extern int _PyObject_IsInstanceDictEmpty(PyObject *);

PyAPI_FUNC(PyObject *) _PyObject_LookupSpecial(PyObject *, PyObject *);

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
#if defined(__EMSCRIPTEN__) && defined(PY_CALL_TRAMPOLINE)
#define _PyCFunction_TrampolineCall(meth, self, args) \
    _PyCFunctionWithKeywords_TrampolineCall( \
        (*(PyCFunctionWithKeywords)(void(*)(void))(meth)), (self), (args), NULL)
extern PyObject* _PyCFunctionWithKeywords_TrampolineCall(
    PyCFunctionWithKeywords meth, PyObject *, PyObject *, PyObject *);
#else
#define _PyCFunction_TrampolineCall(meth, self, args) \
    (meth)((self), (args))
#define _PyCFunctionWithKeywords_TrampolineCall(meth, self, args, kw) \
    (meth)((self), (args), (kw))
#endif // __EMSCRIPTEN__ && PY_CALL_TRAMPOLINE

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_OBJECT_H */
