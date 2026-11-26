/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_sysconfig_config_vars__doc__,
"config_vars($module, /)\n"
"--\n"
"\n"
"Returns a dictionary containing build variables intended to be exposed by sysconfig.");

#define _SYSCONFIG_CONFIG_VARS_METHODDEF    \
    {"config_vars", (PyCFunction)_sysconfig_config_vars, METH_NOARGS, _sysconfig_config_vars__doc__},

static PyObject *
_sysconfig_config_vars_impl(PyObject *module);

static PyObject *
_sysconfig_config_vars(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _sysconfig_config_vars_impl(module);
}

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_sysconfig_get_platform__doc__,
"get_platform($module, /)\n"
"--\n"
"\n"
"Return a string that identifies the current platform.");

#define _SYSCONFIG_GET_PLATFORM_METHODDEF    \
    {"get_platform", (PyCFunction)_sysconfig_get_platform, METH_NOARGS, _sysconfig_get_platform__doc__},

static PyObject *
_sysconfig_get_platform_impl(PyObject *module);

static PyObject *
_sysconfig_get_platform(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _sysconfig_get_platform_impl(module);
}

#endif /* defined(MS_WINDOWS) */

#ifndef _SYSCONFIG_GET_PLATFORM_METHODDEF
    #define _SYSCONFIG_GET_PLATFORM_METHODDEF
#endif /* !defined(_SYSCONFIG_GET_PLATFORM_METHODDEF) */
/*[clinic end generated code: output=558531e148f92320 input=a9049054013a1b77]*/
