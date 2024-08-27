#include "parts.h"
#include "util.h"


/* Test _PyBytes_Resize() */
static PyObject *
bytes_resize(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t newsize;
    int new;

    if (!PyArg_ParseTuple(args, "Onp", &obj, &newsize, &new))
        return NULL;

    NULLABLE(obj);
    if (new) {
        assert(obj != NULL);
        assert(PyBytes_CheckExact(obj));
        PyObject *newobj = PyBytes_FromStringAndSize(NULL, PyBytes_Size(obj));
        if (newobj == NULL) {
            return NULL;
        }
        memcpy(PyBytes_AsString(newobj), PyBytes_AsString(obj), PyBytes_Size(obj));
        obj = newobj;
    }
    else {
        Py_XINCREF(obj);
    }
    if (_PyBytes_Resize(&obj, newsize) < 0) {
        assert(obj == NULL);
    }
    else {
        assert(obj != NULL);
    }
    return obj;
}


static PyMethodDef test_methods[] = {
    {"bytes_resize", bytes_resize, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Bytes(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
