#ifndef Py_CORE_CROSSINTERP_DATA_REGISTRY_H
#  error "this header must not be included directly"
#endif


// For now we use a global registry of shareable classes.  An
// alternative would be to add a tp_* slot for a class's
// xidatafunc. It would be simpler and more efficient.

struct _xidregitem;

struct _xidregitem {
    struct _xidregitem *prev;
    struct _xidregitem *next;
    /* This can be a dangling pointer, but only if weakref is set. */
    PyTypeObject *cls;
    /* This is NULL for builtin types. */
    PyObject *weakref;
    size_t refcount;
    xidatafunc getdata;
};

struct _xidregistry {
    int global;  /* builtin types or heap types */
    int initialized;
    PyMutex mutex;
    struct _xidregitem *head;
};

PyAPI_FUNC(int) _PyXIData_RegisterClass(PyTypeObject *, xidatafunc);
PyAPI_FUNC(int) _PyXIData_UnregisterClass(PyTypeObject *);

struct _xid_lookup_state {
    // XXX Remove this field once we have a tp_* slot.
    struct _xidregistry registry;
};
