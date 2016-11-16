#include "Python.h"
#include "frameobject.h"

#define MODULE_NAME "_warnings"

PyDoc_STRVAR(warnings__doc__,
MODULE_NAME " provides basic warning filtering support.\n"
"It is a helper module to speed up interpreter start-up.");

/* Both 'filters' and 'onceregistry' can be set in warnings.py;
   get_warnings_attr() will reset these variables accordingly. */
static PyObject *_filters;  /* List */
static PyObject *_once_registry;  /* Dict */
static PyObject *_default_action; /* String */
static long _filters_version;

_Py_IDENTIFIER(argv);
_Py_IDENTIFIER(stderr);

static int
check_matched(PyObject *obj, PyObject *arg)
{
    PyObject *result;
    _Py_IDENTIFIER(match);
    int rc;

    if (obj == Py_None)
        return 1;
    result = _PyObject_CallMethodId(obj, &PyId_match, "O", arg);
    if (result == NULL)
        return -1;

    rc = PyObject_IsTrue(result);
    Py_DECREF(result);
    return rc;
}

/*
   Returns a new reference.
   A NULL return value can mean false or an error.
*/
static PyObject *
get_warnings_attr(const char *attr, int try_import)
{
    static PyObject *warnings_str = NULL;
    PyObject *all_modules;
    PyObject *warnings_module, *obj;

    if (warnings_str == NULL) {
        warnings_str = PyUnicode_InternFromString("warnings");
        if (warnings_str == NULL)
            return NULL;
    }

    /* don't try to import after the start of the Python finallization */
    if (try_import && _Py_Finalizing == NULL) {
        warnings_module = PyImport_Import(warnings_str);
        if (warnings_module == NULL) {
            /* Fallback to the C implementation if we cannot get
               the Python implementation */
            PyErr_Clear();
            return NULL;
        }
    }
    else {
        all_modules = PyImport_GetModuleDict();

        warnings_module = PyDict_GetItem(all_modules, warnings_str);
        if (warnings_module == NULL)
            return NULL;

        Py_INCREF(warnings_module);
    }

    if (!PyObject_HasAttrString(warnings_module, attr)) {
        Py_DECREF(warnings_module);
        return NULL;
    }

    obj = PyObject_GetAttrString(warnings_module, attr);
    Py_DECREF(warnings_module);
    return obj;
}


static PyObject *
get_once_registry(void)
{
    PyObject *registry;

    registry = get_warnings_attr("onceregistry", 0);
    if (registry == NULL) {
        if (PyErr_Occurred())
            return NULL;
        return _once_registry;
    }
    Py_DECREF(_once_registry);
    _once_registry = registry;
    return registry;
}


static PyObject *
get_default_action(void)
{
    PyObject *default_action;

    default_action = get_warnings_attr("defaultaction", 0);
    if (default_action == NULL) {
        if (PyErr_Occurred()) {
            return NULL;
        }
        return _default_action;
    }

    Py_DECREF(_default_action);
    _default_action = default_action;
    return default_action;
}


/* The item is a new reference. */
static PyObject*
get_filter(PyObject *category, PyObject *text, Py_ssize_t lineno,
           PyObject *module, PyObject **item)
{
    PyObject *action;
    Py_ssize_t i;
    PyObject *warnings_filters;

    warnings_filters = get_warnings_attr("filters", 0);
    if (warnings_filters == NULL) {
        if (PyErr_Occurred())
            return NULL;
    }
    else {
        Py_DECREF(_filters);
        _filters = warnings_filters;
    }

    if (_filters == NULL || !PyList_Check(_filters)) {
        PyErr_SetString(PyExc_ValueError,
                        MODULE_NAME ".filters must be a list");
        return NULL;
    }

    /* _filters could change while we are iterating over it. */
    for (i = 0; i < PyList_GET_SIZE(_filters); i++) {
        PyObject *tmp_item, *action, *msg, *cat, *mod, *ln_obj;
        Py_ssize_t ln;
        int is_subclass, good_msg, good_mod;

        tmp_item = PyList_GET_ITEM(_filters, i);
        if (!PyTuple_Check(tmp_item) || PyTuple_GET_SIZE(tmp_item) != 5) {
            PyErr_Format(PyExc_ValueError,
                         MODULE_NAME ".filters item %zd isn't a 5-tuple", i);
            return NULL;
        }

        /* Python code: action, msg, cat, mod, ln = item */
        Py_INCREF(tmp_item);
        action = PyTuple_GET_ITEM(tmp_item, 0);
        msg = PyTuple_GET_ITEM(tmp_item, 1);
        cat = PyTuple_GET_ITEM(tmp_item, 2);
        mod = PyTuple_GET_ITEM(tmp_item, 3);
        ln_obj = PyTuple_GET_ITEM(tmp_item, 4);

        good_msg = check_matched(msg, text);
        if (good_msg == -1) {
            Py_DECREF(tmp_item);
            return NULL;
        }

        good_mod = check_matched(mod, module);
        if (good_mod == -1) {
            Py_DECREF(tmp_item);
            return NULL;
        }

        is_subclass = PyObject_IsSubclass(category, cat);
        if (is_subclass == -1) {
            Py_DECREF(tmp_item);
            return NULL;
        }

        ln = PyLong_AsSsize_t(ln_obj);
        if (ln == -1 && PyErr_Occurred()) {
            Py_DECREF(tmp_item);
            return NULL;
        }

        if (good_msg && is_subclass && good_mod && (ln == 0 || lineno == ln)) {
            *item = tmp_item;
            return action;
        }

        Py_DECREF(tmp_item);
    }

    action = get_default_action();
    if (action != NULL) {
        Py_INCREF(Py_None);
        *item = Py_None;
        return action;
    }

    PyErr_SetString(PyExc_ValueError,
                    MODULE_NAME ".defaultaction not found");
    return NULL;
}


static int
already_warned(PyObject *registry, PyObject *key, int should_set)
{
    PyObject *version_obj, *already_warned;
    _Py_IDENTIFIER(version);

    if (key == NULL)
        return -1;

    version_obj = _PyDict_GetItemId(registry, &PyId_version);
    if (version_obj == NULL
        || !PyLong_CheckExact(version_obj)
        || PyLong_AsLong(version_obj) != _filters_version) {
        PyDict_Clear(registry);
        version_obj = PyLong_FromLong(_filters_version);
        if (version_obj == NULL)
            return -1;
        if (_PyDict_SetItemId(registry, &PyId_version, version_obj) < 0) {
            Py_DECREF(version_obj);
            return -1;
        }
        Py_DECREF(version_obj);
    }
    else {
        already_warned = PyDict_GetItem(registry, key);
        if (already_warned != NULL) {
            int rc = PyObject_IsTrue(already_warned);
            if (rc != 0)
                return rc;
        }
    }

    /* This warning wasn't found in the registry, set it. */
    if (should_set)
        return PyDict_SetItem(registry, key, Py_True);
    return 0;
}

/* New reference. */
static PyObject *
normalize_module(PyObject *filename)
{
    PyObject *module;
    int kind;
    void *data;
    Py_ssize_t len;

    len = PyUnicode_GetLength(filename);
    if (len < 0)
        return NULL;

    if (len == 0)
        return PyUnicode_FromString("<unknown>");

    kind = PyUnicode_KIND(filename);
    data = PyUnicode_DATA(filename);

    /* if filename.endswith(".py"): */
    if (len >= 3 &&
        PyUnicode_READ(kind, data, len-3) == '.' &&
        PyUnicode_READ(kind, data, len-2) == 'p' &&
        PyUnicode_READ(kind, data, len-1) == 'y')
    {
        module = PyUnicode_Substring(filename, 0, len-3);
    }
    else {
        module = filename;
        Py_INCREF(module);
    }
    return module;
}

static int
update_registry(PyObject *registry, PyObject *text, PyObject *category,
                int add_zero)
{
    PyObject *altkey, *zero = NULL;
    int rc;

    if (add_zero) {
        zero = PyLong_FromLong(0);
        if (zero == NULL)
            return -1;
        altkey = PyTuple_Pack(3, text, category, zero);
    }
    else
        altkey = PyTuple_Pack(2, text, category);

    rc = already_warned(registry, altkey, 1);
    Py_XDECREF(zero);
    Py_XDECREF(altkey);
    return rc;
}

static void
show_warning(PyObject *filename, int lineno, PyObject *text,
             PyObject *category, PyObject *sourceline)
{
    PyObject *f_stderr;
    PyObject *name;
    char lineno_str[128];
    _Py_IDENTIFIER(__name__);

    PyOS_snprintf(lineno_str, sizeof(lineno_str), ":%d: ", lineno);

    name = _PyObject_GetAttrId(category, &PyId___name__);
    if (name == NULL)  /* XXX Can an object lack a '__name__' attribute? */
        goto error;

    f_stderr = _PySys_GetObjectId(&PyId_stderr);
    if (f_stderr == NULL) {
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
        void *data;
        Py_ssize_t i, len;
        Py_UCS4 ch;
        PyObject *truncated;

        if (PyUnicode_READY(sourceline) < 1)
            goto error;

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
        _Py_DisplaySourceLine(f_stderr, filename, lineno, 2);
    }

error:
    Py_XDECREF(name);
    PyErr_Clear();
}

static int
call_show_warning(PyObject *category, PyObject *text, PyObject *message,
                  PyObject *filename, int lineno, PyObject *lineno_obj,
                  PyObject *sourceline, PyObject *source)
{
    PyObject *show_fn, *msg, *res, *warnmsg_cls = NULL;

    /* If the source parameter is set, try to get the Python implementation.
       The Python implementation is able to log the traceback where the source
       was allocated, whereas the C implementation doesnt. */
    show_fn = get_warnings_attr("_showwarnmsg", source != NULL);
    if (show_fn == NULL) {
        if (PyErr_Occurred())
            return -1;
        show_warning(filename, lineno, text, category, sourceline);
        return 0;
    }

    if (!PyCallable_Check(show_fn)) {
        PyErr_SetString(PyExc_TypeError,
                "warnings._showwarnmsg() must be set to a callable");
        goto error;
    }

    warnmsg_cls = get_warnings_attr("WarningMessage", 0);
    if (warnmsg_cls == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                "unable to get warnings.WarningMessage");
        goto error;
    }

    msg = PyObject_CallFunctionObjArgs(warnmsg_cls, message, category,
            filename, lineno_obj, Py_None, Py_None, source,
            NULL);
    Py_DECREF(warnmsg_cls);
    if (msg == NULL)
        goto error;

    res = PyObject_CallFunctionObjArgs(show_fn, msg, NULL);
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
warn_explicit(PyObject *category, PyObject *message,
              PyObject *filename, int lineno,
              PyObject *module, PyObject *registry, PyObject *sourceline,
              PyObject *source)
{
    PyObject *key = NULL, *text = NULL, *result = NULL, *lineno_obj = NULL;
    PyObject *item = NULL;
    PyObject *action;
    int rc;

    /* module can be None if a warning is emitted late during Python shutdown.
       In this case, the Python warnings module was probably unloaded, filters
       are no more available to choose as action. It is safer to ignore the
       warning and do nothing. */
    if (module == Py_None)
        Py_RETURN_NONE;

    if (registry && !PyDict_Check(registry) && (registry != Py_None)) {
        PyErr_SetString(PyExc_TypeError, "'registry' must be a dict");
        return NULL;
    }

    /* Normalize module. */
    if (module == NULL) {
        module = normalize_module(filename);
        if (module == NULL)
            return NULL;
    }
    else
        Py_INCREF(module);

    /* Normalize message. */
    Py_INCREF(message);  /* DECREF'ed in cleanup. */
    rc = PyObject_IsInstance(message, PyExc_Warning);
    if (rc == -1) {
        goto cleanup;
    }
    if (rc == 1) {
        text = PyObject_Str(message);
        if (text == NULL)
            goto cleanup;
        category = (PyObject*)message->ob_type;
    }
    else {
        text = message;
        message = PyObject_CallFunction(category, "O", message);
        if (message == NULL)
            goto cleanup;
    }

    lineno_obj = PyLong_FromLong(lineno);
    if (lineno_obj == NULL)
        goto cleanup;

    /* Create key. */
    key = PyTuple_Pack(3, text, category, lineno_obj);
    if (key == NULL)
        goto cleanup;

    if ((registry != NULL) && (registry != Py_None)) {
        rc = already_warned(registry, key, 0);
        if (rc == -1)
            goto cleanup;
        else if (rc == 1)
            goto return_none;
        /* Else this warning hasn't been generated before. */
    }

    action = get_filter(category, text, lineno, module, &item);
    if (action == NULL)
        goto cleanup;

    if (_PyUnicode_EqualToASCIIString(action, "error")) {
        PyErr_SetObject(category, message);
        goto cleanup;
    }

    /* Store in the registry that we've been here, *except* when the action
       is "always". */
    rc = 0;
    if (!_PyUnicode_EqualToASCIIString(action, "always")) {
        if (registry != NULL && registry != Py_None &&
                PyDict_SetItem(registry, key, Py_True) < 0)
            goto cleanup;
        else if (_PyUnicode_EqualToASCIIString(action, "ignore"))
            goto return_none;
        else if (_PyUnicode_EqualToASCIIString(action, "once")) {
            if (registry == NULL || registry == Py_None) {
                registry = get_once_registry();
                if (registry == NULL)
                    goto cleanup;
            }
            /* _once_registry[(text, category)] = 1 */
            rc = update_registry(registry, text, category, 0);
        }
        else if (_PyUnicode_EqualToASCIIString(action, "module")) {
            /* registry[(text, category, 0)] = 1 */
            if (registry != NULL && registry != Py_None)
                rc = update_registry(registry, text, category, 0);
        }
        else if (!_PyUnicode_EqualToASCIIString(action, "default")) {
            PyErr_Format(PyExc_RuntimeError,
                        "Unrecognized action (%R) in warnings.filters:\n %R",
                        action, item);
            goto cleanup;
        }
    }

    if (rc == 1)  /* Already warned for this module. */
        goto return_none;
    if (rc == 0) {
        if (call_show_warning(category, text, message, filename, lineno,
                              lineno_obj, sourceline, source) < 0)
            goto cleanup;
    }
    else /* if (rc == -1) */
        goto cleanup;

 return_none:
    result = Py_None;
    Py_INCREF(result);

 cleanup:
    Py_XDECREF(item);
    Py_XDECREF(key);
    Py_XDECREF(text);
    Py_XDECREF(lineno_obj);
    Py_DECREF(module);
    Py_XDECREF(message);
    return result;  /* Py_None or NULL. */
}

static int
is_internal_frame(PyFrameObject *frame)
{
    static PyObject *importlib_string = NULL;
    static PyObject *bootstrap_string = NULL;
    PyObject *filename;
    int contains;

    if (importlib_string == NULL) {
        importlib_string = PyUnicode_FromString("importlib");
        if (importlib_string == NULL) {
            return 0;
        }

        bootstrap_string = PyUnicode_FromString("_bootstrap");
        if (bootstrap_string == NULL) {
            Py_DECREF(importlib_string);
            return 0;
        }
        Py_INCREF(importlib_string);
        Py_INCREF(bootstrap_string);
    }

    if (frame == NULL || frame->f_code == NULL ||
            frame->f_code->co_filename == NULL) {
        return 0;
    }
    filename = frame->f_code->co_filename;
    if (!PyUnicode_Check(filename)) {
        return 0;
    }
    contains = PyUnicode_Contains(filename, importlib_string);
    if (contains < 0) {
        return 0;
    }
    else if (contains > 0) {
        contains = PyUnicode_Contains(filename, bootstrap_string);
        if (contains < 0) {
            return 0;
        }
        else if (contains > 0) {
            return 1;
        }
    }

    return 0;
}

static PyFrameObject *
next_external_frame(PyFrameObject *frame)
{
    do {
        frame = frame->f_back;
    } while (frame != NULL && is_internal_frame(frame));

    return frame;
}

/* filename, module, and registry are new refs, globals is borrowed */
/* Returns 0 on error (no new refs), 1 on success */
static int
setup_context(Py_ssize_t stack_level, PyObject **filename, int *lineno,
              PyObject **module, PyObject **registry)
{
    PyObject *globals;

    /* Setup globals and lineno. */
    PyFrameObject *f = PyThreadState_GET()->frame;
    // Stack level comparisons to Python code is off by one as there is no
    // warnings-related stack level to avoid.
    if (stack_level <= 0 || is_internal_frame(f)) {
        while (--stack_level > 0 && f != NULL) {
            f = f->f_back;
        }
    }
    else {
        while (--stack_level > 0 && f != NULL) {
            f = next_external_frame(f);
        }
    }

    if (f == NULL) {
        globals = PyThreadState_Get()->interp->sysdict;
        *lineno = 1;
    }
    else {
        globals = f->f_globals;
        *lineno = PyFrame_GetLineNumber(f);
    }

    *module = NULL;

    /* Setup registry. */
    assert(globals != NULL);
    assert(PyDict_Check(globals));
    *registry = PyDict_GetItemString(globals, "__warningregistry__");
    if (*registry == NULL) {
        int rc;

        *registry = PyDict_New();
        if (*registry == NULL)
            return 0;

         rc = PyDict_SetItemString(globals, "__warningregistry__", *registry);
         if (rc < 0)
            goto handle_error;
    }
    else
        Py_INCREF(*registry);

    /* Setup module. */
    *module = PyDict_GetItemString(globals, "__name__");
    if (*module == NULL) {
        *module = PyUnicode_FromString("<string>");
        if (*module == NULL)
            goto handle_error;
    }
    else
        Py_INCREF(*module);

    /* Setup filename. */
    *filename = PyDict_GetItemString(globals, "__file__");
    if (*filename != NULL && PyUnicode_Check(*filename)) {
        Py_ssize_t len;
        int kind;
        void *data;

        if (PyUnicode_READY(*filename))
            goto handle_error;

        len = PyUnicode_GetLength(*filename);
        kind = PyUnicode_KIND(*filename);
        data = PyUnicode_DATA(*filename);

#define ascii_lower(c) ((c <= 127) ? Py_TOLOWER(c) : 0)
        /* if filename.lower().endswith(".pyc"): */
        if (len >= 4 &&
            PyUnicode_READ(kind, data, len-4) == '.' &&
            ascii_lower(PyUnicode_READ(kind, data, len-3)) == 'p' &&
            ascii_lower(PyUnicode_READ(kind, data, len-2)) == 'y' &&
            ascii_lower(PyUnicode_READ(kind, data, len-1)) == 'c')
        {
            *filename = PyUnicode_Substring(*filename, 0,
                                            PyUnicode_GET_LENGTH(*filename)-1);
            if (*filename == NULL)
                goto handle_error;
        }
        else
            Py_INCREF(*filename);
    }
    else {
        *filename = NULL;
        if (*module != Py_None && _PyUnicode_EqualToASCIIString(*module, "__main__")) {
            PyObject *argv = _PySys_GetObjectId(&PyId_argv);
            /* PyList_Check() is needed because sys.argv is set to None during
               Python finalization */
            if (argv != NULL && PyList_Check(argv) && PyList_Size(argv) > 0) {
                int is_true;
                *filename = PyList_GetItem(argv, 0);
                Py_INCREF(*filename);
                /* If sys.argv[0] is false, then use '__main__'. */
                is_true = PyObject_IsTrue(*filename);
                if (is_true < 0) {
                    Py_DECREF(*filename);
                    goto handle_error;
                }
                else if (!is_true) {
                    Py_SETREF(*filename, PyUnicode_FromString("__main__"));
                    if (*filename == NULL)
                        goto handle_error;
                }
            }
            else {
                /* embedded interpreters don't have sys.argv, see bug #839151 */
                *filename = PyUnicode_FromString("__main__");
                if (*filename == NULL)
                    goto handle_error;
            }
        }
        if (*filename == NULL) {
            *filename = *module;
            Py_INCREF(*filename);
        }
    }

    return 1;

 handle_error:
    /* filename not XDECREF'ed here as there is no way to jump here with a
       dangling reference. */
    Py_XDECREF(*registry);
    Py_XDECREF(*module);
    return 0;
}

static PyObject *
get_category(PyObject *message, PyObject *category)
{
    int rc;

    /* Get category. */
    rc = PyObject_IsInstance(message, PyExc_Warning);
    if (rc == -1)
        return NULL;

    if (rc == 1)
        category = (PyObject*)message->ob_type;
    else if (category == NULL || category == Py_None)
        category = PyExc_UserWarning;

    /* Validate category. */
    rc = PyObject_IsSubclass(category, PyExc_Warning);
    /* category is not a subclass of PyExc_Warning or
       PyObject_IsSubclass raised an error */
    if (rc == -1 || rc == 0) {
        PyErr_Format(PyExc_TypeError,
                     "category must be a Warning subclass, not '%s'",
                     Py_TYPE(category)->tp_name);
        return NULL;
    }

    return category;
}

static PyObject *
do_warn(PyObject *message, PyObject *category, Py_ssize_t stack_level,
        PyObject *source)
{
    PyObject *filename, *module, *registry, *res;
    int lineno;

    if (!setup_context(stack_level, &filename, &lineno, &module, &registry))
        return NULL;

    res = warn_explicit(category, message, filename, lineno, module, registry,
                        NULL, source);
    Py_DECREF(filename);
    Py_DECREF(registry);
    Py_DECREF(module);
    return res;
}

static PyObject *
warnings_warn(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kw_list[] = {"message", "category", "stacklevel",
                              "source", NULL};
    PyObject *message, *category = NULL, *source = NULL;
    Py_ssize_t stack_level = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|OnO:warn", kw_list,
                                     &message, &category, &stack_level, &source))
        return NULL;

    category = get_category(message, category);
    if (category == NULL)
        return NULL;
    return do_warn(message, category, stack_level, source);
}

static PyObject *
warnings_warn_explicit(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwd_list[] = {"message", "category", "filename", "lineno",
                                "module", "registry", "module_globals",
                                "source", 0};
    PyObject *message;
    PyObject *category;
    PyObject *filename;
    int lineno;
    PyObject *module = NULL;
    PyObject *registry = NULL;
    PyObject *module_globals = NULL;
    PyObject *sourceobj = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OOUi|OOOO:warn_explicit",
                kwd_list, &message, &category, &filename, &lineno, &module,
                &registry, &module_globals, &sourceobj))
        return NULL;

    if (module_globals) {
        _Py_IDENTIFIER(get_source);
        _Py_IDENTIFIER(splitlines);
        PyObject *tmp;
        PyObject *loader;
        PyObject *module_name;
        PyObject *source;
        PyObject *source_list;
        PyObject *source_line;
        PyObject *returned;

        if ((tmp = _PyUnicode_FromId(&PyId_get_source)) == NULL)
            return NULL;
        if ((tmp = _PyUnicode_FromId(&PyId_splitlines)) == NULL)
            return NULL;

        /* Check/get the requisite pieces needed for the loader. */
        loader = PyDict_GetItemString(module_globals, "__loader__");
        module_name = PyDict_GetItemString(module_globals, "__name__");

        if (loader == NULL || module_name == NULL)
            goto standard_call;

        /* Make sure the loader implements the optional get_source() method. */
        if (!_PyObject_HasAttrId(loader, &PyId_get_source))
                goto standard_call;
        /* Call get_source() to get the source code. */
        source = PyObject_CallMethodObjArgs(loader, PyId_get_source.object,
                                            module_name, NULL);
        if (!source)
            return NULL;
        else if (source == Py_None) {
            Py_DECREF(Py_None);
            goto standard_call;
        }

        /* Split the source into lines. */
        source_list = PyObject_CallMethodObjArgs(source,
                                                 PyId_splitlines.object,
                                                 NULL);
        Py_DECREF(source);
        if (!source_list)
            return NULL;

        /* Get the source line. */
        source_line = PyList_GetItem(source_list, lineno-1);
        if (!source_line) {
            Py_DECREF(source_list);
            return NULL;
        }

        /* Handle the warning. */
        returned = warn_explicit(category, message, filename, lineno, module,
                                 registry, source_line, sourceobj);
        Py_DECREF(source_list);
        return returned;
    }

 standard_call:
    return warn_explicit(category, message, filename, lineno, module,
                         registry, NULL, sourceobj);
}

static PyObject *
warnings_filters_mutated(PyObject *self, PyObject *args)
{
    _filters_version++;
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

    res = do_warn(message, category, stack_level, source);
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

#ifdef HAVE_STDARG_PROTOTYPES
    va_start(vargs, format);
#else
    va_start(vargs);
#endif
    res = _PyErr_WarnFormatV(NULL, category, stack_level, format, vargs);
    va_end(vargs);
    return res;
}

int
PyErr_ResourceWarning(PyObject *source, Py_ssize_t stack_level,
                      const char *format, ...)
{
    int res;
    va_list vargs;

#ifdef HAVE_STDARG_PROTOTYPES
    va_start(vargs, format);
#else
    va_start(vargs);
#endif
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

PyAPI_FUNC(int)
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
    res = warn_explicit(category, message, filename, lineno,
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
    PyObject *filename = PyUnicode_DecodeFSDefault(filename_str);
    PyObject *module = NULL;
    int ret = -1;

    if (message == NULL || filename == NULL)
        goto exit;
    if (module_str != NULL) {
        module = PyUnicode_FromString(module_str);
        if (module == NULL)
            goto exit;
    }

    ret = PyErr_WarnExplicitObject(category, message, filename, lineno,
                                   module, registry);

 exit:
    Py_XDECREF(message);
    Py_XDECREF(module);
    Py_XDECREF(filename);
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

#ifdef HAVE_STDARG_PROTOTYPES
    va_start(vargs, format);
#else
    va_start(vargs);
#endif
    message = PyUnicode_FromFormatV(format, vargs);
    if (message != NULL) {
        PyObject *res;
        res = warn_explicit(category, message, filename, lineno,
                            module, registry, NULL, NULL);
        Py_DECREF(message);
        if (res != NULL) {
            Py_DECREF(res);
            ret = 0;
        }
    }
    va_end(vargs);
exit:
    Py_XDECREF(module);
    Py_XDECREF(filename);
    return ret;
}


PyDoc_STRVAR(warn_doc,
"Issue a warning, or maybe ignore it or raise an exception.");

PyDoc_STRVAR(warn_explicit_doc,
"Low-level inferface to warnings functionality.");

static PyMethodDef warnings_functions[] = {
    {"warn", (PyCFunction)warnings_warn, METH_VARARGS | METH_KEYWORDS,
        warn_doc},
    {"warn_explicit", (PyCFunction)warnings_warn_explicit,
        METH_VARARGS | METH_KEYWORDS, warn_explicit_doc},
    {"_filters_mutated", (PyCFunction)warnings_filters_mutated, METH_NOARGS,
        NULL},
    /* XXX(brett.cannon): add showwarning? */
    /* XXX(brett.cannon): Reasonable to add formatwarning? */
    {NULL, NULL}                /* sentinel */
};


static PyObject *
create_filter(PyObject *category, const char *action)
{
    static PyObject *ignore_str = NULL;
    static PyObject *error_str = NULL;
    static PyObject *default_str = NULL;
    static PyObject *always_str = NULL;
    PyObject *action_obj = NULL;
    PyObject *lineno, *result;

    if (!strcmp(action, "ignore")) {
        if (ignore_str == NULL) {
            ignore_str = PyUnicode_InternFromString("ignore");
            if (ignore_str == NULL)
                return NULL;
        }
        action_obj = ignore_str;
    }
    else if (!strcmp(action, "error")) {
        if (error_str == NULL) {
            error_str = PyUnicode_InternFromString("error");
            if (error_str == NULL)
                return NULL;
        }
        action_obj = error_str;
    }
    else if (!strcmp(action, "default")) {
        if (default_str == NULL) {
            default_str = PyUnicode_InternFromString("default");
            if (default_str == NULL)
                return NULL;
        }
        action_obj = default_str;
    }
    else if (!strcmp(action, "always")) {
        if (always_str == NULL) {
            always_str = PyUnicode_InternFromString("always");
            if (always_str == NULL)
                return NULL;
        }
        action_obj = always_str;
    }
    else {
        Py_FatalError("unknown action");
    }

    /* This assumes the line number is zero for now. */
    lineno = PyLong_FromLong(0);
    if (lineno == NULL)
        return NULL;
    result = PyTuple_Pack(5, action_obj, Py_None, category, Py_None, lineno);
    Py_DECREF(lineno);
    return result;
}

static PyObject *
init_filters(void)
{
    PyObject *filters = PyList_New(5);
    unsigned int pos = 0;  /* Post-incremented in each use. */
    unsigned int x;
    const char *bytes_action, *resource_action;

    if (filters == NULL)
        return NULL;

    PyList_SET_ITEM(filters, pos++,
                    create_filter(PyExc_DeprecationWarning, "ignore"));
    PyList_SET_ITEM(filters, pos++,
                    create_filter(PyExc_PendingDeprecationWarning, "ignore"));
    PyList_SET_ITEM(filters, pos++,
                    create_filter(PyExc_ImportWarning, "ignore"));
    if (Py_BytesWarningFlag > 1)
        bytes_action = "error";
    else if (Py_BytesWarningFlag)
        bytes_action = "default";
    else
        bytes_action = "ignore";
    PyList_SET_ITEM(filters, pos++, create_filter(PyExc_BytesWarning,
                    bytes_action));
    /* resource usage warnings are enabled by default in pydebug mode */
#ifdef Py_DEBUG
    resource_action = "always";
#else
    resource_action = "ignore";
#endif
    PyList_SET_ITEM(filters, pos++, create_filter(PyExc_ResourceWarning,
                    resource_action));
    for (x = 0; x < pos; x += 1) {
        if (PyList_GET_ITEM(filters, x) == NULL) {
            Py_DECREF(filters);
            return NULL;
        }
    }

    return filters;
}

static struct PyModuleDef warningsmodule = {
        PyModuleDef_HEAD_INIT,
        MODULE_NAME,
        warnings__doc__,
        0,
        warnings_functions,
        NULL,
        NULL,
        NULL,
        NULL
};


PyMODINIT_FUNC
_PyWarnings_Init(void)
{
    PyObject *m;

    m = PyModule_Create(&warningsmodule);
    if (m == NULL)
        return NULL;

    if (_filters == NULL) {
        _filters = init_filters();
        if (_filters == NULL)
            return NULL;
    }
    Py_INCREF(_filters);
    if (PyModule_AddObject(m, "filters", _filters) < 0)
        return NULL;

    if (_once_registry == NULL) {
        _once_registry = PyDict_New();
        if (_once_registry == NULL)
            return NULL;
    }
    Py_INCREF(_once_registry);
    if (PyModule_AddObject(m, "_onceregistry", _once_registry) < 0)
        return NULL;

    if (_default_action == NULL) {
        _default_action = PyUnicode_FromString("default");
        if (_default_action == NULL)
            return NULL;
    }
    Py_INCREF(_default_action);
    if (PyModule_AddObject(m, "_defaultaction", _default_action) < 0)
        return NULL;

    _filters_version = 0;
    return m;
}
