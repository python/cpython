#ifndef Py_INTERNAL_STACKREF_H
#define Py_INTERNAL_STACKREF_H
#ifdef __cplusplus
extern "C" {
#endif

// Define this to get precise tracking of stackrefs.
// #define Py_STACKREF_DEBUG 1

// Define this to get precise tracking of closed stackrefs.
// This will use unbounded memory, as it can only grow.
// Use this to track double closes in short-lived programs
// #define Py_STACKREF_CLOSE_DEBUG 1

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_object_deferred.h"

#include <stddef.h>
#include <stdbool.h>

/*
  This file introduces a new API for handling references on the stack, called
  _PyStackRef. This API is inspired by HPy.

  There are 3 main operations, that convert _PyStackRef to PyObject* and
  vice versa:

    1. Borrow (discouraged)
    2. Steal
    3. New

  Borrow means that the reference is converted without any change in ownership.
  This is discouraged because it makes verification much harder. It also makes
  unboxed integers harder in the future.

  Steal means that ownership is transferred to something else. The total
  number of references to the object stays the same.

  New creates a new reference from the old reference. The old reference
  is still valid.

  With these 3 API, a strict stack discipline must be maintained. All
  _PyStackRef must be operated on by the new reference operations:

    1. DUP
    2. CLOSE

   DUP is roughly equivalent to Py_NewRef. It creates a new reference from an old
   reference. The old reference remains unchanged.

   CLOSE is roughly equivalent to Py_DECREF. It destroys a reference.

   Note that it is unsafe to borrow a _PyStackRef and then do normal
   CPython refcounting operations on it!
*/


#if !defined(Py_GIL_DISABLED) && defined(Py_STACKREF_DEBUG)



typedef union _PyStackRef {
    uint64_t index;
} _PyStackRef;

#define Py_TAG_BITS 0

PyAPI_FUNC(PyObject *) _Py_stackref_get_object(_PyStackRef ref);
PyAPI_FUNC(PyObject *) _Py_stackref_close(_PyStackRef ref, const char *filename, int linenumber);
PyAPI_FUNC(_PyStackRef) _Py_stackref_create(PyObject *obj, const char *filename, int linenumber);
PyAPI_FUNC(void) _Py_stackref_record_borrow(_PyStackRef ref, const char *filename, int linenumber);
extern void _Py_stackref_associate(PyInterpreterState *interp, PyObject *obj, _PyStackRef ref);

static const _PyStackRef PyStackRef_NULL = { .index = 0 };

#define PyStackRef_None ((_PyStackRef){ .index = 1 } )
#define PyStackRef_False ((_PyStackRef){ .index = 2 })
#define PyStackRef_True ((_PyStackRef){ .index = 3 })

#define LAST_PREDEFINED_STACKREF_INDEX 3

static inline int
PyStackRef_IsNull(_PyStackRef ref)
{
    return ref.index == 0;
}

static inline int
PyStackRef_IsTrue(_PyStackRef ref)
{
    return _Py_stackref_get_object(ref) == Py_True;
}

static inline int
PyStackRef_IsFalse(_PyStackRef ref)
{
    return _Py_stackref_get_object(ref) == Py_False;
}

static inline int
PyStackRef_IsNone(_PyStackRef ref)
{
    return _Py_stackref_get_object(ref) == Py_None;
}

static inline PyObject *
_PyStackRef_AsPyObjectBorrow(_PyStackRef ref, const char *filename, int linenumber)
{
    _Py_stackref_record_borrow(ref, filename, linenumber);
    return _Py_stackref_get_object(ref);
}

#define PyStackRef_AsPyObjectBorrow(REF) _PyStackRef_AsPyObjectBorrow((REF), __FILE__, __LINE__)

static inline PyObject *
_PyStackRef_AsPyObjectSteal(_PyStackRef ref, const char *filename, int linenumber)
{
    return _Py_stackref_close(ref, filename, linenumber);
}
#define PyStackRef_AsPyObjectSteal(REF) _PyStackRef_AsPyObjectSteal((REF), __FILE__, __LINE__)

static inline _PyStackRef
_PyStackRef_FromPyObjectNew(PyObject *obj, const char *filename, int linenumber)
{
    Py_INCREF(obj);
    return _Py_stackref_create(obj, filename, linenumber);
}
#define PyStackRef_FromPyObjectNew(obj) _PyStackRef_FromPyObjectNew(_PyObject_CAST(obj), __FILE__, __LINE__)

static inline _PyStackRef
_PyStackRef_FromPyObjectSteal(PyObject *obj, const char *filename, int linenumber)
{
    return _Py_stackref_create(obj, filename, linenumber);
}
#define PyStackRef_FromPyObjectSteal(obj) _PyStackRef_FromPyObjectSteal(_PyObject_CAST(obj), __FILE__, __LINE__)

static inline _PyStackRef
_PyStackRef_FromPyObjectImmortal(PyObject *obj, const char *filename, int linenumber)
{
    assert(_Py_IsImmortal(obj));
    return _Py_stackref_create(obj, filename, linenumber);
}
#define PyStackRef_FromPyObjectImmortal(obj) _PyStackRef_FromPyObjectImmortal(_PyObject_CAST(obj), __FILE__, __LINE__)

static inline void
_PyStackRef_CLOSE(_PyStackRef ref, const char *filename, int linenumber)
{
    PyObject *obj = _Py_stackref_close(ref, filename, linenumber);
    Py_DECREF(obj);
}
#define PyStackRef_CLOSE(REF) _PyStackRef_CLOSE((REF), __FILE__, __LINE__)

static inline _PyStackRef
_PyStackRef_DUP(_PyStackRef ref, const char *filename, int linenumber)
{
    PyObject *obj = _Py_stackref_get_object(ref);
    Py_INCREF(obj);
    return _Py_stackref_create(obj, filename, linenumber);
}
#define PyStackRef_DUP(REF) _PyStackRef_DUP(REF, __FILE__, __LINE__)

#define PyStackRef_CLOSE_SPECIALIZED(stackref, dealloc) PyStackRef_CLOSE(stackref)

#else

typedef union _PyStackRef {
    uintptr_t bits;
} _PyStackRef;


#define Py_TAG_DEFERRED (1)

#define Py_TAG_PTR      ((uintptr_t)0)
#define Py_TAG_BITS     ((uintptr_t)1)

#ifdef Py_GIL_DISABLED

static const _PyStackRef PyStackRef_NULL = { .bits = Py_TAG_DEFERRED};
#define PyStackRef_IsNull(stackref) ((stackref).bits == PyStackRef_NULL.bits)
#define PyStackRef_True ((_PyStackRef){.bits = ((uintptr_t)&_Py_TrueStruct) | Py_TAG_DEFERRED })
#define PyStackRef_False ((_PyStackRef){.bits = ((uintptr_t)&_Py_FalseStruct) | Py_TAG_DEFERRED })
#define PyStackRef_None ((_PyStackRef){.bits = ((uintptr_t)&_Py_NoneStruct) | Py_TAG_DEFERRED })

static inline PyObject *
PyStackRef_AsPyObjectBorrow(_PyStackRef stackref)
{
    PyObject *cleared = ((PyObject *)((stackref).bits & (~Py_TAG_BITS)));
    return cleared;
}

#define PyStackRef_IsDeferred(ref) (((ref).bits & Py_TAG_BITS) == Py_TAG_DEFERRED)

static inline PyObject *
PyStackRef_NotDeferred_AsPyObject(_PyStackRef stackref)
{
    assert(!PyStackRef_IsDeferred(stackref));
    return (PyObject *)stackref.bits;
}

static inline PyObject *
PyStackRef_AsPyObjectSteal(_PyStackRef stackref)
{
    assert(!PyStackRef_IsNull(stackref));
    if (PyStackRef_IsDeferred(stackref)) {
        return Py_NewRef(PyStackRef_AsPyObjectBorrow(stackref));
    }
    return PyStackRef_AsPyObjectBorrow(stackref);
}

static inline _PyStackRef
_PyStackRef_FromPyObjectSteal(PyObject *obj)
{
    assert(obj != NULL);
    // Make sure we don't take an already tagged value.
    assert(((uintptr_t)obj & Py_TAG_BITS) == 0);
    return (_PyStackRef){ .bits = (uintptr_t)obj };
}
#   define PyStackRef_FromPyObjectSteal(obj) _PyStackRef_FromPyObjectSteal(_PyObject_CAST(obj))

static inline _PyStackRef
PyStackRef_FromPyObjectNew(PyObject *obj)
{
    // Make sure we don't take an already tagged value.
    assert(((uintptr_t)obj & Py_TAG_BITS) == 0);
    assert(obj != NULL);
    if (_PyObject_HasDeferredRefcount(obj)) {
        return (_PyStackRef){ .bits = (uintptr_t)obj | Py_TAG_DEFERRED };
    }
    else {
        return (_PyStackRef){ .bits = (uintptr_t)(Py_NewRef(obj)) | Py_TAG_PTR };
    }
}
#define PyStackRef_FromPyObjectNew(obj) PyStackRef_FromPyObjectNew(_PyObject_CAST(obj))

static inline _PyStackRef
PyStackRef_FromPyObjectImmortal(PyObject *obj)
{
    // Make sure we don't take an already tagged value.
    assert(((uintptr_t)obj & Py_TAG_BITS) == 0);
    assert(obj != NULL);
    assert(_Py_IsImmortal(obj));
    return (_PyStackRef){ .bits = (uintptr_t)obj | Py_TAG_DEFERRED };
}
#define PyStackRef_FromPyObjectImmortal(obj) PyStackRef_FromPyObjectImmortal(_PyObject_CAST(obj))

#define PyStackRef_CLOSE(REF)                                        \
        do {                                                            \
            _PyStackRef _close_tmp = (REF);                             \
            assert(!PyStackRef_IsNull(_close_tmp));                     \
            if (!PyStackRef_IsDeferred(_close_tmp)) {                   \
                Py_DECREF(PyStackRef_AsPyObjectBorrow(_close_tmp));     \
            }                                                           \
        } while (0)

static inline _PyStackRef
PyStackRef_DUP(_PyStackRef stackref)
{
    assert(!PyStackRef_IsNull(stackref));
    if (PyStackRef_IsDeferred(stackref)) {
        assert(_Py_IsImmortal(PyStackRef_AsPyObjectBorrow(stackref)) ||
               _PyObject_HasDeferredRefcount(PyStackRef_AsPyObjectBorrow(stackref))
        );
        return stackref;
    }
    Py_INCREF(PyStackRef_AsPyObjectBorrow(stackref));
    return stackref;
}

// Convert a possibly deferred reference to a strong reference.
static inline _PyStackRef
PyStackRef_AsStrongReference(_PyStackRef stackref)
{
    return PyStackRef_FromPyObjectSteal(PyStackRef_AsPyObjectSteal(stackref));
}

#define PyStackRef_CLOSE_SPECIALIZED(stackref, dealloc) PyStackRef_CLOSE(stackref)


#else // Py_GIL_DISABLED

// With GIL
static const _PyStackRef PyStackRef_NULL = { .bits = 0 };
#define PyStackRef_IsNull(stackref) ((stackref).bits == 0)
#define PyStackRef_True ((_PyStackRef){.bits = (uintptr_t)&_Py_TrueStruct })
#define PyStackRef_False ((_PyStackRef){.bits = ((uintptr_t)&_Py_FalseStruct) })
#define PyStackRef_None ((_PyStackRef){.bits = ((uintptr_t)&_Py_NoneStruct) })

#define PyStackRef_AsPyObjectBorrow(stackref) ((PyObject *)(stackref).bits)

#define PyStackRef_AsPyObjectSteal(stackref) PyStackRef_AsPyObjectBorrow(stackref)

#define PyStackRef_FromPyObjectSteal(obj) ((_PyStackRef){.bits = ((uintptr_t)(obj))})

#define PyStackRef_FromPyObjectNew(obj) ((_PyStackRef){ .bits = (uintptr_t)(Py_NewRef(obj)) })

#define PyStackRef_FromPyObjectImmortal(obj) ((_PyStackRef){ .bits = (uintptr_t)(obj) })

#define PyStackRef_CLOSE(stackref) Py_DECREF(PyStackRef_AsPyObjectBorrow(stackref))

#define PyStackRef_DUP(stackref) PyStackRef_FromPyObjectSteal(Py_NewRef(PyStackRef_AsPyObjectBorrow(stackref)))

#define PyStackRef_CLOSE_SPECIALIZED(stackref, dealloc) _Py_DECREF_SPECIALIZED(PyStackRef_AsPyObjectBorrow(stackref), dealloc)

#endif // Py_GIL_DISABLED

// Check if a stackref is exactly the same as another stackref, including the
// the deferred bit. This can only be used safely if you know that the deferred
// bits of `a` and `b` match.
#define PyStackRef_IsExactly(a, b) \
    (assert(((a).bits & Py_TAG_BITS) == ((b).bits & Py_TAG_BITS)), (a).bits == (b).bits)

// Checks that mask out the deferred bit in the free threading build.
#define PyStackRef_IsNone(ref) (PyStackRef_AsPyObjectBorrow(ref) == Py_None)
#define PyStackRef_IsTrue(ref) (PyStackRef_AsPyObjectBorrow(ref) == Py_True)
#define PyStackRef_IsFalse(ref) (PyStackRef_AsPyObjectBorrow(ref) == Py_False)

#endif

// Converts a PyStackRef back to a PyObject *, converting the
// stackref to a new reference.
#define PyStackRef_AsPyObjectNew(stackref) Py_NewRef(PyStackRef_AsPyObjectBorrow(stackref))

#define PyStackRef_TYPE(stackref) Py_TYPE(PyStackRef_AsPyObjectBorrow(stackref))


#define PyStackRef_CLEAR(op) \
    do { \
        _PyStackRef *_tmp_op_ptr = &(op); \
        _PyStackRef _tmp_old_op = (*_tmp_op_ptr); \
        if (!PyStackRef_IsNull(_tmp_old_op)) { \
            *_tmp_op_ptr = PyStackRef_NULL; \
            PyStackRef_CLOSE(_tmp_old_op); \
        } \
    } while (0)

#define PyStackRef_XCLOSE(stackref) \
    do {                            \
        _PyStackRef _tmp = (stackref); \
        if (!PyStackRef_IsNull(_tmp)) { \
            PyStackRef_CLOSE(_tmp); \
        } \
    } while (0);



// StackRef type checks

static inline bool
PyStackRef_GenCheck(_PyStackRef stackref)
{
    return PyGen_Check(PyStackRef_AsPyObjectBorrow(stackref));
}

static inline bool
PyStackRef_BoolCheck(_PyStackRef stackref)
{
    return PyBool_Check(PyStackRef_AsPyObjectBorrow(stackref));
}

static inline bool
PyStackRef_LongCheck(_PyStackRef stackref)
{
    return PyLong_Check(PyStackRef_AsPyObjectBorrow(stackref));
}

static inline bool
PyStackRef_ExceptionInstanceCheck(_PyStackRef stackref)
{
    return PyExceptionInstance_Check(PyStackRef_AsPyObjectBorrow(stackref));
}

static inline bool
PyStackRef_CodeCheck(_PyStackRef stackref)
{
    return PyCode_Check(PyStackRef_AsPyObjectBorrow(stackref));
}

static inline bool
PyStackRef_FunctionCheck(_PyStackRef stackref)
{
    return PyFunction_Check(PyStackRef_AsPyObjectBorrow(stackref));
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_STACKREF_H */
