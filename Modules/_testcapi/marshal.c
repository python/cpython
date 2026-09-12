#include "parts.h"
#include "util.h"

// mashal.h is not included by <Python.h>, so include it explicitly
#include "marshal.h"


// Test PyMarshal_ReadObjectFromString()
static PyObject*
pymarshal_readobjectfromstring(PyObject* self, PyObject *args)
{
    const char *str;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "s#", &str, &size)) {
        return NULL;
    }

    return PyMarshal_ReadObjectFromString(str, size);
}


// Test PyMarshal_WriteObjectToString()
static PyObject*
pymarshal_writeobjecttostring(PyObject* self, PyObject *args)
{
    PyObject *obj;
    int version;
    if (!PyArg_ParseTuple(args, "Oi", &obj, &version)) {
        return NULL;
    }
    NULLABLE(obj);

    return PyMarshal_WriteObjectToString(obj, version);
}


// Test PyMarshal_WriteLongToFile()
static PyObject*
pymarshal_writelongtofile(PyObject* self, PyObject *args)
{
    long value;
    PyObject *filename;
    int version;
    if (!PyArg_ParseTuple(args, "lOi", &value, &filename, &version)) {
        return NULL;
    }

    FILE *fp = Py_fopen(filename, "w");
    if (fp == NULL) {
        return NULL;
    }

    assert(!PyErr_Occurred());
    PyMarshal_WriteLongToFile(value, fp, version);
    fclose(fp);
    if (PyErr_Occurred()) {
        return NULL;
    }

    Py_RETURN_NONE;
}


// Test PyMarshal_WriteObjectToFile()
static PyObject*
pymarshal_writeobjecttofile(PyObject* self, PyObject *args)
{
    PyObject *obj;
    PyObject *filename;
    int version;
    if (!PyArg_ParseTuple(args, "OOi", &obj, &filename, &version)) {
        return NULL;
    }
    NULLABLE(obj);

    FILE *fp = Py_fopen(filename, "w");
    if (fp == NULL) {
        return NULL;
    }

    assert(!PyErr_Occurred());
    PyMarshal_WriteObjectToFile(obj, fp, version);
    fclose(fp);
    if (PyErr_Occurred()) {
        return NULL;
    }

    Py_RETURN_NONE;
}


// Test PyMarshal_ReadShortFromFile()
static PyObject*
pymarshal_readshortfromfile(PyObject* self, PyObject *args)
{
    PyObject *filename;
    if (!PyArg_ParseTuple(args, "O", &filename)) {
        return NULL;
    }

    FILE *fp = Py_fopen(filename, "r");
    if (fp == NULL) {
        return NULL;
    }

    assert(!PyErr_Occurred());
    int value = PyMarshal_ReadShortFromFile(fp);
    fclose(fp);
    if (value == -1 && PyErr_Occurred()) {
        return NULL;
    }
    assert(!PyErr_Occurred());

    return PyLong_FromLong(value);
}


// Test PyMarshal_ReadLongFromFile()
static PyObject*
pymarshal_readlongfromfile(PyObject* self, PyObject *args)
{
    PyObject *filename;
    if (!PyArg_ParseTuple(args, "O", &filename)) {
        return NULL;
    }

    FILE *fp = Py_fopen(filename, "r");
    if (fp == NULL) {
        return NULL;
    }

    assert(!PyErr_Occurred());
    long value = PyMarshal_ReadLongFromFile(fp);
    fclose(fp);
    if (value == -1 && PyErr_Occurred()) {
        return NULL;
    }
    assert(!PyErr_Occurred());

    return PyLong_FromLong(value);
}


// Test PyMarshal_ReadObjectFromFile()
static PyObject*
pymarshal_readobjectfromfile(PyObject* self, PyObject *args)
{
    PyObject *filename;
    if (!PyArg_ParseTuple(args, "O", &filename)) {
        return NULL;
    }

    FILE *fp = Py_fopen(filename, "r");
    if (fp == NULL) {
        return NULL;
    }

    assert(!PyErr_Occurred());
    PyObject *obj = PyMarshal_ReadObjectFromFile(fp);
    fclose(fp);
    if (obj == NULL) {
        assert(PyErr_Occurred());
        return NULL;
    }
    assert(!PyErr_Occurred());

    return obj;
}


// Test PyMarshal_ReadLastObjectFromFile()
static PyObject*
pymarshal_readlastobjectfromfile(PyObject* self, PyObject *args)
{
    PyObject *filename;
    if (!PyArg_ParseTuple(args, "O", &filename)) {
        return NULL;
    }

    FILE *fp = Py_fopen(filename, "r");
    if (fp == NULL) {
        return NULL;
    }

    assert(!PyErr_Occurred());
    PyObject *obj = PyMarshal_ReadLastObjectFromFile(fp);
    fclose(fp);
    if (obj == NULL) {
        assert(PyErr_Occurred());
        return NULL;
    }
    assert(!PyErr_Occurred());

    return obj;
}


static PyMethodDef TestMethods[] = {
    {"pymarshal_readobjectfromstring", pymarshal_readobjectfromstring, METH_VARARGS},
    {"pymarshal_writeobjecttostring", pymarshal_writeobjecttostring, METH_VARARGS},
    {"pymarshal_writelongtofile", pymarshal_writelongtofile, METH_VARARGS},
    {"pymarshal_writeobjecttofile", pymarshal_writeobjecttofile, METH_VARARGS},
    {"pymarshal_readshortfromfile", pymarshal_readshortfromfile, METH_VARARGS},
    {"pymarshal_readlongfromfile", pymarshal_readlongfromfile, METH_VARARGS},
    {"pymarshal_readobjectfromfile", pymarshal_readobjectfromfile, METH_VARARGS},
    {"pymarshal_readlastobjectfromfile", pymarshal_readlastobjectfromfile, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Marshal(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, TestMethods) < 0) {
        return -1;
    }
    if (PyModule_AddIntMacro(mod, Py_MARSHAL_VERSION) < 0) {
        return -1;
    }
    return 0;
}
