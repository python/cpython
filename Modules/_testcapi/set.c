#include "parts.h"
#include "util.h"

static PyObject *
set_get_size(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PySet_GET_SIZE(obj));
}


static PyObject*
test_set_type_size(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *obj = PyList_New(0);
    if (obj == NULL) {
        return NULL;
    }

    // Ensure that following tests don't modify the object,
    // to ensure that Py_DECREF() will not crash.
    assert(Py_TYPE(obj) == &PyList_Type);
    assert(Py_SIZE(obj) == 0);

    // bpo-39573: Test Py_SET_TYPE() and Py_SET_SIZE() functions.
    Py_SET_TYPE(obj, &PyList_Type);
    Py_SET_SIZE(obj, 0);

    Py_DECREF(obj);
    Py_RETURN_NONE;
}


static PyMethodDef test_methods[] = {
    {"set_get_size", set_get_size, METH_O},
    {"test_set_type_size", test_set_type_size, METH_NOARGS},
    {NULL},
};

int
_PyTestCapi_Init_Set(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
