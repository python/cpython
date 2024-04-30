#ifndef Py_INTERNAL_STACKREF_H
#define Py_INTERNAL_STACKREF_H
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

static const _PyStackRef Py_STACKREF_NULL = ((_PyStackRef) { .bits = 0 });

#ifdef Py_GIL_DISABLED
    #define Py_TAG_DEFERRED (1)
    #define Py_TAG (Py_TAG_DEFERRED)
#else
    #define Py_TAG 0
#endif

// Gets a PyObject * from a _PyStackRef
#if defined(Py_GIL_DISABLED)
    static inline PyObject *
    PyStackRef_Get(_PyStackRef tagged) {
        PyObject *cleared = ((PyObject *)((tagged).bits & (~Py_TAG)));
        return cleared;
    }
#else
    #define PyStackRef_Get(tagged) ((PyObject *)(uintptr_t)((tagged).bits))
#endif

// Converts a PyObject * to a PyStackRef, stealing the reference.
#if defined(Py_GIL_DISABLED)
    static inline _PyStackRef
    _PyStackRef_StealRef(PyObject *obj) {
        // Make sure we don't take an already tagged value.
        assert(PyStackRef_Get(((_PyStackRef){.bits = ((uintptr_t)(obj))})) == obj);
        return ((_PyStackRef){.bits = ((uintptr_t)(obj))});
    }
    #define PyStackRef_StealRef(obj) _PyStackRef_StealRef(_PyObject_CAST(obj))
#else
    #define PyStackRef_StealRef(obj) ((_PyStackRef){.bits = ((uintptr_t)(obj))})
#endif

#if defined(Py_GIL_DISABLED)
    static inline PyObject *
    PyStackRef_StealObject(_PyStackRef tagged) {
        if ((tagged.bits & Py_TAG_DEFERRED) == Py_TAG_DEFERRED) {
            assert(_PyObject_HasDeferredRefcount(PyStackRef_Get(tagged)));
            return Py_NewRef(PyStackRef_Get(tagged));
        }
        return PyStackRef_Get(tagged);
    }
#else
    #define PyStackRef_StealObject(tagged) PyStackRef_Get(tagged)
#endif

static inline void
_Py_untag_stack_borrowed(PyObject **dst, const _PyStackRef *src, size_t length) {
    for (size_t i = 0; i < length; i++) {
        dst[i] = PyStackRef_Get(src[i]);
    }
}

static inline void
_Py_untag_stack_steal(PyObject **dst, const _PyStackRef *src, size_t length) {
    for (size_t i = 0; i < length; i++) {
        dst[i] = PyStackRef_StealObject(src[i]);
    }
}


#define PyStackRef_XSETREF(dst, src) \
    do { \
        _PyStackRef *_tmp_dst_ptr = _Py_CAST(_PyStackRef*, &(dst)); \
        _PyStackRef _tmp_old_dst = (*_tmp_dst_ptr); \
        *_tmp_dst_ptr = (src); \
        PyStackRef_XDECREF(_tmp_old_dst); \
    } while (0)

#define PyStackRef_SETREF(dst, src) \
    do { \
        _PyStackRef *_tmp_dst_ptr = _Py_CAST(_PyStackRef*, &(dst)); \
        _PyStackRef _tmp_old_dst = (*_tmp_dst_ptr); \
        *_tmp_dst_ptr = (src); \
        PyStackRef_DECREF(_tmp_old_dst); \
    } while (0)

#define PyStackRef_CLEAR(op) \
    do { \
        _PyStackRef *_tmp_op_ptr = _Py_CAST(_PyStackRef*, &(op)); \
        _PyStackRef _tmp_old_op = (*_tmp_op_ptr); \
        if (PyStackRef_Get(_tmp_old_op) != NULL) { \
            *_tmp_op_ptr = PyStackRef_StealRef(_Py_NULL); \
            PyStackRef_DECREF(_tmp_old_op); \
        } \
    } while (0)

#if defined(Py_GIL_DISABLED)
    static inline void
    PyStackRef_DECREF(_PyStackRef tagged) {
        if ((tagged.bits & Py_TAG_DEFERRED) == Py_TAG_DEFERRED) {
            return;
        }
        Py_DECREF(PyStackRef_Get(tagged));
    }
#else
    #define PyStackRef_DECREF(op) Py_DECREF(PyStackRef_Get(op))
#endif

#define PyStackRef_DECREF_OWNED(op) Py_DECREF(PyStackRef_Get(op));

#if defined(Py_GIL_DISABLED)
    static inline void
    PyStackRef_INCREF(_PyStackRef tagged) {
        if ((tagged.bits & Py_TAG_DEFERRED) == Py_TAG_DEFERRED) {
            assert(_PyObject_HasDeferredRefcount(PyStackRef_Get(tagged)));
            return;
        }
        Py_INCREF(PyStackRef_Get(tagged));
    }
#else
    #define PyStackRef_INCREF(op) Py_INCREF(PyStackRef_Get(op))
#endif

static inline void
PyStackRef_XDECREF(_PyStackRef op)
{
    if (op.bits != Py_STACKREF_NULL.bits) {
        PyStackRef_DECREF(op);
    }
}

static inline _PyStackRef
PyStackRef_NewRef(_PyStackRef obj)
{
    PyStackRef_INCREF(obj);
    return obj;
}

static inline _PyStackRef
PyStackRef_XNewRef(_PyStackRef obj)
{
    if (obj.bits == Py_STACKREF_NULL.bits) {
        return obj;
    }
    return PyStackRef_NewRef(obj);
}

// Converts a PyObject * to a PyStackRef, with a new reference
#if defined(Py_GIL_DISABLED)
    static inline _PyStackRef
    _PyStackRef_NewRefDeferred(PyObject *obj) {
        // Make sure we don't take an already tagged value.
        assert(PyStackRef_Get(((_PyStackRef){.bits = ((uintptr_t)(obj))})) == obj);
        int is_deferred = (obj != NULL && _PyObject_HasDeferredRefcount(obj));
        int tag = (is_deferred ? Py_TAG_DEFERRED : 0);
        return PyStackRef_XNewRef((_PyStackRef){.bits = ((uintptr_t)(obj) | tag)});
    }
    #define PyStackRef_NewRefDeferred(obj) _PyStackRef_NewRefDeferred(_PyObject_CAST(obj))
#else
    #define PyStackRef_NewRefDeferred(obj) PyStackRef_NewRef(((_PyStackRef){.bits = ((uintptr_t)(obj))}))
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_STACKREF_H */
