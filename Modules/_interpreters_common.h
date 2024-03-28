
#define _RESOLVE_MODINIT_FUNC_NAME(NAME) \
    PyInit_ ## NAME
#define RESOLVE_MODINIT_FUNC_NAME(NAME) \
    _RESOLVE_MODINIT_FUNC_NAME(NAME)


static int
ensure_xid_class(PyTypeObject *cls, crossinterpdatafunc getdata)
{
    //assert(cls->tp_flags & Py_TPFLAGS_HEAPTYPE);
    return _PyCrossInterpreterData_RegisterClass(cls, getdata);
}

#ifdef REGISTERS_HEAP_TYPES
static int
clear_xid_class(PyTypeObject *cls)
{
    return _PyCrossInterpreterData_UnregisterClass(cls);
}
#endif


#ifdef RETURNS_INTERPID_OBJECT
static PyObject *
get_interpid_obj(PyInterpreterState *interp)
{
    if (_PyInterpreterState_IDInitref(interp) != 0) {
        return NULL;
    };
    int64_t id = PyInterpreterState_GetID(interp);
    if (id < 0) {
        return NULL;
    }
    assert(id < LLONG_MAX);
    return PyLong_FromLongLong(id);
}
#endif
