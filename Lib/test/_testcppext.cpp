// gh-91321: Very basic C++ test extension to check that the Python C API is
// compatible with C++ and does not emit C++ compiler warnings.

// Always enable assertions
#undef NDEBUG

#include "Python.h"

#if __cplusplus >= 201103
#  define NAME _testcpp11ext
#else
#  define NAME _testcpp03ext
#endif

PyDoc_STRVAR(_testcppext_add_doc,
"add(x, y)\n"
"\n"
"Return the sum of two integers: x + y.");

static PyObject *
_testcppext_add(PyObject *Py_UNUSED(module), PyObject *args)
{
    long i, j;
    if (!PyArg_ParseTuple(args, "ll:foo", &i, &j)) {
        return _Py_NULL;
    }
    long res = i + j;
    return PyLong_FromLong(res);
}


// Class to test operator casting an object to PyObject*
class StrongRef
{
public:
    StrongRef(PyObject *obj) : m_obj(obj) {
        Py_INCREF(this->m_obj);
    }

    ~StrongRef() {
        Py_DECREF(this->m_obj);
    }

    // Cast to PyObject*: get a borrowed reference
    inline operator PyObject*() const { return this->m_obj; }

private:
    PyObject *m_obj;  // Strong reference
};


static PyObject *
test_api_casts(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    PyObject *obj = Py_BuildValue("(ii)", 1, 2);
    if (obj == _Py_NULL) {
        return _Py_NULL;
    }
    Py_ssize_t refcnt = Py_REFCNT(obj);
    assert(refcnt >= 1);

    // gh-92138: For backward compatibility, functions of Python C API accepts
    // "const PyObject*". Check that using it does not emit C++ compiler
    // warnings.
    const PyObject *const_obj = obj;
    Py_INCREF(const_obj);
    Py_DECREF(const_obj);
    PyTypeObject *type = Py_TYPE(const_obj);
    assert(Py_REFCNT(const_obj) == refcnt);
    assert(type == &PyTuple_Type);
    assert(PyTuple_GET_SIZE(const_obj) == 2);
    PyObject *one = PyTuple_GET_ITEM(const_obj, 0);
    assert(PyLong_AsLong(one) == 1);

    // gh-92898: StrongRef doesn't inherit from PyObject but has an operator to
    // cast to PyObject*.
    StrongRef strong_ref(obj);
    assert(Py_TYPE(strong_ref) == &PyTuple_Type);
    assert(Py_REFCNT(strong_ref) == (refcnt + 1));
    Py_INCREF(strong_ref);
    Py_DECREF(strong_ref);

    // gh-93442: Pass 0 as NULL for PyObject*
    Py_XINCREF(0);
    Py_XDECREF(0);
#if _cplusplus >= 201103
    // Test nullptr passed as PyObject*
    Py_XINCREF(nullptr);
    Py_XDECREF(nullptr);
#endif

    Py_DECREF(obj);
    Py_RETURN_NONE;
}


static PyObject *
test_unicode(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    PyObject *str = PyUnicode_FromString("abc");
    if (str == _Py_NULL) {
        return _Py_NULL;
    }

    assert(PyUnicode_Check(str));
    assert(PyUnicode_GET_LENGTH(str) == 3);

    // gh-92800: test PyUnicode_READ()
    const void* data = PyUnicode_DATA(str);
    assert(data != _Py_NULL);
    int kind = PyUnicode_KIND(str);
    assert(kind == PyUnicode_1BYTE_KIND);
    assert(PyUnicode_READ(kind, data, 0) == 'a');

    // gh-92800: test PyUnicode_READ() casts
    const void* const_data = PyUnicode_DATA(str);
    unsigned int ukind = static_cast<unsigned int>(kind);
    assert(PyUnicode_READ(ukind, const_data, 2) == 'c');

    assert(PyUnicode_READ_CHAR(str, 1) == 'b');

    Py_DECREF(str);
    Py_RETURN_NONE;
}


static PyMethodDef _testcppext_methods[] = {
    {"add", _testcppext_add, METH_VARARGS, _testcppext_add_doc},
    {"test_api_casts", test_api_casts, METH_NOARGS, _Py_NULL},
    {"test_unicode", test_unicode, METH_NOARGS, _Py_NULL},
    // Note: _testcppext_exec currently runs all test functions directly.
    // When adding a new one, add a call there.

    {_Py_NULL, _Py_NULL, 0, _Py_NULL}  /* sentinel */
};


static int
_testcppext_exec(PyObject *module)
{
    if (PyModule_AddIntMacro(module, __cplusplus) < 0) {
        return -1;
    }

    PyObject *result;

    result = PyObject_CallMethod(module, "test_api_casts", "");
    if (!result) return -1;
    Py_DECREF(result);

    result = PyObject_CallMethod(module, "test_unicode", "");
    if (!result) return -1;
    Py_DECREF(result);

    return 0;
}

static PyModuleDef_Slot _testcppext_slots[] = {
    {Py_mod_exec, reinterpret_cast<void*>(_testcppext_exec)},
    {0, _Py_NULL}
};


PyDoc_STRVAR(_testcppext_doc, "C++ test extension.");

#define _STR(NAME) #NAME
#define STR(NAME) _STR(NAME)

static struct PyModuleDef _testcppext_module = {
    PyModuleDef_HEAD_INIT,  // m_base
    STR(NAME),  // m_name
    _testcppext_doc,  // m_doc
    0,  // m_size
    _testcppext_methods,  // m_methods
    _testcppext_slots,  // m_slots
    _Py_NULL,  // m_traverse
    _Py_NULL,  // m_clear
    _Py_NULL,  // m_free
};

#define _FUNC_NAME(NAME) PyInit_ ## NAME
#define FUNC_NAME(NAME) _FUNC_NAME(NAME)

PyMODINIT_FUNC
FUNC_NAME(NAME)(void)
{
    return PyModuleDef_Init(&_testcppext_module);
}
