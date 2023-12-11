#include "parts.h"
#include "util.h"


static PyObject *
tuple_fromarray_impl(PyObject *args, int move_ref)
{
    PyObject *src;
    if (!PyArg_ParseTuple(args, "O!", &PyTuple_Type, &src)) {
        return NULL;
    }

    Py_ssize_t size = PyTuple_GET_SIZE(src);
    PyObject **array = PyMem_Malloc(size * sizeof(PyObject**));
    if (array == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    for (Py_ssize_t i = 0; i < size; i++) {
        array[i] = Py_NewRef(PyTuple_GET_ITEM(src, i));
    }

    PyObject *tuple;
    if (move_ref) {
        tuple = PyTuple_FromArrayMoveRef(array, size);
    }
    else {
        tuple = PyTuple_FromArray(array, size);
    }

    for (Py_ssize_t i = 0; i < size; i++) {
        // check that the array is not modified
        assert(array[i] == PyTuple_GET_ITEM(src, i));

        // check that the array was copied properly
        assert(PyTuple_GET_ITEM(tuple, i) == array[i]);
    }

    if (!move_ref || tuple == NULL) {
        for (Py_ssize_t i = 0; i < size; i++) {
            Py_DECREF(array[i]);
        }
    }
    PyMem_Free(array);

    return tuple;
}


static PyObject *
tuple_fromarray(PyObject* Py_UNUSED(module), PyObject *args)
{
    return tuple_fromarray_impl(args, 0);
}


static PyObject *
tuple_fromarraymoveref(PyObject* Py_UNUSED(module), PyObject *args)
{
    return tuple_fromarray_impl(args, 1);
}


static PyMethodDef test_methods[] = {
    {"tuple_fromarray", tuple_fromarray, METH_VARARGS},
    {"tuple_fromarraymoveref", tuple_fromarraymoveref, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Tuple(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0){
        return -1;
    }

    return 0;
}
