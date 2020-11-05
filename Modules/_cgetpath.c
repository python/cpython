#include "Python.h"
#include "pycore_initconfig.h"    // _PyStatus_EXCEPTION()
#include "clinic/_cgetpath.c.h"
#include "osdefs.h"               // DELIM

#if !defined(PREFIX) || !defined(EXEC_PREFIX) || !defined(VERSION) || !defined(VPATH)
#  error "PREFIX, EXEC_PREFIX, VERSION and VPATH macros must be defined"
#endif

/*[clinic input]
module _cgetpath
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=2ee51ad17253a32a]*/

static PyObject*
decode_locale(const char *str)
{
    size_t len;
    wchar_t *wstr = Py_DecodeLocale(str, &len);
    if (wstr == NULL) {
        if (len == (size_t)-2) {
            PyErr_SetString(PyExc_ValueError,
                            "Py_DecodeLocale failed to decode");
        }
        else {
            PyErr_NoMemory();
        }
        return 0;
    }

    PyObject *unicode = PyUnicode_FromWideChar(wstr, -1);
    PyMem_RawFree(wstr);
    return unicode;
}

/*[clinic input]
_cgetpath.decode_locale

    str: str(accept={robuffer})
    /

[clinic start generated code]*/

static PyObject *
_cgetpath_decode_locale_impl(PyObject *module, const char *str)
/*[clinic end generated code: output=53291003c8f4dc65 input=39ef5541c1823ef7]*/
{
    return decode_locale(str);
}


/*[clinic input]
_cgetpath.get_config

[clinic start generated code]*/

static PyObject *
_cgetpath_get_config_impl(PyObject *module)
/*[clinic end generated code: output=38e434c217fc678f input=c33d510b06e8ed50]*/
/*[clinic end generated code]*/
{
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);
    if (_PyInterpreterState_GetConfigCopy(&config) < 0) {
        PyConfig_Clear(&config);
        return NULL;
    }
    PyObject *dict = _PyConfig_AsDict(&config);
    PyConfig_Clear(&config);
    return dict;
}


/*[clinic input]
_cgetpath.set_config

    config as dict: object
    /

[clinic start generated code]*/

static PyObject *
_cgetpath_set_config(PyObject *module, PyObject *dict)
/*[clinic end generated code: output=73272b1a67b90ddf input=a222d368b44090f8]*/
/*[clinic end generated code]*/
{
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);
    if (_PyConfig_FromDict(&config, dict) < 0) {
        PyConfig_Clear(&config);
        return NULL;
    }
    if (_PyInterpreterState_SetConfig(&config) < 0) {
        return NULL;
    }
    PyConfig_Clear(&config);
    Py_RETURN_NONE;
}


#ifdef __APPLE__
/*[clinic input]
_cgetpath._NSGetExecutablePath
[clinic start generated code]*/

static PyObject *
_cgetpath__NSGetExecutablePath_impl(PyObject *module)
/*[clinic end generated code: output=37ff1ed4de8a75cb input=2675b451446eca9a]*/
{
    char execpath[MAXPATHLEN + 1];
    uint32_t nsexeclength = Py_ARRAY_LENGTH(execpath) - 1;
    execpath[nsexeclength] = '\0';

    if (_NSGetExecutablePath(execpath, &nsexeclength) != 0) {
        Py_RETURN_NONE;
    }

    return decode_locale(execpath);
}
#endif  /* __APPLE__ */


#ifdef WITH_NEXT_FRAMEWORK
/*[clinic input]
_cgetpath.NSLibraryName
[clinic start generated code]*/

static PyObject *
_cgetpath_NSLibraryName_impl(PyObject *module)
/*[clinic end generated code: output=2c1aade0f8e2dd09 input=3c4a291cd9952e45]*/
{
    NSModule pythonModule;

    /* On Mac OS X we have a special case if we're running from a framework.
       This is because the python home should be set relative to the library,
       which is in the framework, not relative to the executable, which may
       be outside of the framework. Except when we're in the build
       directory... */
    pythonModule = NSModuleForSymbol(NSLookupAndBindSymbol("_Py_Initialize"));

    /* Use dylib functions to find out where the framework was loaded from */
    const char* modPath = NSLibraryNameForModule(pythonModule);
    if (modPath == NULL) {
        Py_RETURN_NONE;
    }

    return decode_locale(modPath);
}
#endif


static PyMethodDef _cgetpath_methods[] = {
    _CGETPATH_DECODE_LOCALE_METHODDEF
    _CGETPATH_GET_CONFIG_METHODDEF
    _CGETPATH_SET_CONFIG_METHODDEF
    _CGETPATH__NSGETEXECUTABLEPATH_METHODDEF
    _CGETPATH_NSLIBRARYNAME_METHODDEF
    {NULL,              NULL}            /* Sentinel */
};


static int
cgetpathmodule_exec(PyObject *mod)
{
    // FIXME: fix ref leaks
    if (PyModule_AddObject(mod, "PYTHONPATH", PyBytes_FromString(PYTHONPATH)) < 0) {
        return -1;
    }
    if (PyModule_AddObject(mod, "PREFIX", PyBytes_FromString(PREFIX)) < 0) {
        return -1;
    }
    if (PyModule_AddObject(mod, "EXEC_PREFIX", PyBytes_FromString(EXEC_PREFIX)) < 0) {
        return -1;
    }
    if (PyModule_AddObject(mod, "VPATH", PyBytes_FromString(VPATH)) < 0) {
        return -1;
    }
    if (PyModule_AddObject(mod, "VERSION", PyBytes_FromString(VERSION)) < 0) {
        return -1;
    }
#ifdef WITH_NEXT_FRAMEWORK
    PyObject *flag = Py_True;
#else
    PyObject *flag = Py_False;
#endif
    if (PyModule_AddObjectRef(mod, "WITH_NEXT_FRAMEWORK", flag) < 0) {
        return -1;
    }
    const wchar_t delimiter[2] = {DELIM, '\0'};
    if (PyModule_AddObject(mod, "DELIM",
                           PyUnicode_FromWideChar(delimiter, -1)) < 0) {
        return -1;
    }
    const wchar_t separator[2] = {SEP, '\0'};
    if (PyModule_AddObject(mod, "SEP",
                           PyUnicode_FromWideChar(separator, -1)) < 0) {
        return -1;
    }
    return 0;
}


static PyModuleDef_Slot cgetpathmodile_slots[] = {
    {Py_mod_exec, cgetpathmodule_exec},
    {0, NULL}
};

static struct PyModuleDef cgetpathmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_cgetpath",
    .m_doc = NULL,
    .m_size = 0,
    .m_methods = _cgetpath_methods,
    .m_slots = cgetpathmodile_slots,
};

PyMODINIT_FUNC
PyInit__cgetpath(void)
{
    return PyModuleDef_Init(&cgetpathmodule);
}

#ifdef __cplusplus
}
#endif

