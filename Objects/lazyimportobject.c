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
#include "pycore_unicodeobject.h"

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
    m->lz_owner_globals = NULL;
    m->lz_owner_key = NULL;
    m->lz_resolved = NULL;
    m->lz_owner_pending = 0;
    m->lz_owner_finalized = 0;

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
    Py_VISIT(m->lz_owner_globals);
    Py_VISIT(m->lz_owner_key);
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
    Py_CLEAR(m->lz_owner_globals);
    Py_CLEAR(m->lz_owner_key);
    Py_CLEAR(m->lz_resolved);
    m->lz_owner_pending = 0;
    m->lz_owner_finalized = 0;
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

int
_PyLazyImport_BindGlobal(PyThreadState *tstate, PyObject *op,
                         PyObject *globals, PyObject *name)
{
    if (!PyLazyImport_CheckExact(op) ||
        !PyDict_CheckExact(globals) ||
        !PyUnicode_CheckExact(name))
    {
        return 0;
    }
    PyLazyImportObject *m = PyLazyImportObject_CAST(op);
    int bound = 0;
    PyInterpreterState *interp = tstate->interp;
    _PyImport_AcquireLock(interp);
    // First owner wins: copies of the proxy do not change the original global
    // binding that resolve() may replace.
    if (m->lz_resolved == NULL &&
        !m->lz_owner_pending &&
        !m->lz_owner_finalized &&
        m->lz_owner_globals == NULL &&
        m->lz_owner_key == NULL)
    {
        m->lz_owner_globals = Py_NewRef(globals);
        m->lz_owner_key = Py_NewRef(name);
        m->lz_owner_pending = 1;
        bound = 1;
    }
    _PyImport_ReleaseLock(interp);
    return bound;
}

static int
owner_matches(PyLazyImportObject *m, PyObject *globals, PyObject *name)
{
    return (m->lz_owner_globals == globals &&
            m->lz_owner_key != NULL &&
            _PyUnicode_Equal(m->lz_owner_key, name));
}

static void
clear_owner_if_matches(PyThreadState *tstate, PyLazyImportObject *m,
                       PyObject *globals, PyObject *name)
{
    PyInterpreterState *interp = tstate->interp;
    _PyImport_AcquireLock(interp);
    if (owner_matches(m, globals, name)) {
        m->lz_owner_pending = 0;
        // Keep lz_owner_finalized set: this owner has been committed or
        // abandoned, so later stores of the same proxy are not canonical.
        Py_CLEAR(m->lz_owner_globals);
        Py_CLEAR(m->lz_owner_key);
    }
    _PyImport_ReleaseLock(interp);
}

void
_PyLazyImport_FinishGlobalBinding(PyThreadState *tstate, PyObject *op,
                                  PyObject *globals, PyObject *name,
                                  int stored)
{
    if (!PyLazyImport_CheckExact(op) ||
        !PyDict_CheckExact(globals) ||
        !PyUnicode_CheckExact(name))
    {
        return;
    }

    PyLazyImportObject *m = PyLazyImportObject_CAST(op);
    PyInterpreterState *interp = tstate->interp;
    _PyImport_AcquireLock(interp);
    if (m->lz_owner_pending && owner_matches(m, globals, name)) {
        m->lz_owner_pending = 0;
        if (!stored) {
            Py_CLEAR(m->lz_owner_globals);
            Py_CLEAR(m->lz_owner_key);
        }
    }
    _PyImport_ReleaseLock(interp);
}

static int
finalize_owner_if_matches(PyThreadState *tstate, PyLazyImportObject *m,
                          PyObject *globals, PyObject *name)
{
    int finalized = 0;
    PyInterpreterState *interp = tstate->interp;
    _PyImport_AcquireLock(interp);
    if (!m->lz_owner_pending &&
        !m->lz_owner_finalized &&
        owner_matches(m, globals, name))
    {
        m->lz_owner_finalized = 1;
        finalized = 1;
    }
    _PyImport_ReleaseLock(interp);
    return finalized;
}

static void
restore_owner_if_matches(PyThreadState *tstate, PyLazyImportObject *m,
                         PyObject *globals, PyObject *name)
{
    PyInterpreterState *interp = tstate->interp;
    _PyImport_AcquireLock(interp);
    if (m->lz_owner_finalized && owner_matches(m, globals, name)) {
        m->lz_owner_finalized = 0;
    }
    _PyImport_ReleaseLock(interp);
}

static int
owner_is_pending(PyThreadState *tstate, PyLazyImportObject *m,
                 PyObject *globals, PyObject *name)
{
    int pending = 0;
    PyInterpreterState *interp = tstate->interp;
    _PyImport_AcquireLock(interp);
    if (m->lz_owner_pending && owner_matches(m, globals, name)) {
        pending = 1;
    }
    _PyImport_ReleaseLock(interp);
    return pending;
}

static int
claim_stored_owner(PyThreadState *tstate, PyLazyImportObject *m,
                   PyObject **globals, PyObject **name)
{
    int claimed = 0;
    PyInterpreterState *interp = tstate->interp;
    _PyImport_AcquireLock(interp);
    if (!m->lz_owner_pending &&
        !m->lz_owner_finalized &&
        m->lz_owner_globals != NULL &&
        m->lz_owner_key != NULL)
    {
        m->lz_owner_finalized = 1;
        *globals = Py_NewRef(m->lz_owner_globals);
        *name = Py_NewRef(m->lz_owner_key);
        claimed = 1;
    }
    _PyImport_ReleaseLock(interp);
    return claimed;
}

static int
commit_if_current(PyObject *op, PyObject *globals, PyObject *name,
                  PyObject *value)
{
    PyObject *current = NULL;
    int err = 0;
    Py_BEGIN_CRITICAL_SECTION(globals);
    int found = _PyDict_GetItemRef_Unicode_LockHeld(
        (PyDictObject *)globals, name, &current);
    if (found < 0) {
        err = -1;
    }
    else if (found > 0 && current == op) {
        err = _PyDict_SetItem_LockHeld((PyDictObject *)globals, name, value);
    }
    Py_END_CRITICAL_SECTION();
    Py_XDECREF(current);
    return err;
}

int
_PyLazyImport_CommitIfCurrent(PyThreadState *tstate, PyObject *op,
                              PyObject *globals, PyObject *name,
                              PyObject *value)
{
    if (!PyLazyImport_CheckExact(op) ||
        !PyDict_CheckExact(globals) ||
        !PyUnicode_CheckExact(name))
    {
        return 0;
    }

    PyLazyImportObject *m = PyLazyImportObject_CAST(op);
    if (owner_is_pending(tstate, m, globals, name)) {
        return 0;
    }

    int finalized_owner = finalize_owner_if_matches(tstate, m, globals, name);
    int err = commit_if_current(op, globals, name, value);
    if (finalized_owner) {
        if (err < 0) {
            restore_owner_if_matches(tstate, m, globals, name);
        }
        else {
            clear_owner_if_matches(tstate, m, globals, name);
        }
    }
    return err;
}

int
_PyLazyImport_CommitStoredIfCurrent(PyThreadState *tstate, PyObject *op,
                                    PyObject *value)
{
    if (!PyLazyImport_CheckExact(op)) {
        return 0;
    }

    PyLazyImportObject *m = PyLazyImportObject_CAST(op);
    PyObject *globals = NULL;
    PyObject *name = NULL;

    if (!claim_stored_owner(tstate, m, &globals, &name)) {
        return 0;
    }

    int err = commit_if_current(op, globals, name, value);
    if (err < 0) {
        restore_owner_if_matches(tstate, m, globals, name);
    }
    else {
        clear_owner_if_matches(tstate, m, globals, name);
    }
    Py_DECREF(globals);
    Py_DECREF(name);
    return err;
}

static PyObject *
lazy_import_resolve(PyObject *self, PyObject *args)
{
    PyThreadState *tstate = PyThreadState_GET();
    PyObject *resolved = _PyImport_LoadLazyImportTstate(tstate, self);
    if (resolved == NULL) {
        return NULL;
    }
    if (_PyLazyImport_CommitStoredIfCurrent(tstate, self, resolved) < 0) {
        Py_DECREF(resolved);
        return NULL;
    }
    return resolved;
}

static PyMethodDef lazy_import_methods[] = {
    {
        "resolve", lazy_import_resolve, METH_NOARGS,
        PyDoc_STR("resolve the lazy import and return the actual object")
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
