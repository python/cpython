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


#define Py_TAG_INT      (0)
#define Py_TAG_DEFERRED (1)
#define Py_TAG_RESERVED (2)
#define Py_TAG_PTR      (3)

#define Py_TAG          (Py_TAG_PTR)

static const _PyStackRef Py_STACKREF_NULL = { .bits = 0 | Py_TAG_DEFERRED};

static inline int
PyStackRef_IsNull(_PyStackRef stackref)
{
    return stackref.bits == Py_STACKREF_NULL.bits;
}

static inline int
PyStackRef_IsDeferred(_PyStackRef ref)
{
    return ((ref.bits & Py_TAG) == Py_TAG_DEFERRED);
}

// Gets a PyObject * from a _PyStackRef
static inline PyObject *
PyStackRef_To_PyObject_Borrow(_PyStackRef tagged)
{
    PyObject *cleared = ((PyObject *)((tagged).bits & (~Py_TAG)));
    return cleared;
}

// Converts a PyObject * to a PyStackRef, stealing the reference
static inline _PyStackRef
_PyObject_To_StackRef_Steal(PyObject *obj)
{
    // Make sure we don't take an already tagged value.
    assert(((uintptr_t)obj & Py_TAG) == 0);
    int tag = (obj == NULL || _Py_IsImmortal(obj)) ? (Py_TAG_DEFERRED) : Py_TAG_PTR;
    return ((_PyStackRef){.bits = ((uintptr_t)(obj)) | tag});
}
#define PyObject_To_StackRef_Steal(obj) _PyObject_To_StackRef_Steal(_PyObject_CAST(obj))

// Converts a PyObject * to a PyStackRef, with a new reference
static inline _PyStackRef
PyObject_To_StackRef_New(PyObject *obj)
{
    // Make sure we don't take an already tagged value.
    assert(((uintptr_t)obj & Py_TAG) == 0);
    assert(obj != NULL);
    // TODO (gh-117139): Add deferred objects later.
    if (_Py_IsImmortal(obj)) {
        return (_PyStackRef){ .bits = (uintptr_t)obj | Py_TAG_DEFERRED };
    }
    else {
        return (_PyStackRef){ .bits = (uintptr_t)(Py_NewRef(obj)) | Py_TAG_PTR };
    }
}
#define PyObject_To_StackRef_New(obj) PyObject_To_StackRef_New(_PyObject_CAST(obj))


// Converts a PyStackRef back to a PyObject *, converting deferred references
// to new references.
static inline PyObject *
PyStackRef_To_PyObject_New(_PyStackRef tagged)
{
    if (!PyStackRef_IsNull(tagged) && PyStackRef_IsDeferred(tagged)) {
        assert(PyStackRef_IsNull(tagged) ||
            _Py_IsImmortal(PyStackRef_To_PyObject_Borrow(tagged)));
        return Py_NewRef(PyStackRef_To_PyObject_Borrow(tagged));
    }
    return PyStackRef_To_PyObject_Borrow(tagged);
}

static inline void
_Py_untag_stack_borrowed(PyObject **dst, const _PyStackRef *src, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        dst[i] = PyStackRef_To_PyObject_Borrow(src[i]);
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
        PyStackRef_DECREF(_tmp_old_dst); \
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
        if (!PyStackRef_IsNull(_tmp_old_op)) { \
            *_tmp_op_ptr = Py_STACKREF_NULL; \
            PyStackRef_DECREF(_tmp_old_op); \
        } \
    } while (0)

static inline void
PyStackRef_DECREF(_PyStackRef tagged)
{
    if (PyStackRef_IsDeferred(tagged)) {
        // No assert for being immortal or deferred here.
        // The GC unsets deferred objects right before clearing.
        return;
    }
    Py_DECREF(PyStackRef_To_PyObject_Borrow(tagged));
}

static inline void
PyStackRef_INCREF(_PyStackRef tagged)
{
    if (PyStackRef_IsDeferred(tagged)) {
        assert(PyStackRef_IsNull(tagged) ||
            _Py_IsImmortal(PyStackRef_To_PyObject_Borrow(tagged)));
        return;
    }
    Py_INCREF(PyStackRef_To_PyObject_Borrow(tagged));
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
