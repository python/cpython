/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(method___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define METHOD___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)method___reduce__, METH_NOARGS, method___reduce____doc__},

static PyObject *
method___reduce___impl(PyMethodObject *self);

static PyObject *
method___reduce__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return method___reduce___impl((PyMethodObject *)self);
}

PyDoc_STRVAR(method_type_new__doc__,
"method(function, instance, /)\n"
"--\n"
"\n"
"Create a bound instance method object.");

static PyObject *
method_type_new_impl(PyTypeObject *type, PyObject *function,
                     PyObject *instance);

static PyObject *
method_type_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = &PyMethod_Type;
    PyObject *function;
    PyObject *instance;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("method", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("method", PyTuple_GET_SIZE(args), 2, 2)) {
        goto exit;
    }
    function = PyTuple_GET_ITEM(args, 0);
    instance = PyTuple_GET_ITEM(args, 1);
    return_value = method_type_new_impl(type, function, instance);

exit:
    return return_value;
}

static PyObject *
method_type_vectorcall(PyObject *type, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *function;
    PyObject *instance;

    assert(_PyType_CAST(type)->tp_new == method_type_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (!_PyArg_NoKwnames("method", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("method", nargs, 2, 2)) {
        goto exit;
    }
    function = args[0];
    instance = args[1];
    return_value = method_type_new_impl(_PyType_CAST(type), function, instance);

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
    if (!_PyArg_CheckPositional("instancemethod", PyTuple_GET_SIZE(args), 1, 1)) {
        goto exit;
    }
    function = PyTuple_GET_ITEM(args, 0);
    return_value = instancemethod_new_impl(type, function);

exit:
    return return_value;
}
/*[clinic end generated code: output=fd5c214ee6e55e88 input=a9049054013a1b77]*/
