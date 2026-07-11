// Lazy object implementation.

#include "Python.h"
#include "pycore_ceval.h"
#include "pycore_critical_section.h"
#include "pycore_dict.h"
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
    m->lz_globals = NULL;
    m->lz_key = NULL;
    m->lz_resolved = NULL;

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
    Py_VISIT(m->lz_globals);
    Py_VISIT(m->lz_key);
    Py_VISIT(m->lz_resolved);
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
    Py_CLEAR(m->lz_globals);
    Py_CLEAR(m->lz_key);
    Py_CLEAR(m->lz_resolved);
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

PyObject *
_PyLazyImport_GetResolved(PyObject *op)
{
    PyLazyImportObject *m = PyLazyImportObject_CAST(op);
    assert(PyLazyImport_CheckExact(op));

    PyObject *resolved = NULL;
    Py_BEGIN_CRITICAL_SECTION(op);
    resolved = Py_XNewRef(m->lz_resolved);
    Py_END_CRITICAL_SECTION();
    return resolved;
}

int
_PyLazyImport_SetGlobalBindingAndDictItem(PyObject *op, PyObject *globals,
                                          PyObject *name)
{
    assert(PyLazyImport_CheckExact(op));
    assert(PyDict_CheckExact(globals));

    PyLazyImportObject *m = PyLazyImportObject_CAST(op);
    Py_hash_t hash = PyObject_Hash(name);
    if (hash == -1) {
        return -1;
    }

    PyObject *discard_globals = NULL;
    PyObject *discard_key = NULL;
    int err;
    int recorded = 0;

    Py_BEGIN_CRITICAL_SECTION2(op, globals);
    if (m->lz_key == NULL && m->lz_resolved == NULL) {
        // Record the owner binding before publishing the proxy.  resolve()
        // may update only this key; aliases must not retarget it.
        m->lz_globals = Py_NewRef(globals);
        m->lz_key = Py_NewRef(name);
        recorded = 1;
    }
    err = _PyDict_SetItem_KnownHash_LockHeld(
        (PyDictObject *)globals, name, op, hash);
    if (err < 0 && recorded) {
        discard_globals = m->lz_globals;
        discard_key = m->lz_key;
        m->lz_globals = NULL;
        m->lz_key = NULL;
    }
    Py_END_CRITICAL_SECTION2();

    Py_XDECREF(discard_globals);
    Py_XDECREF(discard_key);
    return err;
}

static int
lazy_import_replace_dict_item_if_current(PyObject *op, PyObject *globals,
                                         PyObject *key, Py_hash_t key_hash,
                                         PyObject *resolved)
{
    assert(PyLazyImport_CheckExact(op));
    assert(PyDict_CheckExact(globals));
    assert(!PyLazyImport_CheckExact(resolved));

    PyObject *current = NULL;
    int err = 0;

    Py_BEGIN_CRITICAL_SECTION(globals);
    int found = _PyDict_GetItemRef_KnownHash_LockHeld(
        (PyDictObject *)globals, key, key_hash, &current);
    if (found < 0) {
        err = -1;
    }
    else if (found && current == op) {
        err = _PyDict_SetItem_KnownHash_LockHeld(
            (PyDictObject *)globals, key, resolved, key_hash);
    }
    Py_END_CRITICAL_SECTION();

    Py_XDECREF(current);
    return err;
}

int
_PyLazyImport_ReplaceDictItemIfCurrent(PyObject *op, PyObject *globals,
                                       PyObject *key, PyObject *resolved)
{
    assert(PyLazyImport_CheckExact(op));
    assert(PyDict_CheckExact(globals));

    Py_hash_t key_hash = PyObject_Hash(key);
    if (key_hash == -1) {
        return -1;
    }
    return lazy_import_replace_dict_item_if_current(
        op, globals, key, key_hash, resolved);
}

int
_PyLazyImport_FinishResolve(PyObject *op, PyObject *resolved)
{
    PyLazyImportObject *m = PyLazyImportObject_CAST(op);
    PyObject *globals = NULL;
    PyObject *key = NULL;
    int already_resolved = 0;

    assert(PyLazyImport_CheckExact(op));
    assert(!PyLazyImport_CheckExact(resolved));

    Py_BEGIN_CRITICAL_SECTION(op);
    if (m->lz_resolved != NULL) {
        already_resolved = 1;
    }
    else if (m->lz_globals != NULL && m->lz_key != NULL) {
        globals = Py_NewRef(m->lz_globals);
        key = Py_NewRef(m->lz_key);
    }
    Py_END_CRITICAL_SECTION();

    if (already_resolved) {
        return 0;
    }

    if (globals != NULL) {
        assert(key != NULL);
        assert(PyDict_CheckExact(globals));

        int err = _PyLazyImport_ReplaceDictItemIfCurrent(
            op, globals, key, resolved);
        if (err < 0) {
            Py_DECREF(globals);
            Py_DECREF(key);
            return err;
        }
    }

    PyObject *discard_globals = NULL;
    PyObject *discard_key = NULL;
    Py_BEGIN_CRITICAL_SECTION(op);
    if (m->lz_resolved == NULL) {
        m->lz_resolved = Py_NewRef(resolved);
    }
    if (globals != NULL &&
        m->lz_globals == globals &&
        m->lz_key == key)
    {
        discard_globals = m->lz_globals;
        discard_key = m->lz_key;
        m->lz_globals = NULL;
        m->lz_key = NULL;
    }
    Py_END_CRITICAL_SECTION();

    Py_XDECREF(discard_globals);
    Py_XDECREF(discard_key);
    Py_XDECREF(globals);
    Py_XDECREF(key);
    return 0;
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
"Represents a lazy import that will be resolved on first use.\n"
"\n"
"A successful resolution is cached. Instances of this object accessed\n"
"from the global scope will be automatically imported based upon their\n"
"name. The original global is replaced only if it still refers to this\n"
"lazy import object.");

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
