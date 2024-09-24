#include "parts.h"
#include "util.h"

static PyObject *
dict_containsstring(PyObject *self, PyObject *args)
{
    PyObject *obj;
    const char *key;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "Oz#", &obj, &key, &size)) {
        return NULL;
    }
    NULLABLE(obj);
    RETURN_INT(PyDict_ContainsString(obj, key));
}

static PyObject *
dict_getitemref(PyObject *self, PyObject *args)
{
    PyObject *obj, *attr_name, *value = UNINITIALIZED_PTR;
    if (!PyArg_ParseTuple(args, "OO", &obj, &attr_name)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(attr_name);

    switch (PyDict_GetItemRef(obj, attr_name, &value)) {
        case -1:
            assert(value == NULL);
            return NULL;
        case 0:
            assert(value == NULL);
            return Py_NewRef(PyExc_KeyError);
        case 1:
            return value;
        default:
            Py_FatalError("PyMapping_GetItemRef() returned invalid code");
            Py_UNREACHABLE();
    }
}

static PyObject *
dict_getitemstringref(PyObject *self, PyObject *args)
{
    PyObject *obj, *value = UNINITIALIZED_PTR;
    const char *attr_name;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "Oz#", &obj, &attr_name, &size)) {
        return NULL;
    }
    NULLABLE(obj);

    switch (PyDict_GetItemStringRef(obj, attr_name, &value)) {
        case -1:
            assert(value == NULL);
            return NULL;
        case 0:
            assert(value == NULL);
            return Py_NewRef(PyExc_KeyError);
        case 1:
            return value;
        default:
            Py_FatalError("PyDict_GetItemStringRef() returned invalid code");
            Py_UNREACHABLE();
    }
}

static PyObject *
dict_setdefault(PyObject *self, PyObject *args)
{
    PyObject *mapping, *key, *defaultobj;
    if (!PyArg_ParseTuple(args, "OOO", &mapping, &key, &defaultobj)) {
        return NULL;
    }
    NULLABLE(mapping);
    NULLABLE(key);
    NULLABLE(defaultobj);
    return PyDict_SetDefault(mapping, key, defaultobj);
}

static PyObject *
dict_setdefaultref(PyObject *self, PyObject *args)
{
    PyObject *obj, *key, *default_value, *result = UNINITIALIZED_PTR;
    if (!PyArg_ParseTuple(args, "OOO", &obj, &key, &default_value)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(key);
    NULLABLE(default_value);
    switch (PyDict_SetDefaultRef(obj, key, default_value, &result)) {
        case -1:
            assert(result == NULL);
            return NULL;
        case 0:
            assert(result == default_value);
            return result;
        case 1:
            return result;
        default:
            Py_FatalError("PyDict_SetDefaultRef() returned invalid code");
            Py_UNREACHABLE();
    }
}

static PyObject *
dict_pop(PyObject *self, PyObject *args)
{
    // Test PyDict_Pop(dict, key, &value)
    PyObject *dict, *key;
    if (!PyArg_ParseTuple(args, "OO", &dict, &key)) {
        return NULL;
    }
    NULLABLE(dict);
    NULLABLE(key);
    PyObject *result = UNINITIALIZED_PTR;
    int res = PyDict_Pop(dict, key,  &result);
    if (res < 0) {
        assert(result == NULL);
        return NULL;
    }
    if (res == 0) {
        assert(result == NULL);
        result = Py_NewRef(Py_None);
    }
    else {
        assert(result != NULL);
    }
    return Py_BuildValue("iN", res, result);
}

static PyObject *
dict_pop_null(PyObject *self, PyObject *args)
{
    // Test PyDict_Pop(dict, key, NULL)
    PyObject *dict, *key;
    if (!PyArg_ParseTuple(args, "OO", &dict, &key)) {
        return NULL;
    }
    NULLABLE(dict);
    NULLABLE(key);
    RETURN_INT(PyDict_Pop(dict, key,  NULL));
}

static PyObject *
dict_popstring(PyObject *self, PyObject *args)
{
    PyObject *dict;
    const char *key;
    Py_ssize_t key_size;
    if (!PyArg_ParseTuple(args, "Oz#", &dict, &key, &key_size)) {
        return NULL;
    }
    NULLABLE(dict);
    PyObject *result = UNINITIALIZED_PTR;
    int res = PyDict_PopString(dict, key,  &result);
    if (res < 0) {
        assert(result == NULL);
        return NULL;
    }
    if (res == 0) {
        assert(result == NULL);
        result = Py_NewRef(Py_None);
    }
    else {
        assert(result != NULL);
    }
    return Py_BuildValue("iN", res, result);
}

static PyObject *
dict_popstring_null(PyObject *self, PyObject *args)
{
    PyObject *dict;
    const char *key;
    Py_ssize_t key_size;
    if (!PyArg_ParseTuple(args, "Oz#", &dict, &key, &key_size)) {
        return NULL;
    }
    NULLABLE(dict);
    RETURN_INT(PyDict_PopString(dict, key,  NULL));
}

static PyObject *
dict_version(PyObject *self, PyObject *dict)
{
    if (!PyDict_Check(dict)) {
        PyErr_SetString(PyExc_TypeError, "expected dict");
        return NULL;
    }
_Py_COMP_DIAG_PUSH
_Py_COMP_DIAG_IGNORE_DEPR_DECLS
    return PyLong_FromUnsignedLongLong(((PyDictObject *)dict)->ma_version_tag);
_Py_COMP_DIAG_POP
}

static PyMethodDef test_methods[] = {
    {"dict_containsstring", dict_containsstring, METH_VARARGS},
    {"dict_getitemref", dict_getitemref, METH_VARARGS},
    {"dict_getitemstringref", dict_getitemstringref, METH_VARARGS},
    {"dict_setdefault", dict_setdefault, METH_VARARGS},
    {"dict_setdefaultref", dict_setdefaultref, METH_VARARGS},
    {"dict_pop", dict_pop, METH_VARARGS},
    {"dict_pop_null", dict_pop_null, METH_VARARGS},
    {"dict_popstring", dict_popstring, METH_VARARGS},
    {"dict_popstring_null", dict_popstring_null, METH_VARARGS},
    {"dict_version", dict_version, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Dict(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
