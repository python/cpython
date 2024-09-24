#ifndef Py_INTERNAL_WEAKREF_H
#define Py_INTERNAL_WEAKREF_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_critical_section.h" // Py_BEGIN_CRITICAL_SECTION()
#include "pycore_lock.h"
#include "pycore_object.h"           // _Py_REF_IS_MERGED()
#include "pycore_pyatomic_ft_wrappers.h"

#ifdef Py_GIL_DISABLED

#define WEAKREF_LIST_LOCK(obj) \
    _PyInterpreterState_GET()  \
        ->weakref_locks[((uintptr_t)obj) % NUM_WEAKREF_LIST_LOCKS]

// Lock using the referenced object
#define LOCK_WEAKREFS(obj) \
    PyMutex_LockFlags(&WEAKREF_LIST_LOCK(obj), _Py_LOCK_DONT_DETACH)
#define UNLOCK_WEAKREFS(obj) PyMutex_Unlock(&WEAKREF_LIST_LOCK(obj))

// Lock using a weakref
#define LOCK_WEAKREFS_FOR_WR(wr) \
    PyMutex_LockFlags(wr->weakrefs_lock, _Py_LOCK_DONT_DETACH)
#define UNLOCK_WEAKREFS_FOR_WR(wr) PyMutex_Unlock(wr->weakrefs_lock)

#else

#define LOCK_WEAKREFS(obj)
#define UNLOCK_WEAKREFS(obj)

#define LOCK_WEAKREFS_FOR_WR(wr)
#define UNLOCK_WEAKREFS_FOR_WR(wr)

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

static inline PyObject* _PyWeakref_GET_REF(PyObject *ref_obj)
{
    assert(PyWeakref_Check(ref_obj));
    PyWeakReference *ref = _Py_CAST(PyWeakReference*, ref_obj);

    PyObject *obj = FT_ATOMIC_LOAD_PTR(ref->wr_object);
    if (obj == Py_None) {
        // clear_weakref() was called
        return NULL;
    }

    LOCK_WEAKREFS(obj);
#ifdef Py_GIL_DISABLED
    if (ref->wr_object == Py_None) {
        // clear_weakref() was called
        UNLOCK_WEAKREFS(obj);
        return NULL;
    }
#endif
    if (_Py_TryIncref(obj)) {
        UNLOCK_WEAKREFS(obj);
        return obj;
    }
    UNLOCK_WEAKREFS(obj);
    return NULL;
}

static inline int _PyWeakref_IS_DEAD(PyObject *ref_obj)
{
    assert(PyWeakref_Check(ref_obj));
    int ret = 0;
    PyWeakReference *ref = _Py_CAST(PyWeakReference*, ref_obj);
    PyObject *obj = FT_ATOMIC_LOAD_PTR(ref->wr_object);
    if (obj == Py_None) {
        // clear_weakref() was called
        ret = 1;
    }
    else {
        LOCK_WEAKREFS(obj);
        // See _PyWeakref_GET_REF() for the rationale of this test
#ifdef Py_GIL_DISABLED
        ret = (ref->wr_object == Py_None) || _is_dead(obj);
#else
        ret = _is_dead(obj);
#endif
        UNLOCK_WEAKREFS(obj);
    }
    return ret;
}

extern Py_ssize_t _PyWeakref_GetWeakrefCount(PyObject *obj);

// Clear all the weak references to obj but leave their callbacks uncalled and
// intact.
extern void _PyWeakref_ClearWeakRefsNoCallbacks(PyObject *obj);

PyAPI_FUNC(int) _PyWeakref_IsDead(PyObject *weakref);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_WEAKREF_H */
