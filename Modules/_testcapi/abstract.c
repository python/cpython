#include "parts.h"
#include "util.h"


static PyObject *
object_getoptionalattr(PyObject *self, PyObject *args)
{
    PyObject *obj, *attr_name, *value = UNINITIALIZED_PTR;
    if (!PyArg_ParseTuple(args, "OO", &obj, &attr_name)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(attr_name);

    switch (PyObject_GetOptionalAttr(obj, attr_name, &value)) {
        case -1:
            assert(value == NULL);
            return NULL;
        case 0:
            assert(value == NULL);
            return Py_NewRef(PyExc_AttributeError);
        case 1:
            return value;
        default:
            Py_FatalError("PyObject_GetOptionalAttr() returned invalid code");
            Py_UNREACHABLE();
    }
}

static PyObject *
object_getoptionalattrstring(PyObject *self, PyObject *args)
{
    PyObject *obj, *value = UNINITIALIZED_PTR;
    const char *attr_name;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "Oz#", &obj, &attr_name, &size)) {
        return NULL;
    }
    NULLABLE(obj);

    switch (PyObject_GetOptionalAttrString(obj, attr_name, &value)) {
        case -1:
            assert(value == NULL);
            return NULL;
        case 0:
            assert(value == NULL);
            return Py_NewRef(PyExc_AttributeError);
        case 1:
            return value;
        default:
            Py_FatalError("PyObject_GetOptionalAttrString() returned invalid code");
            Py_UNREACHABLE();
    }
}

static PyObject *
object_hasattrwitherror(PyObject *self, PyObject *args)
{
    PyObject *obj, *attr_name;
    if (!PyArg_ParseTuple(args, "OO", &obj, &attr_name)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(attr_name);
    RETURN_INT(PyObject_HasAttrWithError(obj, attr_name));
}

static PyObject *
object_hasattrstringwitherror(PyObject *self, PyObject *args)
{
    PyObject *obj;
    const char *attr_name;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "Oz#", &obj, &attr_name, &size)) {
        return NULL;
    }
    NULLABLE(obj);
    RETURN_INT(PyObject_HasAttrStringWithError(obj, attr_name));
}

static PyObject *
mapping_getoptionalitemstring(PyObject *self, PyObject *args)
{
    PyObject *obj, *value = UNINITIALIZED_PTR;
    const char *attr_name;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "Oz#", &obj, &attr_name, &size)) {
        return NULL;
    }
    NULLABLE(obj);

    switch (PyMapping_GetOptionalItemString(obj, attr_name, &value)) {
        case -1:
            assert(value == NULL);
            return NULL;
        case 0:
            assert(value == NULL);
            return Py_NewRef(PyExc_KeyError);
        case 1:
            return value;
        default:
            Py_FatalError("PyMapping_GetOptionalItemString() returned invalid code");
            Py_UNREACHABLE();
    }
}

static PyObject *
mapping_getoptionalitem(PyObject *self, PyObject *args)
{
    PyObject *obj, *attr_name, *value = UNINITIALIZED_PTR;
    if (!PyArg_ParseTuple(args, "OO", &obj, &attr_name)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(attr_name);

    switch (PyMapping_GetOptionalItem(obj, attr_name, &value)) {
        case -1:
            assert(value == NULL);
            return NULL;
        case 0:
            assert(value == NULL);
            return Py_NewRef(PyExc_KeyError);
        case 1:
            return value;
        default:
            Py_FatalError("PyMapping_GetOptionalItem() returned invalid code");
            Py_UNREACHABLE();
    }
}

static PyObject *
pyiter_next(PyObject *self, PyObject *iter)
{
    PyObject *item = PyIter_Next(iter);
    if (item == NULL && !PyErr_Occurred()) {
        Py_RETURN_NONE;
    }
    return item;
}

static PyObject *
pyiter_nextitem(PyObject *self, PyObject *iter)
{
    PyObject *item;
    int rc = PyIter_NextItem(iter, &item);
    if (rc < 0) {
        assert(PyErr_Occurred());
        assert(item == NULL);
        return NULL;
    }
    assert(!PyErr_Occurred());
    if (item == NULL) {
        Py_RETURN_NONE;
    }
    return item;
}


static PyObject *
sequence_fast_get_size(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromSsize_t(PySequence_Fast_GET_SIZE(obj));
}


static PyObject *
sequence_fast_get_item(PyObject *self, PyObject *args)
{
    PyObject *obj;
    Py_ssize_t index;
    if (!PyArg_ParseTuple(args, "On", &obj, &index)) {
        return NULL;
    }
    NULLABLE(obj);
    return PySequence_Fast_GET_ITEM(obj, index);
}


static PyObject *
object_setattr_null_exc(PyObject *self, PyObject *args)
{
    PyObject *obj, *name, *exc;
    if (!PyArg_ParseTuple(args, "OOO", &obj, &name, &exc)) {
        return NULL;
    }

    PyErr_SetObject((PyObject*)Py_TYPE(exc), exc);
    if (PyObject_SetAttr(obj, name, NULL) < 0) {
        return NULL;
    }
    assert(PyErr_Occurred());
    return NULL;
}


static PyObject *
object_setattrstring_null_exc(PyObject *self, PyObject *args)
{
    PyObject *obj, *exc;
    const char *name;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "Oz#O", &obj, &name, &size, &exc)) {
        return NULL;
    }

    PyErr_SetObject((PyObject*)Py_TYPE(exc), exc);
    if (PyObject_SetAttrString(obj, name, NULL) < 0) {
        return NULL;
    }
    assert(PyErr_Occurred());
    return NULL;
}


static PyMethodDef test_methods[] = {
    {"object_getoptionalattr", object_getoptionalattr, METH_VARARGS},
    {"object_getoptionalattrstring", object_getoptionalattrstring, METH_VARARGS},
    {"object_hasattrwitherror", object_hasattrwitherror, METH_VARARGS},
    {"object_hasattrstringwitherror", object_hasattrstringwitherror, METH_VARARGS},
    {"mapping_getoptionalitem", mapping_getoptionalitem, METH_VARARGS},
    {"mapping_getoptionalitemstring", mapping_getoptionalitemstring, METH_VARARGS},

    {"PyIter_Next", pyiter_next, METH_O},
    {"PyIter_NextItem", pyiter_nextitem, METH_O},

    {"sequence_fast_get_size", sequence_fast_get_size, METH_O},
    {"sequence_fast_get_item", sequence_fast_get_item, METH_VARARGS},
    {"object_setattr_null_exc", object_setattr_null_exc, METH_VARARGS},
    {"object_setattrstring_null_exc", object_setattrstring_null_exc, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Abstract(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
