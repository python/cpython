/* Weak references objects for Python. */

#ifndef Py_WEAKREFOBJECT_H
#define Py_WEAKREFOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif


typedef struct _PyWeakReference PyWeakReference;

struct _PyWeakReference {
    PyObject_HEAD
    PyObject *wr_object;
    PyObject *wr_callback;
    long hash;
    PyWeakReference *wr_prev;
    PyWeakReference *wr_next;
};

extern DL_IMPORT(PyTypeObject) _PyWeakref_RefType;
extern DL_IMPORT(PyTypeObject) _PyWeakref_ProxyType;
extern DL_IMPORT(PyTypeObject) _PyWeakref_CallableProxyType;

#define PyWeakref_CheckRef(op) \
        ((op)->ob_type == &_PyWeakref_RefType)
#define PyWeakref_CheckProxy(op) \
        (((op)->ob_type == &_PyWeakref_ProxyType) || \
         ((op)->ob_type == &_PyWeakref_CallableProxyType))
#define PyWeakref_Check(op) \
        (PyWeakref_CheckRef(op) || PyWeakref_CheckProxy(op))


extern DL_IMPORT(PyObject *) PyWeakref_NewRef(PyObject *ob,
                                              PyObject *callback);
extern DL_IMPORT(PyObject *) PyWeakref_NewProxy(PyObject *ob,
                                                PyObject *callback);
extern DL_IMPORT(PyObject *) PyWeakref_GetObject(PyObject *ref);

extern DL_IMPORT(long) _PyWeakref_GetWeakrefCount(PyWeakReference *head);

#define PyWeakref_GET_OBJECT(ref) (((PyWeakReference *)(ref))->wr_object)


#ifdef __cplusplus
}
#endif
#endif /* !Py_WEAKREFOBJECT_H */
