// Test PyMarshal C API

#include "parts.h"
#include "util.h"
#include "marshal.h"              // PyMarshal_WriteLongToFile()


// Test PyMarshal_WriteLongToFile()
static PyObject*
pymarshal_write_long_to_file(PyObject* self, PyObject *args)
{
    long value;
    PyObject *filename;
    int version;

    if (!PyArg_ParseTuple(args, "lOi:pymarshal_write_long_to_file",
                          &value, &filename, &version))
        return NULL;

    FILE *fp = Py_fopen(filename, "wb");
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
pymarshal_write_object_to_file(PyObject* self, PyObject *args)
{
    PyObject *obj;
    PyObject *filename;
    int version;

    if (!PyArg_ParseTuple(args, "OOi:pymarshal_write_object_to_file",
                          &obj, &filename, &version)) {
        return NULL;
    }
    NULLABLE(obj);

    FILE *fp = Py_fopen(filename, "wb");
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
pymarshal_read_short_from_file(PyObject* self, PyObject *args)
{
    int value;
    long pos;
    PyObject *filename;
    if (!PyArg_ParseTuple(args, "O:pymarshal_read_short_from_file", &filename))
        return NULL;

    FILE *fp = Py_fopen(filename, "rb");
    if (fp == NULL) {
        return NULL;
    }

    assert(!PyErr_Occurred());
    value = PyMarshal_ReadShortFromFile(fp);
    pos = ftell(fp);

    fclose(fp);
    if (PyErr_Occurred()) {
        assert(value == -1);
        return NULL;
    }

    assert(pos == 2);
    return PyLong_FromLong(value);
}


// Test PyMarshal_ReadLongFromFile()
static PyObject*
pymarshal_read_long_from_file(PyObject* self, PyObject *args)
{
    long pos;
    PyObject *filename;
    if (!PyArg_ParseTuple(args, "O:pymarshal_read_long_from_file", &filename))
        return NULL;

    FILE *fp = Py_fopen(filename, "rb");
    if (fp == NULL) {
        return NULL;
    }

    assert(!PyErr_Occurred());
    long value = PyMarshal_ReadLongFromFile(fp);
    pos = ftell(fp);

    fclose(fp);
    if (PyErr_Occurred()) {
        assert(value == -1);
        return NULL;
    }

    assert(pos == 4);
    return PyLong_FromLong(value);
}


// Test PyMarshal_ReadLastObjectFromFile()
static PyObject*
pymarshal_read_last_object_from_file(PyObject* self, PyObject *args)
{
    PyObject *filename;
    if (!PyArg_ParseTuple(args, "O:pymarshal_read_last_object_from_file", &filename))
        return NULL;

    FILE *fp = Py_fopen(filename, "rb");
    if (fp == NULL) {
        return NULL;
    }

    assert(!PyErr_Occurred());
    PyObject *obj = PyMarshal_ReadLastObjectFromFile(fp);
    long pos = ftell(fp);

    fclose(fp);
    if (obj == NULL) {
        return NULL;
    }
    return Py_BuildValue("Nl", obj, pos);
}


// Test PyMarshal_ReadObjectFromFile()
static PyObject*
pymarshal_read_object_from_file(PyObject* self, PyObject *args)
{
    PyObject *filename;
    if (!PyArg_ParseTuple(args, "O:pymarshal_read_object_from_file", &filename))
        return NULL;

    FILE *fp = Py_fopen(filename, "rb");
    if (fp == NULL) {
        return NULL;
    }

    assert(!PyErr_Occurred());
    PyObject *obj = PyMarshal_ReadObjectFromFile(fp);
    long pos = ftell(fp);

    fclose(fp);
    if (obj == NULL) {
        return NULL;
    }
    return Py_BuildValue("Nl", obj, pos);
}


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


static PyMethodDef test_methods[] = {
    {"pymarshal_write_long_to_file",
        pymarshal_write_long_to_file, METH_VARARGS},
    {"pymarshal_write_object_to_file",
        pymarshal_write_object_to_file, METH_VARARGS},
    {"pymarshal_read_short_from_file",
        pymarshal_read_short_from_file, METH_VARARGS},
    {"pymarshal_read_long_from_file",
        pymarshal_read_long_from_file, METH_VARARGS},
    {"pymarshal_read_last_object_from_file",
        pymarshal_read_last_object_from_file, METH_VARARGS},
    {"pymarshal_read_object_from_file",
        pymarshal_read_object_from_file, METH_VARARGS},
    {"pymarshal_readobjectfromstring",
        pymarshal_readobjectfromstring, METH_VARARGS},
    {"pymarshal_writeobjecttostring",
        pymarshal_writeobjecttostring, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Marshal(PyObject *mod)
{
    return PyModule_AddFunctions(mod, test_methods);
}
