/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(method___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define METHOD___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)method___reduce__, METH_NOARGS, method___reduce____doc__},

static PyObject *
method___reduce___impl(PyMethodObject *self);

static PyObject *
method___reduce__(PyMethodObject *self, PyObject *Py_UNUSED(ignored))
{
    return method___reduce___impl(self);
}

PyDoc_STRVAR(method__doc__,
"method(function, instance, /)\n"
"--\n"
"\n"
"Create a bound instance method object.");

static PyObject *
method_impl(PyTypeObject *type, PyObject *function, PyObject *instance);

static PyObject *
method(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *function;
    PyObject *instance;

    if ((type == &PyMethod_Type ||
         type->tp_init == PyMethod_Type.tp_init) &&
        !_PyArg_NoKeywords("method", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("method", PyTuple_GET_SIZE(args), 2, 2)) {
        goto exit;
    }
    function = PyTuple_GET_ITEM(args, 0);
    instance = PyTuple_GET_ITEM(args, 1);
    return_value = method_impl(type, function, instance);

exit:
    return return_value;
}

PyDoc_STRVAR(instancemethod__doc__,
"instancemethod(func)\n"
"--\n"
"\n"
"Bind a function to a class.");

static PyObject *
instancemethod_impl(PyTypeObject *type, PyObject *func);

static PyObject *
instancemethod(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"func", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "instancemethod", 0};
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *func;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    func = fastargs[0];
    return_value = instancemethod_impl(type, func);

exit:
    return return_value;
}
/*[clinic end generated code: output=87d655932436f47d input=a9049054013a1b77]*/
