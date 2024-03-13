/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_NoKeywords()

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

PyDoc_STRVAR(method_new__doc__,
"method(function, instance, /)\n"
"--\n"
"\n"
"Create a bound instance method object.");

static PyObject *
method_new_impl(PyTypeObject *type, PyObject *function, PyObject *instance);

static PyObject *
method_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = &PyMethod_Type;
    PyObject *function;
    PyObject *instance;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("method", kwargs)) {
        goto exit;
    }
    if (PyTuple_GET_SIZE(args) < 2) {
        PyErr_Format(
            PyExc_TypeError,
            "%s expected 2 arguments, got %zd",
            "method", PyTuple_GET_SIZE(args));
        goto exit;
    }

    if (PyTuple_GET_SIZE(args) != 0 && PyTuple_GET_SIZE(args) > 2) {
        PyErr_Format(
            PyExc_TypeError,
            "%s expected 2 arguments, got %zd",
            "method", PyTuple_GET_SIZE(args));
        goto exit;
    }
    function = PyTuple_GET_ITEM(args, 0);
    instance = PyTuple_GET_ITEM(args, 1);
    return_value = method_new_impl(type, function, instance);

exit:
    return return_value;
}

PyDoc_STRVAR(instancemethod_new__doc__,
"instancemethod(function, /)\n"
"--\n"
"\n"
"Bind a function to a class.");

static PyObject *
instancemethod_new_impl(PyTypeObject *type, PyObject *function);

static PyObject *
instancemethod_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = &PyInstanceMethod_Type;
    PyObject *function;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("instancemethod", kwargs)) {
        goto exit;
    }
    if (PyTuple_GET_SIZE(args) < 1) {
        PyErr_Format(
            PyExc_TypeError,
            "%s expected 1 argument, got %zd",
            "instancemethod", PyTuple_GET_SIZE(args));
        goto exit;
    }

    if (PyTuple_GET_SIZE(args) != 0 && PyTuple_GET_SIZE(args) > 1) {
        PyErr_Format(
            PyExc_TypeError,
            "%s expected 1 argument, got %zd",
            "instancemethod", PyTuple_GET_SIZE(args));
        goto exit;
    }
    function = PyTuple_GET_ITEM(args, 0);
    return_value = instancemethod_new_impl(type, function);

exit:
    return return_value;
}
/*[clinic end generated code: output=f25d0b51e94fe9dc input=a9049054013a1b77]*/
