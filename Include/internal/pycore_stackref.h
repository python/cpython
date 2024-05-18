#ifndef Py_INTERNAL_STACKREF_H
#define Py_INTERNAL_STACKREF_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stddef.h>

typedef union {
    uintptr_t bits;
} _PyStackRef;

static const _PyStackRef Py_STACKREF_NULL = { .bits = 0 };

#define Py_TAG_DEFERRED (1)

// Gets a PyObject * from a _PyStackRef
#if defined(Py_GIL_DISABLED)
static inline PyObject *
PyStackRef_Get(_PyStackRef tagged)
{
    PyObject *cleared = ((PyObject *)((tagged).bits & (~Py_TAG_DEFERRED)));
    return cleared;
}
#else
#   define PyStackRef_Get(tagged) ((PyObject *)((tagged).bits))
#endif

// Converts a PyObject * to a PyStackRef, stealing the reference.
#if defined(Py_GIL_DISABLED)
static inline _PyStackRef
_PyStackRef_StealRef(PyObject *obj)
{
    // Make sure we don't take an already tagged value.
    assert(((uintptr_t)obj & Py_TAG_DEFERRED) == 0);
    return ((_PyStackRef){.bits = ((uintptr_t)(obj))});
}
#   define PyStackRef_StealRef(obj) _PyStackRef_StealRef(_PyObject_CAST(obj))
#else
#   define PyStackRef_StealRef(obj) ((_PyStackRef){.bits = ((uintptr_t)(obj))})
#endif

// Converts a PyObject * to a PyStackRef, with a new reference
#if defined(Py_GIL_DISABLED)
static inline _PyStackRef
_PyStackRef_NewRefDeferred(PyObject *obj)
{
    // Make sure we don't take an already tagged value.
    assert(((uintptr_t)obj & Py_TAG_DEFERRED) == 0);
    assert(obj != NULL);
    if (_PyObject_HasDeferredRefcount(obj)) {
        return (_PyStackRef){ .bits = (uintptr_t)obj | Py_TAG_DEFERRED };
    }
    else {
        return (_PyStackRef){ .bits = (uintptr_t)Py_NewRef(obj) };
    }
}
#   define PyStackRef_NewRefDeferred(obj) _PyStackRef_NewRefDeferred(_PyObject_CAST(obj))
#else
#   define PyStackRef_NewRefDeferred(obj) PyStackRef_NewRef(((_PyStackRef){.bits = ((uintptr_t)(obj))}))
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
    return _PyStackRef_NewRefDeferred(obj);
}
#   define PyStackRef_XNewRefDeferred(obj) _PyStackRef_XNewRefDeferred(_PyObject_CAST(obj))
#else
#   define PyStackRef_XNewRefDeferred(obj) PyStackRef_XNewRef(((_PyStackRef){.bits = ((uintptr_t)(obj))}))
#endif

// Converts a PyStackRef back to a PyObject *.
#if defined(Py_GIL_DISABLED)
static inline PyObject *
PyStackRef_StealObject(_PyStackRef tagged)
{
    if ((tagged.bits & Py_TAG_DEFERRED) == Py_TAG_DEFERRED) {
        assert(_PyObject_HasDeferredRefcount(PyStackRef_Get(tagged)));
        return Py_NewRef(PyStackRef_Get(tagged));
    }
    return PyStackRef_Get(tagged);
}
#else
#   define PyStackRef_StealObject(tagged) PyStackRef_Get(tagged)
#endif

static inline void
_Py_untag_stack_borrowed(PyObject **dst, const _PyStackRef *src, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        dst[i] = PyStackRef_Get(src[i]);
    }
}

static inline void
_Py_untag_stack_steal(PyObject **dst, const _PyStackRef *src, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        dst[i] = PyStackRef_StealObject(src[i]);
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
    if ((tagged.bits & Py_TAG_DEFERRED) == Py_TAG_DEFERRED) {
        return;
    }
    Py_DECREF(PyStackRef_Get(tagged));
}
#else
#   define PyStackRef_DECREF(op) Py_DECREF(PyStackRef_Get(op))
#endif

#if defined(Py_GIL_DISABLED)
static inline void
PyStackRef_INCREF(_PyStackRef tagged)
{
    if ((tagged.bits & Py_TAG_DEFERRED) == Py_TAG_DEFERRED) {
        assert(_PyObject_HasDeferredRefcount(PyStackRef_Get(tagged)));
        return;
    }
    Py_INCREF(PyStackRef_Get(tagged));
}
#else
#   define PyStackRef_INCREF(op) Py_INCREF(PyStackRef_Get(op))
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

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_STACKREF_H */
