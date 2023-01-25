/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(MethodType___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define METHODTYPE___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)MethodType___reduce__, METH_NOARGS, MethodType___reduce____doc__},

static PyObject *
MethodType___reduce___impl(PyMethodObject *self);

static PyObject *
MethodType___reduce__(PyMethodObject *self, PyObject *Py_UNUSED(ignored))
{
    return MethodType___reduce___impl(self);
}

PyDoc_STRVAR(method_new__doc__,
"MethodType(function, instance, /)\n"
"--\n"
"\n"
"Create a bound instance method object.");

static PyObject *
method_new_impl(PyTypeObject *type, PyObject *function, PyObject *instance);

static PyObject *
method_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *function;
    PyObject *instance;

    if ((type == &PyMethod_Type ||
         type->tp_init == PyMethod_Type.tp_init) &&
        !_PyArg_NoKeywords("MethodType", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("MethodType", PyTuple_GET_SIZE(args), 2, 2)) {
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
    PyObject *function;

    if ((type == &PyInstanceMethod_Type ||
         type->tp_init == PyInstanceMethod_Type.tp_init) &&
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
/*[clinic end generated code: output=b6a95b6c0ae95a3d input=a9049054013a1b77]*/
