/*
 * C Extension module to test Python internal C APIs (Include/internal).
 */

#if !defined(Py_BUILD_CORE_BUILTIN) && !defined(Py_BUILD_CORE_MODULE)
#  error "Py_BUILD_CORE_BUILTIN or Py_BUILD_CORE_MODULE must be defined"
#endif

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "pycore_initconfig.h"


#ifdef MS_WINDOWS
#include <windows.h>

static int
_add_windows_config(PyObject *configs)
{
    HMODULE hPython3;
    wchar_t py3path[MAX_PATH];
    PyObject *dict = PyDict_New();
    PyObject *obj = NULL;
    if (!dict) {
        return -1;
    }

    hPython3 = GetModuleHandleW(PY3_DLLNAME);
    if (hPython3 && GetModuleFileNameW(hPython3, py3path, MAX_PATH)) {
        obj = PyUnicode_FromWideChar(py3path, -1);
    } else {
        obj = Py_None;
        Py_INCREF(obj);
    }
    if (obj &&
        !PyDict_SetItemString(dict, "python3_dll", obj) &&
        !PyDict_SetItemString(configs, "windows", dict)) {
        Py_DECREF(obj);
        Py_DECREF(dict);
        return 0;
    }
    Py_DECREF(obj);
    Py_DECREF(dict);
    return -1;
}
#endif


static PyObject *
get_configs(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *dict = _Py_GetConfigsAsDict();
#ifdef MS_WINDOWS
    if (dict) {
        if (_add_windows_config(dict) < 0) {
            Py_CLEAR(dict);
        }
    }
#endif
    return dict;
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
