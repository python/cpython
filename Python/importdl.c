
/* Support for dynamic loading of extension modules */

#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallMethod()
#include "pycore_import.h"        // _PyImport_SwapPackageContext()
#include "pycore_importdl.h"
#include "pycore_moduleobject.h"  // _PyModule_GetDef()
#include "pycore_moduleobject.h"  // _PyModule_GetDef()
#include "pycore_pyerrors.h"      // _PyErr_FormatFromCause()
#include "pycore_runtime.h"       // _Py_ID()


/* ./configure sets HAVE_DYNAMIC_LOADING if dynamic loading of modules is
   supported on this platform. configure will then compile and link in one
   of the dynload_*.c files, as appropriate. We will call a function in
   those modules to get a function pointer to the module's init function.
*/
#ifdef HAVE_DYNAMIC_LOADING

#ifdef MS_WINDOWS
extern dl_funcptr _PyImport_FindSharedFuncptrWindows(const char *prefix,
                                                     const char *shortname,
                                                     PyObject *pathname,
                                                     FILE *fp);
#else
extern dl_funcptr _PyImport_FindSharedFuncptr(const char *prefix,
                                              const char *shortname,
                                              const char *pathname, FILE *fp);
#endif

#endif /* HAVE_DYNAMIC_LOADING */


/***********************************/
/* module info to use when loading */
/***********************************/

static const char * const ascii_only_prefix = "PyInit";
static const char * const nonascii_prefix = "PyInitU";

/* Get the variable part of a module's export symbol name.
 * Returns a bytes instance. For non-ASCII-named modules, the name is
 * encoded as per PEP 489.
 * The hook_prefix pointer is set to either ascii_only_prefix or
 * nonascii_prefix, as appropriate.
 */
static PyObject *
get_encoded_name(PyObject *name, const char **hook_prefix) {
    PyObject *tmp;
    PyObject *encoded = NULL;
    PyObject *modname = NULL;
    Py_ssize_t name_len, lastdot;

    /* Get the short name (substring after last dot) */
    name_len = PyUnicode_GetLength(name);
    if (name_len < 0) {
        return NULL;
    }
    lastdot = PyUnicode_FindChar(name, '.', 0, name_len, -1);
    if (lastdot < -1) {
        return NULL;
    } else if (lastdot >= 0) {
        tmp = PyUnicode_Substring(name, lastdot + 1, name_len);
        if (tmp == NULL)
            return NULL;
        name = tmp;
        /* "name" now holds a new reference to the substring */
    } else {
        Py_INCREF(name);
    }

    /* Encode to ASCII or Punycode, as needed */
    encoded = PyUnicode_AsEncodedString(name, "ascii", NULL);
    if (encoded != NULL) {
        *hook_prefix = ascii_only_prefix;
    } else {
        if (PyErr_ExceptionMatches(PyExc_UnicodeEncodeError)) {
            PyErr_Clear();
            encoded = PyUnicode_AsEncodedString(name, "punycode", NULL);
            if (encoded == NULL) {
                goto error;
            }
            *hook_prefix = nonascii_prefix;
        } else {
            goto error;
        }
    }

    /* Replace '-' by '_' */
    modname = _PyObject_CallMethod(encoded, &_Py_ID(replace), "cc", '-', '_');
    if (modname == NULL)
        goto error;

    Py_DECREF(name);
    Py_DECREF(encoded);
    return modname;
error:
    Py_DECREF(name);
    Py_XDECREF(encoded);
    return NULL;
}

void
_Py_ext_module_loader_info_clear(struct _Py_ext_module_loader_info *info)
{
    Py_CLEAR(info->filename);
#ifndef MS_WINDOWS
    Py_CLEAR(info->filename_encoded);
#endif
    Py_CLEAR(info->name);
    Py_CLEAR(info->name_encoded);
}

int
_Py_ext_module_loader_info_init(struct _Py_ext_module_loader_info *p_info,
                                PyObject *name, PyObject *filename,
                                _Py_ext_module_origin origin)
{
    struct _Py_ext_module_loader_info info = {
        .origin=origin,
    };

    assert(name != NULL);
    if (!PyUnicode_Check(name)) {
        PyErr_SetString(PyExc_TypeError,
                        "module name must be a string");
        _Py_ext_module_loader_info_clear(&info);
        return -1;
    }
    assert(PyUnicode_GetLength(name) > 0);
    info.name = Py_NewRef(name);

    info.name_encoded = get_encoded_name(info.name, &info.hook_prefix);
    if (info.name_encoded == NULL) {
        _Py_ext_module_loader_info_clear(&info);
        return -1;
    }

    info.newcontext = PyUnicode_AsUTF8(info.name);
    if (info.newcontext == NULL) {
        _Py_ext_module_loader_info_clear(&info);
        return -1;
    }

    if (filename != NULL) {
        if (!PyUnicode_Check(filename)) {
            PyErr_SetString(PyExc_TypeError,
                            "module filename must be a string");
            _Py_ext_module_loader_info_clear(&info);
            return -1;
        }
        info.filename = Py_NewRef(filename);

#ifndef MS_WINDOWS
        info.filename_encoded = PyUnicode_EncodeFSDefault(info.filename);
        if (info.filename_encoded == NULL) {
            _Py_ext_module_loader_info_clear(&info);
            return -1;
        }
#endif

        info.path = info.filename;
    }
    else {
        info.path = info.name;
    }

    *p_info = info;
    return 0;
}

int
_Py_ext_module_loader_info_init_for_builtin(
                            struct _Py_ext_module_loader_info *info,
                            PyObject *name)
{
    assert(PyUnicode_Check(name));
    assert(PyUnicode_FindChar(name, '.', 0, PyUnicode_GetLength(name), -1) == -1);
    assert(PyUnicode_GetLength(name) > 0);

    PyObject *name_encoded = PyUnicode_AsEncodedString(name, "ascii", NULL);
    if (name_encoded == NULL) {
        return -1;
    }

    *info = (struct _Py_ext_module_loader_info){
        .name=Py_NewRef(name),
        .name_encoded=name_encoded,
        /* We won't need filename. */
        .path=name,
        .origin=_Py_ext_module_origin_BUILTIN,
        .hook_prefix=ascii_only_prefix,
        .newcontext=NULL,
    };
    return 0;
}

int
_Py_ext_module_loader_info_init_for_core(
                            struct _Py_ext_module_loader_info *info,
                            PyObject *name)
{
    if (_Py_ext_module_loader_info_init_for_builtin(info, name) < 0) {
        return -1;
    }
    info->origin = _Py_ext_module_origin_CORE;
    return 0;
}

#ifdef HAVE_DYNAMIC_LOADING
int
_Py_ext_module_loader_info_init_from_spec(
                            struct _Py_ext_module_loader_info *p_info,
                            PyObject *spec)
{
    PyObject *name = PyObject_GetAttrString(spec, "name");
    if (name == NULL) {
        return -1;
    }
    PyObject *filename = PyObject_GetAttrString(spec, "origin");
    if (filename == NULL) {
        Py_DECREF(name);
        return -1;
    }
    /* We could also accommodate builtin modules here without much trouble. */
    _Py_ext_module_origin origin = _Py_ext_module_origin_DYNAMIC;
    int err = _Py_ext_module_loader_info_init(p_info, name, filename, origin);
    Py_DECREF(name);
    Py_DECREF(filename);
    return err;
}
#endif /* HAVE_DYNAMIC_LOADING */


/********************************/
/* module init function results */
/********************************/

void
_Py_ext_module_loader_result_clear(struct _Py_ext_module_loader_result *res)
{
    /* Instead, the caller should have called
     * _Py_ext_module_loader_result_apply_error(). */
    assert(res->err == NULL);
    *res = (struct _Py_ext_module_loader_result){0};
}

static void
_Py_ext_module_loader_result_set_error(
                            struct _Py_ext_module_loader_result *res,
                            enum _Py_ext_module_loader_result_error_kind kind)
{
#ifndef NDEBUG
    switch (kind) {
    case _Py_ext_module_loader_result_EXCEPTION: _Py_FALLTHROUGH;
    case _Py_ext_module_loader_result_ERR_UNREPORTED_EXC:
        assert(PyErr_Occurred());
        break;
    case _Py_ext_module_loader_result_ERR_MISSING: _Py_FALLTHROUGH;
    case _Py_ext_module_loader_result_ERR_UNINITIALIZED: _Py_FALLTHROUGH;
    case _Py_ext_module_loader_result_ERR_NONASCII_NOT_MULTIPHASE: _Py_FALLTHROUGH;
    case _Py_ext_module_loader_result_ERR_NOT_MODULE: _Py_FALLTHROUGH;
    case _Py_ext_module_loader_result_ERR_MISSING_DEF:
        assert(!PyErr_Occurred());
        break;
    default:
        /* We added a new error kind but forgot to add it to this switch. */
        assert(0);
    }
#endif

    assert(res->err == NULL && res->_err.exc == NULL);
    res->err = &res->_err;
    *res->err = (struct _Py_ext_module_loader_result_error){
        .kind=kind,
        .exc=PyErr_GetRaisedException(),
    };

    /* For some kinds, we also set/check res->kind. */
    switch (kind) {
    case _Py_ext_module_loader_result_ERR_UNINITIALIZED:
        assert(res->kind == _Py_ext_module_kind_UNKNOWN);
        res->kind = _Py_ext_module_kind_INVALID;
        break;
    /* None of the rest affect the result kind. */
    case _Py_ext_module_loader_result_EXCEPTION: _Py_FALLTHROUGH;
    case _Py_ext_module_loader_result_ERR_MISSING: _Py_FALLTHROUGH;
    case _Py_ext_module_loader_result_ERR_UNREPORTED_EXC: _Py_FALLTHROUGH;
    case _Py_ext_module_loader_result_ERR_NONASCII_NOT_MULTIPHASE: _Py_FALLTHROUGH;
    case _Py_ext_module_loader_result_ERR_NOT_MODULE: _Py_FALLTHROUGH;
    case _Py_ext_module_loader_result_ERR_MISSING_DEF:
        break;
    default:
        /* We added a new error kind but forgot to add it to this switch. */
        assert(0);
    }
}

void
_Py_ext_module_loader_result_apply_error(
                            struct _Py_ext_module_loader_result *res,
                            const char *name)
{
    assert(!PyErr_Occurred());
    assert(res->err != NULL && res->err == &res->_err);
    struct _Py_ext_module_loader_result_error err = *res->err;
    res->err = NULL;

    /* We're otherwise done with the result at this point. */
    _Py_ext_module_loader_result_clear(res);

#ifndef NDEBUG
    switch (err.kind) {
    case _Py_ext_module_loader_result_EXCEPTION: _Py_FALLTHROUGH;
    case _Py_ext_module_loader_result_ERR_UNREPORTED_EXC:
        assert(err.exc != NULL);
        break;
    case _Py_ext_module_loader_result_ERR_MISSING: _Py_FALLTHROUGH;
    case _Py_ext_module_loader_result_ERR_UNINITIALIZED: _Py_FALLTHROUGH;
    case _Py_ext_module_loader_result_ERR_NONASCII_NOT_MULTIPHASE: _Py_FALLTHROUGH;
    case _Py_ext_module_loader_result_ERR_NOT_MODULE: _Py_FALLTHROUGH;
    case _Py_ext_module_loader_result_ERR_MISSING_DEF:
        assert(err.exc == NULL);
        break;
    default:
        /* We added a new error kind but forgot to add it to this switch. */
        assert(0);
    }
#endif

    const char *msg = NULL;
    switch (err.kind) {
    case _Py_ext_module_loader_result_EXCEPTION:
        break;
    case _Py_ext_module_loader_result_ERR_MISSING:
        msg = "initialization of %s failed without raising an exception";
        break;
    case _Py_ext_module_loader_result_ERR_UNREPORTED_EXC:
        msg = "initialization of %s raised unreported exception";
        break;
    case _Py_ext_module_loader_result_ERR_UNINITIALIZED:
        msg = "init function of %s returned uninitialized object";
        break;
    case _Py_ext_module_loader_result_ERR_NONASCII_NOT_MULTIPHASE:
        msg = "initialization of %s did not return PyModuleDef";
        break;
    case _Py_ext_module_loader_result_ERR_NOT_MODULE:
        msg = "initialization of %s did not return an extension module";
        break;
    case _Py_ext_module_loader_result_ERR_MISSING_DEF:
        msg = "initialization of %s did not return a valid extension module";
        break;
    default:
        /* We added a new error kind but forgot to add it to this switch. */
        assert(0);
        PyErr_Format(PyExc_SystemError,
                     "loading %s failed due to init function", name);
        return;
    }

    if (err.exc != NULL) {
        PyErr_SetRaisedException(err.exc);
        err.exc = NULL;  /* PyErr_SetRaisedException() stole our reference. */
        if (msg != NULL) {
            _PyErr_FormatFromCause(PyExc_SystemError, msg, name);
        }
    }
    else {
        assert(msg != NULL);
        PyErr_Format(PyExc_SystemError, msg, name);
    }
}


/********************************************/
/* getting/running the module init function */
/********************************************/

#ifdef HAVE_DYNAMIC_LOADING
PyModInitFunction
_PyImport_GetModInitFunc(struct _Py_ext_module_loader_info *info,
                         FILE *fp)
{
    const char *name_buf = PyBytes_AS_STRING(info->name_encoded);
    dl_funcptr exportfunc;
#ifdef MS_WINDOWS
    exportfunc = _PyImport_FindSharedFuncptrWindows(
            info->hook_prefix, name_buf, info->filename, fp);
#else
    {
        const char *path_buf = PyBytes_AS_STRING(info->filename_encoded);
        exportfunc = _PyImport_FindSharedFuncptr(
                        info->hook_prefix, name_buf, path_buf, fp);
    }
#endif

    if (exportfunc == NULL) {
        if (!PyErr_Occurred()) {
            PyObject *msg;
            msg = PyUnicode_FromFormat(
                "dynamic module does not define "
                "module export function (%s_%s)",
                info->hook_prefix, name_buf);
            if (msg != NULL) {
                PyErr_SetImportError(msg, info->name, info->filename);
                Py_DECREF(msg);
            }
        }
        return NULL;
    }

    return (PyModInitFunction)exportfunc;
}
#endif /* HAVE_DYNAMIC_LOADING */

int
_PyImport_RunModInitFunc(PyModInitFunction p0,
                         struct _Py_ext_module_loader_info *info,
                         struct _Py_ext_module_loader_result *p_res)
{
    struct _Py_ext_module_loader_result res = {
        .kind=_Py_ext_module_kind_UNKNOWN,
    };

    /* Call the module init function. */

    /* Package context is needed for single-phase init */
    const char *oldcontext = _PyImport_SwapPackageContext(info->newcontext);
    PyObject *m = p0();
    _PyImport_SwapPackageContext(oldcontext);

    /* Validate the result (and populate "res". */

    if (m == NULL) {
        /* The init func for multi-phase init modules is expected
         * to return a PyModuleDef after calling PyModuleDef_Init().
         * That function never raises an exception nor returns NULL,
         * so at this point it must be a single-phase init modules. */
        res.kind = _Py_ext_module_kind_SINGLEPHASE;
        if (PyErr_Occurred()) {
            _Py_ext_module_loader_result_set_error(
                        &res, _Py_ext_module_loader_result_EXCEPTION);
        }
        else {
            _Py_ext_module_loader_result_set_error(
                        &res, _Py_ext_module_loader_result_ERR_MISSING);
        }
        goto error;
    } else if (PyErr_Occurred()) {
        /* Likewise, we infer that this is a single-phase init module. */
        res.kind = _Py_ext_module_kind_SINGLEPHASE;
        _Py_ext_module_loader_result_set_error(
                &res, _Py_ext_module_loader_result_ERR_UNREPORTED_EXC);
        /* We would probably be correct to decref m here,
         * but we weren't doing so before,
         * so we stick with doing nothing. */
        m = NULL;
        goto error;
    }

    if (Py_IS_TYPE(m, NULL)) {
        /* This can happen when a PyModuleDef is returned without calling
         * PyModuleDef_Init on it
         */
        _Py_ext_module_loader_result_set_error(
                &res, _Py_ext_module_loader_result_ERR_UNINITIALIZED);
        /* Likewise, decref'ing here makes sense.  However, the original
         * code has a note about "prevent segfault in DECREF",
         * so we play it safe and leave it alone. */
        m = NULL; /* prevent segfault in DECREF */
        goto error;
    }

    if (PyObject_TypeCheck(m, &PyModuleDef_Type)) {
        /* multi-phase init */
        res.kind = _Py_ext_module_kind_MULTIPHASE;
        res.def = (PyModuleDef *)m;
        /* Run PyModule_FromDefAndSpec() to finish loading the module. */
    }
    else if (info->hook_prefix == nonascii_prefix) {
        /* Non-ASCII is only supported for multi-phase init. */
        res.kind = _Py_ext_module_kind_MULTIPHASE;
        /* Don't allow legacy init for non-ASCII module names. */
        _Py_ext_module_loader_result_set_error(
                &res, _Py_ext_module_loader_result_ERR_NONASCII_NOT_MULTIPHASE);
        goto error;
    }
    else {
        /* single-phase init (legacy) */
        res.kind = _Py_ext_module_kind_SINGLEPHASE;
        res.module = m;

        if (!PyModule_Check(m)) {
            _Py_ext_module_loader_result_set_error(
                    &res, _Py_ext_module_loader_result_ERR_NOT_MODULE);
            goto error;
        }

        res.def = _PyModule_GetDef(m);
        if (res.def == NULL) {
            PyErr_Clear();
            _Py_ext_module_loader_result_set_error(
                    &res, _Py_ext_module_loader_result_ERR_MISSING_DEF);
            goto error;
        }
    }

    assert(!PyErr_Occurred());
    assert(res.err == NULL);
    *p_res = res;
    return 0;

error:
    assert(!PyErr_Occurred());
    assert(res.err != NULL);
    Py_CLEAR(res.module);
    res.def = NULL;
    *p_res = res;
    p_res->err = &p_res->_err;
    return -1;
}
