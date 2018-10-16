/*[clinic input]
preserve
[clinic start generated code]*/

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
    {"_remove_dead_weakref", (PyCFunction)_weakref__remove_dead_weakref, METH_VARARGS, _weakref__remove_dead_weakref__doc__},

static PyObject *
_weakref__remove_dead_weakref_impl(PyObject *module, PyObject *dct,
                                   PyObject *key);

static PyObject *
_weakref__remove_dead_weakref(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *dct;
    PyObject *key;

    if (!PyArg_ParseTuple(args, "O!O:_remove_dead_weakref",
        &PyDict_Type, &dct, &key)) {
        goto exit;
    }
    return_value = _weakref__remove_dead_weakref_impl(module, dct, key);

exit:
    return return_value;
}
/*[clinic end generated code: output=e860dd818a44bc9b input=a9049054013a1b77]*/
