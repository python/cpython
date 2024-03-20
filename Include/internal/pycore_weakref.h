#ifndef Py_INTERNAL_WEAKREF_H
#define Py_INTERNAL_WEAKREF_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_lock.h"
#include "pycore_object.h"           // _Py_REF_IS_MERGED()

#ifdef Py_GIL_DISABLED
typedef struct _PyWeakRefClearState {
    PyMutex mutex;
    _PyOnceFlag once;
    int cleared;
    Py_ssize_t refcount;
} _PyWeakRefClearState;
#endif

static inline int _is_dead(PyObject *obj)
{
    // Explanation for the Py_REFCNT() check: when a weakref's target is part
    // of a long chain of deallocations which triggers the trashcan mechanism,
    // clearing the weakrefs can be delayed long after the target's refcount
    // has dropped to zero.  In the meantime, code accessing the weakref will
    // be able to "see" the target object even though it is supposed to be
    // unreachable.  See issue gh-60806.
#if defined(Py_GIL_DISABLED)
    Py_ssize_t shared = _Py_atomic_load_ssize_relaxed(&obj->ob_ref_shared);
    return shared == _Py_REF_SHARED(0, _Py_REF_MERGED);
#else
    return (Py_REFCNT(obj) == 0);
#endif
}

#if defined(Py_GIL_DISABLED)
#    define LOCK_WR_OBJECT(weakref) PyMutex_Lock(&(weakref)->clear_state->mutex);
#    define UNLOCK_WR_OBJECT(weakref) PyMutex_Unlock(&(weakref)->clear_state->mutex);
#else
#    define LOCK_WR_OBJECT(weakref)
#    define UNLOCK_WR_OBJECT(weakref)
#endif

// NB: In free-threaded builds nothing between the LOCK/UNLOCK calls below can
// suspend.

static inline PyObject* _PyWeakref_GET_REF(PyObject *ref_obj)
{
    assert(PyWeakref_Check(ref_obj));
    PyObject *ret = NULL;
    PyWeakReference *ref = _Py_CAST(PyWeakReference*, ref_obj);
    LOCK_WR_OBJECT(ref)
    PyObject *obj = ref->wr_object;

    if (obj == Py_None) {
        // clear_weakref() was called
        goto end;
    }

    if (_is_dead(obj)) {
        goto end;
    }
#if !defined(Py_GIL_DISABLED)
    assert(Py_REFCNT(obj) > 0);
#endif
    ret = Py_NewRef(obj);
end:
    UNLOCK_WR_OBJECT(ref);
    return ret;
}

static inline int _PyWeakref_IS_DEAD(PyObject *ref_obj)
{
    assert(PyWeakref_Check(ref_obj));
    int ret = 0;
    PyWeakReference *ref = _Py_CAST(PyWeakReference*, ref_obj);
    LOCK_WR_OBJECT(ref);
    PyObject *obj = ref->wr_object;
    if (obj == Py_None) {
        // clear_weakref() was called
        ret = 1;
    }
    else {
        // See _PyWeakref_GET_REF() for the rationale of this test
        ret = _is_dead(obj);
    }
    UNLOCK_WR_OBJECT(ref);
    return ret;
}

// NB: In free-threaded builds the referenced object must be live for the
// duration of the call.
extern Py_ssize_t _PyWeakref_GetWeakrefCount(PyWeakReference *head);

// Clear all the weak references to obj but leave their callbacks uncalled and
// intact.
extern void _PyWeakref_ClearWeakRefsExceptCallbacks(PyObject *obj);

extern void _PyWeakref_ClearRef(PyWeakReference *self);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_WEAKREF_H */
