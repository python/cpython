#ifndef Py_INTERNAL_STACKREF_H
#define Py_INTERNAL_STACKREF_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_object_deferred.h"

#include <stddef.h>

typedef union {
    uintptr_t bits;
} _PyStackRef;

static const _PyStackRef Py_STACKREF_NULL = { .bits = 0 };

#define Py_TAG_DEFERRED (1)

static inline int
PyStackRef_IsNull(_PyStackRef stackref)
{
    return stackref.bits == 0;
}

static inline int
PyStackRef_IsDeferred(_PyStackRef ref)
{
    return ((ref.bits & Py_TAG_DEFERRED) == Py_TAG_DEFERRED);
}

// Gets a PyObject * from a _PyStackRef
#if defined(Py_GIL_DISABLED)
static inline PyObject *
PyStackRef_To_PyObject_Steal(_PyStackRef tagged)
{
    PyObject *cleared = ((PyObject *)((tagged).bits & (~Py_TAG_DEFERRED)));
    return cleared;
}
#else
#   define PyStackRef_To_PyObject_Steal(tagged) ((PyObject *)((tagged).bits))
#endif

// Converts a PyObject * to a PyStackRef, stealing the reference.
#if defined(Py_GIL_DISABLED)
static inline _PyStackRef
_PyObject_To_StackRef_Steal(PyObject *obj)
{
    // Make sure we don't take an already tagged value.
    assert(((uintptr_t)obj & Py_TAG_DEFERRED) == 0);
    return ((_PyStackRef){.bits = ((uintptr_t)(obj))});
}
#   define PyObject_To_StackRef_Steal(obj) _PyObject_To_StackRef_Steal(_PyObject_CAST(obj))
#else
#   define PyObject_To_StackRef_Steal(obj) ((_PyStackRef){.bits = ((uintptr_t)(obj))})
#endif

// Converts a PyObject * to a PyStackRef, with a new reference
#if defined(Py_GIL_DISABLED)
static inline _PyStackRef
PyObject_To_StackRef_New(PyObject *obj)
{
    // Make sure we don't take an already tagged value.
    assert(((uintptr_t)obj & Py_TAG_DEFERRED) == 0);
    assert(obj != NULL);
    if (_PyObject_HasDeferredRefcount(obj) || _Py_IsImmortal(obj)) {
        return (_PyStackRef){ .bits = (uintptr_t)obj | Py_TAG_DEFERRED };
    }
    else {
        return (_PyStackRef){ .bits = (uintptr_t)Py_NewRef(obj) };
    }
}
#   define PyObject_To_StackRef_New(obj) PyObject_To_StackRef_New(_PyObject_CAST(obj))
#else
#   define PyObject_To_StackRef_New(obj) PyStackRef_NewRef(((_PyStackRef){.bits = ((uintptr_t)(obj))}))
#endif

#if defined(Py_GIL_DISABLED)
static inline _PyStackRef
_PyStackRef_XNewRefDeferred(PyObject *obj)
{
    // Make sure we don't take an already tagged value.
    assert(((uintptr_t)obj & Py_TAG_DEFERRED) == 0);
    if (obj == NULL) {
        return Py_STACKREF_NULL;
    }
    return PyObject_To_StackRef_New(obj);
}
#   define PyStackRef_XNewRefDeferred(obj) _PyStackRef_XNewRefDeferred(_PyObject_CAST(obj))
#else
#   define PyStackRef_XNewRefDeferred(obj) PyStackRef_XNewRef(((_PyStackRef){.bits = ((uintptr_t)(obj))}))
#endif

// Converts a PyStackRef back to a PyObject *.
#if defined(Py_GIL_DISABLED)
static inline PyObject *
PyStackRef_To_PyObject_New(_PyStackRef tagged)
{
    if (PyStackRef_IsDeferred(tagged)) {
        assert(_PyObject_HasDeferredRefcount(PyStackRef_To_PyObject_Steal(tagged)) ||
            _Py_IsImmortal(PyStackRef_To_PyObject_Steal(tagged)));
        return Py_NewRef(PyStackRef_To_PyObject_Steal(tagged));
    }
    return PyStackRef_To_PyObject_Steal(tagged);
}
#else
#   define PyStackRef_To_PyObject_New(tagged) PyStackRef_To_PyObject_Steal(tagged)
#endif

static inline void
_Py_untag_stack_borrowed(PyObject **dst, const _PyStackRef *src, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        dst[i] = PyStackRef_To_PyObject_Steal(src[i]);
    }
}

static inline void
_Py_untag_stack_steal(PyObject **dst, const _PyStackRef *src, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        dst[i] = PyStackRef_To_PyObject_New(src[i]);
    }
}


#define PyStackRef_XSETREF(dst, src) \
    do { \
        _PyStackRef *_tmp_dst_ptr = &(dst); \
        _PyStackRef _tmp_old_dst = (*_tmp_dst_ptr); \
        *_tmp_dst_ptr = (src); \
        PyStackRef_XDECREF(_tmp_old_dst); \
    } while (0)

#define PyStackRef_SETREF(dst, src) \
    do { \
        _PyStackRef *_tmp_dst_ptr = &(dst); \
        _PyStackRef _tmp_old_dst = (*_tmp_dst_ptr); \
        *_tmp_dst_ptr = (src); \
        PyStackRef_DECREF(_tmp_old_dst); \
    } while (0)

#define PyStackRef_CLEAR(op) \
    do { \
        _PyStackRef *_tmp_op_ptr = &(op); \
        _PyStackRef _tmp_old_op = (*_tmp_op_ptr); \
        if (_tmp_old_op.bits != Py_STACKREF_NULL.bits) { \
            *_tmp_op_ptr = Py_STACKREF_NULL; \
            PyStackRef_DECREF(_tmp_old_op); \
        } \
    } while (0)

#if defined(Py_GIL_DISABLED)
static inline void
PyStackRef_DECREF(_PyStackRef tagged)
{
    if (PyStackRef_IsDeferred(tagged)) {
        // No assert for being immortal or deferred here.
        // The GC unsets deferred objects right before clearing.
        return;
    }
    Py_DECREF(PyStackRef_To_PyObject_Steal(tagged));
}
#else
#   define PyStackRef_DECREF(op) Py_DECREF(PyStackRef_To_PyObject_Steal(op))
#endif

#if defined(Py_GIL_DISABLED)
static inline void
PyStackRef_INCREF(_PyStackRef tagged)
{
    if (PyStackRef_IsDeferred(tagged)) {
        assert(_PyObject_HasDeferredRefcount(PyStackRef_To_PyObject_Steal(tagged)) ||
                   _Py_IsImmortal(PyStackRef_To_PyObject_Steal(tagged)));
        return;
    }
    Py_INCREF(PyStackRef_To_PyObject_Steal(tagged));
}
#else
#   define PyStackRef_INCREF(op) Py_INCREF(PyStackRef_To_PyObject_Steal(op))
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
    if (PyStackRef_IsNull(obj)) {
        return obj;
    }
    return PyStackRef_NewRef(obj);
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_STACKREF_H */
