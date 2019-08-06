/*
 * C Extension module to test Python internal C APIs (Include/internal).
 */

#if !defined(Py_BUILD_CORE_BUILTIN) && !defined(Py_BUILD_CORE_MODULE)
#  error "Py_BUILD_CORE_BUILTIN or Py_BUILD_CORE_MODULE must be defined"
#endif

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "pycore_initconfig.h"


static PyObject *
get_configs(PyObject *self, PyObject *Py_UNUSED(args))
{
    return _Py_GetConfigsAsDict();
}


static PyMethodDef TestMethods[] = {
    {"get_configs", get_configs, METH_NOARGS},
    {NULL, NULL} /* sentinel */
};


static struct PyModuleDef _testcapimodule = {
    PyModuleDef_HEAD_INIT,
    "_testinternalcapi",
    NULL,
    -1,
    TestMethods,
    NULL,
    NULL,
    NULL,
    NULL
};


PyMODINIT_FUNC
PyInit__testinternalcapi(void)
{
    return PyModule_Create(&_testcapimodule);
}
