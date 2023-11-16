#define PY_SSIZE_T_CLEAN
#include "parts.h"
#include "util.h"


/* Test PyByteArray_Check() */
static PyObject *
bytearray_check(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyByteArray_Check(obj));
}

/* Test PyByteArray_CheckExact() */
static PyObject *
bytearray_checkexact(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyByteArray_CheckExact(obj));
}

/* Test PyByteArray_FromStringAndSize() */
static PyObject *
bytearray_fromstringandsize(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *s;
    Py_ssize_t bsize;
    Py_ssize_t size = -100;

    if (!PyArg_ParseTuple(args, "z#|n", &s, &bsize, &size)) {
        return NULL;
    }

    if (size == -100) {
        size = bsize;
    }
    return PyByteArray_FromStringAndSize(s, size);
}

/* Test PyByteArray_FromObject() */
static PyObject *
bytearray_fromobject(PyObject *Py_UNUSED(module), PyObject *arg)
{
    NULLABLE(arg);
    return PyByteArray_FromObject(arg);
}

/* Test PyByteArray_Size() */
static PyObject *
bytearray_size(PyObject *Py_UNUSED(module), PyObject *arg)
{
    NULLABLE(arg);
    RETURN_SIZE(PyByteArray_Size(arg));
}

/* Test PyUnicode_AsString() */
static PyObject *
bytearray_asstring(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t buflen;
    const char *s;

    if (!PyArg_ParseTuple(args, "On", &obj, &buflen))
        return NULL;

    NULLABLE(obj);
    s = PyByteArray_AsString(obj);
    if (s == NULL)
        return NULL;

    return PyByteArray_FromStringAndSize(s, buflen);
}

/* Test PyByteArray_Concat() */
static PyObject *
bytearray_concat(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *left, *right;

    if (!PyArg_ParseTuple(args, "OO", &left, &right))
        return NULL;

    NULLABLE(left);
    NULLABLE(right);
    return PyByteArray_Concat(left, right);
}

/* Test PyByteArray_Resize() */
static PyObject *
bytearray_resize(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(args, "On", &obj, &size))
        return NULL;

    NULLABLE(obj);
    RETURN_INT(PyByteArray_Resize(obj, size));
}


static PyMethodDef test_methods[] = {
    {"bytearray_check", bytearray_check, METH_O},
    {"bytearray_checkexact", bytearray_checkexact, METH_O},
    {"bytearray_fromstringandsize", bytearray_fromstringandsize, METH_VARARGS},
    {"bytearray_fromobject", bytearray_fromobject, METH_O},
    {"bytearray_size", bytearray_size, METH_O},
    {"bytearray_asstring", bytearray_asstring, METH_VARARGS},
    {"bytearray_concat", bytearray_concat, METH_VARARGS},
    {"bytearray_resize", bytearray_resize, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_ByteArray(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
