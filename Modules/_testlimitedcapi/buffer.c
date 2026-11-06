#include "pyconfig.h"   // Py_GIL_DISABLED
#ifdef Py_GIL_DISABLED
#  define Py_TARGET_ABI3T 0x030f0000
#else
   // Need limited C API 3.11 for the buffer protocol
#  define Py_LIMITED_API 0x030b0000
#endif

#include "parts.h"
#include "util.h"


static PyObject *
object_copydata(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *dest, *src;
    if (!PyArg_ParseTuple(args, "OO", &dest, &src)) {
        return NULL;
    }
    RETURN_INT(PyObject_CopyData(dest, src));
}

static PyObject *
buffer_fillcontiguousstrides(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *shape_obj;
    int itemsize, order;
    if (!PyArg_ParseTuple(args, "O!iC", &PyTuple_Type, &shape_obj,
                          &itemsize, &order))
    {
        return NULL;
    }
    Py_ssize_t size = PyTuple_Size(shape_obj);
    if (size < 0) {
        return NULL;
    }
    int nd = (int)size;
    Py_ssize_t *shape = PyMem_Malloc(size * sizeof(Py_ssize_t));
    Py_ssize_t *strides = PyMem_Malloc(size * sizeof(Py_ssize_t));
    if (shape == NULL || strides == NULL) {
        PyErr_NoMemory();
        goto error;
    }
    for (int k = 0; k < nd; k++) {
        shape[k] = PyLong_AsSsize_t(PyTuple_GetItem(shape_obj, k));
        if (shape[k] == -1 && PyErr_Occurred()) {
            goto error;
        }
    }

    PyBuffer_FillContiguousStrides(nd, shape, strides, itemsize, (char)order);

    PyObject *result = PyTuple_New(size);
    if (result == NULL) {
        goto error;
    }
    for (int k = 0; k < nd; k++) {
        PyObject *item = PyLong_FromSsize_t(strides[k]);
        if (item == NULL) {
            Py_DECREF(result);
            goto error;
        }
        PyTuple_SetItem(result, k, item);
    }
    PyMem_Free(shape);
    PyMem_Free(strides);
    return result;

error:
    PyMem_Free(shape);
    PyMem_Free(strides);
    return NULL;
}


static PyMethodDef test_methods[] = {
    {"object_copydata", object_copydata, METH_VARARGS},
    {"buffer_fillcontiguousstrides", buffer_fillcontiguousstrides, METH_VARARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Buffer(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
