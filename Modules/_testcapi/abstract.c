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


static PyMethodDef test_methods[] = {
    {"object_getoptionalattr", object_getoptionalattr, METH_VARARGS},
    {"object_getoptionalattrstring", object_getoptionalattrstring, METH_VARARGS},
    {"object_hasattrwitherror", object_hasattrwitherror, METH_VARARGS},
    {"object_hasattrstringwitherror", object_hasattrstringwitherror, METH_VARARGS},
    {"mapping_getoptionalitem", mapping_getoptionalitem, METH_VARARGS},
    {"mapping_getoptionalitemstring", mapping_getoptionalitemstring, METH_VARARGS},

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
