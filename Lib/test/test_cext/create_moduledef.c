// Workaround for testing _Py_OPAQUE_PYOBJECT.
// See end of 'extension.c'


#undef _Py_OPAQUE_PYOBJECT
#undef Py_LIMITED_API
#include "Python.h"


// (repeated definition to avoid creating a header)
extern PyObject *testcext_create_moduledef(
    const char *name, const char *doc,
    PyMethodDef *methods, PyModuleDef_Slot *slots);

PyObject *testcext_create_moduledef(
    const char *name, const char *doc,
    PyMethodDef *methods, PyModuleDef_Slot *slots) {

    static struct PyModuleDef _testcext_module = {
        PyModuleDef_HEAD_INIT,
    };
    if (!_testcext_module.m_name) {
        _testcext_module.m_name = name;
        _testcext_module.m_doc = doc;
        _testcext_module.m_methods = methods;
        _testcext_module.m_slots = slots;
    }
    return PyModuleDef_Init(&_testcext_module);
}
