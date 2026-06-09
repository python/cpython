#include "parts.h"
#include "util.h"


static PyObject *
pyweakref_getref(PyObject *module, PyObject *ref)
{
    NULLABLE(ref);
    PyObject *obj = UNINITIALIZED_PTR;
    int rc = PyWeakref_GetRef(ref, &obj);
    if (rc == -1 && PyErr_Occurred()) {
        assert(obj == NULL);
        return NULL;
    }
    if (obj == NULL) {
        return Py_BuildValue("i", rc);
    }
    else {
        assert(obj != UNINITIALIZED_PTR);
        return Py_BuildValue("iN", rc, obj);
    }
}


static PyMethodDef test_methods[] = {
    {"pyweakref_getref", pyweakref_getref, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Weakref(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
