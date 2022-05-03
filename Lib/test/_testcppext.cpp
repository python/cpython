// gh-91321: Very basic C++ test extension to check that the Python C API is
// compatible with C++ and does not emit C++ compiler warnings.

// Always enable assertions
#undef NDEBUG

#include "Python.h"

PyDoc_STRVAR(_testcppext_add_doc,
"add(x, y)\n"
"\n"
"Return the sum of two integers: x + y.");

static PyObject *
_testcppext_add(PyObject *Py_UNUSED(module), PyObject *args)
{
    long i, j;
    if (!PyArg_ParseTuple(args, "ll:foo", &i, &j)) {
        return nullptr;
    }
    long res = i + j;
    return PyLong_FromLong(res);
}


static PyObject *
test_api_casts(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    PyObject *obj = Py_BuildValue("(ii)", 1, 2);
    if (obj == nullptr) {
        return nullptr;
    }

    // gh-92138: For backward compatibility, functions of Python C API accepts
    // "const PyObject*". Check that using it does not emit C++ compiler
    // warnings.
    const PyObject *const_obj = obj;
    Py_INCREF(const_obj);
    Py_DECREF(const_obj);
    PyTypeObject *type = Py_TYPE(const_obj);
    assert(Py_REFCNT(const_obj) >= 1);

    assert(type == &PyTuple_Type);
    assert(PyTuple_GET_SIZE(const_obj) == 2);
    PyObject *one = PyTuple_GET_ITEM(const_obj, 0);
    assert(PyLong_AsLong(one) == 1);

    Py_DECREF(obj);
    Py_RETURN_NONE;
}


static PyMethodDef _testcppext_methods[] = {
    {"add", _testcppext_add, METH_VARARGS, _testcppext_add_doc},
    {"test_api_casts", test_api_casts, METH_NOARGS, nullptr},
    {nullptr, nullptr, 0, nullptr}  /* sentinel */
};


static int
_testcppext_exec(PyObject *module)
{
    if (PyModule_AddIntMacro(module, __cplusplus) < 0) {
        return -1;
    }
    return 0;
}

static PyModuleDef_Slot _testcppext_slots[] = {
    {Py_mod_exec, reinterpret_cast<void*>(_testcppext_exec)},
    {0, nullptr}
};


PyDoc_STRVAR(_testcppext_doc, "C++ test extension.");

static struct PyModuleDef _testcppext_module = {
    PyModuleDef_HEAD_INIT,  // m_base
    "_testcppext",  // m_name
    _testcppext_doc,  // m_doc
    0,  // m_size
    _testcppext_methods,  // m_methods
    _testcppext_slots,  // m_slots
    nullptr,  // m_traverse
    nullptr,  // m_clear
    nullptr,  // m_free
};

PyMODINIT_FUNC
PyInit__testcppext(void)
{
    return PyModuleDef_Init(&_testcppext_module);
}
