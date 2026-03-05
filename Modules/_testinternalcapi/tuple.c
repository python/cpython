#include "parts.h"

#include "pycore_tuple.h"


static PyObject *
_tuple_from_pair(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *one, *two;
    if (!PyArg_ParseTuple(args, "OO", &one, &two)) {
        return NULL;
    }

    return _PyTuple_FromPair(one, two);
}

static PyObject *
_tuple_from_pair_steal(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *one, *two;
    if (!PyArg_ParseTuple(args, "OO", &one, &two)) {
        return NULL;
    }

    return _PyTuple_FromPairSteal(Py_NewRef(one), Py_NewRef(two));
}


static PyMethodDef test_methods[] = {
    {"_tuple_from_pair", _tuple_from_pair, METH_VARARGS},
    {"_tuple_from_pair_steal", _tuple_from_pair_steal, METH_VARARGS},
    {NULL},
};

int
_PyTestInternalCapi_Init_Tuple(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
