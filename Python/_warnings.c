#include "Python.h"
#include "pycore_critical_section.h" // _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED()
#include "pycore_frame.h"         // PyFrameObject
#include "pycore_genobject.h"     // PyAsyncGenObject
#include "pycore_import.h"        // _PyImport_GetModules()
#include "pycore_interpframe.h"   // _PyFrame_GetCode()
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_object.h"        // _PyObject_XDecRefDelayed()
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_LOAD_SSIZE_ACQUIRE()
#include "pycore_pylifecycle.h"   // _Py_IsInterpreterFinalizing()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_traceback.h"     // _Py_DisplaySourceLine()
#include "pycore_tuple.h"         // _PyTuple_FromPair
#include "pycore_unicodeobject.h" // _PyUnicode_EqualToASCIIString()
#include "pycore_warnings.h"      // _PyWarnings_AfterFork()

#include <stdbool.h>
#include "clinic/_warnings.c.h"


#define MODULE_NAME "_warnings"

PyDoc_STRVAR(warnings__doc__,
MODULE_NAME " provides basic warning filtering support.\n"
"It is a helper module to speed up interpreter start-up.");


/*************************************************************************/

typedef struct _warnings_runtime_state WarningsState;

static inline int
check_interp(PyInterpreterState *interp)
{
    if (interp == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "warnings_get_state: could not identify "
                        "current interpreter");
        return 0;
    }
    return 1;
}

static inline PyInterpreterState *
get_current_interp(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return check_interp(interp) ? interp : NULL;
}

static inline PyThreadState *
get_current_tstate(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate == NULL) {
        (void)check_interp(NULL);
        return NULL;
    }
    return check_interp(tstate->interp) ? tstate : NULL;
}

/* Given a module object, get its per-module state. */
static WarningsState *
warnings_get_state(PyInterpreterState *interp)
{
    return &interp->warnings;
}

/* Clear the given warnings module state. */
static void
warnings_clear_state(WarningsState *st)
{
    Py_CLEAR(st->filters);
    Py_CLEAR(st->once_registry);
    Py_CLEAR(st->default_action);
    Py_CLEAR(st->context);
}

#ifndef Py_DEBUG
static PyObject *
create_filter(PyObject *category, PyObject *action_str, const char *modname)
{
    PyObject *modname_obj = NULL;

    /* Default to "no module name" for initial filter set */
    if (modname != NULL) {
        modname_obj = PyUnicode_InternFromString(modname);
        if (modname_obj == NULL) {
            return NULL;
        }
    } else {
        modname_obj = Py_NewRef(Py_None);
    }

    /* This assumes the line number is zero for now. */
    PyObject *filter = PyTuple_Pack(5, action_str, Py_None,
                                    category, modname_obj, _PyLong_GetZero());
    Py_DECREF(modname_obj);
    return filter;
}
#endif

static PyObject *
init_filters(PyInterpreterState *interp)
{
#ifdef Py_DEBUG
    /* Py_DEBUG builds show all warnings by default */
    return PyList_New(0);
#else
    /* Other builds ignore a number of warning categories by default */
    PyObject *filters = PyList_New(5);
    if (filters == NULL) {
        return NULL;
    }

    size_t pos = 0;  /* Post-incremented in each use. */
#define ADD(TYPE, ACTION, MODNAME) \
    PyList_SET_ITEM(filters, pos++, \
                    create_filter(TYPE, &_Py_ID(ACTION), MODNAME));
    ADD(PyExc_DeprecationWarning, default, "__main__");
    ADD(PyExc_DeprecationWarning, ignore, NULL);
    ADD(PyExc_PendingDeprecationWarning, ignore, NULL);
    ADD(PyExc_ImportWarning, ignore, NULL);
    ADD(PyExc_ResourceWarning, ignore, NULL);
#undef ADD

    for (size_t x = 0; x < pos; x++) {
        if (PyList_GET_ITEM(filters, x) == NULL) {
            Py_DECREF(filters);
            return NULL;
        }
    }
    return filters;
#endif
}

/* Initialize the given warnings module state. */
int
_PyWarnings_InitState(PyInterpreterState *interp)
{
    WarningsState *st = &interp->warnings;

    if (st->filters == NULL) {
        st->filters = init_filters(interp);
        if (st->filters == NULL) {
            return -1;
        }
    }

    if (st->once_registry == NULL) {
        st->once_registry = PyDict_New();
        if (st->once_registry == NULL) {
            return -1;
        }
    }

    if (st->default_action == NULL) {
        st->default_action = PyUnicode_FromString("default");
        if (st->default_action == NULL) {
            return -1;
        }
    }

    if (st->context == NULL) {
        st->context = PyContextVar_New("_warnings_context", NULL);
        if (st->context == NULL) {
            return -1;
        }
    }

    st->filters_version = 0;
    return 0;
}


/*************************************************************************/

static int
check_matched(PyInterpreterState *interp, PyObject *obj, PyObject *arg, PyObject *arg2)
{
    PyObject *result;
    int rc;

    /* A 'None' filter always matches */
    if (obj == Py_None)
        return 1;

    /* An internal plain text default filter must match exactly */
    if (PyUnicode_CheckExact(obj)) {
        if (arg == NULL) {
            return 0;
        }
        int cmp_result = PyUnicode_Compare(obj, arg);
        if (cmp_result == -1 && PyErr_Occurred()) {
            return -1;
        }
        return !cmp_result;
    }

    /* Otherwise assume a regex filter and call its match() method */
    if (arg != NULL) {
        result = PyObject_CallMethodOneArg(obj, &_Py_ID(match), arg);
    }
    else {
        PyObject *match = PyImport_ImportModuleAttrString("_py_warnings", "_match_filename");
        if (match == NULL) {
            return -1;
        }
        result = PyObject_CallFunctionObjArgs(match, obj, arg2, NULL);
        Py_DECREF(match);
    }
    if (result == NULL)
        return -1;
    rc = PyObject_IsTrue(result);
    Py_DECREF(result);
    return rc;
}

#define GET_WARNINGS_ATTR(interp, ATTR, try_import) \
    get_warnings_attr(interp, &_Py_ID(ATTR), try_import)

/*
   Returns a new reference.
   A NULL return value can mean false or an error.
*/
static PyObject *
get_warnings_attr(PyInterpreterState *interp, PyObject *attr, int try_import)
{
    PyObject *warnings_module, *obj;

    /* don't try to import after the start of the Python finallization */
    if (try_import && !_Py_IsInterpreterFinalizing(interp)) {
        warnings_module = PyImport_Import(&_Py_ID(warnings));
        if (warnings_module == NULL) {
            /* Fallback to the C implementation if we cannot get
               the Python implementation */
            if (PyErr_ExceptionMatches(PyExc_ImportError)) {
                PyErr_Clear();
            }
            return NULL;
        }
    }
    else {
        /* if we're so late into Python finalization that the module dict is
           gone, then we can't even use PyImport_GetModule without triggering
           an interpreter abort.
        */
        if (!_PyImport_GetModules(interp)) {
            return NULL;
        }
        warnings_module = PyImport_GetModule(&_Py_ID(warnings));
        if (warnings_module == NULL)
            return NULL;
    }

    (void)PyObject_GetOptionalAttr(warnings_module, attr, &obj);
    Py_DECREF(warnings_module);
    return obj;
}

static inline void
warnings_lock(PyInterpreterState *interp)
{
    WarningsState *st = warnings_get_state(interp);
    assert(st != NULL);
    _PyRecursiveMutex_Lock(&st->lock);
}

static inline int
warnings_unlock(PyInterpreterState *interp)
{
    WarningsState *st = warnings_get_state(interp);
    assert(st != NULL);
    return _PyRecursiveMutex_TryUnlock(&st->lock);
}

static inline bool
warnings_lock_held(WarningsState *st)
{
    return PyMutex_IsLocked(&st->lock.mutex);
}

#ifdef HAVE_FORK
void
_PyWarnings_BeforeFork(PyInterpreterState *interp)
{
    WarningsState *st = warnings_get_state(interp);
    st->lock_held_at_fork = _PyRecursiveMutex_IsLockedByCurrentThread(&st->lock);
}

void
_PyWarnings_AfterFork(PyInterpreterState *interp)
{
    WarningsState *st = warnings_get_state(interp);
    _PyRecursiveMutex_at_fork_reinit(&st->lock, st->lock_held_at_fork);
    st->lock_held_at_fork = false;
}
#endif

static PyObject *
get_warnings_context(PyInterpreterState *interp)
{
    WarningsState *st = warnings_get_state(interp);
    assert(PyContextVar_CheckExact(st->context));
    PyObject *ctx;
    if (PyContextVar_Get(st->context, NULL, &ctx) < 0) {
        return NULL;
    }
    if (ctx == NULL) {
        Py_RETURN_NONE;
    }
    return ctx;
}

static PyObject *
get_warnings_context_filters(PyInterpreterState *interp)
{
    PyObject *ctx = get_warnings_context(interp);
    if (ctx == NULL) {
        return NULL;
    }
    if (ctx == Py_None) {
        Py_RETURN_NONE;
    }
    PyObject *context_filters = PyObject_GetAttr(ctx, &_Py_ID(_filters));
    Py_DECREF(ctx);
    if (context_filters == NULL) {
        return NULL;
    }
    if (!PyList_Check(context_filters)) {
        PyErr_SetString(PyExc_ValueError,
                        "_filters of warnings._warnings_context must be a list");
        Py_DECREF(context_filters);
        return NULL;
    }
    return context_filters;
}

/* Store a new reference to 'value' in the warnings state field '*ptr',
   unless it is already stored there.  Other threads may be using the old
   value without holding a reference, so on the free-threaded build it is
   released only after a safe point. */
static void
update_state_ref(PyObject **ptr, PyObject *value)
{
    if (FT_ATOMIC_LOAD_PTR_ACQUIRE(*ptr) == value) {
        return;
    }
#ifdef Py_GIL_DISABLED
    PyObject *old = _Py_atomic_exchange_ptr(ptr, Py_NewRef(value));
    _PyObject_XDecRefDelayed(old);
#else
    Py_XSETREF(*ptr, Py_NewRef(value));
#endif
}

/* Return a new reference to the warnings state field '*ptr'.  The value
   cannot be freed between the load and the incref, see update_state_ref(). */
static PyObject *
get_state_ref(PyObject **ptr)
{
    return Py_XNewRef(FT_ATOMIC_LOAD_PTR_ACQUIRE(*ptr));
}

// Returns a new reference to the list.
static PyObject *
get_warnings_filters(PyInterpreterState *interp)
{
    WarningsState *st = warnings_get_state(interp);
    PyObject *filters = GET_WARNINGS_ATTR(interp, filters, 0);
    if (filters == NULL) {
        if (PyErr_Occurred())
            return NULL;
        filters = get_state_ref(&st->filters);
    }
    else {
        update_state_ref(&st->filters, filters);
    }

    if (filters == NULL || !PyList_Check(filters)) {
        PyErr_SetString(PyExc_ValueError,
                        MODULE_NAME ".filters must be a list");
        Py_XDECREF(filters);
        return NULL;
    }
    return filters;
}

/*[clinic input]
_acquire_lock as warnings_acquire_lock

[clinic start generated code]*/

static PyObject *
warnings_acquire_lock_impl(PyObject *module)
/*[clinic end generated code: output=594313457d1bf8e1 input=46ec20e55acca52f]*/
{
    PyInterpreterState *interp = get_current_interp();
    if (interp == NULL) {
        return NULL;
    }
    warnings_lock(interp);
    Py_RETURN_NONE;
}

/*[clinic input]
_release_lock as warnings_release_lock

[clinic start generated code]*/

static PyObject *
warnings_release_lock_impl(PyObject *module)
/*[clinic end generated code: output=d73d5a8789396750 input=ea01bb77870c5693]*/
{
    PyInterpreterState *interp = get_current_interp();
    if (interp == NULL) {
        return NULL;
    }
    if (warnings_unlock(interp) < 0) {
        PyErr_SetString(PyExc_RuntimeError, "cannot release un-acquired lock");
        return NULL;
    }
    Py_RETURN_NONE;
}

// Returns a new reference.
static PyObject *
get_once_registry(PyInterpreterState *interp)
{
    WarningsState *st = warnings_get_state(interp);
    assert(st != NULL);

    PyObject *registry = GET_WARNINGS_ATTR(interp, onceregistry, 0);
    if (registry == NULL) {
        if (PyErr_Occurred())
            return NULL;
        registry = get_state_ref(&st->once_registry);
        assert(registry != NULL);
        return registry;
    }
    if (!PyDict_Check(registry)) {
        PyErr_Format(PyExc_TypeError,
                     MODULE_NAME ".onceregistry must be a dict, "
                     "not '%.200s'",
                     Py_TYPE(registry)->tp_name);
        Py_DECREF(registry);
        return NULL;
    }
    update_state_ref(&st->once_registry, registry);
    return registry;
}


// Returns a new reference.
static PyObject *
get_default_action(PyInterpreterState *interp)
{
    WarningsState *st = warnings_get_state(interp);
    assert(st != NULL);

    PyObject *default_action = GET_WARNINGS_ATTR(interp, defaultaction, 0);
    if (default_action == NULL) {
        if (PyErr_Occurred()) {
            return NULL;
        }
        default_action = get_state_ref(&st->default_action);
        assert(default_action != NULL);
        return default_action;
    }
    if (!PyUnicode_Check(default_action)) {
        PyErr_Format(PyExc_TypeError,
                     MODULE_NAME ".defaultaction must be a string, "
                     "not '%.200s'",
                     Py_TYPE(default_action)->tp_name);
        Py_DECREF(default_action);
        return NULL;
    }
    update_state_ref(&st->default_action, default_action);
    return default_action;
}

/* Search filters list of match, returns false on error.  If no match
 * then 'matched_action' is NULL.  */
static bool
filter_search(PyInterpreterState *interp, PyObject *category,
              PyObject *text, Py_ssize_t lineno,
              PyObject *module, PyObject *filename, char *list_name, PyObject *filters,
              PyObject **item, PyObject **matched_action) {
    bool result = true;
    *matched_action = NULL;
    /* Avoid the filters list changing while we iterate over it. */
    Py_BEGIN_CRITICAL_SECTION(filters);
    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(filters); i++) {
        PyObject *tmp_item, *action, *msg, *cat, *mod, *ln_obj;
        Py_ssize_t ln;
        int is_subclass, good_msg, good_mod;

        tmp_item = PyList_GET_ITEM(filters, i);
        if (!PyTuple_Check(tmp_item) || PyTuple_GET_SIZE(tmp_item) != 5) {
            PyErr_Format(PyExc_ValueError,
                         "warnings.%s item %zd isn't a 5-tuple", list_name, i);
            result = false;
            break;
        }

        /* Python code: action, msg, cat, mod, ln = item */
        Py_INCREF(tmp_item);
        action = PyTuple_GET_ITEM(tmp_item, 0);
        msg = PyTuple_GET_ITEM(tmp_item, 1);
        cat = PyTuple_GET_ITEM(tmp_item, 2);
        mod = PyTuple_GET_ITEM(tmp_item, 3);
        ln_obj = PyTuple_GET_ITEM(tmp_item, 4);

        if (!PyUnicode_Check(action)) {
            PyErr_Format(PyExc_TypeError,
                         "action must be a string, not '%.200s'",
                         Py_TYPE(action)->tp_name);
            Py_DECREF(tmp_item);
            result = false;
            break;
        }

        good_msg = check_matched(interp, msg, text, NULL);
        if (good_msg == -1) {
            Py_DECREF(tmp_item);
            result = false;
            break;
        }

        good_mod = check_matched(interp, mod, module, filename);
        if (good_mod == -1) {
            Py_DECREF(tmp_item);
            result = false;
            break;
        }

        is_subclass = PyObject_IsSubclass(category, cat);
        if (is_subclass == -1) {
            Py_DECREF(tmp_item);
            result = false;
            break;
        }

        ln = PyLong_AsSsize_t(ln_obj);
        if (ln == -1 && PyErr_Occurred()) {
            Py_DECREF(tmp_item);
            result = false;
            break;
        }

        if (good_msg && is_subclass && good_mod && (ln == 0 || lineno == ln)) {
            *item = tmp_item;
            *matched_action = action;
            result = true;
            break;
        }

        Py_DECREF(tmp_item);
    }
    Py_END_CRITICAL_SECTION();
    return result;
}

/* The returned action and the item are new references. */
static PyObject*
get_filter(PyInterpreterState *interp, PyObject *category,
           PyObject *text, Py_ssize_t lineno,
           PyObject *module, PyObject *filename, PyObject **item)
{
    /* check _warning_context _filters list */
    PyObject *context_filters = get_warnings_context_filters(interp);
    if (context_filters == NULL) {
        return NULL;
    }
    bool use_global_filters = false;
    if (context_filters == Py_None) {
        use_global_filters = true;
    } else {
        PyObject *context_action = NULL;
        if (!filter_search(interp, category, text, lineno, module, filename, "_warnings_context _filters",
                           context_filters, item, &context_action)) {
            Py_DECREF(context_filters);
            return NULL;
        }
        Py_DECREF(context_filters);
        if (context_action != NULL) {
            return Py_NewRef(context_action);
        }
    }

    PyObject *action;

    if (use_global_filters) {
        /* check warnings.filters list */
        PyObject *filters = get_warnings_filters(interp);
        if (filters == NULL) {
            return NULL;
        }
        bool ok = filter_search(interp, category, text, lineno, module,
                                filename, "filters", filters, item, &action);
        Py_DECREF(filters);
        if (!ok) {
            return NULL;
        }
        if (action != NULL) {
            return Py_NewRef(action);
        }
    }

    action = get_default_action(interp);
    if (action != NULL) {
        *item = Py_NewRef(Py_None);
        return action;
    }

    return NULL;
}


/* Returns 1 if 'registry' is stamped with filters 'version', 0 if not, -1
   on error. */
static int
registry_version_matches(PyObject *registry, Py_ssize_t version)
{
    PyObject *version_obj;
    if (PyDict_GetItemRef(registry, &_Py_ID(version), &version_obj) < 0) {
        return -1;
    }
    int rc = 0;
    if (version_obj != NULL && PyLong_CheckExact(version_obj)) {
        int overflow;
        long long v = PyLong_AsLongLongAndOverflow(version_obj, &overflow);
        rc = (!overflow && v == version);
    }
    Py_XDECREF(version_obj);
    return rc;
}

static int
set_registry_version(PyObject *registry, Py_ssize_t version)
{
    PyObject *version_obj = PyLong_FromSsize_t(version);
    if (version_obj == NULL) {
        return -1;
    }
    int rc = PyDict_SetItem(registry, &_Py_ID(version), version_obj);
    Py_DECREF(version_obj);
    return rc;
}

/* Validate the version stamp of 'registry', a dict of warnings already
   shown.  Its entries are only valid for the filters version it is stamped
   with.  'version' is the filters version the caller searched the filters
   with.  If the stamp differs and 'version' is still the current version,
   clear the registry and stamp it with 'version'.  If 'version' is stale,
   leave the registry alone: the caller's result predates the filter change
   and must not be recorded.

   Returns 1 if the registry can be used, 0 if not, -1 on error. */
static int
check_registry_version(PyInterpreterState *interp, PyObject *registry,
                       Py_ssize_t version)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(registry);
    WarningsState *st = warnings_get_state(interp);

    int rc = registry_version_matches(registry, version);
    if (rc != 0) {
        return rc;
    }
    if (version != FT_ATOMIC_LOAD_SSIZE_ACQUIRE(st->filters_version)) {
        return 0;
    }

    PyDict_Clear(registry);
    return set_registry_version(registry, version) < 0 ? -1 : 1;
}

static int
registry_contains(PyObject *registry, PyObject *key)
{
    PyObject *value;
    if (PyDict_GetItemRef(registry, key, &value) < 0) {
        return -1;
    }
    if (value == NULL) {
        return 0;
    }
    int rc = PyObject_IsTrue(value);
    Py_DECREF(value);
    return rc;
}

/* Returns 1 if 'key' is recorded in 'registry' for filters 'version', 0 if
   not, -1 on error. */
static int
already_warned(PyInterpreterState *interp, PyObject *registry, PyObject *key,
               Py_ssize_t version)
{
    int rc;
    Py_BEGIN_CRITICAL_SECTION(registry);
    rc = check_registry_version(interp, registry, version);
    if (rc > 0) {
        rc = registry_contains(registry, key);
    }
    Py_END_CRITICAL_SECTION();
    return rc;
}

/* Set 'key' in 'registry' unless it is already present.  Returns 1 if it
   was present, 0 if it was set, -1 on error. */
static int
registry_add(PyObject *registry, PyObject *key)
{
    int rc = registry_contains(registry, key);
    if (rc == 0 && PyDict_SetItem(registry, key, Py_True) < 0) {
        rc = -1;
    }
    return rc;
}

/* Record that a warning was shown, based on a search of filters 'version'.
   'key' and 'altkey' (either may be NULL) are set in 'registry'.  If one of
   them is already present, another thread showed the warning first and it
   must not be shown again.  Nothing is recorded if 'version' is stale.
   Returns 0 if the warning should be shown, 1 if not, -1 on error. */
static int
record_warning(PyInterpreterState *interp, PyObject *registry,
               PyObject *key, PyObject *altkey, Py_ssize_t version)
{
    int rc;
    Py_BEGIN_CRITICAL_SECTION(registry);
    rc = check_registry_version(interp, registry, version);
    if (rc > 0) {
        rc = 0;
        if (key != NULL) {
            rc = registry_add(registry, key);
        }
        if (rc == 0 && altkey != NULL) {
            rc = registry_add(registry, altkey);
        }
        if (rc >= 0) {
            /* Another thread may have cleared the registry and stamped it
            with a newer version before our keys were written, making them look
            valid for the new filters.  Detect this by checking the stamp again
            and invalidating the registry if it changed. */
            int match = registry_version_matches(registry, version);
            if (match < 0 ||
                (match == 0 && set_registry_version(registry, -1) < 0))
            {
                rc = -1;
            }
        }
    }
    Py_END_CRITICAL_SECTION();
    return rc;
}

static void
show_warning(PyThreadState *tstate, PyObject *filename, int lineno,
             PyObject *text, PyObject *category, PyObject *sourceline)
{
    PyObject *f_stderr = NULL;
    PyObject *name;
    char lineno_str[128];

    PyOS_snprintf(lineno_str, sizeof(lineno_str), ":%d: ", lineno);

    name = PyObject_GetAttr(category, &_Py_ID(__name__));
    if (name == NULL) {
        goto error;
    }

    if (PySys_GetOptionalAttr(&_Py_ID(stderr), &f_stderr) <= 0) {
        fprintf(stderr, "lost sys.stderr\n");
        goto error;
    }

    /* Print "filename:lineno: category: text\n" */
    if (PyFile_WriteObject(filename, f_stderr, Py_PRINT_RAW) < 0)
        goto error;
    if (PyFile_WriteString(lineno_str, f_stderr) < 0)
        goto error;
    if (PyFile_WriteObject(name, f_stderr, Py_PRINT_RAW) < 0)
        goto error;
    if (PyFile_WriteString(": ", f_stderr) < 0)
        goto error;
    if (PyFile_WriteObject(text, f_stderr, Py_PRINT_RAW) < 0)
        goto error;
    if (PyFile_WriteString("\n", f_stderr) < 0)
        goto error;
    Py_CLEAR(name);

    /* Print "  source_line\n" */
    if (sourceline) {
        int kind;
        const void *data;
        Py_ssize_t i, len;
        Py_UCS4 ch;
        PyObject *truncated;

        kind = PyUnicode_KIND(sourceline);
        data = PyUnicode_DATA(sourceline);
        len = PyUnicode_GET_LENGTH(sourceline);
        for (i=0; i<len; i++) {
            ch = PyUnicode_READ(kind, data, i);
            if (ch != ' ' && ch != '\t' && ch != '\014')
                break;
        }

        truncated = PyUnicode_Substring(sourceline, i, len);
        if (truncated == NULL)
            goto error;

        PyFile_WriteObject(sourceline, f_stderr, Py_PRINT_RAW);
        Py_DECREF(truncated);
        PyFile_WriteString("\n", f_stderr);
    }
    else {
        _Py_DisplaySourceLine(f_stderr, filename, lineno, 2, NULL, NULL);
    }

error:
    Py_XDECREF(f_stderr);
    Py_XDECREF(name);
    PyErr_Clear();
}

static int
call_show_warning(PyThreadState *tstate, PyObject *category,
                  PyObject *text, PyObject *message,
                  PyObject *filename, int lineno, PyObject *lineno_obj,
                  PyObject *sourceline, PyObject *source, PyObject *module)
{
    PyObject *show_fn, *msg, *res, *warnmsg_cls = NULL;
    PyInterpreterState *interp = tstate->interp;

    /* The Python implementation is able to log the traceback where the source
       was allocated, whereas the C implementation doesn't. */
    show_fn = GET_WARNINGS_ATTR(interp, _showwarnmsg, 1);
    if (show_fn == NULL) {
        if (PyErr_Occurred())
            return -1;
        show_warning(tstate, filename, lineno, text, category, sourceline);
        return 0;
    }

    if (!PyCallable_Check(show_fn)) {
        PyErr_SetString(PyExc_TypeError,
                "warnings._showwarnmsg() must be set to a callable");
        goto error;
    }

    warnmsg_cls = GET_WARNINGS_ATTR(interp, WarningMessage, 0);
    if (warnmsg_cls == NULL) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                    "unable to get warnings.WarningMessage");
        }
        goto error;
    }

    msg = PyObject_CallFunctionObjArgs(warnmsg_cls, message, category,
            filename, lineno_obj, Py_None,
            sourceline ? sourceline : Py_None,
            source ? source : Py_None, module,
            NULL);
    Py_DECREF(warnmsg_cls);
    if (msg == NULL)
        goto error;

    res = PyObject_CallOneArg(show_fn, msg);
    Py_DECREF(show_fn);
    Py_DECREF(msg);

    if (res == NULL)
        return -1;

    Py_DECREF(res);
    return 0;

error:
    Py_XDECREF(show_fn);
    return -1;
}

static PyObject *
warn_explicit(PyThreadState *tstate, PyObject *category, PyObject *message,
              PyObject *filename, int lineno,
              PyObject *module, PyObject *registry, PyObject *sourceline,
              PyObject *source)
{
    PyObject *key = NULL, *text = NULL, *result = NULL, *lineno_obj = NULL;
    PyObject *item = NULL, *action = NULL;
    PyObject *altkey = NULL, *once_registry = NULL;
    int rc;
    PyInterpreterState *interp = tstate->interp;

    /* module can be None if a warning is emitted late during Python shutdown.
       In this case, the Python warnings module was probably unloaded, filters
       are no more available to choose as action. It is safer to ignore the
       warning and do nothing. */
    if (module == Py_None)
        Py_RETURN_NONE;

    if (registry && !PyDict_Check(registry) && (registry != Py_None)) {
        PyErr_SetString(PyExc_TypeError, "'registry' must be a dict or None");
        return NULL;
    }

    /* Normalize message. */
    Py_INCREF(message);  /* DECREF'ed in cleanup. */
    if (PyObject_TypeCheck(message, (PyTypeObject *)PyExc_Warning)) {
        text = PyObject_Str(message);
        if (text == NULL)
            goto cleanup;
        category = (PyObject*)Py_TYPE(message);
    }
    else {
        text = message;
        message = PyObject_CallOneArg(category, message);
        if (message == NULL)
            goto cleanup;
    }

    lineno_obj = PyLong_FromLong(lineno);
    if (lineno_obj == NULL)
        goto cleanup;

    if (source == Py_None) {
        source = NULL;
    }

    /* Create key. */
    key = PyTuple_Pack(3, text, category, lineno_obj);
    if (key == NULL)
        goto cleanup;

    /* The warnings lock is not taken here.  Filters are searched under a
       critical section on the filters list and the registries are checked
       and updated under a critical section on the registry.  'version' is
       the filters version used for the search, so that a result computed
       from filters that have since changed is never recorded. */
    WarningsState *st = warnings_get_state(interp);
    Py_ssize_t version = FT_ATOMIC_LOAD_SSIZE_ACQUIRE(st->filters_version);

    if ((registry != NULL) && (registry != Py_None)) {
        /* Compute the hash now (it is cached by the tuple) so that a user
           __hash__ does not run inside the registry critical section. */
        if (PyObject_Hash(key) == -1)
            goto cleanup;
        rc = already_warned(interp, registry, key, version);
        if (rc == -1)
            goto cleanup;
        else if (rc == 1)
            goto return_none;
        /* Else this warning hasn't been generated before. */
    }
    else {
        registry = NULL;
    }

    action = get_filter(interp, category, text, lineno, module, filename, &item);
    if (action == NULL)
        goto cleanup;

    if (_PyUnicode_EqualToASCIIString(action, "error")) {
        PyErr_SetObject(category, message);
        goto cleanup;
    }

    if (_PyUnicode_EqualToASCIIString(action, "ignore")) {
        goto return_none;
    }

    /* Store in the registry that we've been here, *except* when the action
       is "always" or "all". */
    if (!_PyUnicode_EqualToASCIIString(action, "always") && !_PyUnicode_EqualToASCIIString(action, "all")) {
        bool is_once = _PyUnicode_EqualToASCIIString(action, "once");
        bool is_module = _PyUnicode_EqualToASCIIString(action, "module");
        if (!is_once && !is_module &&
            !_PyUnicode_EqualToASCIIString(action, "default"))
        {
            PyErr_Format(PyExc_RuntimeError,
                        "Unrecognized action (%R) in warnings.filters:\n %R",
                        action, item);
            goto cleanup;
        }

        if (is_once && registry == NULL) {
            /* WarningsState.once_registry[(text, category)] = 1 */
            once_registry = get_once_registry(interp);
            if (once_registry == NULL)
                goto cleanup;
            altkey = _PyTuple_FromPair(text, category);
            if (altkey == NULL || PyObject_Hash(altkey) == -1)
                goto cleanup;
            rc = record_warning(interp, once_registry, NULL, altkey, version);
        }
        else if (registry != NULL) {
            /* registry[key] = 1, and for "once" and "module" also
               registry[(text, category)] = 1 */
            if (is_once || is_module) {
                altkey = _PyTuple_FromPair(text, category);
                if (altkey == NULL || PyObject_Hash(altkey) == -1)
                    goto cleanup;
            }
            rc = record_warning(interp, registry, key, altkey, version);
        }
        else {
            rc = 0;
        }
        if (rc == -1)
            goto cleanup;
        if (rc == 1)  /* Already warned for this module. */
            goto return_none;
    }

    if (call_show_warning(tstate, category, text, message, filename,
                          lineno, lineno_obj, sourceline, source, module) < 0)
        goto cleanup;

 return_none:
    result = Py_NewRef(Py_None);

 cleanup:
    Py_XDECREF(once_registry);
    Py_XDECREF(altkey);
    Py_XDECREF(action);
    Py_XDECREF(item);
    Py_XDECREF(key);
    Py_XDECREF(text);
    Py_XDECREF(lineno_obj);
    Py_XDECREF(message);
    return result;  /* Py_None or NULL. */
}

static PyObject *
get_frame_filename(PyFrameObject *frame)
{
    PyCodeObject *code = PyFrame_GetCode(frame);
    PyObject *filename = code->co_filename;
    Py_DECREF(code);
    return filename;
}

static bool
is_internal_filename(PyObject *filename)
{
    if (!PyUnicode_Check(filename)) {
        return false;
    }

    int contains = PyUnicode_Contains(filename, &_Py_ID(importlib));
    if (contains < 0) {
        return false;
    }
    else if (contains > 0) {
        contains = PyUnicode_Contains(filename, &_Py_ID(_bootstrap));
        if (contains < 0) {
            return false;
        }
        else if (contains > 0) {
            return true;
        }
    }

    return false;
}

static bool
is_filename_to_skip(PyObject *filename, PyTupleObject *skip_file_prefixes)
{
    if (skip_file_prefixes) {
        if (!PyUnicode_Check(filename)) {
            return false;
        }

        Py_ssize_t prefixes = PyTuple_GET_SIZE(skip_file_prefixes);
        for (Py_ssize_t idx = 0; idx < prefixes; ++idx)
        {
            PyObject *prefix = PyTuple_GET_ITEM(skip_file_prefixes, idx);
            Py_ssize_t found = PyUnicode_Tailmatch(filename, prefix,
                                                   0, PY_SSIZE_T_MAX, -1);
            if (found == 1) {
                return true;
            }
            if (found < 0) {
                return false;
            }
        }
    }
    return false;
}

static bool
is_internal_frame(PyFrameObject *frame)
{
    if (frame == NULL) {
        return false;
    }

    PyObject *filename = get_frame_filename(frame);
    if (filename == NULL) {
        return false;
    }

    return is_internal_filename(filename);
}

static PyFrameObject *
next_external_frame(PyFrameObject *frame, PyTupleObject *skip_file_prefixes)
{
    PyObject *frame_filename;
    do {
        PyFrameObject *back = PyFrame_GetBack(frame);
        Py_SETREF(frame, back);
    } while (frame != NULL && (frame_filename = get_frame_filename(frame)) &&
             (is_internal_filename(frame_filename) ||
              is_filename_to_skip(frame_filename, skip_file_prefixes)));

    return frame;
}

/* filename, module, and registry are new refs, globals is borrowed */
/* skip_file_prefixes is either NULL or a tuple of strs. */
/* Returns 0 on error (no new refs), 1 on success */
static int
setup_context(Py_ssize_t stack_level,
              PyTupleObject *skip_file_prefixes,
              PyObject **filename, int *lineno,
              PyObject **module, PyObject **registry)
{
    PyObject *globals;

    /* Setup globals, filename and lineno. */
    PyThreadState *tstate = get_current_tstate();
    if (tstate == NULL) {
        return 0;
    }
    if (skip_file_prefixes) {
        /* Type check our data structure up front. Later code that uses it
         * isn't structured to report errors. */
        Py_ssize_t prefixes = PyTuple_GET_SIZE(skip_file_prefixes);
        for (Py_ssize_t idx = 0; idx < prefixes; ++idx)
        {
            PyObject *prefix = PyTuple_GET_ITEM(skip_file_prefixes, idx);
            if (!PyUnicode_Check(prefix)) {
                PyErr_Format(PyExc_TypeError,
                             "Found non-str '%s' in skip_file_prefixes.",
                             Py_TYPE(prefix)->tp_name);
                return 0;
            }
        }
    }
    PyInterpreterState *interp = tstate->interp;
    PyFrameObject *f = PyThreadState_GetFrame(tstate);
    // Stack level comparisons to Python code is off by one as there is no
    // warnings-related stack level to avoid.
    if (stack_level <= 0 || is_internal_frame(f)) {
        while (--stack_level > 0 && f != NULL) {
            PyFrameObject *back = PyFrame_GetBack(f);
            Py_SETREF(f, back);
        }
    }
    else {
        while (--stack_level > 0 && f != NULL) {
            f = next_external_frame(f, skip_file_prefixes);
        }
    }

    if (f == NULL) {
        globals = interp->sysdict;
        *filename = PyUnicode_FromString("<sys>");
        if (*filename == NULL) {
            return 0;
        }
        *lineno = 0;
    }
    else {
        globals = f->f_frame->f_globals;
        *filename = Py_NewRef(_PyFrame_GetCode(f->f_frame)->co_filename);
        *lineno = PyFrame_GetLineNumber(f);
        Py_DECREF(f);
    }

    *module = NULL;

    /* Setup registry. */
    assert(globals != NULL);
    assert(PyAnyDict_Check(globals));
    int rc = PyDict_GetItemRef(globals, &_Py_ID(__warningregistry__),
                               registry);
    if (rc < 0) {
        goto handle_error;
    }
    if (*registry == NULL) {
        *registry = PyDict_New();
        if (*registry == NULL)
            goto handle_error;

         rc = PyDict_SetItem(globals, &_Py_ID(__warningregistry__), *registry);
         if (rc < 0)
            goto handle_error;
    }

    /* Setup module. */
    rc = PyDict_GetItemRef(globals, &_Py_ID(__name__), module);
    if (rc < 0) {
        goto handle_error;
    }
    if (rc > 0) {
        if (Py_IsNone(*module) || PyUnicode_Check(*module)) {
            return 1;
        }
        Py_DECREF(*module);
    }
    *module = PyUnicode_FromString("<string>");
    if (*module == NULL) {
        goto handle_error;
    }

    return 1;

 handle_error:
    Py_XDECREF(*registry);
    Py_XDECREF(*module);
    Py_DECREF(*filename);
    return 0;
}

static PyObject *
get_category(PyObject *message, PyObject *category)
{
    if (PyObject_TypeCheck(message, (PyTypeObject *)PyExc_Warning)) {
        /* Ignore the category argument. */
        return (PyObject*)Py_TYPE(message);
    }
    if (category == NULL || category == Py_None) {
        return PyExc_UserWarning;
    }

    /* Validate category. */
    if (!PyType_Check(category)) {
        PyErr_Format(PyExc_TypeError,
                     "category must be a Warning subclass, not '%T'",
                     category);
        return NULL;
    }
    if (!PyType_IsSubtype((PyTypeObject *)category, (PyTypeObject *)PyExc_Warning)) {
        PyErr_Format(PyExc_TypeError,
                     "category must be a Warning subclass, not class '%N'",
                     (PyTypeObject *)category);
        return NULL;
    }

    return category;
}

static PyObject *
do_warn(PyObject *message, PyObject *category, Py_ssize_t stack_level,
        PyObject *source, PyTupleObject *skip_file_prefixes)
{
    PyObject *filename, *module, *registry, *res;
    int lineno;

    PyThreadState *tstate = get_current_tstate();
    if (tstate == NULL) {
        return NULL;
    }

    if (!setup_context(stack_level, skip_file_prefixes,
                       &filename, &lineno, &module, &registry))
        return NULL;

    res = warn_explicit(tstate, category, message, filename, lineno, module, registry,
                        NULL, source);
    Py_DECREF(filename);
    Py_DECREF(registry);
    Py_DECREF(module);
    return res;
}

/*[clinic input]
warn as warnings_warn

    message: object
      Text of the warning message.
    category: object = None
      The Warning category subclass. Defaults to UserWarning.
    stacklevel: Py_ssize_t = 1
      How far up the call stack to make this warning appear. A value of 2 for
      example attributes the warning to the caller of the code calling warn().
    source: object = None
      If supplied, the destroyed object which emitted a ResourceWarning
    *
    skip_file_prefixes: object(type='PyTupleObject *', subclass_of='&PyTuple_Type', c_default='NULL') = ()
      An optional tuple of module filename prefixes indicating frames to skip
      during stacklevel computations for stack frame attribution.

Issue a warning, or maybe ignore it or raise an exception.
[clinic start generated code]*/

static PyObject *
warnings_warn_impl(PyObject *module, PyObject *message, PyObject *category,
                   Py_ssize_t stacklevel, PyObject *source,
                   PyTupleObject *skip_file_prefixes)
/*[clinic end generated code: output=a68e0f6906c65f80 input=2b52e8b20f508f51]*/
{
    category = get_category(message, category);
    if (category == NULL)
        return NULL;
    if (skip_file_prefixes) {
        if (PyTuple_GET_SIZE(skip_file_prefixes) > 0) {
            if (stacklevel < 2) {
                stacklevel = 2;
            }
        } else {
            skip_file_prefixes = NULL;
        }
    }
    return do_warn(message, category, stacklevel, source, skip_file_prefixes);
}

static PyObject *
get_source_line(PyInterpreterState *interp, PyObject *module_globals, int lineno)
{
    PyObject *loader;
    PyObject *module_name;
    PyObject *get_source;
    PyObject *source;
    PyObject *source_list;
    PyObject *source_line;

    /* stolen from import.c */
    loader = _PyImport_BlessMyLoader(interp, module_globals);
    if (loader == NULL) {
        return NULL;
    }

    /* Prefer __spec__.name: __name__ is "__main__" for the module executed
       as a script, but the loader can only handle its own module name. */
    PyObject *spec;
    if (PyDict_GetItemRef(module_globals, &_Py_ID(__spec__), &spec) < 0) {
        Py_DECREF(loader);
        return NULL;
    }
    module_name = NULL;
    if (spec != NULL) {
        int rc = PyObject_GetOptionalAttr(spec, &_Py_ID(name), &module_name);
        Py_DECREF(spec);
        if (rc < 0) {
            Py_DECREF(loader);
            return NULL;
        }
        if (module_name == Py_None) {
            Py_CLEAR(module_name);
        }
    }
    if (module_name == NULL) {
        int rc = PyDict_GetItemRef(module_globals, &_Py_ID(__name__),
                                   &module_name);
        if (rc <= 0) {  // not found or error
            Py_DECREF(loader);
            return NULL;
        }
    }

    /* Make sure the loader implements the optional get_source() method. */
    (void)PyObject_GetOptionalAttr(loader, &_Py_ID(get_source), &get_source);
    Py_DECREF(loader);
    if (!get_source) {
        Py_DECREF(module_name);
        return NULL;
    }
    /* Call get_source() to get the source code. */
    source = PyObject_CallOneArg(get_source, module_name);
    Py_DECREF(get_source);
    Py_DECREF(module_name);
    if (!source) {
        /* The source line is optional: the loader can be unable to provide
           the source of the module, for example if it is not its loader. */
        if (PyErr_ExceptionMatches(PyExc_ImportError)) {
            PyErr_Clear();
        }
        return NULL;
    }
    if (source == Py_None) {
        Py_DECREF(source);
        return NULL;
    }

    /* Split the source into lines. */
    source_list = PyUnicode_Splitlines(source, 0);
    Py_DECREF(source);
    if (!source_list) {
        return NULL;
    }

    /* Get the source line. */
    if (lineno < 1 || lineno > PyList_GET_SIZE(source_list)) {
        Py_DECREF(source_list);
        return NULL;
    }
    source_line = Py_NewRef(PyList_GET_ITEM(source_list, lineno-1));
    Py_DECREF(source_list);
    return source_line;
}

/*[clinic input]
warn_explicit as warnings_warn_explicit

    message: object
    category: object
    filename: unicode
    lineno: int
    module as mod: object = NULL
    registry: object = None
    module_globals: object = None
    source as sourceobj: object = None

Issue a warning, or maybe ignore it or raise an exception.
[clinic start generated code]*/

static PyObject *
warnings_warn_explicit_impl(PyObject *module, PyObject *message,
                            PyObject *category, PyObject *filename,
                            int lineno, PyObject *mod, PyObject *registry,
                            PyObject *module_globals, PyObject *sourceobj)
/*[clinic end generated code: output=c49c62b15a49a186 input=df6eeb8b45e712f1]*/
{
    PyObject *source_line = NULL;
    PyObject *returned;

    PyThreadState *tstate = get_current_tstate();
    if (tstate == NULL) {
        return NULL;
    }

    if (module_globals && module_globals != Py_None) {
        if (!PyAnyDict_Check(module_globals)) {
            PyErr_Format(PyExc_TypeError,
                         "module_globals must be a dict or a frozendict, "
                         "not %T",
                         module_globals);
            return NULL;
        }

        source_line = get_source_line(tstate->interp, module_globals, lineno);
        if (source_line == NULL && PyErr_Occurred()) {
            return NULL;
        }
    }

    returned = warn_explicit(tstate, category, message, filename, lineno,
                             mod, registry, source_line, sourceobj);
    Py_XDECREF(source_line);
    return returned;
}

/*[clinic input]
_filters_mutated_lock_held as warnings_filters_mutated_lock_held

[clinic start generated code]*/

static PyObject *
warnings_filters_mutated_lock_held_impl(PyObject *module)
/*[clinic end generated code: output=df5c84f044e856ec input=34208bf03d70e432]*/
{
    PyInterpreterState *interp = get_current_interp();
    if (interp == NULL) {
        return NULL;
    }

    WarningsState *st = warnings_get_state(interp);
    assert(st != NULL);

    // Note that the lock must be held by the caller.
    if (!warnings_lock_held(st)) {
        PyErr_SetString(PyExc_RuntimeError, "warnings lock is not held");
        return NULL;
    }

    FT_ATOMIC_ADD_SSIZE(st->filters_version, 1);

    Py_RETURN_NONE;
}

/* Function to issue a warning message; may raise an exception. */

static int
warn_unicode(PyObject *category, PyObject *message,
             Py_ssize_t stack_level, PyObject *source)
{
    PyObject *res;

    if (category == NULL)
        category = PyExc_RuntimeWarning;

    res = do_warn(message, category, stack_level, source, NULL);
    if (res == NULL)
        return -1;
    Py_DECREF(res);

    return 0;
}

static int
_PyErr_WarnFormatV(PyObject *source,
                   PyObject *category, Py_ssize_t stack_level,
                   const char *format, va_list vargs)
{
    PyObject *message;
    int res;

    message = PyUnicode_FromFormatV(format, vargs);
    if (message == NULL)
        return -1;

    res = warn_unicode(category, message, stack_level, source);
    Py_DECREF(message);
    return res;
}

int
PyErr_WarnFormat(PyObject *category, Py_ssize_t stack_level,
                 const char *format, ...)
{
    int res;
    va_list vargs;

    va_start(vargs, format);
    res = _PyErr_WarnFormatV(NULL, category, stack_level, format, vargs);
    va_end(vargs);
    return res;
}

static int
_PyErr_WarnFormat(PyObject *source, PyObject *category, Py_ssize_t stack_level,
                  const char *format, ...)
{
    int res;
    va_list vargs;

    va_start(vargs, format);
    res = _PyErr_WarnFormatV(source, category, stack_level, format, vargs);
    va_end(vargs);
    return res;
}

int
PyErr_ResourceWarning(PyObject *source, Py_ssize_t stack_level,
                      const char *format, ...)
{
    int res;
    va_list vargs;

    va_start(vargs, format);
    res = _PyErr_WarnFormatV(source, PyExc_ResourceWarning,
                             stack_level, format, vargs);
    va_end(vargs);
    return res;
}


int
PyErr_WarnEx(PyObject *category, const char *text, Py_ssize_t stack_level)
{
    int ret;
    PyObject *message = PyUnicode_FromString(text);
    if (message == NULL)
        return -1;
    ret = warn_unicode(category, message, stack_level, NULL);
    Py_DECREF(message);
    return ret;
}

/* PyErr_Warn is only for backwards compatibility and will be removed.
   Use PyErr_WarnEx instead. */

#undef PyErr_Warn

int
PyErr_Warn(PyObject *category, const char *text)
{
    return PyErr_WarnEx(category, text, 1);
}

/* Warning with explicit origin */
int
PyErr_WarnExplicitObject(PyObject *category, PyObject *message,
                         PyObject *filename, int lineno,
                         PyObject *module, PyObject *registry)
{
    PyObject *res;
    if (category == NULL)
        category = PyExc_RuntimeWarning;
    PyThreadState *tstate = get_current_tstate();
    if (tstate == NULL) {
        return -1;
    }

    res = warn_explicit(tstate, category, message, filename, lineno,
                        module, registry, NULL, NULL);
    if (res == NULL)
        return -1;
    Py_DECREF(res);
    return 0;
}

int
PyErr_WarnExplicit(PyObject *category, const char *text,
                   const char *filename_str, int lineno,
                   const char *module_str, PyObject *registry)
{
    PyObject *message = PyUnicode_FromString(text);
    if (message == NULL) {
        return -1;
    }
    PyObject *filename = PyUnicode_DecodeFSDefault(filename_str);
    if (filename == NULL) {
        Py_DECREF(message);
        return -1;
    }
    PyObject *module = NULL;
    if (module_str != NULL) {
        module = PyUnicode_FromString(module_str);
        if (module == NULL) {
            Py_DECREF(filename);
            Py_DECREF(message);
            return -1;
        }
    }

    int ret = PyErr_WarnExplicitObject(category, message, filename, lineno,
                                       module, registry);
    Py_XDECREF(module);
    Py_DECREF(filename);
    Py_DECREF(message);
    return ret;
}

int
PyErr_WarnExplicitFormat(PyObject *category,
                         const char *filename_str, int lineno,
                         const char *module_str, PyObject *registry,
                         const char *format, ...)
{
    PyObject *message;
    PyObject *module = NULL;
    PyObject *filename = PyUnicode_DecodeFSDefault(filename_str);
    int ret = -1;
    va_list vargs;

    if (filename == NULL)
        goto exit;
    if (module_str != NULL) {
        module = PyUnicode_FromString(module_str);
        if (module == NULL)
            goto exit;
    }

    va_start(vargs, format);
    message = PyUnicode_FromFormatV(format, vargs);
    if (message != NULL) {
        PyObject *res;
        PyThreadState *tstate = get_current_tstate();
        if (tstate != NULL) {
            res = warn_explicit(tstate, category, message, filename, lineno,
                                module, registry, NULL, NULL);
            Py_DECREF(message);
            if (res != NULL) {
                Py_DECREF(res);
                ret = 0;
            }
        }
    }
    va_end(vargs);
exit:
    Py_XDECREF(module);
    Py_XDECREF(filename);
    return ret;
}

void
_PyErr_WarnUnawaitedAgenMethod(PyAsyncGenObject *agen, PyObject *method)
{
    PyObject *exc = PyErr_GetRaisedException();
    if (_PyErr_WarnFormat((PyObject *)agen, PyExc_RuntimeWarning, 1,
                          "coroutine method %R of %R was never awaited",
                          method, agen->ag_qualname) < 0)
    {
        PyErr_FormatUnraisable("Exception ignored while "
                               "finalizing async generator %R", agen);
    }
    PyErr_SetRaisedException(exc);
}


void
_PyErr_WarnUnawaitedCoroutine(PyObject *coro)
{
    /* First, we attempt to funnel the warning through
       warnings._warn_unawaited_coroutine.

       This could raise an exception, due to:
       - a bug
       - some kind of shutdown-related brokenness
       - succeeding, but with an "error" warning filter installed, so the
         warning is converted into a RuntimeWarning exception

       In the first two cases, we want to print the error (so we know what it
       is!), and then print a warning directly as a fallback. In the last
       case, we want to print the error (since it's the warning!), but *not*
       do a fallback. And after we print the error we can't check for what
       type of error it was (because PyErr_WriteUnraisable clears it), so we
       need a flag to keep track.

       Since this is called from __del__ context, it's careful to never raise
       an exception.
    */
    int warned = 0;
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(interp != NULL);
    PyObject *fn = GET_WARNINGS_ATTR(interp, _warn_unawaited_coroutine, 1);
    if (fn) {
        PyObject *res = PyObject_CallOneArg(fn, coro);
        Py_DECREF(fn);
        if (res || PyErr_ExceptionMatches(PyExc_RuntimeWarning)) {
            warned = 1;
        }
        Py_XDECREF(res);
    }

    if (PyErr_Occurred()) {
        PyErr_FormatUnraisable("Exception ignored while "
                               "finalizing coroutine %R", coro);
    }

    if (!warned) {
        if (_PyErr_WarnFormat(coro, PyExc_RuntimeWarning, 1,
                              "coroutine '%S' was never awaited",
                              ((PyCoroObject *)coro)->cr_qualname) < 0)
        {
            PyErr_FormatUnraisable("Exception ignored while "
                                   "finalizing coroutine %R", coro);
        }
    }
}

static PyMethodDef warnings_functions[] = {
    WARNINGS_WARN_METHODDEF
    WARNINGS_WARN_EXPLICIT_METHODDEF
    WARNINGS_FILTERS_MUTATED_LOCK_HELD_METHODDEF
    WARNINGS_ACQUIRE_LOCK_METHODDEF
    WARNINGS_RELEASE_LOCK_METHODDEF
    /* XXX(brett.cannon): add showwarning? */
    /* XXX(brett.cannon): Reasonable to add formatwarning? */
    {NULL, NULL}                /* sentinel */
};


static int
warnings_module_exec(PyObject *module)
{
    PyInterpreterState *interp = get_current_interp();
    if (interp == NULL) {
        return -1;
    }
    WarningsState *st = warnings_get_state(interp);
    if (st == NULL) {
        return -1;
    }
    if (PyModule_AddObjectRef(module, "filters", st->filters) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(module, "_onceregistry", st->once_registry) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(module, "_defaultaction", st->default_action) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(module, "_warnings_context", st->context) < 0) {
        return -1;
    }
    return 0;
}


static PyModuleDef_Slot warnings_slots[] = {
    _Py_ABI_SLOT,
    {Py_mod_exec, warnings_module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef warnings_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = MODULE_NAME,
    .m_doc = warnings__doc__,
    .m_size = 0,
    .m_methods = warnings_functions,
    .m_slots = warnings_slots,
};


PyMODINIT_FUNC
_PyWarnings_Init(void)
{
    return PyModuleDef_Init(&warnings_module);
}

// We need this to ensure that warnings still work until late in finalization.
void
_PyWarnings_Fini(PyInterpreterState *interp)
{
    warnings_clear_state(&interp->warnings);
}
