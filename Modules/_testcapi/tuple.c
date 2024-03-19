#include "parts.h"
#include "util.h"


static PyObject *
tuple_check(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyTuple_Check(obj));
}

static PyObject *
tuple_checkexact(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyTuple_CheckExact(obj));
}

static PyObject *
tuple_new(PyObject* Py_UNUSED(module), PyObject *obj)
{
    return PyTuple_New(PyLong_AsSsize_t(obj));
}

static PyObject *
tuple_pack(PyObject *Py_UNUSED(module), PyObject *args)
{
  Py_RETURN_NONE;
}

static PyObject *
tuple_size(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PyTuple_Size(obj));
}

static PyObject *
tuple_get_size(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PyTuple_GET_SIZE(obj));
}

static PyObject *
tuple_getitem(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &obj, &i)){
        return NULL;
    }
    NULLABLE(obj);
    return Py_XNewRef(PyTuple_GetItem(obj, i));
}

static PyObject *
tuple_get_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &obj, &i)){
        return NULL;
    }
    NULLABLE(obj);
    return Py_XNewRef(PyTuple_GET_ITEM(obj, i));
}

static PyObject *
tuple_getslice(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t ilow, ihigh;
    if ( !PyArg_ParseTuple(args, "Onn", &obj, &ilow, &ihigh)){
        return NULL;
    }
    NULLABLE(obj);
    return PyTuple_GetSlice(obj, ilow, ihigh);

}

static PyObject *
tuple_setitem(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value;
    Py_ssize_t i;
    if ( !PyArg_ParseTuple(args, "OnO", &obj, &i, &value)){
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(value);
    RETURN_INT(PyTuple_SetItem(obj, i, Py_XNewRef(value)));

}

static PyObject *
tuple_set_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value;
    Py_ssize_t i;
    if ( !PyArg_ParseTuple(args, "OnO", &obj, &i, &value)){
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(value);
    PyTuple_SET_ITEM(obj, i, Py_XNewRef(value));
    Py_RETURN_NONE;

}

static PyObject *
tuple_resize(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t newsize;
    if ( !PyArg_ParseTuple(args, "On", &obj, &newsize)){
        return NULL;
    }
    NULLABLE(obj);
    RETURN_INT(_PyTuple_Resize(obj, newsize));

}



static PyMethodDef test_methods[] = {
    {"tuple_check", tuple_check, METH_O},
    {"tuple_checkexact", tuple_checkexact, METH_O},
    {"tuple_new", tuple_new, METH_O},
    {"tuple_pack", tuple_pack, METH_VARARGS},
    {"tuple_size", tuple_size, METH_O},
    {"tuple_get_size", tuple_get_size, METH_O},
    {"tuple_getitem", tuple_getitem, METH_VARARGS},
    {"tuple_get_item", tuple_get_item, METH_VARARGS},
    {"tuple_getslice", tuple_getslice, METH_VARARGS},
    {"tuple_setitem", tuple_setitem, METH_VARARGS},
    {"tuple_set_item", tuple_set_item, METH_VARARGS},
    {"tuple_resize", tuple_resize, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Tuple(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0){
        return -1;
    }

    return 0;
}
