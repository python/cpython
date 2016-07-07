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
/*[clinic end generated code: output=e1ad587147323e19 input=a9049054013a1b77]*/
