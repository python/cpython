/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_testcapi_watch_dict__doc__,
"watch_dict($module, watcher_id, dict, /)\n"
"--\n"
"\n");

#define _TESTCAPI_WATCH_DICT_METHODDEF    \
    {"watch_dict", _PyCFunction_CAST(_testcapi_watch_dict), METH_FASTCALL, _testcapi_watch_dict__doc__},

static PyObject *
_testcapi_watch_dict_impl(PyObject *module, int watcher_id, PyObject *dict);

static PyObject *
_testcapi_watch_dict(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int watcher_id;
    PyObject *dict;

    if (!_PyArg_CheckPositional("watch_dict", nargs, 2, 2)) {
        goto exit;
    }
    watcher_id = PyLong_AsInt(args[0]);
    if (watcher_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    dict = args[1];
    return_value = _testcapi_watch_dict_impl(module, watcher_id, dict);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_unwatch_dict__doc__,
"unwatch_dict($module, watcher_id, dict, /)\n"
"--\n"
"\n");

#define _TESTCAPI_UNWATCH_DICT_METHODDEF    \
    {"unwatch_dict", _PyCFunction_CAST(_testcapi_unwatch_dict), METH_FASTCALL, _testcapi_unwatch_dict__doc__},

static PyObject *
_testcapi_unwatch_dict_impl(PyObject *module, int watcher_id, PyObject *dict);

static PyObject *
_testcapi_unwatch_dict(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int watcher_id;
    PyObject *dict;

    if (!_PyArg_CheckPositional("unwatch_dict", nargs, 2, 2)) {
        goto exit;
    }
    watcher_id = PyLong_AsInt(args[0]);
    if (watcher_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    dict = args[1];
    return_value = _testcapi_unwatch_dict_impl(module, watcher_id, dict);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_watch_type__doc__,
"watch_type($module, watcher_id, type, /)\n"
"--\n"
"\n");

#define _TESTCAPI_WATCH_TYPE_METHODDEF    \
    {"watch_type", _PyCFunction_CAST(_testcapi_watch_type), METH_FASTCALL, _testcapi_watch_type__doc__},

static PyObject *
_testcapi_watch_type_impl(PyObject *module, int watcher_id, PyObject *type);

static PyObject *
_testcapi_watch_type(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int watcher_id;
    PyObject *type;

    if (!_PyArg_CheckPositional("watch_type", nargs, 2, 2)) {
        goto exit;
    }
    watcher_id = PyLong_AsInt(args[0]);
    if (watcher_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    type = args[1];
    return_value = _testcapi_watch_type_impl(module, watcher_id, type);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_unwatch_type__doc__,
"unwatch_type($module, watcher_id, type, /)\n"
"--\n"
"\n");

#define _TESTCAPI_UNWATCH_TYPE_METHODDEF    \
    {"unwatch_type", _PyCFunction_CAST(_testcapi_unwatch_type), METH_FASTCALL, _testcapi_unwatch_type__doc__},

static PyObject *
_testcapi_unwatch_type_impl(PyObject *module, int watcher_id, PyObject *type);

static PyObject *
_testcapi_unwatch_type(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int watcher_id;
    PyObject *type;

    if (!_PyArg_CheckPositional("unwatch_type", nargs, 2, 2)) {
        goto exit;
    }
    watcher_id = PyLong_AsInt(args[0]);
    if (watcher_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    type = args[1];
    return_value = _testcapi_unwatch_type_impl(module, watcher_id, type);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_set_func_defaults_via_capi__doc__,
"set_func_defaults_via_capi($module, func, defaults, /)\n"
"--\n"
"\n");

#define _TESTCAPI_SET_FUNC_DEFAULTS_VIA_CAPI_METHODDEF    \
    {"set_func_defaults_via_capi", _PyCFunction_CAST(_testcapi_set_func_defaults_via_capi), METH_FASTCALL, _testcapi_set_func_defaults_via_capi__doc__},

static PyObject *
_testcapi_set_func_defaults_via_capi_impl(PyObject *module, PyObject *func,
                                          PyObject *defaults);

static PyObject *
_testcapi_set_func_defaults_via_capi(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *func;
    PyObject *defaults;

    if (!_PyArg_CheckPositional("set_func_defaults_via_capi", nargs, 2, 2)) {
        goto exit;
    }
    func = args[0];
    defaults = args[1];
    return_value = _testcapi_set_func_defaults_via_capi_impl(module, func, defaults);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_set_func_kwdefaults_via_capi__doc__,
"set_func_kwdefaults_via_capi($module, func, defaults, /)\n"
"--\n"
"\n");

#define _TESTCAPI_SET_FUNC_KWDEFAULTS_VIA_CAPI_METHODDEF    \
    {"set_func_kwdefaults_via_capi", _PyCFunction_CAST(_testcapi_set_func_kwdefaults_via_capi), METH_FASTCALL, _testcapi_set_func_kwdefaults_via_capi__doc__},

static PyObject *
_testcapi_set_func_kwdefaults_via_capi_impl(PyObject *module, PyObject *func,
                                            PyObject *defaults);

static PyObject *
_testcapi_set_func_kwdefaults_via_capi(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *func;
    PyObject *defaults;

    if (!_PyArg_CheckPositional("set_func_kwdefaults_via_capi", nargs, 2, 2)) {
        goto exit;
    }
    func = args[0];
    defaults = args[1];
    return_value = _testcapi_set_func_kwdefaults_via_capi_impl(module, func, defaults);

exit:
    return return_value;
}
/*[clinic end generated code: output=0e07ce7f295917a5 input=a9049054013a1b77]*/
