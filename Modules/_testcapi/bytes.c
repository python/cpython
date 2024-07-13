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


static int
bytes_equal(PyObject *obj, const char *str)
{
    return (PyBytes_Size(obj) == (Py_ssize_t)strlen(str)
            && strcmp(PyBytes_AsString(obj), str) == 0);
}


/* Test PyBytesWriter API */
static PyObject *
test_byteswriter(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    char *str;
    PyBytesWriter *writer = PyBytesWriter_Create(3, &str);
    if (writer == NULL) {
        return NULL;
    }

    if (PyBytesWriter_WriteBytes(writer, &str, "abc", 3) < 0) {
        goto error;
    }

    // write empty string
    if (PyBytesWriter_WriteBytes(writer, &str, "", 0) < 0) {
        goto error;
    }

    PyObject *obj = PyBytesWriter_Finish(writer, str);
    if (obj == NULL) {
        return NULL;
    }

    assert(bytes_equal(obj, "abc"));
    Py_DECREF(obj);

    Py_RETURN_NONE;

error:
    PyBytesWriter_Discard(writer);
    return NULL;
}


/* Test PyBytesWriter_Discard() */
static PyObject *
test_byteswriter_discard(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    char *str;
    PyBytesWriter *writer = PyBytesWriter_Create(3, &str);
    if (writer == NULL) {
        return NULL;
    }
    assert(PyBytesWriter_WriteBytes(writer, &str, "abc", 3) == 0);

    PyBytesWriter_Discard(writer);
    Py_RETURN_NONE;
}


/* Test PyBytesWriter_WriteBytes() */
static PyObject *
test_byteswriter_writebytes(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    char *str;
    PyBytesWriter *writer = PyBytesWriter_Create(0, &str);
    if (writer == NULL) {
        return NULL;
    }

    if (PyBytesWriter_WriteBytes(writer, &str, "abc", 3) < 0) {
        goto error;
    }
    if (PyBytesWriter_WriteBytes(writer, &str, "def", 3) < 0) {
        goto error;
    }

    PyObject *obj = PyBytesWriter_Finish(writer, str);
    if (obj == NULL) {
        return NULL;
    }

    assert(bytes_equal(obj, "abcdef"));
    Py_DECREF(obj);

    Py_RETURN_NONE;

error:
    PyBytesWriter_Discard(writer);
    return NULL;
}


/* Test PyBytesWriter_Prepare() */
static PyObject *
test_byteswriter_prepare(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    char *str;
    PyBytesWriter *writer = PyBytesWriter_Create(0, &str);
    if (writer == NULL) {
        return NULL;
    }

    // test error on purpose (negative size)
    assert(PyBytesWriter_Prepare(writer, &str, -3) < 0);
    assert(PyErr_ExceptionMatches(PyExc_ValueError));
    PyErr_Clear();

    if (PyBytesWriter_Prepare(writer, &str, 3) < 0) {
        PyBytesWriter_Discard(writer);
        return NULL;
    }

    // Write "abc"
    memcpy(str, "abc", 3);
    str += 3;

    PyObject *obj = PyBytesWriter_Finish(writer, str);
    if (obj == NULL) {
        return NULL;
    }

    assert(bytes_equal(obj, "abc"));
    Py_DECREF(obj);

    Py_RETURN_NONE;
}


static PyMethodDef test_methods[] = {
    {"bytes_resize", bytes_resize, METH_VARARGS},
    {"test_byteswriter", test_byteswriter, METH_NOARGS},
    {"test_byteswriter_discard", test_byteswriter_discard, METH_NOARGS},
    {"test_byteswriter_writebytes", test_byteswriter_writebytes, METH_NOARGS},
    {"test_byteswriter_prepare", test_byteswriter_prepare, METH_NOARGS},
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
