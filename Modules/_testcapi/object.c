#include "parts.h"
#include "util.h"

static PyObject *
call_pyobject_print(PyObject *self, PyObject * args)
{
    PyObject *object;
    PyObject *filename;
    PyObject *print_raw;
    FILE *fp;
    int flags = 0;

    if (!PyArg_UnpackTuple(args, "call_pyobject_print", 3, 3,
                           &object, &filename, &print_raw)) {
        return NULL;
    }

    fp = _Py_fopen_obj(filename, "w+");

    if (Py_IsTrue(print_raw)) {
        flags = Py_PRINT_RAW;
    }

    if (PyObject_Print(object, fp, flags) < 0) {
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    Py_RETURN_NONE;
}

static PyObject *
pyobject_print_null(PyObject *self, PyObject *args)
{
    PyObject *filename;
    FILE *fp;

    if (!PyArg_UnpackTuple(args, "call_pyobject_print", 1, 1, &filename)) {
        return NULL;
    }

    fp = _Py_fopen_obj(filename, "w+");

    if (PyObject_Print(NULL, fp, 0) < 0) {
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    Py_RETURN_NONE;
}

static PyObject *
pyobject_print_noref_object(PyObject *self, PyObject *args)
{
    PyObject *test_string;
    PyObject *filename;
    FILE *fp;
    char correct_string[100];

    test_string = PyUnicode_FromString("Spam spam spam");

    Py_SET_REFCNT(test_string, 0);

    PyOS_snprintf(correct_string, 100, "<refcnt %zd at %p>",
                  Py_REFCNT(test_string), (void *)test_string);

    if (!PyArg_UnpackTuple(args, "call_pyobject_print", 1, 1, &filename)) {
        return NULL;
    }

    fp = _Py_fopen_obj(filename, "w+");

    if (PyObject_Print(test_string, fp, 0) < 0){
        fclose(fp);
        Py_SET_REFCNT(test_string, 1);
        Py_DECREF(test_string);
        return NULL;
    }

    fclose(fp);

    Py_SET_REFCNT(test_string, 1);
    Py_DECREF(test_string);

    return PyUnicode_FromString(correct_string);
}

static PyObject *
pyobject_print_os_error(PyObject *self, PyObject *args)
{
    PyObject *test_string;
    PyObject *filename;
    FILE *fp;

    test_string = PyUnicode_FromString("Spam spam spam");

    if (!PyArg_UnpackTuple(args, "call_pyobject_print", 1, 1, &filename)) {
        return NULL;
    }

    // open file in read mode to induce OSError
    fp = _Py_fopen_obj(filename, "r");

    if (PyObject_Print(test_string, fp, 0) < 0) {
        fclose(fp);
        Py_DECREF(test_string);
        return NULL;
    }

    fclose(fp);
    Py_DECREF(test_string);

    Py_RETURN_NONE;
}

static PyObject *
pyobject_clear_weakrefs_no_callbacks(PyObject *self, PyObject *obj)
{
    PyUnstable_Object_ClearWeakRefsNoCallbacks(obj);
    Py_RETURN_NONE;
}

static PyMethodDef test_methods[] = {
    {"call_pyobject_print", call_pyobject_print, METH_VARARGS},
    {"pyobject_print_null", pyobject_print_null, METH_VARARGS},
    {"pyobject_print_noref_object", pyobject_print_noref_object, METH_VARARGS},
    {"pyobject_print_os_error", pyobject_print_os_error, METH_VARARGS},
    {"pyobject_clear_weakrefs_no_callbacks", pyobject_clear_weakrefs_no_callbacks, METH_O},

    {NULL},
};

int
_PyTestCapi_Init_Object(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
