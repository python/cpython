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
/*[clinic end generated code: output=25d395cf02eced1f input=a9049054013a1b77]*/
