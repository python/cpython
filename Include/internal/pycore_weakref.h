#ifndef Py_INTERNAL_WEAKREF_H
#define Py_INTERNAL_WEAKREF_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_critical_section.h" // Py_BEGIN_CRITICAL_SECTION()

static inline PyObject* _PyWeakref_GET_REF(PyObject *ref_obj) {
    assert(PyWeakref_Check(ref_obj));
    PyWeakReference *ref = _Py_CAST(PyWeakReference*, ref_obj);
    PyObject *obj = ref->wr_object;

    if (obj == Py_None) {
        // clear_weakref() was called
        return NULL;
    }

    // Explanation for the Py_REFCNT() check: when a weakref's target is part
    // of a long chain of deallocations which triggers the trashcan mechanism,
    // clearing the weakrefs can be delayed long after the target's refcount
    // has dropped to zero.  In the meantime, code accessing the weakref will
    // be able to "see" the target object even though it is supposed to be
    // unreachable.  See issue gh-60806.
    Py_ssize_t refcnt = Py_REFCNT(obj);
    if (refcnt == 0) {
        return NULL;
    }

    assert(refcnt > 0);
    return Py_NewRef(obj);
}

static inline int _PyWeakref_IS_DEAD(PyObject *ref_obj) {
    assert(PyWeakref_Check(ref_obj));
    int is_dead;
    Py_BEGIN_CRITICAL_SECTION(ref_obj);
    PyWeakReference *ref = _Py_CAST(PyWeakReference*, ref_obj);
    PyObject *obj = ref->wr_object;
    if (obj == Py_None) {
        // clear_weakref() was called
        is_dead = 1;
    }
    else {
        // See _PyWeakref_GET_REF() for the rationale of this test
        is_dead = (Py_REFCNT(obj) == 0);
    }
    Py_END_CRITICAL_SECTION();
    return is_dead;
}

extern Py_ssize_t _PyWeakref_GetWeakrefCount(PyWeakReference *head);

extern void _PyWeakref_ClearRef(PyWeakReference *self);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_WEAKREF_H */

