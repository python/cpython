#define PY_SSIZE_T_CLEAN
#include "parts.h"
#include "util.h"


/* Test PyBytes_Check() */
static PyObject *
bytes_check(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyBytes_Check(obj));
}

/* Test PyBytes_CheckExact() */
static PyObject *
bytes_checkexact(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyBytes_CheckExact(obj));
}

/* Test PyBytes_FromStringAndSize() */
static PyObject *
bytes_fromstringandsize(PyObject *Py_UNUSED(module), PyObject *args)
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
    return PyBytes_FromStringAndSize(s, size);
}

/* Test PyBytes_FromString() */
static PyObject *
bytes_fromstring(PyObject *Py_UNUSED(module), PyObject *arg)
{
    const char *s;
    Py_ssize_t size;

    if (!PyArg_Parse(arg, "z#", &s, &size)) {
        return NULL;
    }
    return PyBytes_FromString(s);
}

/* Test PyBytes_FromObject() */
static PyObject *
bytes_fromobject(PyObject *Py_UNUSED(module), PyObject *arg)
{
    NULLABLE(arg);
    return PyBytes_FromObject(arg);
}

/* Test PyBytes_Size() */
static PyObject *
bytes_size(PyObject *Py_UNUSED(module), PyObject *arg)
{
    NULLABLE(arg);
    RETURN_SIZE(PyBytes_Size(arg));
}

/* Test PyUnicode_AsString() */
static PyObject *
bytes_asstring(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t buflen;
    const char *s;

    if (!PyArg_ParseTuple(args, "On", &obj, &buflen))
        return NULL;

    NULLABLE(obj);
    s = PyBytes_AsString(obj);
    if (s == NULL)
        return NULL;

    return PyBytes_FromStringAndSize(s, buflen);
}

/* Test PyBytes_AsStringAndSize() */
static PyObject *
bytes_asstringandsize(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t buflen;
    char *s = UNINITIALIZED_PTR;
    Py_ssize_t size = UNINITIALIZED_SIZE;

    if (!PyArg_ParseTuple(args, "On", &obj, &buflen))
        return NULL;

    NULLABLE(obj);
    if (PyBytes_AsStringAndSize(obj, &s, &size) < 0) {
        return NULL;
    }

    if (s == NULL) {
        return Py_BuildValue("(On)", Py_None, size);
    }
    else {
        return Py_BuildValue("(y#n)", s, buflen, size);
    }
}

static PyObject *
bytes_asstringandsize_null(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t buflen;
    char *s = UNINITIALIZED_PTR;

    if (!PyArg_ParseTuple(args, "On", &obj, &buflen))
        return NULL;

    NULLABLE(obj);
    if (PyBytes_AsStringAndSize(obj, &s, NULL) < 0) {
        return NULL;
    }

    if (s == NULL) {
        Py_RETURN_NONE;
    }
    else {
        return PyBytes_FromStringAndSize(s, buflen);
    }
}

/* Test PyBytes_Repr() */
static PyObject *
bytes_repr(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    int smartquotes;
    if (!PyArg_ParseTuple(args, "Oi", &obj, &smartquotes))
        return NULL;

    NULLABLE(obj);
    return PyBytes_Repr(obj, smartquotes);
}

/* Test PyBytes_Concat() */
static PyObject *
bytes_concat(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *left, *right;
    int new = 0;

    if (!PyArg_ParseTuple(args, "OO|p", &left, &right, &new))
        return NULL;

    NULLABLE(left);
    NULLABLE(right);
    if (new) {
        assert(left != NULL);
        assert(PyBytes_CheckExact(left));
        left = PyBytes_FromStringAndSize(PyBytes_AS_STRING(left),
                                         PyBytes_GET_SIZE(left));
        if (left == NULL) {
            return NULL;
        }
    }
    else {
        Py_XINCREF(left);
    }
    PyBytes_Concat(&left, right);
    if (left == NULL && !PyErr_Occurred()) {
        Py_RETURN_NONE;
    }
    return left;
}

/* Test PyBytes_ConcatAndDel() */
static PyObject *
bytes_concatanddel(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *left, *right;
    int new = 0;

    if (!PyArg_ParseTuple(args, "OO|p", &left, &right, &new))
        return NULL;

    NULLABLE(left);
    NULLABLE(right);
    if (new) {
        assert(left != NULL);
        assert(PyBytes_CheckExact(left));
        left = PyBytes_FromStringAndSize(PyBytes_AS_STRING(left),
                                         PyBytes_GET_SIZE(left));
        if (left == NULL) {
            return NULL;
        }
    }
    else {
        Py_XINCREF(left);
    }
    Py_XINCREF(right);
    PyBytes_ConcatAndDel(&left, right);
    if (left == NULL && !PyErr_Occurred()) {
        Py_RETURN_NONE;
    }
    return left;
}

/* Test PyBytes_DecodeEscape() */
static PyObject *
bytes_decodeescape(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *s;
    Py_ssize_t bsize;
    Py_ssize_t size = -100;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "z#|zn", &s, &bsize, &errors, &size))
        return NULL;

    if (size == -100) {
        size = bsize;
    }
    return PyBytes_DecodeEscape(s, size, errors, 0, NULL);
}


static PyMethodDef test_methods[] = {
    {"bytes_check", bytes_check, METH_O},
    {"bytes_checkexact", bytes_checkexact, METH_O},
    {"bytes_fromstringandsize", bytes_fromstringandsize, METH_VARARGS},
    {"bytes_fromstring", bytes_fromstring, METH_O},
    {"bytes_fromobject", bytes_fromobject, METH_O},
    {"bytes_size", bytes_size, METH_O},
    {"bytes_asstring", bytes_asstring, METH_VARARGS},
    {"bytes_asstringandsize", bytes_asstringandsize, METH_VARARGS},
    {"bytes_asstringandsize_null", bytes_asstringandsize_null, METH_VARARGS},
    {"bytes_repr", bytes_repr, METH_VARARGS},
    {"bytes_concat", bytes_concat, METH_VARARGS},
    {"bytes_concatanddel", bytes_concatanddel, METH_VARARGS},
    {"bytes_decodeescape", bytes_decodeescape, METH_VARARGS},
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
