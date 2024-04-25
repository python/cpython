
/* Support for dynamic loading of extension modules */

#include "Python.h"
#include "pycore_call.h"
#include "pycore_import.h"
#include "pycore_pyerrors.h"      // _PyErr_FormatFromCause()
#include "pycore_pystate.h"
#include "pycore_runtime.h"

/* ./configure sets HAVE_DYNAMIC_LOADING if dynamic loading of modules is
   supported on this platform. configure will then compile and link in one
   of the dynload_*.c files, as appropriate. We will call a function in
   those modules to get a function pointer to the module's init function.
*/
#ifdef HAVE_DYNAMIC_LOADING

#include "pycore_importdl.h"

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
                                PyObject *name, PyObject *filename)
{
    struct _Py_ext_module_loader_info info = {0};

    assert(name != NULL);
    if (!PyUnicode_Check(name)) {
        PyErr_SetString(PyExc_TypeError,
                        "module name must be a string");
        _Py_ext_module_loader_info_clear(&info);
        return -1;
    }
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
    int err = _Py_ext_module_loader_info_init(p_info, name, filename);
    Py_DECREF(name);
    Py_DECREF(filename);
    return err;
}


PyObject *
_PyImport_LoadDynamicModuleWithSpec(struct _Py_ext_module_loader_info *info,
                                    PyObject *spec, FILE *fp)
{
    PyObject *m = NULL;
    const char *name_buf = PyBytes_AS_STRING(info->name_encoded);
    const char *oldcontext;
    dl_funcptr exportfunc;
    PyModInitFunction p0;
    PyModuleDef *def;

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
        goto error;
    }

    p0 = (PyModInitFunction)exportfunc;

    /* Package context is needed for single-phase init */
    oldcontext = _PyImport_SwapPackageContext(info->newcontext);
    m = p0();
    _PyImport_SwapPackageContext(oldcontext);

    if (m == NULL) {
        if (!PyErr_Occurred()) {
            PyErr_Format(
                PyExc_SystemError,
                "initialization of %s failed without raising an exception",
                name_buf);
        }
        goto error;
    } else if (PyErr_Occurred()) {
        _PyErr_FormatFromCause(
            PyExc_SystemError,
            "initialization of %s raised unreported exception",
            name_buf);
        m = NULL;
        goto error;
    }
    if (Py_IS_TYPE(m, NULL)) {
        /* This can happen when a PyModuleDef is returned without calling
         * PyModuleDef_Init on it
         */
        PyErr_Format(PyExc_SystemError,
                     "init function of %s returned uninitialized object",
                     name_buf);
        m = NULL; /* prevent segfault in DECREF */
        goto error;
    }
    if (PyObject_TypeCheck(m, &PyModuleDef_Type)) {
        return PyModule_FromDefAndSpec((PyModuleDef*)m, spec);
    }

    /* Fall back to single-phase init mechanism */

    if (_PyImport_CheckSubinterpIncompatibleExtensionAllowed(name_buf) < 0) {
        goto error;
    }

    if (info->hook_prefix == nonascii_prefix) {
        /* don't allow legacy init for non-ASCII module names */
        PyErr_Format(
            PyExc_SystemError,
            "initialization of %s did not return PyModuleDef",
            name_buf);
        goto error;
    }

    /* Remember pointer to module init function. */
    def = PyModule_GetDef(m);
    if (def == NULL) {
        PyErr_Format(PyExc_SystemError,
                     "initialization of %s did not return an extension "
                     "module", name_buf);
        goto error;
    }
    def->m_base.m_init = p0;

    /* Remember the filename as the __file__ attribute */
    if (PyModule_AddObjectRef(m, "__file__", info->filename) < 0) {
        PyErr_Clear(); /* Not important enough to report */
    }

    PyObject *modules = PyImport_GetModuleDict();
    if (_PyImport_FixupExtensionObject(
                m, info->name, info->filename, modules) < 0)
    {
        goto error;
    }

    return m;

error:
    Py_XDECREF(m);
    return NULL;
}

#endif /* HAVE_DYNAMIC_LOADING */
