
/* Function object implementation */

#include "Python.h"
#include "pycore_ceval.h"               // _PyEval_BuiltinsFromGlobals()
#include "pycore_dict.h"                // _Py_INCREF_DICT()
#include "pycore_long.h"                // _PyLong_GetOne()
#include "pycore_modsupport.h"          // _PyArg_NoKeywords()
#include "pycore_object.h"              // _PyObject_GC_UNTRACK()
#include "pycore_pyerrors.h"            // _PyErr_Occurred()


static const char *
func_event_name(PyFunction_WatchEvent event) {
    switch (event) {
        #define CASE(op)                \
        case PyFunction_EVENT_##op:         \
            return "PyFunction_EVENT_" #op;
        PY_FOREACH_FUNC_EVENT(CASE)
        #undef CASE
    }
    Py_UNREACHABLE();
}

static void
notify_func_watchers(PyInterpreterState *interp, PyFunction_WatchEvent event,
                     PyFunctionObject *func, PyObject *new_value)
{
    uint8_t bits = interp->active_func_watchers;
    int i = 0;
    while (bits) {
        assert(i < FUNC_MAX_WATCHERS);
        if (bits & 1) {
            PyFunction_WatchCallback cb = interp->func_watchers[i];
            // callback must be non-null if the watcher bit is set
            assert(cb != NULL);
            if (cb(event, func, new_value) < 0) {
                PyErr_FormatUnraisable(
                    "Exception ignored in %s watcher callback for function %U at %p",
                    func_event_name(event), func->func_qualname, func);
            }
        }
        i++;
        bits >>= 1;
    }
}

static inline void
handle_func_event(PyFunction_WatchEvent event, PyFunctionObject *func,
                  PyObject *new_value)
{
    assert(Py_REFCNT(func) > 0);
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(interp->_initialized);
    if (interp->active_func_watchers) {
        notify_func_watchers(interp, event, func, new_value);
    }
    switch (event) {
        case PyFunction_EVENT_MODIFY_CODE:
        case PyFunction_EVENT_MODIFY_DEFAULTS:
        case PyFunction_EVENT_MODIFY_KWDEFAULTS:
            RARE_EVENT_INTERP_INC(interp, func_modification);
            break;
        default:
            break;
    }
}

int
PyFunction_AddWatcher(PyFunction_WatchCallback callback)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(interp->_initialized);
    for (int i = 0; i < FUNC_MAX_WATCHERS; i++) {
        if (interp->func_watchers[i] == NULL) {
            interp->func_watchers[i] = callback;
            interp->active_func_watchers |= (1 << i);
            return i;
        }
    }
    PyErr_SetString(PyExc_RuntimeError, "no more func watcher IDs available");
    return -1;
}

int
PyFunction_ClearWatcher(int watcher_id)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (watcher_id < 0 || watcher_id >= FUNC_MAX_WATCHERS) {
        PyErr_Format(PyExc_ValueError, "invalid func watcher ID %d",
                     watcher_id);
        return -1;
    }
    if (!interp->func_watchers[watcher_id]) {
        PyErr_Format(PyExc_ValueError, "no func watcher set for ID %d",
                     watcher_id);
        return -1;
    }
    interp->func_watchers[watcher_id] = NULL;
    interp->active_func_watchers &= ~(1 << watcher_id);
    return 0;
}
PyFunctionObject *
_PyFunction_FromConstructor(PyFrameConstructor *constr)
{
    PyObject *module;
    if (PyDict_GetItemRef(constr->fc_globals, &_Py_ID(__name__), &module) < 0) {
        return NULL;
    }

    PyFunctionObject *op = PyObject_GC_New(PyFunctionObject, &PyFunction_Type);
    if (op == NULL) {
        Py_XDECREF(module);
        return NULL;
    }
    op->func_globals = Py_NewRef(constr->fc_globals);
    op->func_builtins = Py_NewRef(constr->fc_builtins);
    op->func_name = Py_NewRef(constr->fc_name);
    op->func_qualname = Py_NewRef(constr->fc_qualname);
    op->func_code = Py_NewRef(constr->fc_code);
    op->func_defaults = Py_XNewRef(constr->fc_defaults);
    op->func_kwdefaults = Py_XNewRef(constr->fc_kwdefaults);
    op->func_closure = Py_XNewRef(constr->fc_closure);
    op->func_doc = Py_NewRef(Py_None);
    op->func_dict = NULL;
    op->func_weakreflist = NULL;
    op->func_module = module;
    op->func_annotations = NULL;
    op->func_typeparams = NULL;
    op->vectorcall = _PyFunction_Vectorcall;
    op->func_version = 0;
    // NOTE: functions created via FrameConstructor do not use deferred
    // reference counting because they are typically not part of cycles
    // nor accessed by multiple threads.
    _PyObject_GC_TRACK(op);
    handle_func_event(PyFunction_EVENT_CREATE, op, NULL);
    return op;
}

PyObject *
PyFunction_NewWithQualName(PyObject *code, PyObject *globals, PyObject *qualname)
{
    assert(globals != NULL);
    assert(PyDict_Check(globals));
    Py_INCREF(globals);

    PyThreadState *tstate = _PyThreadState_GET();

    PyCodeObject *code_obj = (PyCodeObject *)Py_NewRef(code);

    assert(code_obj->co_name != NULL);
    PyObject *name = Py_NewRef(code_obj->co_name);

    if (!qualname) {
        qualname = code_obj->co_qualname;
    }
    assert(qualname != NULL);
    Py_INCREF(qualname);

    PyObject *consts = code_obj->co_consts;
    assert(PyTuple_Check(consts));
    PyObject *doc;
    if (PyTuple_Size(consts) >= 1) {
        doc = PyTuple_GetItem(consts, 0);
        if (!PyUnicode_Check(doc)) {
            doc = Py_None;
        }
    }
    else {
        doc = Py_None;
    }
    Py_INCREF(doc);

    // __module__: Use globals['__name__'] if it exists, or NULL.
    PyObject *module;
    PyObject *builtins = NULL;
    if (PyDict_GetItemRef(globals, &_Py_ID(__name__), &module) < 0) {
        goto error;
    }

    builtins = _PyEval_BuiltinsFromGlobals(tstate, globals); // borrowed ref
    if (builtins == NULL) {
        goto error;
    }
    Py_INCREF(builtins);

    PyFunctionObject *op = PyObject_GC_New(PyFunctionObject, &PyFunction_Type);
    if (op == NULL) {
        goto error;
    }
    /* Note: No failures from this point on, since func_dealloc() does not
       expect a partially-created object. */

    op->func_globals = globals;
    op->func_builtins = builtins;
    op->func_name = name;
    op->func_qualname = qualname;
    op->func_code = (PyObject*)code_obj;
    op->func_defaults = NULL;    // No default positional arguments
    op->func_kwdefaults = NULL;  // No default keyword arguments
    op->func_closure = NULL;
    op->func_doc = doc;
    op->func_dict = NULL;
    op->func_weakreflist = NULL;
    op->func_module = module;
    op->func_annotations = NULL;
    op->func_typeparams = NULL;
    op->vectorcall = _PyFunction_Vectorcall;
    op->func_version = 0;
    if ((code_obj->co_flags & CO_NESTED) == 0) {
        // Use deferred reference counting for top-level functions, but not
        // nested functions because they are more likely to capture variables,
        // which makes prompt deallocation more important.
        _PyObject_SetDeferredRefcount((PyObject *)op);
    }
    _PyObject_GC_TRACK(op);
    handle_func_event(PyFunction_EVENT_CREATE, op, NULL);
    return (PyObject *)op;

error:
    Py_DECREF(globals);
    Py_DECREF(code_obj);
    Py_DECREF(name);
    Py_DECREF(qualname);
    Py_DECREF(doc);
    Py_XDECREF(module);
    Py_XDECREF(builtins);
    return NULL;
}

/*
(This is purely internal documentation. There are no public APIs here.)

Function (and code) versions
----------------------------

The Tier 1 specializer generates CALL variants that can be invalidated
by changes to critical function attributes:

- __code__
- __defaults__
- __kwdefaults__
- __closure__

For this purpose function objects have a 32-bit func_version member
that the specializer writes to the specialized instruction's inline
cache and which is checked by a guard on the specialized instructions.

The MAKE_FUNCTION bytecode sets func_version from the code object's
co_version field.  The latter is initialized from a counter in the
interpreter state (interp->func_state.next_version) and never changes.
When this counter overflows, it remains zero and the specializer loses
the ability to specialize calls to new functions.

The func_version is reset to zero when any of the critical attributes
is modified; after this point the specializer will no longer specialize
calls to this function, and the guard will always fail.

The function and code version cache
-----------------------------------

The Tier 2 optimizer now has a problem, since it needs to find the
function and code objects given only the version number from the inline
cache.  Our solution is to maintain a cache mapping version numbers to
function and code objects.  To limit the cache size we could hash
the version number, but for now we simply use it modulo the table size.

There are some corner cases (e.g. generator expressions) where we will
be unable to find the function object in the cache but we can still
find the code object.  For this reason the cache stores both the
function object and the code object.

The cache doesn't contain strong references; cache entries are
invalidated whenever the function or code object is deallocated.

Invariants
----------

These should hold at any time except when one of the cache-mutating
functions is running.

- For any slot s at index i:
    - s->func == NULL or s->func->func_version % FUNC_VERSION_CACHE_SIZE == i
    - s->code == NULL or s->code->co_version % FUNC_VERSION_CACHE_SIZE == i
    if s->func != NULL, then s->func->func_code == s->code

*/

void
_PyFunction_SetVersion(PyFunctionObject *func, uint32_t version)
{
#ifndef Py_GIL_DISABLED
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (func->func_version != 0) {
        struct _func_version_cache_item *slot =
            interp->func_state.func_version_cache
            + (func->func_version % FUNC_VERSION_CACHE_SIZE);
        if (slot->func == func) {
            slot->func = NULL;
            // Leave slot->code alone, there may be use for it.
        }
    }
#endif
    func->func_version = version;
#ifndef Py_GIL_DISABLED
    if (version != 0) {
        struct _func_version_cache_item *slot =
            interp->func_state.func_version_cache
            + (version % FUNC_VERSION_CACHE_SIZE);
        slot->func = func;
        slot->code = func->func_code;
    }
#endif
}

void
_PyFunction_ClearCodeByVersion(uint32_t version)
{
#ifndef Py_GIL_DISABLED
    PyInterpreterState *interp = _PyInterpreterState_GET();
    struct _func_version_cache_item *slot =
        interp->func_state.func_version_cache
        + (version % FUNC_VERSION_CACHE_SIZE);
    if (slot->code) {
        assert(PyCode_Check(slot->code));
        PyCodeObject *code = (PyCodeObject *)slot->code;
        if (code->co_version == version) {
            slot->code = NULL;
            slot->func = NULL;
        }
    }
#endif
}

PyFunctionObject *
_PyFunction_LookupByVersion(uint32_t version, PyObject **p_code)
{
#ifdef Py_GIL_DISABLED
    return NULL;
#else
    PyInterpreterState *interp = _PyInterpreterState_GET();
    struct _func_version_cache_item *slot =
        interp->func_state.func_version_cache
        + (version % FUNC_VERSION_CACHE_SIZE);
    if (slot->code) {
        assert(PyCode_Check(slot->code));
        PyCodeObject *code = (PyCodeObject *)slot->code;
        if (code->co_version == version) {
            *p_code = slot->code;
        }
    }
    else {
        *p_code = NULL;
    }
    if (slot->func && slot->func->func_version == version) {
        assert(slot->func->func_code == slot->code);
        return slot->func;
    }
    return NULL;
#endif
}

uint32_t
_PyFunction_GetVersionForCurrentState(PyFunctionObject *func)
{
    return func->func_version;
}

PyObject *
PyFunction_New(PyObject *code, PyObject *globals)
{
    return PyFunction_NewWithQualName(code, globals, NULL);
}

PyObject *
PyFunction_GetCode(PyObject *op)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return ((PyFunctionObject *) op) -> func_code;
}

PyObject *
PyFunction_GetGlobals(PyObject *op)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return ((PyFunctionObject *) op) -> func_globals;
}

PyObject *
PyFunction_GetModule(PyObject *op)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return ((PyFunctionObject *) op) -> func_module;
}

PyObject *
PyFunction_GetDefaults(PyObject *op)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return ((PyFunctionObject *) op) -> func_defaults;
}

int
PyFunction_SetDefaults(PyObject *op, PyObject *defaults)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (defaults == Py_None)
        defaults = NULL;
    else if (defaults && PyTuple_Check(defaults)) {
        Py_INCREF(defaults);
    }
    else {
        PyErr_SetString(PyExc_SystemError, "non-tuple default args");
        return -1;
    }
    handle_func_event(PyFunction_EVENT_MODIFY_DEFAULTS,
                      (PyFunctionObject *) op, defaults);
    _PyFunction_SetVersion((PyFunctionObject *)op, 0);
    Py_XSETREF(((PyFunctionObject *)op)->func_defaults, defaults);
    return 0;
}

void
PyFunction_SetVectorcall(PyFunctionObject *func, vectorcallfunc vectorcall)
{
    assert(func != NULL);
    _PyFunction_SetVersion(func, 0);
    func->vectorcall = vectorcall;
}

PyObject *
PyFunction_GetKwDefaults(PyObject *op)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return ((PyFunctionObject *) op) -> func_kwdefaults;
}

int
PyFunction_SetKwDefaults(PyObject *op, PyObject *defaults)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (defaults == Py_None)
        defaults = NULL;
    else if (defaults && PyDict_Check(defaults)) {
        Py_INCREF(defaults);
    }
    else {
        PyErr_SetString(PyExc_SystemError,
                        "non-dict keyword only default args");
        return -1;
    }
    handle_func_event(PyFunction_EVENT_MODIFY_KWDEFAULTS,
                      (PyFunctionObject *) op, defaults);
    _PyFunction_SetVersion((PyFunctionObject *)op, 0);
    Py_XSETREF(((PyFunctionObject *)op)->func_kwdefaults, defaults);
    return 0;
}

PyObject *
PyFunction_GetClosure(PyObject *op)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return ((PyFunctionObject *) op) -> func_closure;
}

int
PyFunction_SetClosure(PyObject *op, PyObject *closure)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (closure == Py_None)
        closure = NULL;
    else if (PyTuple_Check(closure)) {
        Py_INCREF(closure);
    }
    else {
        PyErr_Format(PyExc_SystemError,
                     "expected tuple for closure, got '%.100s'",
                     Py_TYPE(closure)->tp_name);
        return -1;
    }
    _PyFunction_SetVersion((PyFunctionObject *)op, 0);
    Py_XSETREF(((PyFunctionObject *)op)->func_closure, closure);
    return 0;
}

static PyObject *
func_get_annotation_dict(PyFunctionObject *op)
{
    if (op->func_annotations == NULL) {
        return NULL;
    }
    if (PyTuple_CheckExact(op->func_annotations)) {
        PyObject *ann_tuple = op->func_annotations;
        PyObject *ann_dict = PyDict_New();
        if (ann_dict == NULL) {
            return NULL;
        }

        assert(PyTuple_GET_SIZE(ann_tuple) % 2 == 0);

        for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(ann_tuple); i += 2) {
            int err = PyDict_SetItem(ann_dict,
                                     PyTuple_GET_ITEM(ann_tuple, i),
                                     PyTuple_GET_ITEM(ann_tuple, i + 1));

            if (err < 0) {
                return NULL;
            }
        }
        Py_SETREF(op->func_annotations, ann_dict);
    }
    assert(PyDict_Check(op->func_annotations));
    return op->func_annotations;
}

PyObject *
PyFunction_GetAnnotations(PyObject *op)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return func_get_annotation_dict((PyFunctionObject *)op);
}

int
PyFunction_SetAnnotations(PyObject *op, PyObject *annotations)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (annotations == Py_None)
        annotations = NULL;
    else if (annotations && PyDict_Check(annotations)) {
        Py_INCREF(annotations);
    }
    else {
        PyErr_SetString(PyExc_SystemError,
                        "non-dict annotations");
        return -1;
    }
    Py_XSETREF(((PyFunctionObject *)op)->func_annotations, annotations);
    return 0;
}

/* Methods */

#define OFF(x) offsetof(PyFunctionObject, x)

static PyMemberDef func_memberlist[] = {
    {"__closure__",   _Py_T_OBJECT,     OFF(func_closure), Py_READONLY},
    {"__doc__",       _Py_T_OBJECT,     OFF(func_doc), 0},
    {"__globals__",   _Py_T_OBJECT,     OFF(func_globals), Py_READONLY},
    {"__module__",    _Py_T_OBJECT,     OFF(func_module), 0},
    {"__builtins__",  _Py_T_OBJECT,     OFF(func_builtins), Py_READONLY},
    {NULL}  /* Sentinel */
};

/*[clinic input]
class function "PyFunctionObject *" "&PyFunction_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=70af9c90aa2e71b0]*/

#include "clinic/funcobject.c.h"

static PyObject *
func_get_code(PyObject *self, void *Py_UNUSED(ignored))
{
    PyFunctionObject* op = _PyFunction_CAST(self);
    if (PySys_Audit("object.__getattr__", "Os", op, "__code__") < 0) {
        return NULL;
    }

    return Py_NewRef(op->func_code);
}

static int
func_set_code(PyObject *self, PyObject *value, void *Py_UNUSED(ignored))
{
    PyFunctionObject* op = _PyFunction_CAST(self);
    Py_ssize_t nclosure;
    int nfree;

    /* Not legal to del f.func_code or to set it to anything
     * other than a code object. */
    if (value == NULL || !PyCode_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "__code__ must be set to a code object");
        return -1;
    }

    if (PySys_Audit("object.__setattr__", "OsO",
                    op, "__code__", value) < 0) {
        return -1;
    }

    nfree = ((PyCodeObject *)value)->co_nfreevars;
    nclosure = (op->func_closure == NULL ? 0 :
            PyTuple_GET_SIZE(op->func_closure));
    if (nclosure != nfree) {
        PyErr_Format(PyExc_ValueError,
                     "%U() requires a code object with %zd free vars,"
                     " not %zd",
                     op->func_name,
                     nclosure, nfree);
        return -1;
    }

    PyObject *func_code = PyFunction_GET_CODE(op);
    int old_flags = ((PyCodeObject *)func_code)->co_flags;
    int new_flags = ((PyCodeObject *)value)->co_flags;
    int mask = CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR;
    if ((old_flags & mask) != (new_flags & mask)) {
        if (PyErr_Warn(PyExc_DeprecationWarning,
            "Assigning a code object of non-matching type is deprecated "
            "(e.g., from a generator to a plain function)") < 0)
        {
            return -1;
        }
    }

    handle_func_event(PyFunction_EVENT_MODIFY_CODE, op, value);
    _PyFunction_SetVersion(op, 0);
    Py_XSETREF(op->func_code, Py_NewRef(value));
    return 0;
}

static PyObject *
func_get_name(PyObject *self, void *Py_UNUSED(ignored))
{
    PyFunctionObject* op = _PyFunction_CAST(self);
    return Py_NewRef(op->func_name);
}

static int
func_set_name(PyObject *self, PyObject *value, void *Py_UNUSED(ignored))
{
    PyFunctionObject * op = _PyFunction_CAST(self);
    /* Not legal to del f.func_name or to set it to anything
     * other than a string object. */
    if (value == NULL || !PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "__name__ must be set to a string object");
        return -1;
    }
    Py_XSETREF(op->func_name, Py_NewRef(value));
    return 0;
}

static PyObject *
func_get_qualname(PyObject *self, void *Py_UNUSED(ignored))
{
    PyFunctionObject *op = _PyFunction_CAST(self);
    return Py_NewRef(op->func_qualname);
}

static int
func_set_qualname(PyObject *self, PyObject *value, void *Py_UNUSED(ignored))
{
    PyFunctionObject *op = _PyFunction_CAST(self);
    /* Not legal to del f.__qualname__ or to set it to anything
     * other than a string object. */
    if (value == NULL || !PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "__qualname__ must be set to a string object");
        return -1;
    }
    Py_XSETREF(op->func_qualname, Py_NewRef(value));
    return 0;
}

static PyObject *
func_get_defaults(PyObject *self, void *Py_UNUSED(ignored))
{
    PyFunctionObject * op = _PyFunction_CAST(self);
    if (PySys_Audit("object.__getattr__", "Os", op, "__defaults__") < 0) {
        return NULL;
    }
    if (op->func_defaults == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(op->func_defaults);
}

static int
func_set_defaults(PyObject *self, PyObject *value, void *Py_UNUSED(ignored))
{
    PyFunctionObject *op = _PyFunction_CAST(self);
    /* Legal to del f.func_defaults.
     * Can only set func_defaults to NULL or a tuple. */
    if (value == Py_None)
        value = NULL;
    if (value != NULL && !PyTuple_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "__defaults__ must be set to a tuple object");
        return -1;
    }
    if (value) {
        if (PySys_Audit("object.__setattr__", "OsO",
                        op, "__defaults__", value) < 0) {
            return -1;
        }
    } else if (PySys_Audit("object.__delattr__", "Os",
                           op, "__defaults__") < 0) {
        return -1;
    }

    handle_func_event(PyFunction_EVENT_MODIFY_DEFAULTS, op, value);
    _PyFunction_SetVersion(op, 0);
    Py_XSETREF(op->func_defaults, Py_XNewRef(value));
    return 0;
}

static PyObject *
func_get_kwdefaults(PyObject *self, void *Py_UNUSED(ignored))
{
    PyFunctionObject * op = _PyFunction_CAST(self);
    if (PySys_Audit("object.__getattr__", "Os",
                    op, "__kwdefaults__") < 0) {
        return NULL;
    }
    if (op->func_kwdefaults == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(op->func_kwdefaults);
}

static int
func_set_kwdefaults(PyObject *self, PyObject *value, void *Py_UNUSED(ignored))
{
    PyFunctionObject* op = _PyFunction_CAST(self);
    if (value == Py_None)
        value = NULL;
    /* Legal to del f.func_kwdefaults.
     * Can only set func_kwdefaults to NULL or a dict. */
    if (value != NULL && !PyDict_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
            "__kwdefaults__ must be set to a dict object");
        return -1;
    }
    if (value) {
        if (PySys_Audit("object.__setattr__", "OsO",
                        op, "__kwdefaults__", value) < 0) {
            return -1;
        }
    } else if (PySys_Audit("object.__delattr__", "Os",
                           op, "__kwdefaults__") < 0) {
        return -1;
    }

    handle_func_event(PyFunction_EVENT_MODIFY_KWDEFAULTS, op, value);
    _PyFunction_SetVersion(op, 0);
    Py_XSETREF(op->func_kwdefaults, Py_XNewRef(value));
    return 0;
}


/*[clinic input]
@critical_section
@getter
function.__annotations__

Dict of annotations in a function object.
[clinic start generated code]*/

static PyObject *
function___annotations___get_impl(PyFunctionObject *self)
/*[clinic end generated code: output=a4cf4c884c934cbb input=92643d7186c1ad0c]*/
{
    PyObject *d = NULL;
    if (self->func_annotations == NULL) {
        self->func_annotations = PyDict_New();
        if (self->func_annotations == NULL)
            return NULL;
    }
    d = func_get_annotation_dict(self);
    return Py_XNewRef(d);
}

/*[clinic input]
@critical_section
@setter
function.__annotations__
[clinic start generated code]*/

static int
function___annotations___set_impl(PyFunctionObject *self, PyObject *value)
/*[clinic end generated code: output=a61795d4a95eede4 input=5302641f686f0463]*/
{
    if (value == Py_None)
        value = NULL;
    /* Legal to del f.func_annotations.
     * Can only set func_annotations to NULL (through C api)
     * or a dict. */
    if (value != NULL && !PyDict_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
            "__annotations__ must be set to a dict object");
        return -1;
    }
    Py_XSETREF(self->func_annotations, Py_XNewRef(value));
    return 0;
}

/*[clinic input]
@critical_section
@getter
function.__type_params__

Get the declared type parameters for a function.
[clinic start generated code]*/

static PyObject *
function___type_params___get_impl(PyFunctionObject *self)
/*[clinic end generated code: output=eb844d7ffca517a8 input=0864721484293724]*/
{
    if (self->func_typeparams == NULL) {
        return PyTuple_New(0);
    }

    assert(PyTuple_Check(self->func_typeparams));
    return Py_NewRef(self->func_typeparams);
}

/*[clinic input]
@critical_section
@setter
function.__type_params__
[clinic start generated code]*/

static int
function___type_params___set_impl(PyFunctionObject *self, PyObject *value)
/*[clinic end generated code: output=038b4cda220e56fb input=3862fbd4db2b70e8]*/
{
    /* Not legal to del f.__type_params__ or to set it to anything
     * other than a tuple object. */
    if (value == NULL || !PyTuple_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "__type_params__ must be set to a tuple");
        return -1;
    }
    Py_XSETREF(self->func_typeparams, Py_NewRef(value));
    return 0;
}

PyObject *
_Py_set_function_type_params(PyThreadState *Py_UNUSED(ignored), PyObject *func,
                             PyObject *type_params)
{
    assert(PyFunction_Check(func));
    assert(PyTuple_Check(type_params));
    PyFunctionObject *f = (PyFunctionObject *)func;
    Py_XSETREF(f->func_typeparams, Py_NewRef(type_params));
    return Py_NewRef(func);
}

static PyGetSetDef func_getsetlist[] = {
    {"__code__", func_get_code, func_set_code},
    {"__defaults__", func_get_defaults, func_set_defaults},
    {"__kwdefaults__", func_get_kwdefaults, func_set_kwdefaults},
    FUNCTION___ANNOTATIONS___GETSETDEF
    {"__dict__", PyObject_GenericGetDict, PyObject_GenericSetDict},
    {"__name__", func_get_name, func_set_name},
    {"__qualname__", func_get_qualname, func_set_qualname},
    FUNCTION___TYPE_PARAMS___GETSETDEF
    {NULL} /* Sentinel */
};

/* function.__new__() maintains the following invariants for closures.
   The closure must correspond to the free variables of the code object.

   if len(code.co_freevars) == 0:
       closure = NULL
   else:
       len(closure) == len(code.co_freevars)
   for every elt in closure, type(elt) == cell
*/

/*[clinic input]
@classmethod
function.__new__ as func_new
    code: object(type="PyCodeObject *", subclass_of="&PyCode_Type")
        a code object
    globals: object(subclass_of="&PyDict_Type")
        the globals dictionary
    name: object = None
        a string that overrides the name from the code object
    argdefs as defaults: object = None
        a tuple that specifies the default argument values
    closure: object = None
        a tuple that supplies the bindings for free variables
    kwdefaults: object = None
        a dictionary that specifies the default keyword argument values

Create a function object.
[clinic start generated code]*/

static PyObject *
func_new_impl(PyTypeObject *type, PyCodeObject *code, PyObject *globals,
              PyObject *name, PyObject *defaults, PyObject *closure,
              PyObject *kwdefaults)
/*[clinic end generated code: output=de72f4c22ac57144 input=20c9c9f04ad2d3f2]*/
{
    PyFunctionObject *newfunc;
    Py_ssize_t nclosure;

    if (name != Py_None && !PyUnicode_Check(name)) {
        PyErr_SetString(PyExc_TypeError,
                        "arg 3 (name) must be None or string");
        return NULL;
    }
    if (defaults != Py_None && !PyTuple_Check(defaults)) {
        PyErr_SetString(PyExc_TypeError,
                        "arg 4 (defaults) must be None or tuple");
        return NULL;
    }
    if (!PyTuple_Check(closure)) {
        if (code->co_nfreevars && closure == Py_None) {
            PyErr_SetString(PyExc_TypeError,
                            "arg 5 (closure) must be tuple");
            return NULL;
        }
        else if (closure != Py_None) {
            PyErr_SetString(PyExc_TypeError,
                "arg 5 (closure) must be None or tuple");
            return NULL;
        }
    }
    if (kwdefaults != Py_None && !PyDict_Check(kwdefaults)) {
        PyErr_SetString(PyExc_TypeError,
                        "arg 6 (kwdefaults) must be None or dict");
        return NULL;
    }

    /* check that the closure is well-formed */
    nclosure = closure == Py_None ? 0 : PyTuple_GET_SIZE(closure);
    if (code->co_nfreevars != nclosure)
        return PyErr_Format(PyExc_ValueError,
                            "%U requires closure of length %zd, not %zd",
                            code->co_name, code->co_nfreevars, nclosure);
    if (nclosure) {
        Py_ssize_t i;
        for (i = 0; i < nclosure; i++) {
            PyObject *o = PyTuple_GET_ITEM(closure, i);
            if (!PyCell_Check(o)) {
                return PyErr_Format(PyExc_TypeError,
                    "arg 5 (closure) expected cell, found %s",
                                    Py_TYPE(o)->tp_name);
            }
        }
    }
    if (PySys_Audit("function.__new__", "O", code) < 0) {
        return NULL;
    }

    newfunc = (PyFunctionObject *)PyFunction_New((PyObject *)code,
                                                 globals);
    if (newfunc == NULL) {
        return NULL;
    }
    if (name != Py_None) {
        Py_SETREF(newfunc->func_name, Py_NewRef(name));
    }
    if (defaults != Py_None) {
        newfunc->func_defaults = Py_NewRef(defaults);
    }
    if (closure != Py_None) {
        newfunc->func_closure = Py_NewRef(closure);
    }
    if (kwdefaults != Py_None) {
        newfunc->func_kwdefaults = Py_NewRef(kwdefaults);
    }

    return (PyObject *)newfunc;
}

static int
func_clear(PyFunctionObject *op)
{
    _PyFunction_SetVersion(op, 0);
    Py_CLEAR(op->func_globals);
    Py_CLEAR(op->func_builtins);
    Py_CLEAR(op->func_module);
    Py_CLEAR(op->func_defaults);
    Py_CLEAR(op->func_kwdefaults);
    Py_CLEAR(op->func_doc);
    Py_CLEAR(op->func_dict);
    Py_CLEAR(op->func_closure);
    Py_CLEAR(op->func_annotations);
    Py_CLEAR(op->func_typeparams);
    // Don't Py_CLEAR(op->func_code), since code is always required
    // to be non-NULL. Similarly, name and qualname shouldn't be NULL.
    // However, name and qualname could be str subclasses, so they
    // could have reference cycles. The solution is to replace them
    // with a genuinely immutable string.
    Py_SETREF(op->func_name, &_Py_STR(empty));
    Py_SETREF(op->func_qualname, &_Py_STR(empty));
    return 0;
}

static void
func_dealloc(PyFunctionObject *op)
{
    _PyObject_ResurrectStart((PyObject *)op);
    handle_func_event(PyFunction_EVENT_DESTROY, op, NULL);
    if (_PyObject_ResurrectEnd((PyObject *)op)) {
        return;
    }
    _PyObject_GC_UNTRACK(op);
    if (op->func_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *) op);
    }
    _PyFunction_SetVersion(op, 0);
    (void)func_clear(op);
    // These aren't cleared by func_clear().
    Py_DECREF(op->func_code);
    Py_DECREF(op->func_name);
    Py_DECREF(op->func_qualname);
    PyObject_GC_Del(op);
}

static PyObject*
func_repr(PyFunctionObject *op)
{
    return PyUnicode_FromFormat("<function %U at %p>",
                                op->func_qualname, op);
}

static int
func_traverse(PyFunctionObject *f, visitproc visit, void *arg)
{
    Py_VISIT(f->func_code);
    Py_VISIT(f->func_globals);
    Py_VISIT(f->func_builtins);
    Py_VISIT(f->func_module);
    Py_VISIT(f->func_defaults);
    Py_VISIT(f->func_kwdefaults);
    Py_VISIT(f->func_doc);
    Py_VISIT(f->func_name);
    Py_VISIT(f->func_dict);
    Py_VISIT(f->func_closure);
    Py_VISIT(f->func_annotations);
    Py_VISIT(f->func_typeparams);
    Py_VISIT(f->func_qualname);
    return 0;
}

/* Bind a function to an object */
static PyObject *
func_descr_get(PyObject *func, PyObject *obj, PyObject *type)
{
    if (obj == Py_None || obj == NULL) {
        return Py_NewRef(func);
    }
    return PyMethod_New(func, obj);
}

PyTypeObject PyFunction_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "function",
    sizeof(PyFunctionObject),
    0,
    (destructor)func_dealloc,                   /* tp_dealloc */
    offsetof(PyFunctionObject, vectorcall),     /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)func_repr,                        /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    PyVectorcall_Call,                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
    Py_TPFLAGS_HAVE_VECTORCALL |
    Py_TPFLAGS_METHOD_DESCRIPTOR,               /* tp_flags */
    func_new__doc__,                            /* tp_doc */
    (traverseproc)func_traverse,                /* tp_traverse */
    (inquiry)func_clear,                        /* tp_clear */
    0,                                          /* tp_richcompare */
    offsetof(PyFunctionObject, func_weakreflist), /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    func_memberlist,                            /* tp_members */
    func_getsetlist,                            /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    func_descr_get,                             /* tp_descr_get */
    0,                                          /* tp_descr_set */
    offsetof(PyFunctionObject, func_dict),      /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    func_new,                                   /* tp_new */
};


static int
functools_copy_attr(PyObject *wrapper, PyObject *wrapped, PyObject *name)
{
    PyObject *value;
    int res = PyObject_GetOptionalAttr(wrapped, name, &value);
    if (value != NULL) {
        res = PyObject_SetAttr(wrapper, name, value);
        Py_DECREF(value);
    }
    return res;
}

// Similar to functools.wraps(wrapper, wrapped)
static int
functools_wraps(PyObject *wrapper, PyObject *wrapped)
{
#define COPY_ATTR(ATTR) \
    do { \
        if (functools_copy_attr(wrapper, wrapped, &_Py_ID(ATTR)) < 0) { \
            return -1; \
        } \
    } while (0) \

    COPY_ATTR(__module__);
    COPY_ATTR(__name__);
    COPY_ATTR(__qualname__);
    COPY_ATTR(__doc__);
    COPY_ATTR(__annotations__);
    return 0;

#undef COPY_ATTR
}


/* Class method object */

/* A class method receives the class as implicit first argument,
   just like an instance method receives the instance.
   To declare a class method, use this idiom:

     class C:
         @classmethod
         def f(cls, arg1, arg2, argN):
             ...

   It can be called either on the class (e.g. C.f()) or on an instance
   (e.g. C().f()); the instance is ignored except for its class.
   If a class method is called for a derived class, the derived class
   object is passed as the implied first argument.

   Class methods are different than C++ or Java static methods.
   If you want those, see static methods below.
*/

typedef struct {
    PyObject_HEAD
    PyObject *cm_callable;
    PyObject *cm_dict;
} classmethod;

static void
cm_dealloc(classmethod *cm)
{
    _PyObject_GC_UNTRACK((PyObject *)cm);
    Py_XDECREF(cm->cm_callable);
    Py_XDECREF(cm->cm_dict);
    Py_TYPE(cm)->tp_free((PyObject *)cm);
}

static int
cm_traverse(classmethod *cm, visitproc visit, void *arg)
{
    Py_VISIT(cm->cm_callable);
    Py_VISIT(cm->cm_dict);
    return 0;
}

static int
cm_clear(classmethod *cm)
{
    Py_CLEAR(cm->cm_callable);
    Py_CLEAR(cm->cm_dict);
    return 0;
}


static PyObject *
cm_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    classmethod *cm = (classmethod *)self;

    if (cm->cm_callable == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "uninitialized classmethod object");
        return NULL;
    }
    if (type == NULL)
        type = (PyObject *)(Py_TYPE(obj));
    return PyMethod_New(cm->cm_callable, type);
}

static int
cm_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    classmethod *cm = (classmethod *)self;
    PyObject *callable;

    if (!_PyArg_NoKeywords("classmethod", kwds))
        return -1;
    if (!PyArg_UnpackTuple(args, "classmethod", 1, 1, &callable))
        return -1;
    Py_XSETREF(cm->cm_callable, Py_NewRef(callable));

    if (functools_wraps((PyObject *)cm, cm->cm_callable) < 0) {
        return -1;
    }
    return 0;
}

static PyMemberDef cm_memberlist[] = {
    {"__func__", _Py_T_OBJECT, offsetof(classmethod, cm_callable), Py_READONLY},
    {"__wrapped__", _Py_T_OBJECT, offsetof(classmethod, cm_callable), Py_READONLY},
    {NULL}  /* Sentinel */
};

static PyObject *
cm_get___isabstractmethod__(classmethod *cm, void *closure)
{
    int res = _PyObject_IsAbstract(cm->cm_callable);
    if (res == -1) {
        return NULL;
    }
    else if (res) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyGetSetDef cm_getsetlist[] = {
    {"__isabstractmethod__",
     (getter)cm_get___isabstractmethod__, NULL, NULL, NULL},
    {"__dict__", PyObject_GenericGetDict, PyObject_GenericSetDict, NULL, NULL},
    {NULL} /* Sentinel */
};

static PyObject*
cm_repr(classmethod *cm)
{
    return PyUnicode_FromFormat("<classmethod(%R)>", cm->cm_callable);
}

PyDoc_STRVAR(classmethod_doc,
"classmethod(function, /)\n\
--\n\
\n\
Convert a function to be a class method.\n\
\n\
A class method receives the class as implicit first argument,\n\
just like an instance method receives the instance.\n\
To declare a class method, use this idiom:\n\
\n\
  class C:\n\
      @classmethod\n\
      def f(cls, arg1, arg2, argN):\n\
          ...\n\
\n\
It can be called either on the class (e.g. C.f()) or on an instance\n\
(e.g. C().f()).  The instance is ignored except for its class.\n\
If a class method is called for a derived class, the derived class\n\
object is passed as the implied first argument.\n\
\n\
Class methods are different than C++ or Java static methods.\n\
If you want those, see the staticmethod builtin.");

PyTypeObject PyClassMethod_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "classmethod",
    sizeof(classmethod),
    0,
    (destructor)cm_dealloc,                     /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)cm_repr,                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    classmethod_doc,                            /* tp_doc */
    (traverseproc)cm_traverse,                  /* tp_traverse */
    (inquiry)cm_clear,                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    cm_memberlist,              /* tp_members */
    cm_getsetlist,                              /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    cm_descr_get,                               /* tp_descr_get */
    0,                                          /* tp_descr_set */
    offsetof(classmethod, cm_dict),             /* tp_dictoffset */
    cm_init,                                    /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    PyType_GenericNew,                          /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
};

PyObject *
PyClassMethod_New(PyObject *callable)
{
    classmethod *cm = (classmethod *)
        PyType_GenericAlloc(&PyClassMethod_Type, 0);
    if (cm != NULL) {
        cm->cm_callable = Py_NewRef(callable);
    }
    return (PyObject *)cm;
}


/* Static method object */

/* A static method does not receive an implicit first argument.
   To declare a static method, use this idiom:

     class C:
         @staticmethod
         def f(arg1, arg2, argN):
             ...

   It can be called either on the class (e.g. C.f()) or on an instance
   (e.g. C().f()). Both the class and the instance are ignored, and
   neither is passed implicitly as the first argument to the method.

   Static methods in Python are similar to those found in Java or C++.
   For a more advanced concept, see class methods above.
*/

typedef struct {
    PyObject_HEAD
    PyObject *sm_callable;
    PyObject *sm_dict;
} staticmethod;

static void
sm_dealloc(staticmethod *sm)
{
    _PyObject_GC_UNTRACK((PyObject *)sm);
    Py_XDECREF(sm->sm_callable);
    Py_XDECREF(sm->sm_dict);
    Py_TYPE(sm)->tp_free((PyObject *)sm);
}

static int
sm_traverse(staticmethod *sm, visitproc visit, void *arg)
{
    Py_VISIT(sm->sm_callable);
    Py_VISIT(sm->sm_dict);
    return 0;
}

static int
sm_clear(staticmethod *sm)
{
    Py_CLEAR(sm->sm_callable);
    Py_CLEAR(sm->sm_dict);
    return 0;
}

static PyObject *
sm_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    staticmethod *sm = (staticmethod *)self;

    if (sm->sm_callable == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "uninitialized staticmethod object");
        return NULL;
    }
    return Py_NewRef(sm->sm_callable);
}

static int
sm_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    staticmethod *sm = (staticmethod *)self;
    PyObject *callable;

    if (!_PyArg_NoKeywords("staticmethod", kwds))
        return -1;
    if (!PyArg_UnpackTuple(args, "staticmethod", 1, 1, &callable))
        return -1;
    Py_XSETREF(sm->sm_callable, Py_NewRef(callable));

    if (functools_wraps((PyObject *)sm, sm->sm_callable) < 0) {
        return -1;
    }
    return 0;
}

static PyObject*
sm_call(PyObject *callable, PyObject *args, PyObject *kwargs)
{
    staticmethod *sm = (staticmethod *)callable;
    return PyObject_Call(sm->sm_callable, args, kwargs);
}

static PyMemberDef sm_memberlist[] = {
    {"__func__", _Py_T_OBJECT, offsetof(staticmethod, sm_callable), Py_READONLY},
    {"__wrapped__", _Py_T_OBJECT, offsetof(staticmethod, sm_callable), Py_READONLY},
    {NULL}  /* Sentinel */
};

static PyObject *
sm_get___isabstractmethod__(staticmethod *sm, void *closure)
{
    int res = _PyObject_IsAbstract(sm->sm_callable);
    if (res == -1) {
        return NULL;
    }
    else if (res) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyGetSetDef sm_getsetlist[] = {
    {"__isabstractmethod__",
     (getter)sm_get___isabstractmethod__, NULL, NULL, NULL},
    {"__dict__", PyObject_GenericGetDict, PyObject_GenericSetDict, NULL, NULL},
    {NULL} /* Sentinel */
};

static PyObject*
sm_repr(staticmethod *sm)
{
    return PyUnicode_FromFormat("<staticmethod(%R)>", sm->sm_callable);
}

PyDoc_STRVAR(staticmethod_doc,
"staticmethod(function, /)\n\
--\n\
\n\
Convert a function to be a static method.\n\
\n\
A static method does not receive an implicit first argument.\n\
To declare a static method, use this idiom:\n\
\n\
     class C:\n\
         @staticmethod\n\
         def f(arg1, arg2, argN):\n\
             ...\n\
\n\
It can be called either on the class (e.g. C.f()) or on an instance\n\
(e.g. C().f()). Both the class and the instance are ignored, and\n\
neither is passed implicitly as the first argument to the method.\n\
\n\
Static methods in Python are similar to those found in Java or C++.\n\
For a more advanced concept, see the classmethod builtin.");

PyTypeObject PyStaticMethod_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "staticmethod",
    sizeof(staticmethod),
    0,
    (destructor)sm_dealloc,                     /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)sm_repr,                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    sm_call,                                    /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    staticmethod_doc,                           /* tp_doc */
    (traverseproc)sm_traverse,                  /* tp_traverse */
    (inquiry)sm_clear,                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    sm_memberlist,              /* tp_members */
    sm_getsetlist,                              /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    sm_descr_get,                               /* tp_descr_get */
    0,                                          /* tp_descr_set */
    offsetof(staticmethod, sm_dict),            /* tp_dictoffset */
    sm_init,                                    /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    PyType_GenericNew,                          /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
};

PyObject *
PyStaticMethod_New(PyObject *callable)
{
    staticmethod *sm = (staticmethod *)
        PyType_GenericAlloc(&PyStaticMethod_Type, 0);
    if (sm != NULL) {
        sm->sm_callable = Py_NewRef(callable);
    }
    return (PyObject *)sm;
}
