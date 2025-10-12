
#define _RESOLVE_MODINIT_FUNC_NAME(NAME) \
    PyInit_ ## NAME
#define RESOLVE_MODINIT_FUNC_NAME(NAME) \
    _RESOLVE_MODINIT_FUNC_NAME(NAME)


#define GETDATA(FUNC) ((_PyXIData_getdata_t){.basic=FUNC})

static int
ensure_xid_class(PyTypeObject *cls, _PyXIData_getdata_t getdata)
{
    PyThreadState *tstate = PyThreadState_Get();
    return _PyXIData_RegisterClass(tstate, cls, getdata);
}

#ifdef REGISTERS_HEAP_TYPES
static int
clear_xid_class(PyTypeObject *cls)
{
    PyThreadState *tstate = PyThreadState_Get();
    return _PyXIData_UnregisterClass(tstate, cls);
}
#endif


static inline int64_t
_get_interpid(_PyXIData_t *data)
{
    int64_t interpid;
    if (data != NULL) {
        interpid = _PyXIData_INTERPID(data);
        assert(!PyErr_Occurred());
    }
    else {
        interpid = PyInterpreterState_GetID(PyInterpreterState_Get());
    }
    return interpid;
}


/* unbound items ************************************************************/

#ifdef HAS_UNBOUND_ITEMS

#define UNBOUND_REMOVE 1
#define UNBOUND_ERROR 2
#define UNBOUND_REPLACE 3

// It would also be possible to add UNBOUND_REPLACE where the replacement
// value is user-provided.  There would be some limitations there, though.
// Another possibility would be something like UNBOUND_COPY, where the
// object is released but the underlying data is copied (with the "raw"
// allocator) and used when the item is popped off the queue.

static int
check_unbound(int unboundop)
{
    switch (unboundop) {
    case UNBOUND_REMOVE:
    case UNBOUND_ERROR:
    case UNBOUND_REPLACE:
        return 1;
    default:
        return 0;
    }
}

#endif
