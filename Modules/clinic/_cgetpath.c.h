/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_cgetpath_decode_locale__doc__,
"decode_locale($module, str, /)\n"
"--\n"
"\n");

#define _CGETPATH_DECODE_LOCALE_METHODDEF    \
    {"decode_locale", (PyCFunction)_cgetpath_decode_locale, METH_O, _cgetpath_decode_locale__doc__},

static PyObject *
_cgetpath_decode_locale_impl(PyObject *module, const char *str);

static PyObject *
_cgetpath_decode_locale(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *str;

    if (!PyArg_Parse(arg, "y:decode_locale", &str)) {
        goto exit;
    }
    return_value = _cgetpath_decode_locale_impl(module, str);

exit:
    return return_value;
}

PyDoc_STRVAR(_cgetpath_get_config__doc__,
"get_config($module, /)\n"
"--\n"
"\n");

#define _CGETPATH_GET_CONFIG_METHODDEF    \
    {"get_config", (PyCFunction)_cgetpath_get_config, METH_NOARGS, _cgetpath_get_config__doc__},

static PyObject *
_cgetpath_get_config_impl(PyObject *module);

static PyObject *
_cgetpath_get_config(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _cgetpath_get_config_impl(module);
}

PyDoc_STRVAR(_cgetpath_set_config__doc__,
"set_config($module, config, /)\n"
"--\n"
"\n");

#define _CGETPATH_SET_CONFIG_METHODDEF    \
    {"set_config", (PyCFunction)_cgetpath_set_config, METH_O, _cgetpath_set_config__doc__},

#if defined(__APPLE__)

PyDoc_STRVAR(_cgetpath__NSGetExecutablePath__doc__,
"_NSGetExecutablePath($module, /)\n"
"--\n"
"\n");

#define _CGETPATH__NSGETEXECUTABLEPATH_METHODDEF    \
    {"_NSGetExecutablePath", (PyCFunction)_cgetpath__NSGetExecutablePath, METH_NOARGS, _cgetpath__NSGetExecutablePath__doc__},

static PyObject *
_cgetpath__NSGetExecutablePath_impl(PyObject *module);

static PyObject *
_cgetpath__NSGetExecutablePath(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _cgetpath__NSGetExecutablePath_impl(module);
}

#endif /* defined(__APPLE__) */

#if defined(WITH_NEXT_FRAMEWORK)

PyDoc_STRVAR(_cgetpath_NSLibraryName__doc__,
"NSLibraryName($module, /)\n"
"--\n"
"\n");

#define _CGETPATH_NSLIBRARYNAME_METHODDEF    \
    {"NSLibraryName", (PyCFunction)_cgetpath_NSLibraryName, METH_NOARGS, _cgetpath_NSLibraryName__doc__},

static PyObject *
_cgetpath_NSLibraryName_impl(PyObject *module);

static PyObject *
_cgetpath_NSLibraryName(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _cgetpath_NSLibraryName_impl(module);
}

#endif /* defined(WITH_NEXT_FRAMEWORK) */

#ifndef _CGETPATH__NSGETEXECUTABLEPATH_METHODDEF
    #define _CGETPATH__NSGETEXECUTABLEPATH_METHODDEF
#endif /* !defined(_CGETPATH__NSGETEXECUTABLEPATH_METHODDEF) */

#ifndef _CGETPATH_NSLIBRARYNAME_METHODDEF
    #define _CGETPATH_NSLIBRARYNAME_METHODDEF
#endif /* !defined(_CGETPATH_NSLIBRARYNAME_METHODDEF) */
/*[clinic end generated code: output=c281e1bff9ca7d4c input=a9049054013a1b77]*/
