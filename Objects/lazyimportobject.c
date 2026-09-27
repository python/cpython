// Lazy object implementation.

#include "Python.h"
#include "pycore_ceval.h"
#include "pycore_frame.h"
#include "pycore_import.h"
#include "pycore_interp.h"
#include "pycore_long.h"
#include "pycore_setobject.h"
#include "pycore_traceback.h"
#include "pycore_interpframe.h"
#include "pycore_lazyimportobject.h"
#include "pycore_modsupport.h"

typedef struct {
    PyObject_HEAD
    PyObject *lz_builtins;
    PyObject *lz_from;
    PyObject *lz_attr;
    // Frame information for the original import location.
    PyCodeObject *lz_code;     // Code object where the lazy import was created.
    int lz_instr_offset;       // Instruction offset where the lazy import was created.
} PyLazyImportObject;

#define PyLazyImportObject_CAST(op) ((PyLazyImportObject *)(op))

PyObject *
_PyLazyImport_New(_PyInterpreterFrame *frame, PyObject *builtins, PyObject *name, PyObject *fromlist)
{
    PyLazyImportObject *m;
    if (!name || !(PyUnicode_Check(name) || PyLazyImport_CheckExact(name))) {
        PyErr_SetString(PyExc_TypeError, "expected str or lazy_import for name");
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

PyObject *
_PyEval_LazyImportFrom(PyThreadState *tstate, _PyInterpreterFrame *frame, PyObject *v, PyObject *name)
{
    assert(PyLazyImport_CheckExact(v));
    assert(name);
    assert(PyUnicode_Check(name));
    PyObject *ret;
    PyLazyImportObject *d = (PyLazyImportObject *)v;
    PyObject *mod = NULL;
    // Only `from a import b` can take b off an already imported a;
    // `import a.b as c` has to import a.b first.
    if (d->lz_attr != NULL && PyTuple_Check(d->lz_attr) &&
        PyTuple_GET_SIZE(d->lz_attr) > 0) {
        mod = PyImport_GetModule(d->lz_from);
    }
    if (mod != NULL) {
        // Check if the module already has the attribute, if so, resolve it
        // eagerly.
        if (PyModule_Check(mod)) {
            PyObject *mod_dict = PyModule_GetDict(mod);
            if (mod_dict != NULL) {
                if (PyDict_GetItemRef(mod_dict, name, &ret) < 0) {
                    Py_DECREF(mod);
                    return NULL;
                }
                if (ret != NULL) {
                    Py_DECREF(mod);
                    return ret;
                }
            }
        }
        Py_DECREF(mod);
    }

    return _PyLazyImport_New(frame, d->lz_builtins, v, name);
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

/* Specialize the error message for failed attribute lookups. */
static PyObject *
lazy_import_getattro(PyObject *op, PyObject *name)
{
    PyObject *value = _PyObject_GenericGetAttrWithDict(op, name, NULL, /* suppress */1);
    if (value == NULL) {
        if (PyErr_Occurred()) {
            // pass up non-AttributeError exception
            return NULL;
        }
        PyObject *lz_name = _PyLazyImport_GetName(op);
        if (lz_name == NULL) {
            return NULL;
        }
        PyErr_Format(PyExc_AttributeError,
                     "cannot access attribute %R on unresolved lazy import %R",
                     name, lz_name);
        Py_DECREF(lz_name);
        return NULL;
    }
    return value;
}

// The dotted name of the object that resolving the placeholder returns.
static PyObject *
lazy_import_path(PyLazyImportObject *m)
{
    if (PyLazyImport_CheckExact(m->lz_from)) {
        PyObject *base = lazy_import_path((PyLazyImportObject *)m->lz_from);
        if (base == NULL) {
            return NULL;
        }
        PyObject *res = PyUnicode_FromFormat("%U.%U", base, m->lz_attr);
        Py_DECREF(base);
        return res;
    }
    if (m->lz_attr != NULL &&
        (!PyTuple_Check(m->lz_attr) || PyTuple_GET_SIZE(m->lz_attr) > 0)) {
        return Py_NewRef(m->lz_from);
    }
    // __import__("a.b") returns the top-level package `a`.
    Py_ssize_t dot = PyUnicode_FindChar(
        m->lz_from, '.', 0, PyUnicode_GET_LENGTH(m->lz_from), 1
    );
    if (dot == -2) {
        return NULL;
    }
    if (dot < 0) {
        return Py_NewRef(m->lz_from);
    }
    return PyUnicode_Substring(m->lz_from, 0, dot);
}

static PyObject *
lazy_import_name(PyLazyImportObject *m)
{
    if (PyLazyImport_CheckExact(m->lz_from)) {
        return lazy_import_path(m);
    }
    if (m->lz_attr != NULL &&
        (!PyTuple_Check(m->lz_attr) || PyTuple_GET_SIZE(m->lz_attr) > 0)) {
        return PyUnicode_FromFormat("%U...", m->lz_from);
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

// Look up, in order, the attributes recorded from the root placeholder to lz
// on the module the root's import returned.
static PyObject *
lazy_import_replay_from(PyThreadState *tstate, PyObject *mod,
                        PyLazyImportObject *lz)
{
    if (!PyLazyImport_CheckExact(lz->lz_from)) {
        return Py_NewRef(mod);
    }
    PyObject *from = lazy_import_replay_from(
        tstate, mod, (PyLazyImportObject *)lz->lz_from);
    if (from == NULL) {
        return NULL;
    }
    PyObject *obj = _PyEval_ImportFrom(tstate, from, lz->lz_attr);
    Py_DECREF(from);
    return obj;
}

PyObject *
_PyImport_LoadLazyImportTstate(PyThreadState *tstate, PyObject *lazy_import)
{
    PyObject *obj = NULL;
    PyObject *fromlist = Py_None;
    PyObject *import_func = NULL;
    assert(lazy_import != NULL);
    assert(PyLazyImport_CheckExact(lazy_import));

    PyLazyImportObject *lz = (PyLazyImportObject *)lazy_import;
    PyInterpreterState *interp = tstate->interp;

    // Walk back to the placeholder IMPORT_NAME left, and the first lookup on it.
    PyLazyImportObject *root = lz, *first = NULL;
    while (PyLazyImport_CheckExact(root->lz_from)) {
        first = root;
        root = (PyLazyImportObject *)root->lz_from;
    }

    // Acquire the global import lock to serialize reification
    _PyImport_AcquireLock(interp);

    // Check if we are already importing this module, if so, then we want to
    // return an error that indicates we've hit a cycle which will indicate
    // the value isn't yet available.
    PyObject *importing = interp->imports.lazy_importing_modules;
    if (importing == NULL) {
        importing = interp->imports.lazy_importing_modules = PySet_New(NULL);
        if (importing == NULL) {
            _PyImport_ReleaseLock(interp);
            return NULL;
        }
    }

    assert(PyAnySet_CheckExact(importing));
    int is_loading = _PySet_Contains((PySetObject *)importing, lazy_import);
    if (is_loading < 0) {
        _PyImport_ReleaseLock(interp);
        return NULL;
    }
    else if (is_loading == 1) {
        PyObject *name = _PyLazyImport_GetName(lazy_import);
        if (name == NULL) {
            _PyImport_ReleaseLock(interp);
            return NULL;
        }
        PyObject *errmsg = PyUnicode_FromFormat(
            "cannot import name %R (most likely due to a circular import)",
            name);
        if (errmsg == NULL) {
            Py_DECREF(name);
            _PyImport_ReleaseLock(interp);
            return NULL;
        }
        PyErr_SetImportErrorSubclass(PyExc_ImportCycleError, errmsg,
                                     root->lz_from, NULL);
        Py_DECREF(errmsg);
        Py_DECREF(name);
        _PyImport_ReleaseLock(interp);
        return NULL;
    }
    else if (PySet_Add(importing, lazy_import) < 0) {
        goto error;
    }

    if (root->lz_attr != NULL) {
        // `from a import b, c`: import only the name being resolved.
        // Keep an empty tuple intact for custom __import__ hooks.
        fromlist = first && PyTuple_GET_SIZE(root->lz_attr) > 0
            ? PyTuple_Pack(1, first->lz_attr)
            : Py_NewRef(root->lz_attr);
        if (fromlist == NULL) {
            goto error;
        }
    }

    PyObject *globals = PyEval_GetGlobals();

    if (PyMapping_GetOptionalItem(root->lz_builtins, &_Py_ID(__import__),
                                  &import_func) < 0) {
        goto error;
    }
    if (import_func == NULL) {
        PyErr_SetString(PyExc_ImportError, "__import__ not found");
        goto error;
    }
    obj = _PyEval_ImportNameWithImport(
        tstate, import_func, globals, globals,
        root->lz_from, fromlist, _PyLong_GetZero()
    );
    if (obj == NULL) {
        goto error;
    }

    PyObject *from = obj;
    obj = lazy_import_replay_from(tstate, from, lz);
    Py_DECREF(from);
    if (obj == NULL) {
        goto error;
    }

    assert(!PyLazyImport_CheckExact(obj));

    goto ok;

error:
    Py_CLEAR(obj);

    // If an error occurred and we have frame information, add it to the
    // exception.
    if (PyErr_Occurred() && lz->lz_code != NULL && lz->lz_instr_offset >= 0) {
        // Get the current exception - this already has the full traceback
        // from the access point.
        PyObject *exc = _PyErr_GetRaisedException(tstate);

        // Get import name - this can fail and set an exception.
        PyObject *import_name = _PyLazyImport_GetName(lazy_import);
        if (!import_name) {
            // Failed to get import name, just restore original exception.
            _PyErr_SetRaisedException(tstate, exc);
            goto ok;
        }

        // Resolve line number from instruction offset on demand.
        int lineno = PyCode_Addr2Line((PyCodeObject *)lz->lz_code,
                                      lz->lz_instr_offset*2);

        // Get strings - these can return NULL on encoding errors.
        const char *filename_str = PyUnicode_AsUTF8(lz->lz_code->co_filename);
        if (!filename_str) {
            // Unicode conversion failed - clear error and restore original
            // exception.
            PyErr_Clear();
            Py_DECREF(import_name);
            _PyErr_SetRaisedException(tstate, exc);
            goto ok;
        }

        const char *funcname_str = PyUnicode_AsUTF8(lz->lz_code->co_name);
        if (!funcname_str) {
            // Unicode conversion failed - clear error and restore original
            // exception.
            PyErr_Clear();
            Py_DECREF(import_name);
            _PyErr_SetRaisedException(tstate, exc);
            goto ok;
        }

        // Create a cause exception showing where the lazy import was declared.
        PyObject *msg = PyUnicode_FromFormat(
            "lazy import of '%U' raised an exception during resolution",
            import_name
        );
        Py_DECREF(import_name); // Done with import_name.

        if (!msg) {
            // Failed to create message - restore original exception.
            _PyErr_SetRaisedException(tstate, exc);
            goto ok;
        }

        PyObject *cause_exc = PyObject_CallOneArg(PyExc_ImportError, msg);
        Py_DECREF(msg);  // Done with msg.

        if (!cause_exc) {
            // Failed to create exception - restore original.
            _PyErr_SetRaisedException(tstate, exc);
            goto ok;
        }

        // Add traceback entry for the lazy import declaration.
        _PyErr_SetRaisedException(tstate, cause_exc);
        _PyTraceback_Add(funcname_str, filename_str, lineno);
        PyObject *cause_with_tb = _PyErr_GetRaisedException(tstate);

        // Set the cause on the original exception.
        PyException_SetCause(exc, cause_with_tb);  // Steals ref to cause_with_tb.

        // Restore the original exception with its full traceback.
        _PyErr_SetRaisedException(tstate, exc);
    }

ok:
    if (PySet_Discard(importing, lazy_import) < 0) {
        Py_CLEAR(obj);
    }

    // Release the global import lock.
    _PyImport_ReleaseLock(interp);

    Py_XDECREF(fromlist);
    Py_XDECREF(import_func);
    return obj;
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
    .tp_getattro = lazy_import_getattro,
    .tp_traverse = lazy_import_traverse,
    .tp_clear = lazy_import_clear,
    .tp_methods = lazy_import_methods,
    .tp_alloc = PyType_GenericAlloc,
    .tp_free = PyObject_GC_Del,
};
