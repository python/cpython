#include "parts.h"
#include "../_testcapi/util.h"  // NULLABLE, RETURN_INT

#include "pycore_setobject.h"


static PyObject *
set_update(PyObject *self, PyObject *args)
{
    PyObject *set, *iterable;
    if (!PyArg_ParseTuple(args, "OO", &set, &iterable)) {
        return NULL;
    }
    NULLABLE(set);
    NULLABLE(iterable);
    RETURN_INT(_PySet_Update(set, iterable));
}

static PyObject *
set_next_entry(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    PyObject *result = PyList_New(0);
    if (result == NULL) {
        return NULL;
    }

    int set_next = 0;
    Py_ssize_t i = 0, count = 0;
    Py_hash_t h;
    PyObject *item;
    while ((set_next = _PySet_NextEntry(obj, &i, &item, &h))) {
        if (set_next == -1 && PyErr_Occurred()) {  // obj is not a set
            goto error;
        }

        count++;
        PyObject *index = PyLong_FromSsize_t(count);
        if (index == NULL) {
            goto error;
        }
        PyObject *hash = PyLong_FromSize_t((size_t)h);
        if (hash == NULL) {
            Py_DECREF(index);
            goto error;
        }
        PyObject *tup = PyTuple_Pack(3, index, item, hash);
        Py_DECREF(index);
        Py_DECREF(item);
        Py_DECREF(hash);
        if (tup == NULL) {
            goto error;
        }
        int res = PyList_Append(result, tup);
        Py_DECREF(tup);
        if (res < 0) {
            goto error;
        }
    }
    assert(count == PyList_GET_SIZE(result));
    return result;

error:
    Py_DECREF(result);
    return NULL;
}


static PyMethodDef TestMethods[] = {
    {"set_update", set_update, METH_VARARGS},
    {"set_next_entry", set_next_entry, METH_O},

    {NULL},
};

int
_PyTestInternalCapi_Init_Set(PyObject *m)
{
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }
    return 0;
}
