#include "parts.h"
#include "util.h"


/* Test _PyBytes_Resize() */
static PyObject *
bytes_resize(PyObject *Py_UNUSED(module), PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"obj", "newsize", "new", "compute_hash", NULL};
    PyObject *obj;
    Py_ssize_t newsize;
    int new;
    int compute_hash = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "Onp|p", kwlist,
                                     &obj, &newsize, &new, &compute_hash)) {
        return NULL;
    }

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

    if (compute_hash) {
        if (PyObject_Hash(obj) == -1) {
            Py_DECREF(obj);
            return NULL;
        }
    }

    PyObject *old_obj = obj;
    if (_PyBytes_Resize(&obj, newsize) < 0) {
        assert(obj == NULL);
        return NULL;
    }
    else {
        assert(obj != NULL);
    }

    Py_ssize_t refcnt = Py_REFCNT(obj);
    return Py_BuildValue("Nnp", obj, refcnt, obj != old_obj);
}


/* Test PyBytes_Join() */
static PyObject *
bytes_join(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *sep, *iterable;
    if (!PyArg_ParseTuple(args, "OO", &sep, &iterable)) {
        return NULL;
    }
    NULLABLE(sep);
    NULLABLE(iterable);
    return PyBytes_Join(sep, iterable);
}


static PyMethodDef test_methods[] = {
    {"bytes_resize", _PyCFunction_CAST(bytes_resize), METH_VARARGS | METH_KEYWORDS},
    {"bytes_join", bytes_join, METH_VARARGS},
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
