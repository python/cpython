// Lazy object implementation.

#include "Python.h"
#include "pycore_ceval.h"
#include "pycore_dict.h"
#include "pycore_gc.h"
#include "pycore_import.h"
#include "pycore_interpframe.h"
#include "pycore_lazyimportobject.h"
#include "pycore_long.h"
#include "pycore_moduleobject.h"
#include "pycore_pyatomic_ft_wrappers.h"
#include "pycore_pyerrors.h"
#include "pycore_traceback.h"
#include "pycore_tstate.h"

typedef struct {
    PyObject_HEAD
    PyObject *lz_builtins;  // Roots own the mapping; projections retain the root.
    // A root stores its absolute name (PyUnicode) in lz_from, and original
    // fromlist in lz_attr.
    // A projection stores its source placeholder (a PyLazyImportObject)
    // in lz_from, and the attribute to import from (PyUnicode) it in lz_attr.
    PyObject *lz_from;
    PyObject *lz_attr;
    // Declaration location.
    PyCodeObject *lz_code;
    int lz_instr_offset;
    // The root was accessed, so its declared path requires import semantics.
    int lz_active;
} PyLazyImportObject;

#define PyLazyImportObject_CAST(op) ((PyLazyImportObject *)(op))

static PyObject *lazy_import_name(PyLazyImportObject *m);

PyObject *
_PyLazyImport_New(_PyInterpreterFrame *frame, PyObject *builtins,
                  PyObject *name, PyObject *fromlist)
{
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
#ifndef NDEBUG
    if (PyLazyImport_CheckExact(name)) {
        // projection
        assert(builtins == NULL);
        assert(fromlist != NULL);
        assert(PyUnicode_Check(fromlist));
    }
    else {
        // root
        assert(builtins != NULL);
    }
#endif
    PyLazyImportObject *m = PyObject_GC_New(
        PyLazyImportObject, &PyLazyImport_Type);
    if (m == NULL) {
        return NULL;
    }
    m->lz_builtins = Py_XNewRef(builtins);
    m->lz_from = Py_NewRef(name);
    m->lz_attr = Py_XNewRef(fromlist);

    m->lz_code = NULL;
    m->lz_instr_offset = -1;
    m->lz_active = 0;

    if (frame != NULL) {
        m->lz_code = (PyCodeObject *)Py_NewRef(_PyFrame_GetCode(frame));
        m->lz_instr_offset = _PyInterpreterFrame_LASTI(frame);
    }

    _PyObject_GC_TRACK(m);
    return (PyObject *)m;
}

// Reuse concrete attributes of initialized modules without waiting for imports
// or resolving lazy attributes. Failed cache lookups are retried at resolution.
// May return NULL with or without an exception set.
static PyObject *
lazy_import_get_loaded_attr(PyThreadState *tstate, PyObject *name,
                            PyObject *attr_name)
{
    PyObject *mod = NULL, *spec = NULL, *current = NULL, *attr = NULL;
    PyObject *modules = Py_XNewRef(_PyImport_GetModules(tstate->interp));
    if (modules == NULL) {
        return NULL;
    }
    int rc = PyMapping_GetOptionalItem(modules, name, &mod);
    if (rc <= 0 || !PyModule_Check(mod)) {
        goto done;
    }
    PyObject *dict = _PyModule_GetDict(mod);
    if (PyObject_GetOptionalAttr(mod, &_Py_ID(__spec__), &spec) < 0 ||
        _PyModuleSpec_IsInitializing(spec) != 0) {
        goto done;
    }
    // An initialization check can replace the module in sys.modules.
    if (modules != _PyImport_GetModules(tstate->interp) ||
        PyMapping_GetOptionalItem(modules, name, &current) <= 0 ||
        current != mod) {
        goto done;
    }
    if (PyDict_GetItemRef(dict, attr_name, &attr) < 0) {
        goto done;
    }
    if (attr != NULL && PyLazyImport_CheckExact(attr)) {
        Py_CLEAR(attr);
    }

done:
    Py_XDECREF(current);
    Py_XDECREF(spec);
    Py_XDECREF(mod);
    Py_DECREF(modules);
    if (PyErr_ExceptionMatches(PyExc_Exception)) {
        PyErr_Clear();
    }
    return attr;
}

PyObject *
_PyEval_LazyImportFrom(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       PyObject *v, PyObject *name)
{
    assert(PyLazyImport_CheckExact(v));
    assert(name);
    assert(PyUnicode_Check(name));
    PyLazyImportObject *lz = PyLazyImportObject_CAST(v);
    // Only `from a import b` can take b off an already imported a;
    // `import a.b as c` has to import a.b first.
    if (lz->lz_attr != NULL && PyTuple_Check(lz->lz_attr) &&
        PyTuple_GET_SIZE(lz->lz_attr) > 0) {
        PyObject *attr = lazy_import_get_loaded_attr(tstate, lz->lz_from, name);
        if (attr != NULL || PyErr_Occurred()) {
            return attr;
        }
    }
    return _PyLazyImport_New(frame, NULL, v, name);
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
    PyObject *value = _PyObject_GenericGetAttrWithDict(
        op, name, NULL, /* suppress */ 1);
    if (value != NULL || PyErr_Occurred()) {
        return value;
    }
    PyObject *lz_name = lazy_import_name(PyLazyImportObject_CAST(op));
    if (lz_name == NULL) {
        return NULL;
    }
    PyErr_Format(PyExc_AttributeError,
                 "cannot access attribute %R on unresolved lazy import %R",
                 name, lz_name);
    Py_DECREF(lz_name);
    return NULL;
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

// Consume a result, resolving a placeholder returned by an import hook or
// an attribute lookup under the same cycle and recursion checks.
static PyObject *
lazy_import_resolve_result(PyThreadState *tstate, PyObject *obj)
{
    if (obj == NULL || !PyLazyImport_CheckExact(obj)) {
        return obj;
    }
    PyObject *result = _PyImport_LoadLazyImportTstate(tstate, obj);
    Py_DECREF(obj);
    return result;
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
    return lazy_import_resolve_result(tstate, obj);
}

// Preserve the resolution error and attach the import's declaration location.
static void
lazy_import_add_exception_cause(PyThreadState *tstate, PyLazyImportObject *lz)
{
    if (!PyErr_Occurred() || lz->lz_code == NULL || lz->lz_instr_offset < 0) {
        return;
    }
    PyObject *exc = _PyErr_GetRaisedException(tstate);
    PyObject *name = lazy_import_name(lz);
    if (name == NULL) {
        goto done;
    }
    int lineno = PyCode_Addr2Line(lz->lz_code, lz->lz_instr_offset * 2);
    const char *filename = PyUnicode_AsUTF8(lz->lz_code->co_filename);
    if (filename == NULL) {
        goto done;
    }
    const char *funcname = PyUnicode_AsUTF8(lz->lz_code->co_name);
    if (funcname == NULL) {
        goto done;
    }
    PyBaseExceptionObject *base_exc = (PyBaseExceptionObject *)exc;
    if (base_exc->cause != NULL || base_exc->context != NULL ||
        base_exc->suppress_context) {
        // Preserve the original chain, including `raise ... from None`.
        PyObject *note = PyUnicode_FromFormat(
            "lazy import of '%U' declared in %s at %s:%d",
            name, funcname, filename, lineno);
        if (note != NULL) {
            PyObject *notes;
            if (PyObject_GetOptionalAttr(exc, &_Py_ID(__notes__), &notes) >= 0) {
                if (notes == NULL || PySequence_Contains(notes, note) == 0) {
                    (void)_PyException_AddNote(exc, note);
                }
                Py_XDECREF(notes);
            }
            Py_DECREF(note);
        }
        goto done;
    }
    PyObject *msg = PyUnicode_FromFormat(
        "lazy import of '%U' raised an exception during resolution", name);
    if (msg == NULL) {
        goto done;
    }
    PyObject *cause = PyObject_CallOneArg(PyExc_ImportError, msg);
    Py_DECREF(msg);
    if (cause == NULL) {
        goto done;
    }
    _PyErr_SetRaisedException(tstate, cause);
    _PyTraceback_Add(funcname, filename, lineno);
    PyException_SetCause(exc, _PyErr_GetRaisedException(tstate));

done:
    Py_XDECREF(name);
    _PyErr_SetRaisedException(tstate, exc);
}

int
_PyLazyImport_IsResolving(PyThreadState *tstate, PyObject *op)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    assert(PyLazyImport_CheckExact(op));
    int active = ts->lazy_imports == NULL ? 0 : PySet_Contains(ts->lazy_imports, op);
    assert(active >= 0);  // Exact placeholders use identity hashing and equality.
    return active;
}

static PyObject *
lazy_import_resolve_impl(PyThreadState *tstate, PyObject *lazy_import,
                         PyObject **imported_module)
{
    PyObject *obj = NULL;
    PyObject *fromlist = NULL;
    PyObject *import_func = NULL;
    PyObject *resolving = NULL;
    assert(lazy_import != NULL);
    assert(PyLazyImport_CheckExact(lazy_import));

    PyLazyImportObject *lz = (PyLazyImportObject *)lazy_import;

    // Walk back to the placeholder IMPORT_NAME left, and the first lookup on it.
    PyLazyImportObject *root = lz, *first = NULL;
    while (PyLazyImport_CheckExact(root->lz_from)) {
        first = root;
        root = (PyLazyImportObject *)root->lz_from;
    }

    if (_PyLazyImport_IsResolving(tstate, lazy_import)) {
        PyObject *name = lazy_import_name(lz);
        if (name == NULL) {
            return NULL;
        }
        PyObject *errmsg = PyUnicode_FromFormat(
            "cannot import name %R (most likely due to a circular import)",
            name);
        Py_DECREF(name);
        if (errmsg != NULL) {
            PyErr_SetImportErrorSubclass(PyExc_ImportCycleError, errmsg,
                                         root->lz_from, NULL);
            Py_DECREF(errmsg);
        }
        return NULL;
    }
    if (_Py_EnterRecursiveCallTstate(tstate, " while resolving a lazy import")) {
        return NULL;
    }
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    if (ts->lazy_imports == NULL) {
        ts->lazy_imports = PySet_New(NULL);
        if (ts->lazy_imports == NULL) {
            goto done;
        }
    }
    resolving = ts->lazy_imports;
    if (PySet_Add(resolving, lazy_import) < 0) {
        goto done;
    }

    // `from a import b, c`: import only the name being resolved.
    // Keep an empty tuple intact for custom __import__ hooks.
    if (first != NULL && root->lz_attr != NULL &&
        PyTuple_Check(root->lz_attr) && PyTuple_GET_SIZE(root->lz_attr) > 0) {
        fromlist = PyTuple_Pack(1, first->lz_attr);
    }
    else {
        fromlist = Py_NewRef(root->lz_attr != NULL ? root->lz_attr : Py_None);
    }
    if (fromlist == NULL) {
        goto done;
    }

    PyObject *globals = PyEval_GetGlobals();
    if (globals == NULL) {
        globals = Py_None;
    }

    if (PyMapping_GetOptionalItem(root->lz_builtins, &_Py_ID(__import__),
                                  &import_func) < 0) {
        goto done;
    }
    if (import_func == NULL) {
        PyErr_SetString(PyExc_ImportError, "__import__ not found");
        goto done;
    }
    PyObject *name = Py_NewRef(root->lz_from);
    if (first == NULL &&
        (fromlist == Py_None ||
         (PyTuple_Check(fromlist) && PyTuple_GET_SIZE(fromlist) == 0)) &&
        _PyImport_IsDefaultImportFunc(tstate->interp, import_func) &&
        root->lz_builtins == tstate->interp->builtins) {
        // A plain import binds the root package. Its pending children are
        // imported separately when their attributes are accessed.
        Py_ssize_t dot = PyUnicode_FindChar(
            name, '.', 0, PyUnicode_GET_LENGTH(name), 1);
        if (dot >= 0) {
            // Preserve eager traversal through aliased or non-package parents.
            int regular = 1;
            int complete = 1;
            Py_ssize_t end = PyUnicode_GET_LENGTH(name);
            for (Py_ssize_t i = dot; i >= 0 && regular;
                 i = PyUnicode_FindChar(name, '.', i + 1, end, 1)) {
                PyObject *prefix = PyUnicode_Substring(name, 0, i);
                PyObject *cached = prefix == NULL ? NULL :
                    lazy_import_get_loaded_attr(tstate, prefix, &_Py_ID(__name__));
                if (cached == NULL) {
                    complete = 0;
                    Py_XDECREF(prefix);
                    break;
                }
                PyObject *path = lazy_import_get_loaded_attr(
                    tstate, prefix, &_Py_ID(__path__));
                regular = path != NULL && PyUnicode_Check(cached) &&
                    PyUnicode_Compare(cached, prefix) == 0;
                Py_XDECREF(path);
                Py_DECREF(cached);
                Py_DECREF(prefix);
            }
            PyObject *loaded = lazy_import_get_loaded_attr(
                tstate, name, &_Py_ID(__name__));
            if (regular && (!complete || loaded == NULL) && !PyErr_Occurred()) {
                Py_SETREF(name, PyUnicode_Substring(name, 0, dot));
            }
            Py_XDECREF(loaded);
            if (PyErr_Occurred()) {
                Py_CLEAR(name);
            }
        }
        else if (dot == -2) {
            Py_CLEAR(name);
        }
        if (name == NULL) {
            goto done;
        }
    }
    obj = _PyEval_ImportNameWithImport(
        tstate, import_func, globals, globals,
        name, fromlist, _PyLong_GetZero()
    );
    if (obj != NULL && name != root->lz_from) {
        FT_ATOMIC_STORE_INT_RELAXED(root->lz_active, 1);
        FT_ATOMIC_STORE_INT_RELAXED(tstate->interp->imports.has_lazy_submodules, 1);
        int package = PyModule_CheckExact(obj) ?
            PyDict_Contains(_PyModule_GetDict(obj), &_Py_ID(__path__)) : 0;
        if (package < 0) {
            Py_CLEAR(obj);
        }
        else if (!package) {
            Py_DECREF(obj);
            obj = _PyEval_ImportNameWithImport(
                tstate, import_func, globals, globals,
                root->lz_from, fromlist, _PyLong_GetZero());
        }
    }
    if (obj != NULL && name == root->lz_from &&
        _PyImport_IsDefaultImportFunc(tstate->interp, import_func) &&
        _PyImport_ClearLazySubmodule(tstate, name, 0) < 0) {
        Py_CLEAR(obj);
    }
    Py_DECREF(name);
    obj = lazy_import_resolve_result(tstate, obj);
    // The normal importer may publish this module on its parent. Custom
    // hooks retain control of their own assignments to the parent.
    if (imported_module != NULL && obj != NULL && PyModule_Check(obj) &&
        _PyImport_IsDefaultImportFunc(tstate->interp, import_func)) {
        *imported_module = Py_NewRef(obj);
    }
    if (obj != NULL && first != NULL) {
        // Keep the hook and root result alive until all attribute lookups finish.
        PyObject *from = obj;
        obj = lazy_import_replay_from(tstate, from, lz);
        Py_DECREF(from);
    }

done:
    if (obj == NULL) {
        lazy_import_add_exception_cause(tstate, lz);
    }
    assert(obj == NULL || !PyLazyImport_CheckExact(obj));
    if (obj != NULL) {
        PyObject *name = lazy_import_path(lz);
        if (name == NULL ||
            _PyImport_DiscardLazyModule(tstate->interp, name) < 0) {
            Py_CLEAR(obj);
        }
        Py_XDECREF(name);
    }
    if (resolving != NULL) {
        // A failed set resize can leave the placeholder inserted. Removing by
        // identity also permits greenlets to finish in a different order.
        if (PySet_Discard(resolving, lazy_import) < 0) {
            Py_CLEAR(obj);
        }
        if (PySet_GET_SIZE(resolving) == 0) {
            // Keep the set, but release the capacity used by deep resolutions.
            (void)PySet_Clear(resolving);
        }
    }

    Py_XDECREF(fromlist);
    Py_XDECREF(import_func);
    _Py_LeaveRecursiveCallTstate(tstate);
    return obj;
}

PyObject *
_PyImport_LoadLazyImportTstate(PyThreadState *tstate, PyObject *lazy_import)
{
    return lazy_import_resolve_impl(tstate, lazy_import, NULL);
}

// Resolve a pending child as an aliased import, preserving its declaration.
PyObject *
_PyLazyImport_LoadChild(PyThreadState *tstate, PyObject *declaration,
                        PyObject *name)
{
    assert(PyLazyImport_CheckExact(declaration));
    PyLazyImportObject *source = PyLazyImportObject_CAST(declaration);
    PyObject *lz = _PyLazyImport_New(NULL, source->lz_builtins, name, NULL);
    Py_ssize_t end = PyUnicode_GET_LENGTH(name);
    Py_ssize_t dot = PyUnicode_FindChar(name, '.', 0, end, 1);
    if (dot == -2) {
        Py_CLEAR(lz);
    }
    while (lz != NULL && dot >= 0) {
        Py_ssize_t start = dot + 1;
        dot = PyUnicode_FindChar(name, '.', start, end, 1);
        if (dot == -2) {
            Py_CLEAR(lz);
            break;
        }
        PyObject *attr = PyUnicode_Substring(name, start, dot < 0 ? end : dot);
        PyObject *next = attr == NULL ? NULL :
            _PyLazyImport_New(NULL, NULL, lz, attr);
        Py_XDECREF(attr);
        Py_SETREF(lz, next);
    }
    if (lz == NULL) {
        return NULL;
    }
    PyLazyImportObject *child = PyLazyImportObject_CAST(lz);
    child->lz_code = (PyCodeObject *)Py_XNewRef(source->lz_code);
    child->lz_instr_offset = source->lz_instr_offset;
    PyObject *result = _PyImport_LoadLazyImportTstate(tstate, lz);
    Py_DECREF(lz);
    if (result != NULL && PyModule_Check(result) &&
        PyUnicode_Compare(source->lz_from, name) != 0 &&
        !PyDict_Contains(_PyModule_GetDict(result), &_Py_ID(__path__))) {
        // Reaching a non-package must still reject an invalid dotted import.
        Py_DECREF(result);
        return _PyLazyImport_LoadChild(tstate, declaration, source->lz_from);
    }
    return result;
}

int
_PyLazyImport_IsActive(PyObject *declaration)
{
    assert(PyLazyImport_CheckExact(declaration));
    return FT_ATOMIC_LOAD_INT_RELAXED(
        PyLazyImportObject_CAST(declaration)->lz_active);
}

// Loading pkg.child can replace a placeholder in pkg.child with the module
// before a from-import retrieves the value that belongs in that binding.
// This is an optimization that can be safely skipped.
static int
lazy_import_replace_child(PyThreadState *tstate, PyObject *placeholder,
                          PyObject *name, PyObject *namespace,
                          PyObject *child, PyObject *value)
{
    PyLazyImportObject *root = (PyLazyImportObject *)placeholder;
    if (!PyLazyImport_CheckExact(root->lz_from)) {
        return 0;
    }
    while (PyLazyImport_CheckExact(root->lz_from)) {
        root = (PyLazyImportObject *)root->lz_from;
    }
    Py_ssize_t end = PyUnicode_GET_LENGTH(root->lz_from);
    Py_ssize_t dot = PyUnicode_FindChar(root->lz_from, '.', 0, end, -1);
    if (dot < 0) {
        return dot == -1 ? 0 : -1;
    }
    if (end - dot - 1 != PyUnicode_GET_LENGTH(name)) {
        return 0;
    }
    Py_ssize_t matches = PyUnicode_Tailmatch(root->lz_from, name, dot + 1, end, 1);
    if (matches <= 0) {
        return matches ? -1 : 0;
    }
    PyObject *parent_name = PyUnicode_Substring(root->lz_from, 0, dot);
    if (parent_name == NULL) {
        return -1;
    }
    PyObject *modules = Py_XNewRef(_PyImport_GetModules(tstate->interp));
    PyObject *parent = NULL;
    int rc = 0;
    if (modules != NULL) {
        rc = PyMapping_GetOptionalItem(modules, parent_name, &parent);
        if (rc > 0 && PyModule_Check(parent) &&
            _PyModule_GetDict(parent) == namespace) {
            rc = _PyDict_ReplaceItemIf(namespace, name, child, value);
        }
    }
    Py_XDECREF(parent);
    Py_XDECREF(modules);
    Py_DECREF(parent_name);
    return rc;
}

PyObject *
_PyLazyImport_Reify(PyThreadState *tstate, PyObject *placeholder,
                    PyObject *name, PyObject *namespace)
{
    PyObject *imported_module = NULL;
    PyObject *value = lazy_import_resolve_impl(
        tstate, placeholder, &imported_module);
    if (value == NULL) {
        Py_XDECREF(imported_module);
        return NULL;
    }
    int rc;
    if (PyDict_CheckExact(namespace)) {
        rc = _PyDict_ReplaceItemIf(namespace, name, placeholder, value);
        if (rc == 0 && imported_module != NULL) {
            rc = lazy_import_replace_child(
                tstate, placeholder, name, namespace, imported_module, value);
        }
    }
    else if (Py_TYPE(namespace)->tp_as_mapping == NULL ||
             Py_TYPE(namespace)->tp_as_mapping->mp_ass_subscript == NULL) {
        // Read-only namespaces can resolve a value without caching it.
        Py_XDECREF(imported_module);
        return value;
    }
    else {
        // Custom namespaces retain their mapping protocol. Atomic replacement
        // is only available for exact dictionaries.
        PyObject *current;
        rc = PyMapping_GetOptionalItem(namespace, name, &current);
        if (rc > 0) {
            if (current == placeholder) {
                rc = PyObject_SetItem(namespace, name, value);
            }
            Py_DECREF(current);
        }
    }
    if (rc < 0) {
        Py_CLEAR(value);
    }
    Py_XDECREF(imported_module);
    return value;
}

static PyObject *
lazy_import_resolve(PyObject *self, PyObject *args)
{
    return _PyImport_LoadLazyImportTstate(PyThreadState_GET(), self);
}

static PyMethodDef lazy_import_methods[] = {
    {
        "resolve", lazy_import_resolve, METH_NOARGS,
        PyDoc_STR("Resolve the lazy import and return the imported object.")
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
