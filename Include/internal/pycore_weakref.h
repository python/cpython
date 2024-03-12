#ifndef Py_INTERNAL_WEAKREF_H
#define Py_INTERNAL_WEAKREF_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_critical_section.h" // Py_BEGIN_CRITICAL_SECTION()
#include "pycore_object.h"           // _Py_REF_IS_MERGED()

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
    PyObject *ret = NULL;
    Py_BEGIN_CRITICAL_SECTION(ref_obj);
    PyWeakReference *ref = _Py_CAST(PyWeakReference*, ref_obj);
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
    Py_END_CRITICAL_SECTION();
    return ret;
}

static inline int _PyWeakref_IS_DEAD(PyObject *ref_obj)
{
    assert(PyWeakref_Check(ref_obj));
    int ret = 0;
    Py_BEGIN_CRITICAL_SECTION(ref_obj);
    PyWeakReference *ref = _Py_CAST(PyWeakReference*, ref_obj);
    PyObject *obj = ref->wr_object;
    if (obj == Py_None) {
        // clear_weakref() was called
        ret = 1;
    }
    else {
        // See _PyWeakref_GET_REF() for the rationale of this test
        ret = _is_dead(obj);
    }
    Py_END_CRITICAL_SECTION();
    return ret;
}

extern Py_ssize_t _PyWeakref_GetWeakrefCount(PyWeakReference *head);

extern void _PyWeakref_ClearRef(PyWeakReference *self);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_WEAKREF_H */

