/*
 * Test the limited C API.
 *
 * The 'test_*' functions exported by this module are run as part of the
 * standard Python regression test, via Lib/test/test_capi.py.
 */

#include "_testlimitedcapi/parts.h"

static PyMethodDef TestMethods[] = {
    {NULL, NULL} /* sentinel */
};

static struct PyModuleDef _testlimitedcapimodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_testlimitedcapi",
    .m_size = 0,
    .m_methods = TestMethods,
};

PyMODINIT_FUNC
PyInit__testlimitedcapi(void)
{
    PyObject *mod = PyModule_Create(&_testlimitedcapimodule);
    if (mod == NULL) {
        return NULL;
    }
#ifdef Py_GIL_DISABLED
    PyUnstable_Module_SetGIL(mod, Py_MOD_GIL_NOT_USED);
#endif

    if (_PyTestLimitedCAPI_Init_Abstract(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_ByteArray(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_Bytes(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_Codec(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_Complex(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_Dict(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_Eval(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_Float(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_HeaptypeRelative(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_Import(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_List(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_Long(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_Object(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_PyOS(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_Set(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_Sys(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_Tuple(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_Unicode(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_VectorcallLimited(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_Version(mod) < 0) {
        return NULL;
    }
    if (_PyTestLimitedCAPI_Init_File(mod) < 0) {
        return NULL;
    }
    return mod;
}
