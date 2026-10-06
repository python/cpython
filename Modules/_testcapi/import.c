#include "parts.h"
#include "util.h"

static PyObject *
pyimport_lazyimportwithoutframe(PyObject *self, PyObject *name)
{
    PyObject *lazy_import = PyImport_ImportModuleAttrString("builtins",
                                                            "__lazy_import__");
    if (lazy_import == NULL) {
        return NULL;
    }

    // Simulate being called with no running Python frame (e.g. from a freshly
    // attached C thread), so that PyEval_GetGlobals() returns NULL.
    PyThreadState *tstate = PyThreadState_Get();
    struct _PyInterpreterFrame *saved = tstate->current_frame;
    tstate->current_frame = NULL;
    PyObject *res = PyObject_CallOneArg(lazy_import, name);
    tstate->current_frame = saved;

    Py_DECREF(lazy_import);
    return res;
}

// Test PyImport_ImportModuleAttr()
static PyObject *
pyimport_importmoduleattr(PyObject *self, PyObject *args)
{
    PyObject *mod_name, *attr_name;
    if (!PyArg_ParseTuple(args, "OO", &mod_name, &attr_name)) {
        return NULL;
    }
    NULLABLE(mod_name);
    NULLABLE(attr_name);

    return PyImport_ImportModuleAttr(mod_name, attr_name);
}


// Test PyImport_ImportModuleAttrString()
static PyObject *
pyimport_importmoduleattrstring(PyObject *self, PyObject *args)
{
    const char *mod_name, *attr_name;
    Py_ssize_t len;
    if (!PyArg_ParseTuple(args, "z#z#", &mod_name, &len, &attr_name, &len)) {
        return NULL;
    }

    return PyImport_ImportModuleAttrString(mod_name, attr_name);
}


static PyObject *
pyimport_setlazyimportsmode(PyObject *self, PyObject *args)
{
    PyObject *mode;
    if (!PyArg_ParseTuple(args, "U", &mode)) {
        return NULL;
    }
    if (strcmp(PyUnicode_AsUTF8(mode), "normal") == 0) {
        PyImport_SetLazyImportsMode(PyImport_LAZY_NORMAL);
    } else if (strcmp(PyUnicode_AsUTF8(mode), "all") == 0) {
        PyImport_SetLazyImportsMode(PyImport_LAZY_ALL);
    } else {
        PyErr_SetString(PyExc_ValueError, "invalid mode");
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
pyimport_getlazyimportsmode(PyObject *self, PyObject *args)
{
    switch (PyImport_GetLazyImportsMode()) {
        case PyImport_LAZY_NORMAL:
            return PyUnicode_FromString("normal");
        case PyImport_LAZY_ALL:
            return PyUnicode_FromString("all");
        default:
            PyErr_SetString(PyExc_ValueError, "unknown mode");
            return NULL;
    }
}

static PyObject *
pyimport_setlazyimportsfilter(PyObject *self, PyObject *args)
{
    PyObject *filter;
    if (!PyArg_ParseTuple(args, "O", &filter)) {
        return NULL;
    }

    if (PyImport_SetLazyImportsFilter(filter) < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
pyimport_getlazyimportsfilter(PyObject *self, PyObject *args)
{
    PyObject *res = PyImport_GetLazyImportsFilter();
    if (res == NULL) {
        Py_RETURN_NONE;
    }
    return res;
}

static PyMethodDef test_methods[] = {
    {"PyImport_ImportModuleAttr", pyimport_importmoduleattr, METH_VARARGS},
    {"PyImport_ImportModuleAttrString", pyimport_importmoduleattrstring, METH_VARARGS},
    {"PyImport_SetLazyImportsMode", pyimport_setlazyimportsmode, METH_VARARGS},
    {"PyImport_GetLazyImportsMode", pyimport_getlazyimportsmode, METH_NOARGS},
    {"PyImport_SetLazyImportsFilter", pyimport_setlazyimportsfilter, METH_VARARGS},
    {"PyImport_GetLazyImportsFilter", pyimport_getlazyimportsfilter, METH_NOARGS},
    {"lazy_import_without_frame", pyimport_lazyimportwithoutframe, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Import(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
