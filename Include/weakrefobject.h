/* Weak references objects for Python. */

#ifndef Py_WEAKREFOBJECT_H
#define Py_WEAKREFOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif


typedef struct _PyWeakReference PyWeakReference;

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
    long hash;

    /* If wr_object is weakly referenced, wr_object has a doubly-linked NULL-
     * terminated list of weak references to it.  These are the list pointers.
     * If wr_object goes away, wr_object is set to Py_None, and these pointers
     * have no meaning then.
     */
    PyWeakReference *wr_prev;
    PyWeakReference *wr_next;
};

PyAPI_DATA(PyTypeObject) _PyWeakref_RefType;
PyAPI_DATA(PyTypeObject) _PyWeakref_ProxyType;
PyAPI_DATA(PyTypeObject) _PyWeakref_CallableProxyType;

#define PyWeakref_CheckRef(op) \
        ((op)->ob_type == &_PyWeakref_RefType)
#define PyWeakref_CheckProxy(op) \
        (((op)->ob_type == &_PyWeakref_ProxyType) || \
         ((op)->ob_type == &_PyWeakref_CallableProxyType))
#define PyWeakref_Check(op) \
        (PyWeakref_CheckRef(op) || PyWeakref_CheckProxy(op))


PyAPI_FUNC(PyObject *) PyWeakref_NewRef(PyObject *ob,
                                              PyObject *callback);
PyAPI_FUNC(PyObject *) PyWeakref_NewProxy(PyObject *ob,
                                                PyObject *callback);
PyAPI_FUNC(PyObject *) PyWeakref_GetObject(PyObject *ref);

PyAPI_FUNC(long) _PyWeakref_GetWeakrefCount(PyWeakReference *head);

PyAPI_FUNC(void) _PyWeakref_ClearRef(PyWeakReference *self);

#define PyWeakref_GET_OBJECT(ref) (((PyWeakReference *)(ref))->wr_object)


#ifdef __cplusplus
}
#endif
#endif /* !Py_WEAKREFOBJECT_H */
