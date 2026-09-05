/*
 * Test the limited C API.
 *
 * The 'test_*' functions exported by this module are run as part of the
 * standard Python regression test, via Lib/test/test_capi/test_misc.py.
 */

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

static struct PyModuleDef _testlimitedcapimodule_def = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_testlimitedcapi",
    .m_size = 0,
    .m_slots = (PyModuleDef_Slot[]){
        {Py_mod_exec, module_exec},
#ifdef Py_GIL_DISABLED
        {Py_mod_gil, Py_MOD_GIL_NOT_USED},
#endif
        {0}
    }
};

PyMODINIT_FUNC
PyInit__testlimitedcapi(void)
{
    return PyModuleDef_Init(&_testlimitedcapimodule_def);
}
