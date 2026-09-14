// Test PyMarshal C API

#include "parts.h"
#include "marshal.h"              // PyMarshal_WriteLongToFile()

static PyObject*
pymarshal_write_long_to_file(PyObject* self, PyObject *args)
{
    long value;
    PyObject *filename;
    int version;
    FILE *fp;

    if (!PyArg_ParseTuple(args, "lOi:pymarshal_write_long_to_file",
                          &value, &filename, &version))
        return NULL;

    fp = Py_fopen(filename, "wb");
    if (fp == NULL) {
        return NULL;
    }

    PyMarshal_WriteLongToFile(value, fp, version);

    fclose(fp);
    if (PyErr_Occurred()) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
pymarshal_write_object_to_file(PyObject* self, PyObject *args)
{
    PyObject *obj;
    PyObject *filename;
    int version;
    FILE *fp;

    if (!PyArg_ParseTuple(args, "OOi:pymarshal_write_object_to_file",
                          &obj, &filename, &version))
        return NULL;

    fp = Py_fopen(filename, "wb");
    if (fp == NULL) {
        return NULL;
    }

    PyMarshal_WriteObjectToFile(obj, fp, version);

    fclose(fp);
    if (PyErr_Occurred()) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
pymarshal_read_short_from_file(PyObject* self, PyObject *args)
{
    int value;
    long pos;
    PyObject *filename;
    FILE *fp;

    if (!PyArg_ParseTuple(args, "O:pymarshal_read_short_from_file", &filename))
        return NULL;

    fp = Py_fopen(filename, "rb");
    if (fp == NULL) {
        return NULL;
    }

    value = PyMarshal_ReadShortFromFile(fp);
    pos = ftell(fp);

    fclose(fp);
    if (PyErr_Occurred())
        return NULL;
    return Py_BuildValue("il", value, pos);
}

static PyObject*
pymarshal_read_long_from_file(PyObject* self, PyObject *args)
{
    long value, pos;
    PyObject *filename;
    FILE *fp;

    if (!PyArg_ParseTuple(args, "O:pymarshal_read_long_from_file", &filename))
        return NULL;

    fp = Py_fopen(filename, "rb");
    if (fp == NULL) {
        return NULL;
    }

    value = PyMarshal_ReadLongFromFile(fp);
    pos = ftell(fp);

    fclose(fp);
    if (PyErr_Occurred())
        return NULL;
    return Py_BuildValue("ll", value, pos);
}

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

    PyObject *obj = PyMarshal_ReadLastObjectFromFile(fp);
    long pos = ftell(fp);

    fclose(fp);
    if (obj == NULL) {
        return NULL;
    }
    return Py_BuildValue("Nl", obj, pos);
}

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

    PyObject *obj = PyMarshal_ReadObjectFromFile(fp);
    long pos = ftell(fp);

    fclose(fp);
    if (obj == NULL) {
        return NULL;
    }
    return Py_BuildValue("Nl", obj, pos);
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
    {NULL},
};

int
_PyTestCapi_Init_Marshal(PyObject *mod)
{
    return PyModule_AddFunctions(mod, test_methods);
}
