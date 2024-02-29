
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
