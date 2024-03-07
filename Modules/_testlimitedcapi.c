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

    if (_PyTestCapi_Init_VectorcallLimited(mod) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_HeaptypeRelative(mod) < 0) {
        return NULL;
    }
    return mod;
}
