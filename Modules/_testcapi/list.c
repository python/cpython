#include "parts.h"
#include "util.h"

static PyObject *
list_check(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyList_Check(obj));
}

static PyObject *
list_check_exact(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyList_CheckExact(obj));
}

static PyObject *
list_new(PyObject* Py_UNUSED(module), PyObject *obj)
{
    return PyList_New(PyLong_AsSsize_t(obj));
}

static PyObject *
list_size(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PyList_Size(obj));
}

static PyObject *
list_get_size(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PyList_GET_SIZE(obj));
}

static PyObject *
list_getitem(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &obj, &i)) {
        return NULL;
    }
    NULLABLE(obj);
    return Py_XNewRef(PyList_GetItem(obj, i));
}

static PyObject *
list_get_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &obj, &i)) {
        return NULL;
    }
    NULLABLE(obj);
    return Py_XNewRef(PyList_GET_ITEM(obj, i));
}

static PyObject *
list_setitem(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "OnO", &obj, &i, &value)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(value);
    RETURN_INT(PyList_SetItem(obj, i, Py_XNewRef(value)));

}

static PyObject *
list_set_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "OnO", &obj, &i, &value)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(value);
    PyList_SET_ITEM(obj, i, Py_XNewRef(value));
    Py_RETURN_NONE;

}

static PyObject *
list_insert(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value;
    Py_ssize_t where;
    if (!PyArg_ParseTuple(args, "OnO", &obj, &where, &value)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(value);
    RETURN_INT(PyList_Insert(obj, where, Py_XNewRef(value)));

}

static PyObject *
list_append(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value;
    if (!PyArg_ParseTuple(args, "OO", &obj, &value)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(value);
    RETURN_INT(PyList_Append(obj, value));
}

static PyObject *
list_getslice(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t ilow, ihigh;
    if (!PyArg_ParseTuple(args, "Onn", &obj, &ilow, &ihigh)) {
        return NULL;
    }
    NULLABLE(obj);
    return PyList_GetSlice(obj, ilow, ihigh);

}

static PyObject *
list_setslice(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value;
    Py_ssize_t ilow, ihigh;
    if (!PyArg_ParseTuple(args, "OnnO", &obj, &ilow, &ihigh, &value)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(value);
    RETURN_INT(PyList_SetSlice(obj, ilow, ihigh, value));
}

static PyObject *
list_sort(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PyList_Sort(obj));
}

static PyObject *
list_reverse(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PyList_Reverse(obj));
}

static PyObject *
list_astuple(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyList_AsTuple(obj);
}


static PyObject *
list_clear(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PyList_Clear(obj));
}


static PyObject *
list_extend(PyObject* Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *arg;
    if (!PyArg_ParseTuple(args, "OO", &obj, &arg)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(arg);
    RETURN_INT(PyList_Extend(obj, arg));
}


static PyObject *
list_fromarraymoveref(PyObject* Py_UNUSED(module), PyObject *args)
{
    PyObject *src;
    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &src)) {
        return NULL;
    }

    Py_ssize_t size = PyList_GET_SIZE(src);
    PyObject **array = PyMem_Malloc(size * sizeof(PyObject**));
    if (array == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    for (Py_ssize_t i = 0; i < size; i++) {
        array[i] = Py_NewRef(PyList_GET_ITEM(src, i));
    }

    PyObject *list = PyList_FromArrayMoveRef(array, size);

    for (Py_ssize_t i = 0; i < size; i++) {
        // check that the array is not modified
        assert(array[i] == PyList_GET_ITEM(src, i));

        // check that the array was copied properly
        assert(PyList_GET_ITEM(list, i) == array[i]);
    }

    if (list == NULL) {
        for (Py_ssize_t i = 0; i < size; i++) {
            Py_DECREF(array[i]);
        }
    }
    PyMem_Free(array);

    return list;
}


static PyMethodDef test_methods[] = {
    {"list_check", list_check, METH_O},
    {"list_check_exact", list_check_exact, METH_O},
    {"list_new", list_new, METH_O},
    {"list_size", list_size, METH_O},
    {"list_get_size", list_get_size, METH_O},
    {"list_getitem", list_getitem, METH_VARARGS},
    {"list_get_item", list_get_item, METH_VARARGS},
    {"list_setitem", list_setitem, METH_VARARGS},
    {"list_set_item", list_set_item, METH_VARARGS},
    {"list_insert", list_insert, METH_VARARGS},
    {"list_append", list_append, METH_VARARGS},
    {"list_getslice", list_getslice, METH_VARARGS},
    {"list_setslice", list_setslice, METH_VARARGS},
    {"list_sort", list_sort, METH_O},
    {"list_reverse", list_reverse, METH_O},
    {"list_astuple", list_astuple, METH_O},
    {"list_clear", list_clear, METH_O},
    {"list_extend", list_extend, METH_VARARGS},
    {"list_fromarraymoveref", list_fromarraymoveref, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_List(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
