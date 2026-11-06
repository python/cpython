#ifndef Py_CORE_CROSSINTERP_DATA_REGISTRY_H
#  error "this header must not be included directly"
#endif


// For now we use a global registry of shareable classes.  An
// alternative would be to add a tp_* slot for a class's
// xidatafunc. It would be simpler and more efficient.

struct _xid_regitem;

typedef struct _xid_regitem {
    struct _xid_regitem *prev;
    struct _xid_regitem *next;
    /* This can be a dangling pointer, but only if weakref is set. */
    PyTypeObject *cls;
    /* This is NULL for builtin types. */
    PyObject *weakref;
    size_t refcount;
    _PyXIData_getdata_t getdata;
} _PyXIData_regitem_t;

typedef struct {
    int global;  /* builtin types or heap types */
    int initialized;
    PyMutex mutex;
    _PyXIData_regitem_t *head;
} _PyXIData_registry_t;

PyAPI_FUNC(int) _PyXIData_RegisterClass(
    PyThreadState *,
    PyTypeObject *,
    _PyXIData_getdata_t);
PyAPI_FUNC(int) _PyXIData_UnregisterClass(
    PyThreadState *,
    PyTypeObject *);

struct _xid_lookup_state {
    // XXX Remove this field once we have a tp_* slot.
    _PyXIData_registry_t registry;
};
