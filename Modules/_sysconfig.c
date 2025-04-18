// _sysconfig provides data for the Python sysconfig module

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"

#include "pycore_importdl.h"   // _PyImport_DynLoadFiletab
#include "pycore_long.h"       // _PyLong_GetZero, _PyLong_GetOne


/*[clinic input]
module _sysconfig
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=0a7c02d3e212ac97]*/

#include "clinic/_sysconfig.c.h"

#ifdef MS_WINDOWS
static int
add_string_value(PyObject *dict, const char *key, const char *str_value)
{
    PyObject *value = PyUnicode_FromString(str_value);
    if (value == NULL) {
        return -1;
    }
    int err = PyDict_SetItemString(dict, key, value);
    Py_DECREF(value);
    return err;
}
#endif

/*[clinic input]
_sysconfig.config_vars

Returns a dictionary containing build variables intended to be exposed by sysconfig.
[clinic start generated code]*/

static PyObject *
_sysconfig_config_vars_impl(PyObject *module)
/*[clinic end generated code: output=9c41cdee63ea9487 input=391ff42f3af57d01]*/
{
    PyObject *config = PyDict_New();
    if (config == NULL) {
        return NULL;
    }

#ifdef MS_WINDOWS
    if (add_string_value(config, "EXT_SUFFIX", PYD_TAGGED_SUFFIX) < 0) {
        Py_DECREF(config);
        return NULL;
    }
    if (add_string_value(config, "SOABI", PYD_SOABI) < 0) {
        Py_DECREF(config);
        return NULL;
    }
#endif

#ifdef Py_GIL_DISABLED
    PyObject *py_gil_disabled = _PyLong_GetOne();
#else
    PyObject *py_gil_disabled = _PyLong_GetZero();
#endif
    if (PyDict_SetItemString(config, "Py_GIL_DISABLED", py_gil_disabled) < 0) {
        Py_DECREF(config);
        return NULL;
    }

#ifdef Py_DEBUG
    PyObject *py_debug = _PyLong_GetOne();
#else
    PyObject *py_debug = _PyLong_GetZero();
#endif
    if (PyDict_SetItemString(config, "Py_DEBUG", py_debug) < 0) {
        Py_DECREF(config);
        return NULL;
    }

    return config;
}

PyDoc_STRVAR(sysconfig__doc__,
"A helper for the sysconfig module.");

static struct PyMethodDef sysconfig_methods[] = {
    _SYSCONFIG_CONFIG_VARS_METHODDEF
    {NULL, NULL}
};

static PyModuleDef_Slot sysconfig_slots[] = {
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static PyModuleDef sysconfig_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_sysconfig",
    .m_doc = sysconfig__doc__,
    .m_methods = sysconfig_methods,
    .m_slots = sysconfig_slots,
};

PyMODINIT_FUNC
PyInit__sysconfig(void)
{
    return PyModuleDef_Init(&sysconfig_module);
}
