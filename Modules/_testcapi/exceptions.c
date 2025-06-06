#include "parts.h"

static PyObject *
err_set_raised(PyObject *self, PyObject *exc)
{
    Py_INCREF(exc);
    PyErr_SetRaisedException(exc);
    assert(PyErr_Occurred());
    return NULL;
}

static PyObject *
err_restore(PyObject *self, PyObject *args) {
    PyObject *type = NULL, *value = NULL, *traceback = NULL;
    switch(PyTuple_Size(args)) {
        case 3:
            traceback = PyTuple_GetItem(args, 2);
            Py_INCREF(traceback);
            /* fall through */
        case 2:
            value = PyTuple_GetItem(args, 1);
            Py_INCREF(value);
            /* fall through */
        case 1:
            type = PyTuple_GetItem(args, 0);
            Py_INCREF(type);
            break;
        default:
            PyErr_SetString(PyExc_TypeError,
                        "wrong number of arguments");
            return NULL;
    }
    PyErr_Restore(type, value, traceback);
    assert(PyErr_Occurred());
    return NULL;
}

/* To test the format of exceptions as printed out. */
static PyObject *
exception_print(PyObject *self, PyObject *args)
{
    PyObject *exc;
    int legacy = 0;

    if (!PyArg_ParseTuple(args, "O|i:exception_print", &exc, &legacy)) {
        return NULL;
    }
    if (legacy) {
        PyObject *tb = NULL;
        if (PyExceptionInstance_Check(exc)) {
            tb = PyException_GetTraceback(exc);
        }
        PyErr_Display((PyObject *) Py_TYPE(exc), exc, tb);
        Py_XDECREF(tb);
    }
    else {
        PyErr_DisplayException(exc);
    }
    Py_RETURN_NONE;
}

/* Test PyErr_NewExceptionWithDoc (also exercise PyErr_NewException).
   Run via Lib/test/test_exceptions.py */
static PyObject *
make_exception_with_doc(PyObject *self, PyObject *args, PyObject *kwargs)
{
    const char *name;
    const char *doc = NULL;
    PyObject *base = NULL;
    PyObject *dict = NULL;

    static char *kwlist[] = {"name", "doc", "base", "dict", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "s|sOO:make_exception_with_doc", kwlist,
                                     &name, &doc, &base, &dict))
    {
        return NULL;
    }

    return PyErr_NewExceptionWithDoc(name, doc, base, dict);
}

static PyObject *
exc_set_object(PyObject *self, PyObject *args)
{
    PyObject *exc;
    PyObject *obj;

    if (!PyArg_ParseTuple(args, "OO:exc_set_object", &exc, &obj)) {
        return NULL;
    }

    PyErr_SetObject(exc, obj);
    return NULL;
}

static PyObject *
exc_set_object_fetch(PyObject *self, PyObject *args)
{
    PyObject *exc;
    PyObject *obj;
    PyObject *type;
    PyObject *value;
    PyObject *tb;

    if (!PyArg_ParseTuple(args, "OO:exc_set_object", &exc, &obj)) {
        return NULL;
    }

    PyErr_SetObject(exc, obj);
    PyErr_Fetch(&type, &value, &tb);
    Py_XDECREF(type);
    Py_XDECREF(tb);
    return value;
}

static PyObject *
raise_exception(PyObject *self, PyObject *args)
{
    PyObject *exc;
    int num_args;

    if (!PyArg_ParseTuple(args, "Oi:raise_exception", &exc, &num_args)) {
        return NULL;
    }

    PyObject *exc_args = PyTuple_New(num_args);
    if (exc_args == NULL) {
        return NULL;
    }
    for (int i = 0; i < num_args; ++i) {
        PyObject *v = PyLong_FromLong(i);
        if (v == NULL) {
            Py_DECREF(exc_args);
            return NULL;
        }
        PyTuple_SET_ITEM(exc_args, i, v);
    }
    PyErr_SetObject(exc, exc_args);
    Py_DECREF(exc_args);
    return NULL;
}

/* reliably raise a MemoryError */
static PyObject *
raise_memoryerror(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyErr_NoMemory();
}

static PyObject *
test_fatal_error(PyObject *self, PyObject *args)
{
    char *message;
    int release_gil = 0;
    if (!PyArg_ParseTuple(args, "y|i:fatal_error", &message, &release_gil)) {
        return NULL;
    }
    if (release_gil) {
        Py_BEGIN_ALLOW_THREADS
        Py_FatalError(message);
        Py_END_ALLOW_THREADS
    }
    else {
        Py_FatalError(message);
    }
    // Py_FatalError() does not return, but exits the process.
    Py_RETURN_NONE;
}

static PyObject *
test_set_exc_info(PyObject *self, PyObject *args)
{
    PyObject *new_type, *new_value, *new_tb;
    PyObject *type, *value, *tb;
    if (!PyArg_ParseTuple(args, "OOO:test_set_exc_info",
                          &new_type, &new_value, &new_tb))
    {
        return NULL;
    }

    PyErr_GetExcInfo(&type, &value, &tb);

    Py_INCREF(new_type);
    Py_INCREF(new_value);
    Py_INCREF(new_tb);
    PyErr_SetExcInfo(new_type, new_value, new_tb);

    PyObject *orig_exc = PyTuple_Pack(3,
            type  ? type  : Py_None,
            value ? value : Py_None,
            tb    ? tb    : Py_None);
    Py_XDECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(tb);
    return orig_exc;
}

static PyObject *
test_set_exception(PyObject *self, PyObject *new_exc)
{
    PyObject *exc = PyErr_GetHandledException();
    assert(PyExceptionInstance_Check(exc) || exc == NULL);

    PyErr_SetHandledException(new_exc);
    return exc;
}

static PyObject *
test_write_unraisable_exc(PyObject *self, PyObject *args)
{
    PyObject *exc, *err_msg, *obj;
    if (!PyArg_ParseTuple(args, "OOO", &exc, &err_msg, &obj)) {
        return NULL;
    }

    const char *err_msg_utf8;
    if (err_msg != Py_None) {
        err_msg_utf8 = PyUnicode_AsUTF8(err_msg);
        if (err_msg_utf8 == NULL) {
            return NULL;
        }
    }
    else {
        err_msg_utf8 = NULL;
    }

    PyErr_SetObject((PyObject *)Py_TYPE(exc), exc);
    _PyErr_WriteUnraisableMsg(err_msg_utf8, obj);
    Py_RETURN_NONE;
}

/* To test the format of tracebacks as printed out. */
static PyObject *
traceback_print(PyObject *self, PyObject *args)
{
    PyObject *file;
    PyObject *traceback;

    if (!PyArg_ParseTuple(args, "OO:traceback_print",
                            &traceback, &file))
    {
        return NULL;
    }

    if (PyTraceBack_Print(traceback, file) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/*
 * Define the PyRecurdingInfinitelyError_Type
 */

static PyTypeObject PyRecursingInfinitelyError_Type;

static int
recurse_infinitely_error_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *type = (PyObject *)&PyRecursingInfinitelyError_Type;

    /* Instantiating this exception starts infinite recursion. */
    Py_INCREF(type);
    PyErr_SetObject(type, NULL);
    return -1;
}

static PyTypeObject PyRecursingInfinitelyError_Type = {
    .tp_name = "RecursingInfinitelyError",
    .tp_basicsize = sizeof(PyBaseExceptionObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = PyDoc_STR("Instantiating this exception starts infinite recursion."),
    .tp_init = (initproc)recurse_infinitely_error_init,
};

static PyMethodDef test_methods[] = {
    {"err_restore",             err_restore,                     METH_VARARGS},
    {"err_set_raised",          err_set_raised,                  METH_O},
    {"exception_print",         exception_print,                 METH_VARARGS},
    {"fatal_error",             test_fatal_error,                METH_VARARGS,
     PyDoc_STR("fatal_error(message, release_gil=False): call Py_FatalError(message)")},
    {"make_exception_with_doc", _PyCFunction_CAST(make_exception_with_doc),
     METH_VARARGS | METH_KEYWORDS},
    {"exc_set_object",          exc_set_object,                  METH_VARARGS},
    {"exc_set_object_fetch",    exc_set_object_fetch,            METH_VARARGS},
    {"raise_exception",         raise_exception,                 METH_VARARGS},
    {"raise_memoryerror",       raise_memoryerror,               METH_NOARGS},
    {"set_exc_info",            test_set_exc_info,               METH_VARARGS},
    {"set_exception",           test_set_exception,              METH_O},
    {"traceback_print",         traceback_print,                 METH_VARARGS},
    {"write_unraisable_exc",    test_write_unraisable_exc,       METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Exceptions(PyObject *mod)
{
    PyRecursingInfinitelyError_Type.tp_base = (PyTypeObject *)PyExc_Exception;
    if (PyType_Ready(&PyRecursingInfinitelyError_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(mod, "RecursingInfinitelyError",
                              (PyObject *)&PyRecursingInfinitelyError_Type) < 0)
    {
        return -1;
    }

    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
