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


#define Py_TAG_DEFERRED (1)

#define Py_TAG_PTR      (0)
#define Py_TAG_BITS     (1)

#ifdef Py_GIL_DISABLED
    static const _PyStackRef Py_STACKREF_NULL = { .bits = 0 | Py_TAG_DEFERRED};
#else
    static const _PyStackRef Py_STACKREF_NULL = { .bits = 0 };
#endif

#define PyStackRef_IsNull(stackref) ((stackref).bits == Py_STACKREF_NULL.bits)


#ifdef Py_GIL_DISABLED
#   define PyStackRef_True ((_PyStackRef){.bits = ((uintptr_t)&_Py_TrueStruct) | Py_TAG_DEFERRED })
#else
#   define PyStackRef_True ((_PyStackRef){.bits = ((uintptr_t)&_Py_TrueStruct) })
#endif



#ifdef Py_GIL_DISABLED
#   define PyStackRef_False ((_PyStackRef){.bits = ((uintptr_t)&_Py_FalseStruct) | Py_TAG_DEFERRED })
#else
#   define PyStackRef_False ((_PyStackRef){.bits = ((uintptr_t)&_Py_FalseStruct) })
#endif


static inline _PyStackRef
PyStackRef_None(void)
{
#ifdef Py_GIL_DISABLED
    const _PyStackRef STACKREF_NONE = {.bits = ((uintptr_t)Py_None |
                                                 Py_TAG_DEFERRED)};
#else
    const _PyStackRef STACKREF_NONE = {.bits = ((uintptr_t)Py_None)};
#endif
    return STACKREF_NONE;
}

static inline int
PyStackRef_Is(_PyStackRef a, _PyStackRef b) {
    return a.bits == b.bits;
}

static inline int
PyStackRef_IsDeferred(_PyStackRef ref)
{
    return ((ref.bits & Py_TAG_BITS) == Py_TAG_DEFERRED);
}

#ifdef Py_GIL_DISABLED
// Gets a PyObject * from a _PyStackRef
static inline PyObject *
PyStackRef_AsPyObjectBorrow(_PyStackRef tagged)
{
    PyObject *cleared = ((PyObject *)((tagged).bits & (~Py_TAG_BITS)));
    return cleared;
}
#else
// Need to define as macro because WASI has very sensitive stack sizes.
#   define PyStackRef_AsPyObjectBorrow(tagged) ((PyObject *)(tagged).bits)
#endif

#ifdef Py_GIL_DISABLED
// Converts a PyStackRef back to a PyObject *, converting deferred references
// to new references.
static inline PyObject *
PyStackRef_AsPyObjectSteal(_PyStackRef tagged)
{
    if (!PyStackRef_IsNull(tagged) && PyStackRef_IsDeferred(tagged)) {
        return Py_NewRef(PyStackRef_AsPyObjectBorrow(tagged));
    }
    return PyStackRef_AsPyObjectBorrow(tagged);
}
#else
#   define PyStackRef_AsPyObjectSteal(tagged) PyStackRef_AsPyObjectBorrow(tagged)
#endif

#ifdef Py_GIL_DISABLED
// Converts a PyStackRef back to a PyObject *, converting the
// stackref to a new reference.
static inline PyObject *
PyStackRef_AsPyObjectNew(_PyStackRef tagged)
{
    return Py_NewRef(PyStackRef_AsPyObjectBorrow(tagged));
}
#else
#   define PyStackRef_AsPyObjectNew(tagged) Py_NewRef(PyStackRef_AsPyObjectBorrow(tagged))
#endif

static inline PyTypeObject *
PyStackRef_TYPE(_PyStackRef stackref)
{
    return Py_TYPE(PyStackRef_AsPyObjectBorrow(stackref));
}

// Converts a PyObject * to a PyStackRef, stealing the reference
static inline _PyStackRef
_PyStackRef_FromPyObjectSteal(PyObject *obj)
{
    // Make sure we don't take an already tagged value.
    assert(((uintptr_t)obj & Py_TAG_BITS) == 0);
    int tag = (obj == NULL || _Py_IsImmortal(obj)) ? (Py_TAG_DEFERRED) : Py_TAG_PTR;
    return ((_PyStackRef){.bits = ((uintptr_t)(obj)) | tag});
}


#ifdef Py_GIL_DISABLED
#   define PyStackRef_FromPyObjectSteal(obj) _PyStackRef_FromPyObjectSteal(_PyObject_CAST(obj))
#else
#   define PyStackRef_FromPyObjectSteal(obj) ((_PyStackRef){.bits = ((uintptr_t)(obj))})
#endif

// Converts a PyObject * to a PyStackRef, with a new reference
static inline _PyStackRef
PyStackRef_FromPyObjectNew(PyObject *obj)
{
    // Make sure we don't take an already tagged value.
    assert(((uintptr_t)obj & Py_TAG_BITS) == 0);
    assert(obj != NULL);
    // TODO (gh-117139): Add deferred objects later.
    if (_Py_IsImmortal(obj)) {
        return (_PyStackRef){ .bits = (uintptr_t)obj | Py_TAG_DEFERRED };
    }
    else {
        return (_PyStackRef){ .bits = (uintptr_t)(Py_NewRef(obj)) | Py_TAG_PTR };
    }
}

#ifdef Py_GIL_DISABLED
#   define PyStackRef_FromPyObjectNew(obj) PyStackRef_FromPyObjectNew(_PyObject_CAST(obj))
#else
#   define PyStackRef_FromPyObjectNew(obj) ((_PyStackRef){ .bits = (uintptr_t)(Py_NewRef(obj)) })
#endif

#define PyStackRef_CLEAR(op) \
    do { \
        _PyStackRef *_tmp_op_ptr = &(op); \
        _PyStackRef _tmp_old_op = (*_tmp_op_ptr); \
        if (!PyStackRef_IsNull(_tmp_old_op)) { \
            *_tmp_op_ptr = Py_STACKREF_NULL; \
            PyStackRef_CLOSE(_tmp_old_op); \
        } \
    } while (0)

#ifdef Py_GIL_DISABLED
static inline void
PyStackRef_CLOSE(_PyStackRef tagged)
{
    if (PyStackRef_IsDeferred(tagged)) {
        // No assert for being immortal or deferred here.
        // The GC unsets deferred objects right before clearing.
        return;
    }
    Py_DECREF(PyStackRef_AsPyObjectBorrow(tagged));
}
#else
#   define PyStackRef_CLOSE(tagged) Py_DECREF(PyStackRef_AsPyObjectBorrow(tagged))
#endif

#define PyStackRef_XCLOSE(stackref) \
    do {                            \
        _PyStackRef _tmp = (stackref); \
        if (!PyStackRef_IsNull(_tmp)) { \
            PyStackRef_CLOSE(_tmp); \
        } \
    } while (0);


#ifdef Py_GIL_DISABLED
static inline _PyStackRef
PyStackRef_DUP(_PyStackRef tagged)
{
    if (PyStackRef_IsDeferred(tagged)) {
        assert(PyStackRef_IsNull(tagged) ||
            _Py_IsImmortal(PyStackRef_AsPyObjectBorrow(tagged)));
        return tagged;
    }
    Py_INCREF(PyStackRef_AsPyObjectBorrow(tagged));
    return tagged;
}
#else
// Needs to be macro to not overflow on WASI debug.
#   define PyStackRef_DUP(stackref) PyStackRef_FromPyObjectSteal(Py_NewRef(PyStackRef_AsPyObjectBorrow(stackref)))
#endif

static inline _PyStackRef
PyStackRef_XDUP(_PyStackRef tagged)
{
    if (!PyStackRef_IsNull(tagged)) {
        return PyStackRef_DUP(tagged);
    }
    return tagged;
}


static inline void
_PyObjectStack_FromStackRefStack(PyObject **dst, const _PyStackRef *src, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        dst[i] = PyStackRef_AsPyObjectBorrow(src[i]);
    }
}


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_STACKREF_H */
