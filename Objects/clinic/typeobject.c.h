/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_BadArgument()

PyDoc_STRVAR(type___instancecheck____doc__,
"__instancecheck__($self, instance, /)\n"
"--\n"
"\n"
"Check if an object is an instance.");

#define TYPE___INSTANCECHECK___METHODDEF    \
    {"__instancecheck__", (PyCFunction)type___instancecheck__, METH_O, type___instancecheck____doc__},

static int
type___instancecheck___impl(PyTypeObject *self, PyObject *instance);

static PyObject *
type___instancecheck__(PyObject *self, PyObject *instance)
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = type___instancecheck___impl((PyTypeObject *)self, instance);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(type___subclasscheck____doc__,
"__subclasscheck__($self, subclass, /)\n"
"--\n"
"\n"
"Check if a class is a subclass.");

#define TYPE___SUBCLASSCHECK___METHODDEF    \
    {"__subclasscheck__", (PyCFunction)type___subclasscheck__, METH_O, type___subclasscheck____doc__},

static int
type___subclasscheck___impl(PyTypeObject *self, PyObject *subclass);

static PyObject *
type___subclasscheck__(PyObject *self, PyObject *subclass)
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = type___subclasscheck___impl((PyTypeObject *)self, subclass);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(type_mro__doc__,
"mro($self, /)\n"
"--\n"
"\n"
"Return a type\'s method resolution order.");

#define TYPE_MRO_METHODDEF    \
    {"mro", (PyCFunction)type_mro, METH_NOARGS, type_mro__doc__},

static PyObject *
type_mro_impl(PyTypeObject *self);

static PyObject *
type_mro(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return type_mro_impl((PyTypeObject *)self);
}

PyDoc_STRVAR(type___subclasses____doc__,
"__subclasses__($self, /)\n"
"--\n"
"\n"
"Return a list of immediate subclasses.");

#define TYPE___SUBCLASSES___METHODDEF    \
    {"__subclasses__", (PyCFunction)type___subclasses__, METH_NOARGS, type___subclasses____doc__},

static PyObject *
type___subclasses___impl(PyTypeObject *self);

static PyObject *
type___subclasses__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return type___subclasses___impl((PyTypeObject *)self);
}

PyDoc_STRVAR(type___dir____doc__,
"__dir__($self, /)\n"
"--\n"
"\n"
"Specialized __dir__ implementation for types.");

#define TYPE___DIR___METHODDEF    \
    {"__dir__", (PyCFunction)type___dir__, METH_NOARGS, type___dir____doc__},

static PyObject *
type___dir___impl(PyTypeObject *self);

static PyObject *
type___dir__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return type___dir___impl((PyTypeObject *)self);
}

PyDoc_STRVAR(type___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Return memory consumption of the type object.");

#define TYPE___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)type___sizeof__, METH_NOARGS, type___sizeof____doc__},

static PyObject *
type___sizeof___impl(PyTypeObject *self);

static PyObject *
type___sizeof__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return type___sizeof___impl((PyTypeObject *)self);
}

PyDoc_STRVAR(object___getstate____doc__,
"__getstate__($self, /)\n"
"--\n"
"\n"
"Helper for pickle.");

#define OBJECT___GETSTATE___METHODDEF    \
    {"__getstate__", (PyCFunction)object___getstate__, METH_NOARGS, object___getstate____doc__},

static PyObject *
object___getstate___impl(PyObject *self);

static PyObject *
object___getstate__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return object___getstate___impl(self);
}

PyDoc_STRVAR(object___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Helper for pickle.");

#define OBJECT___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)object___reduce__, METH_NOARGS, object___reduce____doc__},

static PyObject *
object___reduce___impl(PyObject *self);

static PyObject *
object___reduce__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return object___reduce___impl(self);
}

PyDoc_STRVAR(object___reduce_ex____doc__,
"__reduce_ex__($self, protocol, /)\n"
"--\n"
"\n"
"Helper for pickle.");

#define OBJECT___REDUCE_EX___METHODDEF    \
    {"__reduce_ex__", (PyCFunction)object___reduce_ex__, METH_O, object___reduce_ex____doc__},

static PyObject *
object___reduce_ex___impl(PyObject *self, int protocol);

static PyObject *
object___reduce_ex__(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int protocol;

    protocol = PyLong_AsInt(arg);
    if (protocol == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = object___reduce_ex___impl(self, protocol);

exit:
    return return_value;
}

PyDoc_STRVAR(object___format____doc__,
"__format__($self, format_spec, /)\n"
"--\n"
"\n"
"Default object formatter.\n"
"\n"
"Return str(self) if format_spec is empty. Raise TypeError otherwise.");

#define OBJECT___FORMAT___METHODDEF    \
    {"__format__", (PyCFunction)object___format__, METH_O, object___format____doc__},

static PyObject *
object___format___impl(PyObject *self, PyObject *format_spec);

static PyObject *
object___format__(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *format_spec;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("__format__", "argument", "str", arg);
        goto exit;
    }
    format_spec = arg;
    return_value = object___format___impl(self, format_spec);

exit:
    return return_value;
}

PyDoc_STRVAR(object___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Size of object in memory, in bytes.");

#define OBJECT___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)object___sizeof__, METH_NOARGS, object___sizeof____doc__},

static PyObject *
object___sizeof___impl(PyObject *self);

static PyObject *
object___sizeof__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return object___sizeof___impl(self);
}

PyDoc_STRVAR(object___dir____doc__,
"__dir__($self, /)\n"
"--\n"
"\n"
"Default dir() implementation.");

#define OBJECT___DIR___METHODDEF    \
    {"__dir__", (PyCFunction)object___dir__, METH_NOARGS, object___dir____doc__},

static PyObject *
object___dir___impl(PyObject *self);

static PyObject *
object___dir__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return object___dir___impl(self);
}
/*[clinic end generated code: output=b55c0d257e2518d2 input=a9049054013a1b77]*/
