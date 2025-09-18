/* Lazy object implementation */

#include "Python.h"
#include "pycore_lazyimportobject.h"

PyObject *
_PyLazyImport_New(PyObject *builtins, PyObject *from, PyObject *attr)
{
    PyLazyImportObject *m;
    if (!from || !PyUnicode_Check(from)) {
        PyErr_BadArgument();
        return NULL;
    }
    if (attr == Py_None) {
        attr = NULL;
    }
    assert(!attr || PyObject_IsTrue(attr));
    m = PyObject_GC_New(PyLazyImportObject, &PyLazyImport_Type);
    if (m == NULL) {
        return NULL;
    }
    Py_XINCREF(builtins);
    m->lz_builtins = builtins;
    Py_INCREF(from);
    m->lz_from = from;
    Py_XINCREF(attr);
    m->lz_attr = attr;
    PyObject_GC_Track(m);
    return (PyObject *)m;
}

static void
lazy_import_dealloc(PyLazyImportObject *m)
{
    PyObject_GC_UnTrack(m);
    Py_XDECREF(m->lz_builtins);
    Py_XDECREF(m->lz_from);
    Py_XDECREF(m->lz_attr);
    Py_TYPE(m)->tp_free((PyObject *)m);
}

static PyObject *
lazy_import_name(PyLazyImportObject *m)
{
    if (m->lz_attr != NULL) {
        if (PyUnicode_Check(m->lz_attr)) {
            return PyUnicode_FromFormat("%U.%U", m->lz_from, m->lz_attr);
        } else {
            return PyUnicode_FromFormat("%U...", m->lz_from);
        }
    }
    Py_INCREF(m->lz_from);
    return m->lz_from;
}

static PyObject *
lazy_import_repr(PyLazyImportObject *m)
{
    PyObject *name = lazy_import_name(m);
    if (name == NULL) {
        return NULL;
    }
    PyObject *res = PyUnicode_FromFormat("<lazy_import '%U'>", name);
    Py_DECREF(name);
    return res;
}

static int
lazy_import_traverse(PyLazyImportObject *m, visitproc visit, void *arg)
{
    Py_VISIT(m->lz_builtins);
    Py_VISIT(m->lz_from);
    Py_VISIT(m->lz_attr);
    return 0;
}

static int
lazy_import_clear(PyLazyImportObject *m)
{
    Py_CLEAR(m->lz_builtins);
    Py_CLEAR(m->lz_from);
    Py_CLEAR(m->lz_attr);
    return 0;
}

static PyObject *
lazy_import_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    if (PyTuple_GET_SIZE(args) != 2 && PyTuple_GET_SIZE(args) != 3) {
        PyErr_SetString(PyExc_ValueError, "lazy_import expected 2-3 arguments");
        return NULL;
    }

    PyObject *builtins = PyTuple_GET_ITEM(args, 0);
    PyObject *from = PyTuple_GET_ITEM(args, 1);
    PyObject *attr = NULL;
    if (PyTuple_GET_SIZE(args) == 3) {
        attr = PyTuple_GET_ITEM(args, 2);
    }

    return _PyLazyImport_New(builtins, from, attr);
}

PyObject *
_PyLazyImport_GetName(PyObject *lazy_import)
{
    assert(PyLazyImport_CheckExact(lazy_import));
    return lazy_import_name((PyLazyImportObject *)lazy_import);
}

PyTypeObject PyLazyImport_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "lazy_import",                              /* tp_name */
    sizeof(PyLazyImportObject),                       /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)lazy_import_dealloc,            /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)lazy_import_repr,                 /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,                    /* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)lazy_import_traverse,         /* tp_traverse */
    (inquiry)lazy_import_clear,                 /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    lazy_import_new,                            /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
};
