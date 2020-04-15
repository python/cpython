/*
 * C Extension module to test Python internal C APIs (Include/internal).
 */

#if !defined(Py_BUILD_CORE_BUILTIN) && !defined(Py_BUILD_CORE_MODULE)
#  error "Py_BUILD_CORE_BUILTIN or Py_BUILD_CORE_MODULE must be defined"
#endif

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "pycore_initconfig.h"   // _Py_GetConfigsAsDict()
#include "pycore_gc.h"           // PyGC_Head


static PyObject *
get_configs(PyObject *self, PyObject *Py_UNUSED(args))
{
    return _Py_GetConfigsAsDict();
}


static PyObject*
get_recursion_depth(PyObject *self, PyObject *args)
{
    PyThreadState *tstate = PyThreadState_Get();

    /* subtract one to ignore the frame of the get_recursion_depth() call */
    return PyLong_FromLong(tstate->recursion_depth - 1);
}


static PyMethodDef TestMethods[] = {
    {"get_configs", get_configs, METH_NOARGS},
    {"get_recursion_depth", get_recursion_depth, METH_NOARGS},
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
    PyObject *module = PyModule_Create(&_testcapimodule);
    if (module == NULL) {
        return NULL;
    }

    if (PyModule_AddObject(module, "SIZEOF_PYGC_HEAD",
                           PyLong_FromSsize_t(sizeof(PyGC_Head))) < 0) {
        goto error;
    }

    return module;

error:
    Py_DECREF(module);
    return NULL;
}
