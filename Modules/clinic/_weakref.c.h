/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_weakref_getweakrefcount__doc__,
"getweakrefcount($module, object, /)\n"
"--\n"
"\n"
"Return the number of weak references to \'object\'.");

#define _WEAKREF_GETWEAKREFCOUNT_METHODDEF    \
    {"getweakrefcount", (PyCFunction)_weakref_getweakrefcount, METH_O, _weakref_getweakrefcount__doc__},

static Py_ssize_t
_weakref_getweakrefcount_impl(PyObject *module, PyObject *object);

static PyObject *
_weakref_getweakrefcount(PyObject *module, PyObject *object)
{
    PyObject *return_value = NULL;
    Py_ssize_t _return_value;

    _return_value = _weakref_getweakrefcount_impl(module, object);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_weakref__remove_dead_weakref__doc__,
"_remove_dead_weakref($module, dct, key, /)\n"
"--\n"
"\n"
"Atomically remove key from dict if it points to a dead weakref.");

#define _WEAKREF__REMOVE_DEAD_WEAKREF_METHODDEF    \
    {"_remove_dead_weakref", _PyCFunction_CAST(_weakref__remove_dead_weakref), METH_FASTCALL, _weakref__remove_dead_weakref__doc__},

static PyObject *
_weakref__remove_dead_weakref_impl(PyObject *module, PyObject *dct,
                                   PyObject *key);

static PyObject *
_weakref__remove_dead_weakref(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *dct;
    PyObject *key;

    if (!_PyArg_CheckPositional("_remove_dead_weakref", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyDict_Check(args[0])) {
        _PyArg_BadArgument("_remove_dead_weakref", "argument 1", "dict", args[0]);
        goto exit;
    }
    dct = args[0];
    key = args[1];
    return_value = _weakref__remove_dead_weakref_impl(module, dct, key);

exit:
    return return_value;
}

PyDoc_STRVAR(_weakref_getweakrefs__doc__,
"getweakrefs($module, object, /)\n"
"--\n"
"\n"
"Return a list of all weak reference objects pointing to \'object\'.");

#define _WEAKREF_GETWEAKREFS_METHODDEF    \
    {"getweakrefs", (PyCFunction)_weakref_getweakrefs, METH_O, _weakref_getweakrefs__doc__},

PyDoc_STRVAR(_weakref_proxy__doc__,
"proxy($module, object, callback=None, /)\n"
"--\n"
"\n"
"Create a proxy object that weakly references \'object\'.\n"
"\n"
"\'callback\', if given, is called with a reference to the\n"
"proxy when \'object\' is about to be finalized.");

#define _WEAKREF_PROXY_METHODDEF    \
    {"proxy", _PyCFunction_CAST(_weakref_proxy), METH_FASTCALL, _weakref_proxy__doc__},

static PyObject *
_weakref_proxy_impl(PyObject *module, PyObject *object, PyObject *callback);

static PyObject *
_weakref_proxy(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *object;
    PyObject *callback = NULL;

    if (!_PyArg_CheckPositional("proxy", nargs, 1, 2)) {
        goto exit;
    }
    object = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    callback = args[1];
skip_optional:
    return_value = _weakref_proxy_impl(module, object, callback);

exit:
    return return_value;
}
/*[clinic end generated code: output=60f59adc1dc9eab8 input=a9049054013a1b77]*/
