#ifndef Py_INTERNAL_TAGGED_H
#define Py_INTERNAL_TAGGED_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stddef.h>
#include "pycore_gc.h"
// Mark an object as supporting deferred reference counting. This is a no-op
// in the default (with GIL) build. Objects that use deferred reference
// counting should be tracked by the GC so that they are eventually collected.
extern void _PyObject_SetDeferredRefcount(PyObject *op);

static inline int
_PyObject_HasDeferredRefcount(PyObject *op)
{
#ifdef Py_GIL_DISABLED
    return (op->ob_gc_bits & _PyGC_BITS_DEFERRED) != 0;
#else
    return 0;
#endif
}

typedef union {
    uintptr_t bits;
} _PyStackRef;

// #define Py_TAG_TEST (0b1)
#define Py_TAG_DEFERRED (0b1)
#define Py_TAG (Py_TAG_DEFERRED)

#if defined(Py_TAG_TEST)
    #define Py_STACK_UNTAG_BORROWED(tagged) ((PyObject *)(uintptr_t)((tagged).bits & (~Py_TAG_TEST)))
#elif defined(Py_GIL_DISABLED)
    static inline PyObject *
    _Py_STACK_UNTAG_BORROWED(_PyStackRef tagged) {
        PyObject *cleared = ((PyObject *)((tagged).bits & (~Py_TAG)));
    #ifdef Py_DEBUG
        if (cleared != NULL && _PyObject_HasDeferredRefcount(cleared)) {
            // Make sure we point to a valid Python object.
            assert(Py_TYPE(cleared) != NULL);
        }
    #endif
        return cleared;
    }
    #define Py_STACK_UNTAG_BORROWED(tagged) _Py_STACK_UNTAG_BORROWED(tagged)
#else
    #define Py_STACK_UNTAG_BORROWED(tagged) ((PyObject *)(uintptr_t)((tagged).bits))
#endif

#if defined(Py_TAG_TEST)
    #define Py_STACK_TAG(obj) ((_PyStackRef){.bits = ((uintptr_t)(obj) | Py_TAG_TEST)})
#elif defined(Py_GIL_DISABLED)
    static inline _PyStackRef
    _Py_STACK_TAG(PyObject *obj) {
        // Make sure we don't take an already tagged value
        assert(Py_STACK_UNTAG_BORROWED(((_PyStackRef){.bits = ((uintptr_t)(obj))})) == obj);
        int tag = ((obj != NULL && _PyObject_HasDeferredRefcount(obj)) ? Py_TAG_DEFERRED : 0);
        return ((_PyStackRef){.bits = ((uintptr_t)(obj) | tag)});
    }
    #define Py_STACK_TAG(obj) _Py_STACK_TAG(_PyObject_CAST(obj))
    // #define Py_STACK_TAG(obj) ((_PyStackRef){.bits = ((uintptr_t)(obj) | Py_TAG_DEFERRED)})
#else
    #define Py_STACK_TAG(obj) ((_PyStackRef){.bits = ((uintptr_t)(obj))})
#endif

#if defined(Py_TAG_TEST)
    #define Py_STACK_UNTAG_OWNED(tagged) Py_STACK_UNTAG_BORROWED(tagged)
#elif defined(Py_GIL_DISABLED)
    static inline PyObject *
    _Py_Stack_Untag_Owned(_PyStackRef tagged) {
        if ((tagged.bits & Py_TAG_DEFERRED) == Py_TAG_DEFERRED) {
            assert(_PyObject_HasDeferredRefcount(Py_STACK_UNTAG_BORROWED(tagged)));
            return Py_NewRef(Py_STACK_UNTAG_BORROWED(tagged));
        }
        assert(!_PyObject_HasDeferredRefcount(Py_STACK_UNTAG_BORROWED(tagged)));
        return Py_STACK_UNTAG_BORROWED(tagged);
    }
    #define Py_STACK_UNTAG_OWNED(tagged) _Py_Stack_Untag_Owned(tagged)
#else
    #define Py_STACK_UNTAG_OWNED(tagged) Py_STACK_UNTAG_BORROWED(tagged)
#endif

#define MAX_UNTAG_SCRATCH 10

static inline void
_Py_untag_stack_borrowed(PyObject **dst, const _PyStackRef *src, size_t length) {
    for (size_t i = 0; i < length; i++) {
        dst[i] = Py_STACK_UNTAG_BORROWED(src[i]);
    }
}

static inline void
_Py_untag_stack_owned(PyObject **dst, const _PyStackRef *src, size_t length) {
    for (size_t i = 0; i < length; i++) {
        dst[i] = Py_STACK_UNTAG_OWNED(src[i]);
    }
}


#define Py_XSETREF_STACKREF(dst, src) \
    do { \
        _PyStackRef *_tmp_dst_ptr = _Py_CAST(_PyStackRef*, &(dst)); \
        _PyStackRef _tmp_old_dst = (*_tmp_dst_ptr); \
        *_tmp_dst_ptr = (src); \
        Py_XDECREF_STACKREF(_tmp_old_dst); \
    } while (0)

#define Py_SETREF_STACKREF(dst, src) \
    do { \
        _PyStackRef *_tmp_dst_ptr = _Py_CAST(_PyStackRef*, &(dst)); \
        _PyStackRef _tmp_old_dst = (*_tmp_dst_ptr); \
        *_tmp_dst_ptr = (src); \
        Py_DECREF_STACKREF(_tmp_old_dst); \
    } while (0)

#define Py_CLEAR_STACKREF(op) \
    do { \
        _PyStackRef *_tmp_op_ptr = _Py_CAST(_PyStackRef*, &(op)); \
        _PyStackRef _tmp_old_op = (*_tmp_op_ptr); \
        if (Py_STACK_UNTAG_BORROWED(_tmp_old_op) != NULL) { \
            *_tmp_op_ptr = Py_STACK_TAG(_Py_NULL); \
            Py_DECREF_STACKREF(_tmp_old_op); \
        } \
    } while (0)

#if defined(Py_GIL_DISABLED)
    static inline void
    _Py_DecRef_StackRef(_PyStackRef tagged) {
        if ((tagged.bits & Py_TAG_DEFERRED) == Py_TAG_DEFERRED) {
            // Note (KJ): this assert may not hold when the GC finalizes deferred references.
            // However, it's still useful for debugging opcodes. So I'm leavingi there.
            // assert(_PyObject_HasDeferredRefcount(Py_STACK_UNTAG_BORROWED(tagged)));
            return;
        }
        assert(!_PyObject_HasDeferredRefcount(Py_STACK_UNTAG_BORROWED(tagged)));
        return Py_DECREF(Py_STACK_UNTAG_BORROWED(tagged));
    }
    #define Py_DECREF_STACKREF(op) _Py_DecRef_StackRef(op)
#else
    #define Py_DECREF_STACKREF(op) Py_DECREF(Py_STACK_UNTAG_BORROWED(op))
#endif

#if defined(Py_GIL_DISABLED)
    static inline void
    _Py_IncRef_StackRef(_PyStackRef tagged) {
        if ((tagged.bits & Py_TAG_DEFERRED) == Py_TAG_DEFERRED) {
            assert(_PyObject_HasDeferredRefcount(Py_STACK_UNTAG_BORROWED(tagged)));
            return;
        }
        assert(!_PyObject_HasDeferredRefcount(Py_STACK_UNTAG_BORROWED(tagged)));
        return Py_INCREF(Py_STACK_UNTAG_BORROWED(tagged));
    }
    #define Py_INCREF_STACKREF(op) _Py_IncRef_StackRef(op)
#else
    #define Py_INCREF_STACKREF(op) Py_INCREF(Py_STACK_UNTAG_BORROWED(op))
#endif

#define Py_XDECREF_STACKREF(op) \
    do {                      \
        if (Py_STACK_UNTAG_BORROWED(op) != NULL) { \
            Py_DECREF_STACKREF(op); \
        } \
    } while (0)

static inline _PyStackRef
Py_NewRef_StackRef(_PyStackRef obj)
{
    Py_INCREF_STACKREF(obj);
    return obj;
}

static inline _PyStackRef
Py_NewRef_StackRef_Test(_PyStackRef obj)
{
    Py_INCREF(Py_STACK_UNTAG_BORROWED(obj));
    return obj;
}

#define Py_NewRef_Tagged(op) Py_NewRef_StackRef(op)
// #define Py_NewRef_Tagged(op) Py_NewRef_StackRef_Test(op)

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TAGGED_H */
