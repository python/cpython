/*
 * Test the limited C API.
 *
 * The 'test_*' functions exported by this module are run as part of the
 * standard Python regression test, via Lib/test/test_capi/test_misc.py.
 */

// Need limited C API version 3.15 for PySlot
#define Py_LIMITED_API 0x030f0000

#include "_testlimitedcapi/parts.h"

static int
module_exec(PyObject *mod)
{
    if (_PyTestLimitedCAPI_Init_Abstract(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_ByteArray(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Bytes(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Capsule(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Codec(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Complex(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Dict(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Eval(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Float(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_HeaptypeRelative(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Import(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_List(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Long(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Object(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_PyOS(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Set(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Slots(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Sys(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_ThreadState(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Tuple(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Unicode(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_VectorcallLimited(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Version(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_File(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Weakref(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Run(mod) < 0) {
        return -1;
    }
    if (_PyTestLimitedCAPI_Init_Type(mod) < 0) {
        return -1;
    }
    return 0;
}

PyABIInfo_VAR(abi_info);

static PySlot _testlimitedcapimodule_slots[] = {
    PySlot_DATA(Py_mod_abi, &abi_info),
    PySlot_STATIC_DATA(Py_mod_name, "_testlimitedcapi"),
    PySlot_SIZE(Py_mod_state_size, 0),
    PySlot_FUNC(Py_mod_exec, module_exec),
    PySlot_SIZE(Py_mod_gil, Py_MOD_GIL_NOT_USED),
    PySlot_END
};

PyMODEXPORT_FUNC
PyModExport__testlimitedcapi(void)
{
    return _testlimitedcapimodule_slots;
}
