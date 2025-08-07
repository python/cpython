/* ABI information object implementation */

#include "Python.h"
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_structseq.h"     // _PyStructSequence_InitBuiltin()

static PyTypeObject AbiInfoType;

PyDoc_STRVAR(abi_info__doc__,
"ABI information object.\n\
This object provides information about the ABI \
features available in the current Python interpreter.\n");

static PyStructSequence_Field abi_info_fields[] = {
    {"pointer_bits", "Is this a 32-bit or 64-bit build?"},
    {"Py_GIL_DISABLED", "Is free-threading enabled?"},
    {"Py_DEBUG", "Is debug mode enabled?"},
    {0}
};

static PyStructSequence_Desc abi_info_desc = {
    "sys.abi_info",
    abi_info__doc__,
    abi_info_fields,
    3
};

PyObject *
PyAbiInfo_GetInfo(void)
{
    PyObject *abi_info, *value;
    int pos = 0;
    abi_info = PyStructSequence_New(&AbiInfoType);
    if (abi_info == NULL) {
        goto error;
    }

    value = PyLong_FromLong(sizeof(void *) * 8);
    if (value == NULL) {
        goto error;
    }
    PyStructSequence_SetItem(abi_info, pos++, value);

#ifdef Py_GIL_DISABLED
    value = Py_True;
#else
    value = Py_False;
#endif
    PyStructSequence_SetItem(abi_info, pos++, value);

#ifdef Py_DEBUG
    value = Py_True;
#else
    value = Py_False;
#endif
    PyStructSequence_SetItem(abi_info, pos++, value);

    return abi_info;

error:
    Py_XDECREF(abi_info);
    return NULL;
}

PyStatus
_PyAbiInfo_InitTypes(PyInterpreterState *interp)
{
    if (_PyStructSequence_InitBuiltin(interp, &AbiInfoType, &abi_info_desc) < 0) {
        return _PyStatus_ERR("Failed to initialize AbiInfoType");
    }

    return _PyStatus_OK();
}

void
_PyAbiInfo_FiniTypes(PyInterpreterState *interp)
{
    _PyStructSequence_FiniBuiltin(interp, &AbiInfoType);
}
