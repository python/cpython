// Lazy object implementation.

#include "Python.h"
#include "pycore_ceval.h"
#include "pycore_frame.h"
#include "pycore_import.h"
#include "pycore_interpframe.h"
#include "pycore_lazyimportobject.h"
#include "pycore_modsupport.h"

#define PyLazyImportObject_CAST(op) ((PyLazyImportObject *)(op))

PyObject *
_PyLazyImport_New(_PyInterpreterFrame *frame, PyObject *builtins, PyObject *name, PyObject *fromlist)
{
    PyLazyImportObject *m;
    if (!name || !PyUnicode_Check(name)) {
        PyErr_SetString(PyExc_TypeError, "expected str for name");
        return NULL;
    }
    if (fromlist == Py_None || fromlist == NULL) {
        fromlist = NULL;
    }
    else if (!PyUnicode_Check(fromlist) && !PyTuple_Check(fromlist)) {
        PyErr_SetString(PyExc_TypeError,
            "lazy_import: fromlist must be None, a string, or a tuple");
        return NULL;
    }
    m = PyObject_GC_New(PyLazyImportObject, &PyLazyImport_Type);
    if (m == NULL) {
        return NULL;
    }
    m->lz_builtins = Py_XNewRef(builtins);
    m->lz_from = Py_NewRef(name);
    m->lz_attr = Py_XNewRef(fromlist);

    // Capture frame information for the original import location.
    m->lz_code = NULL;
    m->lz_instr_offset = -1;

    if (frame != NULL) {
        PyCodeObject *code = _PyFrame_GetCode(frame);
        if (code != NULL) {
            m->lz_code = (PyCodeObject *)Py_NewRef(code);
            // Calculate the instruction offset from the current frame.
            m->lz_instr_offset = _PyInterpreterFrame_LASTI(frame);
        }
    }

    _PyObject_GC_TRACK(m);
    return (PyObject *)m;
}

static int
lazy_import_traverse(PyObject *op, visitproc visit, void *arg)
{
    PyLazyImportObject *m = PyLazyImportObject_CAST(op);
    Py_VISIT(m->lz_builtins);
    Py_VISIT(m->lz_from);
    Py_VISIT(m->lz_attr);
    Py_VISIT(m->lz_code);
    return 0;
}

static int
lazy_import_clear(PyObject *op)
{
    PyLazyImportObject *m = PyLazyImportObject_CAST(op);
    Py_CLEAR(m->lz_builtins);
    Py_CLEAR(m->lz_from);
    Py_CLEAR(m->lz_attr);
    Py_CLEAR(m->lz_code);
    return 0;
}

static void
lazy_import_dealloc(PyObject *op)
{
    _PyObject_GC_UNTRACK(op);
    (void)lazy_import_clear(op);
    Py_TYPE(op)->tp_free(op);
}

static PyObject *
lazy_import_name(PyLazyImportObject *m)
{
    if (m->lz_attr != NULL) {
        if (PyUnicode_Check(m->lz_attr)) {
            return PyUnicode_FromFormat("%U.%U", m->lz_from, m->lz_attr);
        }
        else {
            return PyUnicode_FromFormat("%U...", m->lz_from);
        }
    }
    return Py_NewRef(m->lz_from);
}

static PyObject *
lazy_import_repr(PyObject *op)
{
    PyLazyImportObject *m = PyLazyImportObject_CAST(op);
    PyObject *name = lazy_import_name(m);
    if (name == NULL) {
        return NULL;
    }
    PyObject *res = PyUnicode_FromFormat("<%T '%U'>", op, name);
    Py_DECREF(name);
    return res;
}

PyObject *
_PyLazyImport_GetName(PyObject *op)
{
    PyLazyImportObject *lazy_import = PyLazyImportObject_CAST(op);
    assert(PyLazyImport_CheckExact(lazy_import));
    return lazy_import_name(lazy_import);
}

static PyObject *
lazy_import_resolve(PyObject *self, PyObject *args)
{
    return _PyImport_LoadLazyImportTstate(PyThreadState_GET(), self);
}

static PyMethodDef lazy_import_methods[] = {
    {
        "resolve", lazy_import_resolve, METH_NOARGS,
        PyDoc_STR("resolves the lazy import and returns the actual object")
    },
    {NULL, NULL}
};


PyDoc_STRVAR(lazy_import_doc,
"lazy_import(builtins, name, fromlist=None, /)\n"
"--\n"
"\n"
"Represents a deferred import that will be resolved on first use.\n"
"\n"
"Instances of this object accessed from the global scope will be\n"
"automatically imported based upon their name and then replaced with\n"
"the imported value.");

PyTypeObject PyLazyImport_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "lazy_import",
    .tp_basicsize = sizeof(PyLazyImportObject),
    .tp_dealloc = lazy_import_dealloc,
    .tp_repr = lazy_import_repr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_doc = lazy_import_doc,
    .tp_traverse = lazy_import_traverse,
    .tp_clear = lazy_import_clear,
    .tp_methods = lazy_import_methods,
    .tp_alloc = PyType_GenericAlloc,
    .tp_free = PyObject_GC_Del,
};
