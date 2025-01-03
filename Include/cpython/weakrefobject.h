#ifndef Py_CPYTHON_WEAKREFOBJECT_H
#  error "this header file must not be included directly"
#endif

/* PyWeakReference is the base struct for the Python ReferenceType, ProxyType,
 * and CallableProxyType.
 */
struct _PyWeakReference {
    PyObject_HEAD

    /* The object to which this is a weak reference, or Py_None if none.
     * Note that this is a stealth reference:  wr_object's refcount is
     * not incremented to reflect this pointer.
     */
    PyObject *wr_object;

    /* A callable to invoke when wr_object dies, or NULL if none. */
    PyObject *wr_callback;

    /* A cache for wr_object's hash code.  As usual for hashes, this is -1
     * if the hash code isn't known yet.
     */
    Py_hash_t hash;

    /* If wr_object is weakly referenced, wr_object has a doubly-linked NULL-
     * terminated list of weak references to it.  These are the list pointers.
     * If wr_object goes away, wr_object is set to Py_None, and these pointers
     * have no meaning then.
     */
    PyWeakReference *wr_prev;
    PyWeakReference *wr_next;
    vectorcallfunc vectorcall;

#ifdef Py_GIL_DISABLED
    /* Pointer to the lock used when clearing in free-threaded builds.
     * Normally this can be derived from wr_object, but in some cases we need
     * to lock after wr_object has been set to Py_None.
     */
    PyMutex *weakrefs_lock;
#endif
};

PyAPI_FUNC(void) _PyWeakref_ClearRef(PyWeakReference *self);

#define _PyWeakref_CAST(op) \
    (assert(PyWeakref_Check(op)), _Py_CAST(PyWeakReference*, (op)))

// Test if a weak reference is dead.
PyAPI_FUNC(int) PyWeakref_IsDead(PyObject *ref);

Py_DEPRECATED(3.13) static inline PyObject* PyWeakref_GET_OBJECT(PyObject *ref_obj)
{
    PyWeakReference *ref = _PyWeakref_CAST(ref_obj);
    PyObject *obj = ref->wr_object;
    // Explanation for the Py_REFCNT() check: when a weakref's target is part
    // of a long chain of deallocations which triggers the trashcan mechanism,
    // clearing the weakrefs can be delayed long after the target's refcount
    // has dropped to zero.  In the meantime, code accessing the weakref will
    // be able to "see" the target object even though it is supposed to be
    // unreachable.  See issue gh-60806.
    if (Py_REFCNT(obj) > 0) {
        return obj;
    }
    return Py_None;
}
#define PyWeakref_GET_OBJECT(ref) PyWeakref_GET_OBJECT(_PyObject_CAST(ref))
