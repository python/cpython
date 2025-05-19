#ifndef Py_INTERNAL_STACKREF_H
#define Py_INTERNAL_STACKREF_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_object.h"        // Py_DECREF_MORTAL
#include "pycore_object_deferred.h" // _PyObject_HasDeferredRefcount()

#include <stdbool.h>              // bool


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
  number of references to the object stays the same.  The old reference is no
  longer valid.

  New creates a new reference from the old reference. The old reference
  is still valid.

  All _PyStackRef must be operated on by the new reference operations:

    1. DUP
    2. CLOSE

   DUP is roughly equivalent to Py_NewRef. It creates a new reference from an old
   reference. The old reference remains unchanged.

   CLOSE is roughly equivalent to Py_DECREF. It destroys a reference.

   Note that it is unsafe to borrow a _PyStackRef and then do normal
   CPython refcounting operations on it!
*/


#if !defined(Py_GIL_DISABLED) && defined(Py_STACKREF_DEBUG)

#define Py_TAG_BITS 0

PyAPI_FUNC(PyObject *) _Py_stackref_get_object(_PyStackRef ref);
PyAPI_FUNC(PyObject *) _Py_stackref_close(_PyStackRef ref, const char *filename, int linenumber);
PyAPI_FUNC(_PyStackRef) _Py_stackref_create(PyObject *obj, const char *filename, int linenumber);
PyAPI_FUNC(void) _Py_stackref_record_borrow(_PyStackRef ref, const char *filename, int linenumber);
extern void _Py_stackref_associate(PyInterpreterState *interp, PyObject *obj, _PyStackRef ref);

static const _PyStackRef PyStackRef_NULL = { .index = 0 };

// Use the first 3 even numbers for None, True and False.
// Odd numbers are reserved for (tagged) integers
#define PyStackRef_None ((_PyStackRef){ .index = 2 } )
#define PyStackRef_False ((_PyStackRef){ .index = 4 })
#define PyStackRef_True ((_PyStackRef){ .index = 6 })

#define INITIAL_STACKREF_INDEX 8

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

static inline bool
PyStackRef_IsTaggedInt(_PyStackRef ref)
{
    return (ref.index & 1) == 1;
}

static inline PyObject *
_PyStackRef_AsPyObjectBorrow(_PyStackRef ref, const char *filename, int linenumber)
{
    assert(!PyStackRef_IsTaggedInt(ref));
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
    if (PyStackRef_IsTaggedInt(ref)) {
        return;
    }
    PyObject *obj = _Py_stackref_close(ref, filename, linenumber);
    Py_DECREF(obj);
}
#define PyStackRef_CLOSE(REF) _PyStackRef_CLOSE((REF), __FILE__, __LINE__)


static inline void
_PyStackRef_XCLOSE(_PyStackRef ref, const char *filename, int linenumber)
{
    if (PyStackRef_IsNull(ref)) {
        return;
    }
    _PyStackRef_CLOSE(ref, filename, linenumber);
}
#define PyStackRef_XCLOSE(REF) _PyStackRef_XCLOSE((REF), __FILE__, __LINE__)

static inline _PyStackRef
_PyStackRef_DUP(_PyStackRef ref, const char *filename, int linenumber)
{
    if (PyStackRef_IsTaggedInt(ref)) {
        return ref;
    }
    else {
        PyObject *obj = _Py_stackref_get_object(ref);
        Py_INCREF(obj);
        return _Py_stackref_create(obj, filename, linenumber);
    }
}
#define PyStackRef_DUP(REF) _PyStackRef_DUP(REF, __FILE__, __LINE__)

extern void _PyStackRef_CLOSE_SPECIALIZED(_PyStackRef ref, destructor destruct, const char *filename, int linenumber);
#define PyStackRef_CLOSE_SPECIALIZED(REF, DESTRUCT) _PyStackRef_CLOSE_SPECIALIZED(REF, DESTRUCT, __FILE__, __LINE__)

static inline _PyStackRef
PyStackRef_MakeHeapSafe(_PyStackRef ref)
{
    return ref;
}

static inline _PyStackRef
PyStackRef_Borrow(_PyStackRef ref)
{
    return PyStackRef_DUP(ref);
}

#define PyStackRef_CLEAR(REF) \
    do { \
        _PyStackRef *_tmp_op_ptr = &(REF); \
        _PyStackRef _tmp_old_op = (*_tmp_op_ptr); \
        *_tmp_op_ptr = PyStackRef_NULL; \
        PyStackRef_XCLOSE(_tmp_old_op); \
    } while (0)

static inline _PyStackRef
_PyStackRef_FromPyObjectStealMortal(PyObject *obj, const char *filename, int linenumber)
{
    assert(!_Py_IsImmortal(obj));
    return _Py_stackref_create(obj, filename, linenumber);
}
#define PyStackRef_FromPyObjectStealMortal(obj) _PyStackRef_FromPyObjectStealMortal(_PyObject_CAST(obj), __FILE__, __LINE__)

static inline bool
PyStackRef_IsHeapSafe(_PyStackRef ref)
{
    return true;
}

static inline _PyStackRef
_PyStackRef_FromPyObjectNewMortal(PyObject *obj, const char *filename, int linenumber)
{
    assert(!_Py_IsStaticImmortal(obj));
    Py_INCREF(obj);
    return _Py_stackref_create(obj, filename, linenumber);
}
#define PyStackRef_FromPyObjectNewMortal(obj) _PyStackRef_FromPyObjectNewMortal(_PyObject_CAST(obj), __FILE__, __LINE__)

#define PyStackRef_RefcountOnObject(REF) 1

extern int PyStackRef_Is(_PyStackRef a, _PyStackRef b);

extern bool PyStackRef_IsTaggedInt(_PyStackRef ref);

extern intptr_t PyStackRef_UntagInt(_PyStackRef ref);

extern _PyStackRef PyStackRef_TagInt(intptr_t i);

extern bool
PyStackRef_IsNullOrInt(_PyStackRef ref);

#else

#define Py_INT_TAG 3

static inline bool
PyStackRef_IsTaggedInt(_PyStackRef i)
{
    return (i.bits & Py_INT_TAG) == Py_INT_TAG;
}

static inline _PyStackRef
PyStackRef_TagInt(intptr_t i)
{
    assert(Py_ARITHMETIC_RIGHT_SHIFT(intptr_t, (i << 2), 2) == i);
    return (_PyStackRef){ .bits = ((((uintptr_t)i) << 2) | Py_INT_TAG) };
}

static inline intptr_t
PyStackRef_UntagInt(_PyStackRef i)
{
    assert(PyStackRef_IsTaggedInt(i));
    intptr_t val = (intptr_t)i.bits;
    return Py_ARITHMETIC_RIGHT_SHIFT(intptr_t, val, 2);
}


#ifdef Py_GIL_DISABLED

#define Py_TAG_DEFERRED (1)

#define Py_TAG_PTR      ((uintptr_t)0)
#define Py_TAG_BITS     ((uintptr_t)1)


static const _PyStackRef PyStackRef_NULL = { .bits = Py_TAG_DEFERRED};
#define PyStackRef_IsNull(stackref) ((stackref).bits == PyStackRef_NULL.bits)
#define PyStackRef_True ((_PyStackRef){.bits = ((uintptr_t)&_Py_TrueStruct) | Py_TAG_DEFERRED })
#define PyStackRef_False ((_PyStackRef){.bits = ((uintptr_t)&_Py_FalseStruct) | Py_TAG_DEFERRED })
#define PyStackRef_None ((_PyStackRef){.bits = ((uintptr_t)&_Py_NoneStruct) | Py_TAG_DEFERRED })

// Checks that mask out the deferred bit in the free threading build.
#define PyStackRef_IsNone(ref) (PyStackRef_AsPyObjectBorrow(ref) == Py_None)
#define PyStackRef_IsTrue(ref) (PyStackRef_AsPyObjectBorrow(ref) == Py_True)
#define PyStackRef_IsFalse(ref) (PyStackRef_AsPyObjectBorrow(ref) == Py_False)

#define PyStackRef_IsNullOrInt(stackref) (PyStackRef_IsNull(stackref) || PyStackRef_IsTaggedInt(stackref))

static inline PyObject *
PyStackRef_AsPyObjectBorrow(_PyStackRef stackref)
{
    assert(!PyStackRef_IsTaggedInt(stackref));
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

static inline bool
PyStackRef_IsHeapSafe(_PyStackRef stackref)
{
    if (PyStackRef_IsDeferred(stackref)) {
        PyObject *obj = PyStackRef_AsPyObjectBorrow(stackref);
        return obj == NULL || _Py_IsImmortal(obj) || _PyObject_HasDeferredRefcount(obj);
    }
    return true;
}

static inline _PyStackRef
PyStackRef_MakeHeapSafe(_PyStackRef stackref)
{
    if (PyStackRef_IsHeapSafe(stackref)) {
        return stackref;
    }
    PyObject *obj = PyStackRef_AsPyObjectBorrow(stackref);
    return (_PyStackRef){ .bits = (uintptr_t)(Py_NewRef(obj)) | Py_TAG_PTR };
}

static inline _PyStackRef
PyStackRef_FromPyObjectStealMortal(PyObject *obj)
{
    assert(obj != NULL);
    assert(!_Py_IsImmortal(obj));
    // Make sure we don't take an already tagged value.
    assert(((uintptr_t)obj & Py_TAG_BITS) == 0);
    return (_PyStackRef){ .bits = (uintptr_t)obj };
}

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

static inline void
PyStackRef_CLOSE_SPECIALIZED(_PyStackRef ref, destructor destruct)
{
    (void)destruct;
    PyStackRef_CLOSE(ref);
}

static inline _PyStackRef
PyStackRef_DUP(_PyStackRef stackref)
{
    assert(!PyStackRef_IsNull(stackref));
    if (PyStackRef_IsDeferred(stackref)) {
        return stackref;
    }
    Py_INCREF(PyStackRef_AsPyObjectBorrow(stackref));
    return stackref;
}

static inline _PyStackRef
PyStackRef_Borrow(_PyStackRef stackref)
{
    return (_PyStackRef){ .bits = stackref.bits | Py_TAG_DEFERRED };
}

// Convert a possibly deferred reference to a strong reference.
static inline _PyStackRef
PyStackRef_AsStrongReference(_PyStackRef stackref)
{
    return PyStackRef_FromPyObjectSteal(PyStackRef_AsPyObjectSteal(stackref));
}

#define PyStackRef_XCLOSE(stackref) \
    do {                            \
        _PyStackRef _tmp = (stackref); \
        if (!PyStackRef_IsNull(_tmp)) { \
            PyStackRef_CLOSE(_tmp); \
        } \
    } while (0);

#define PyStackRef_CLEAR(op) \
    do { \
        _PyStackRef *_tmp_op_ptr = &(op); \
        _PyStackRef _tmp_old_op = (*_tmp_op_ptr); \
        if (!PyStackRef_IsNull(_tmp_old_op)) { \
            *_tmp_op_ptr = PyStackRef_NULL; \
            PyStackRef_CLOSE(_tmp_old_op); \
        } \
    } while (0)

#define PyStackRef_FromPyObjectNewMortal PyStackRef_FromPyObjectNew

#else // Py_GIL_DISABLED

// With GIL

/* References to immortal objects always have their tag bit set to Py_TAG_REFCNT
 * as they can (must) have their reclamation deferred */

#define Py_TAG_BITS 1
#define Py_TAG_REFCNT 1
#if _Py_IMMORTAL_FLAGS != Py_TAG_REFCNT
#  error "_Py_IMMORTAL_FLAGS != Py_TAG_REFCNT"
#endif

#define BITS_TO_PTR(REF) ((PyObject *)((REF).bits))
#define BITS_TO_PTR_MASKED(REF) ((PyObject *)(((REF).bits) & (~Py_TAG_BITS)))

#define PyStackRef_NULL_BITS Py_TAG_REFCNT
static const _PyStackRef PyStackRef_NULL = { .bits = PyStackRef_NULL_BITS };

#define PyStackRef_IsNull(ref) ((ref).bits == PyStackRef_NULL_BITS)
#define PyStackRef_True ((_PyStackRef){.bits = ((uintptr_t)&_Py_TrueStruct) | Py_TAG_REFCNT })
#define PyStackRef_False ((_PyStackRef){.bits = ((uintptr_t)&_Py_FalseStruct) | Py_TAG_REFCNT })
#define PyStackRef_None ((_PyStackRef){.bits = ((uintptr_t)&_Py_NoneStruct) | Py_TAG_REFCNT })

#define PyStackRef_IsTrue(REF) ((REF).bits == (((uintptr_t)&_Py_TrueStruct) | Py_TAG_REFCNT))
#define PyStackRef_IsFalse(REF) ((REF).bits == (((uintptr_t)&_Py_FalseStruct) | Py_TAG_REFCNT))
#define PyStackRef_IsNone(REF) ((REF).bits == (((uintptr_t)&_Py_NoneStruct) | Py_TAG_REFCNT))

#ifdef Py_DEBUG

static inline void PyStackRef_CheckValid(_PyStackRef ref) {
    assert(ref.bits != 0);
    int tag = ref.bits & Py_TAG_BITS;
    PyObject *obj = BITS_TO_PTR_MASKED(ref);
    switch (tag) {
        case 0:
            /* Can be immortal if object was made immortal after reference came into existence */
            assert(!_Py_IsStaticImmortal(obj));
            break;
        case Py_TAG_REFCNT:
            break;
        default:
            assert(0);
    }
}

#else

#define PyStackRef_CheckValid(REF) ((void)0)

#endif

#ifdef _WIN32
#define PyStackRef_RefcountOnObject(REF) (((REF).bits & Py_TAG_REFCNT) == 0)
#define PyStackRef_AsPyObjectBorrow BITS_TO_PTR_MASKED
#define PyStackRef_Borrow(REF) (_PyStackRef){ .bits = ((REF).bits) | Py_TAG_REFCNT};
#else
/* Does this ref not have an embedded refcount and thus not refer to a declared immmortal object? */
static inline int
PyStackRef_RefcountOnObject(_PyStackRef ref)
{
    return (ref.bits & Py_TAG_REFCNT) == 0;
}

static inline PyObject *
PyStackRef_AsPyObjectBorrow(_PyStackRef ref)
{
    assert(!PyStackRef_IsTaggedInt(ref));
    return BITS_TO_PTR_MASKED(ref);
}

static inline _PyStackRef
PyStackRef_Borrow(_PyStackRef ref)
{
    return (_PyStackRef){ .bits = ref.bits | Py_TAG_REFCNT };
}
#endif

static inline PyObject *
PyStackRef_AsPyObjectSteal(_PyStackRef ref)
{
    if (PyStackRef_RefcountOnObject(ref)) {
        return BITS_TO_PTR(ref);
    }
    else {
        return Py_NewRef(BITS_TO_PTR_MASKED(ref));
    }
}

static inline _PyStackRef
PyStackRef_FromPyObjectSteal(PyObject *obj)
{
    assert(obj != NULL);
#if SIZEOF_VOID_P > 4
    unsigned int tag = obj->ob_flags & Py_TAG_BITS;
#else
    unsigned int tag = _Py_IsImmortal(obj) ? Py_TAG_REFCNT : 0;
#endif
    _PyStackRef ref = ((_PyStackRef){.bits = ((uintptr_t)(obj)) | tag});
    PyStackRef_CheckValid(ref);
    return ref;
}

static inline _PyStackRef
PyStackRef_FromPyObjectStealMortal(PyObject *obj)
{
    assert(obj != NULL);
    assert(!_Py_IsImmortal(obj));
    _PyStackRef ref = ((_PyStackRef){.bits = ((uintptr_t)(obj)) });
    PyStackRef_CheckValid(ref);
    return ref;
}

// Check if a stackref is exactly the same as another stackref, including the
// the deferred bit. This can only be used safely if you know that the deferred
// bits of `a` and `b` match.
#define PyStackRef_IsExactly(a, b) \
    (assert(((a).bits & Py_TAG_BITS) == ((b).bits & Py_TAG_BITS)), (a).bits == (b).bits)

static inline _PyStackRef
_PyStackRef_FromPyObjectNew(PyObject *obj)
{
    assert(obj != NULL);
    if (_Py_IsImmortal(obj)) {
        return (_PyStackRef){ .bits = ((uintptr_t)obj) | Py_TAG_REFCNT};
    }
    Py_INCREF_MORTAL(obj);
    _PyStackRef ref = (_PyStackRef){ .bits = (uintptr_t)obj };
    PyStackRef_CheckValid(ref);
    return ref;
}
#define PyStackRef_FromPyObjectNew(obj) _PyStackRef_FromPyObjectNew(_PyObject_CAST(obj))

static inline _PyStackRef
_PyStackRef_FromPyObjectNewMortal(PyObject *obj)
{
    assert(obj != NULL);
    Py_INCREF_MORTAL(obj);
    _PyStackRef ref = (_PyStackRef){ .bits = (uintptr_t)obj };
    PyStackRef_CheckValid(ref);
    return ref;
}
#define PyStackRef_FromPyObjectNewMortal(obj) _PyStackRef_FromPyObjectNewMortal(_PyObject_CAST(obj))

/* Create a new reference from an object with an embedded reference count */
static inline _PyStackRef
PyStackRef_FromPyObjectImmortal(PyObject *obj)
{
    assert(_Py_IsImmortal(obj));
    return (_PyStackRef){ .bits = (uintptr_t)obj | Py_TAG_REFCNT};
}

/* WARNING: This macro evaluates its argument more than once */
#ifdef _WIN32
#define PyStackRef_DUP(REF) \
    (PyStackRef_RefcountOnObject(REF) ? (Py_INCREF_MORTAL(BITS_TO_PTR(REF)), (REF)) : (REF))
#else
static inline _PyStackRef
PyStackRef_DUP(_PyStackRef ref)
{
    assert(!PyStackRef_IsNull(ref));
    if (PyStackRef_RefcountOnObject(ref)) {
        Py_INCREF_MORTAL(BITS_TO_PTR(ref));
    }
    return ref;
}
#endif

static inline bool
PyStackRef_IsHeapSafe(_PyStackRef ref)
{
    return (ref.bits & Py_TAG_BITS) == 0 || ref.bits == PyStackRef_NULL_BITS ||  _Py_IsImmortal(BITS_TO_PTR_MASKED(ref));
}

static inline _PyStackRef
PyStackRef_MakeHeapSafe(_PyStackRef ref)
{
    if (PyStackRef_IsHeapSafe(ref)) {
        return ref;
    }
    PyObject *obj = BITS_TO_PTR_MASKED(ref);
    Py_INCREF(obj);
    ref.bits = (uintptr_t)obj;
    PyStackRef_CheckValid(ref);
    return ref;
}

#ifdef _WIN32
#define PyStackRef_CLOSE(REF) \
do { \
    _PyStackRef _temp = (REF); \
    if (PyStackRef_RefcountOnObject(_temp)) Py_DECREF_MORTAL(BITS_TO_PTR(_temp)); \
} while (0)
#else
static inline void
PyStackRef_CLOSE(_PyStackRef ref)
{
    assert(!PyStackRef_IsNull(ref));
    if (PyStackRef_RefcountOnObject(ref)) {
        Py_DECREF_MORTAL(BITS_TO_PTR(ref));
    }
}
#endif

static inline bool
PyStackRef_IsNullOrInt(_PyStackRef ref)
{
    return PyStackRef_IsNull(ref) || PyStackRef_IsTaggedInt(ref);
}

static inline void
PyStackRef_CLOSE_SPECIALIZED(_PyStackRef ref, destructor destruct)
{
    assert(!PyStackRef_IsNull(ref));
    if (PyStackRef_RefcountOnObject(ref)) {
        Py_DECREF_MORTAL_SPECIALIZED(BITS_TO_PTR(ref), destruct);
    }
}

#ifdef _WIN32
#define PyStackRef_XCLOSE PyStackRef_CLOSE
#else
static inline void
PyStackRef_XCLOSE(_PyStackRef ref)
{
    assert(ref.bits != 0);
    if (PyStackRef_RefcountOnObject(ref)) {
        assert(!PyStackRef_IsNull(ref));
        Py_DECREF_MORTAL(BITS_TO_PTR(ref));
    }
}
#endif

#define PyStackRef_CLEAR(REF) \
    do { \
        _PyStackRef *_tmp_op_ptr = &(REF); \
        _PyStackRef _tmp_old_op = (*_tmp_op_ptr); \
        *_tmp_op_ptr = PyStackRef_NULL; \
        PyStackRef_XCLOSE(_tmp_old_op); \
    } while (0)


#endif // Py_GIL_DISABLED

// Note: this is a macro because MSVC (Windows) has trouble inlining it.

#define PyStackRef_Is(a, b) (((a).bits & (~Py_TAG_BITS)) == ((b).bits & (~Py_TAG_BITS)))


#endif // !defined(Py_GIL_DISABLED) && defined(Py_STACKREF_DEBUG)

#define PyStackRef_TYPE(stackref) Py_TYPE(PyStackRef_AsPyObjectBorrow(stackref))

// Converts a PyStackRef back to a PyObject *, converting the
// stackref to a new reference.
#define PyStackRef_AsPyObjectNew(stackref) Py_NewRef(PyStackRef_AsPyObjectBorrow(stackref))

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

static inline void
_PyThreadState_PushCStackRef(PyThreadState *tstate, _PyCStackRef *ref)
{
#ifdef Py_GIL_DISABLED
    _PyThreadStateImpl *tstate_impl = (_PyThreadStateImpl *)tstate;
    ref->next = tstate_impl->c_stack_refs;
    tstate_impl->c_stack_refs = ref;
#endif
    ref->ref = PyStackRef_NULL;
}

static inline void
_PyThreadState_PopCStackRef(PyThreadState *tstate, _PyCStackRef *ref)
{
#ifdef Py_GIL_DISABLED
    _PyThreadStateImpl *tstate_impl = (_PyThreadStateImpl *)tstate;
    assert(tstate_impl->c_stack_refs == ref);
    tstate_impl->c_stack_refs = ref->next;
#endif
    PyStackRef_XCLOSE(ref->ref);
}

#ifdef Py_GIL_DISABLED

static inline int
_Py_TryIncrefCompareStackRef(PyObject **src, PyObject *op, _PyStackRef *out)
{
    if (_PyObject_HasDeferredRefcount(op)) {
        *out = (_PyStackRef){ .bits = (uintptr_t)op | Py_TAG_DEFERRED };
        return 1;
    }
    if (_Py_TryIncrefCompare(src, op)) {
        *out = PyStackRef_FromPyObjectSteal(op);
        return 1;
    }
    return 0;
}

static inline int
_Py_TryXGetStackRef(PyObject **src, _PyStackRef *out)
{
    PyObject *op = _Py_atomic_load_ptr_relaxed(src);
    if (op == NULL) {
        *out = PyStackRef_NULL;
        return 1;
    }
    return _Py_TryIncrefCompareStackRef(src, op, out);
}

#endif

// Like Py_VISIT but for _PyStackRef fields
#define _Py_VISIT_STACKREF(ref)                                         \
    do {                                                                \
        if (!PyStackRef_IsNullOrInt(ref)) {                             \
            int vret = _PyGC_VisitStackRef(&(ref), visit, arg);         \
            if (vret)                                                   \
                return vret;                                            \
        }                                                               \
    } while (0)

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_STACKREF_H */
