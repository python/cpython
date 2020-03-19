/*
 * C Extension module to test Python internal C APIs (Include/internal).
 */

#if !defined(Py_BUILD_CORE_BUILTIN) && !defined(Py_BUILD_CORE_MODULE)
#  error "Py_BUILD_CORE_BUILTIN or Py_BUILD_CORE_MODULE must be defined"
#endif

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "pycore_initconfig.h"   // _Py_GetConfigsAsDict()
#include "pycore_pystate.h"


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


static PyObject *
codecs_unregister(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyListObject *codec_search_path;
    PyObject **item;
    Py_ssize_t i;

    PyThreadState *tstate = PyThreadState_Get();
    PyInterpreterState *interp = PyThreadState_GetInterpreter(tstate);
    codec_search_path = (PyListObject *)interp->codec_search_path;
    item = codec_search_path->ob_item; 

    if (item != NULL) {
        i = Py_SIZE(codec_search_path);
        Py_SET_SIZE(codec_search_path, 0);
        codec_search_path->ob_item = NULL;
        codec_search_path->allocated = 0;
        while (--i >= 0) {
            Py_XDECREF(item[i]);
        }
    }
    PyMem_FREE(item);

    Py_RETURN_NONE;
}


static PyMethodDef TestMethods[] = {
    {"get_configs", get_configs, METH_NOARGS},
    {"get_recursion_depth", get_recursion_depth, METH_NOARGS},
    {"codecs_unregister", codecs_unregister, METH_NOARGS},
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
