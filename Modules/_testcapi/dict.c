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


static int
test_dict_inner(PyObject *self, int count)
{
    Py_ssize_t pos = 0, iterations = 0;
    int i;
    PyObject *dict = PyDict_New();
    PyObject *v, *k;

    if (dict == NULL)
        return -1;

    for (i = 0; i < count; i++) {
        v = PyLong_FromLong(i);
        if (v == NULL) {
            goto error;
        }
        if (PyDict_SetItem(dict, v, v) < 0) {
            Py_DECREF(v);
            goto error;
        }
        Py_DECREF(v);
    }

    k = v = UNINITIALIZED_PTR;
    while (PyDict_Next(dict, &pos, &k, &v)) {
        PyObject *o;
        iterations++;

        assert(k != UNINITIALIZED_PTR);
        assert(v != UNINITIALIZED_PTR);
        i = PyLong_AS_LONG(v) + 1;
        o = PyLong_FromLong(i);
        if (o == NULL) {
            goto error;
        }
        if (PyDict_SetItem(dict, k, o) < 0) {
            Py_DECREF(o);
            goto error;
        }
        Py_DECREF(o);
        k = v = UNINITIALIZED_PTR;
    }
    assert(k == UNINITIALIZED_PTR);
    assert(v == UNINITIALIZED_PTR);

    Py_DECREF(dict);

    if (iterations != count) {
        PyErr_SetString(
            PyExc_AssertionError,
            "test_dict_iteration: dict iteration went wrong ");
        return -1;
    } else {
        return 0;
    }
error:
    Py_DECREF(dict);
    return -1;
}


static PyObject*
test_dict_iteration(PyObject* self, PyObject *Py_UNUSED(ignored))
{
    int i;

    for (i = 0; i < 200; i++) {
        if (test_dict_inner(self, i) < 0) {
            return NULL;
        }
    }

    Py_RETURN_NONE;
}


static PyObject*
dict_fromkeysandvalues(PyObject* self, PyObject *args)
{
    PyObject *keys_obj = UNINITIALIZED_PTR, *values_obj = UNINITIALIZED_PTR;
    Py_ssize_t length = UNINITIALIZED_SIZE;
    if (!PyArg_ParseTuple(args, "|O!O!n",
                          &PyTuple_Type, &keys_obj,
                          &PyTuple_Type, &values_obj,
                          &length)) {
        return NULL;
    }

    PyObject **keys, **values;
    if (keys_obj != UNINITIALIZED_PTR) {
        keys = &PyTuple_GET_ITEM(keys_obj, 0);
        if (values_obj != UNINITIALIZED_PTR) {
            values = &PyTuple_GET_ITEM(values_obj, 0);
        }
        else {
            values = keys + 1;
        }
    }
    else {
        keys = NULL;
        values = NULL;
    }

    if (length == UNINITIALIZED_SIZE) {
        if (keys_obj != UNINITIALIZED_PTR) {
            length = PyTuple_GET_SIZE(keys_obj);
        }
        else {
            length = 0;
        }
    }

    return PyDict_FromKeysAndValues(keys, values, length);
}


static PyObject*
dict_fromitems(PyObject* self, PyObject *args)
{
    PyObject *items_obj = UNINITIALIZED_PTR;
    Py_ssize_t length = UNINITIALIZED_SIZE;
    if (!PyArg_ParseTuple(args, "|O!n", &PyTuple_Type, &items_obj, &length)) {
        return NULL;
    }

    PyObject **items;
    if (items_obj != UNINITIALIZED_PTR) {
        items = &PyTuple_GET_ITEM(items_obj, 0);
    }
    else {
        items = NULL;
    }

    if (length == UNINITIALIZED_SIZE) {
        if (items_obj != UNINITIALIZED_PTR) {
            length = PyTuple_GET_SIZE(items_obj) / 2;
        }
        else {
            length = 0;
        }
    }

    return PyDict_FromItems(items, length);
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
    {"test_dict_iteration", test_dict_iteration, METH_NOARGS},
    {"dict_fromkeysandvalues", dict_fromkeysandvalues, METH_VARARGS},
    {"dict_fromitems", dict_fromitems, METH_VARARGS},
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
