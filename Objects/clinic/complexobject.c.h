/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(complex_conjugate__doc__,
"conjugate($self, /)\n"
"--\n"
"\n"
"Return the complex conjugate of its argument. (3-4j).conjugate() == 3+4j.");

#define COMPLEX_CONJUGATE_METHODDEF    \
    {"conjugate", (PyCFunction)complex_conjugate, METH_NOARGS, complex_conjugate__doc__},

static PyObject *
complex_conjugate_impl(PyComplexObject *self);

static PyObject *
complex_conjugate(PyComplexObject *self, PyObject *Py_UNUSED(ignored))
{
    return complex_conjugate_impl(self);
}

PyDoc_STRVAR(complex___getnewargs____doc__,
"__getnewargs__($self, /)\n"
"--\n"
"\n");

#define COMPLEX___GETNEWARGS___METHODDEF    \
    {"__getnewargs__", (PyCFunction)complex___getnewargs__, METH_NOARGS, complex___getnewargs____doc__},

static PyObject *
complex___getnewargs___impl(PyComplexObject *self);

static PyObject *
complex___getnewargs__(PyComplexObject *self, PyObject *Py_UNUSED(ignored))
{
    return complex___getnewargs___impl(self);
}

PyDoc_STRVAR(complex___format____doc__,
"__format__($self, format_spec, /)\n"
"--\n"
"\n"
"Convert to a string according to format_spec.");

#define COMPLEX___FORMAT___METHODDEF    \
    {"__format__", (PyCFunction)complex___format__, METH_O, complex___format____doc__},

static PyObject *
complex___format___impl(PyComplexObject *self, PyObject *format_spec);

static PyObject *
complex___format__(PyComplexObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *format_spec;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("__format__", "argument", "str", arg);
        goto exit;
    }
    if (PyUnicode_READY(arg) == -1) {
        goto exit;
    }
    format_spec = arg;
    return_value = complex___format___impl(self, format_spec);

exit:
    return return_value;
}

PyDoc_STRVAR(complex___complex____doc__,
"__complex__($self, /)\n"
"--\n"
"\n"
"Convert this value to exact type complex.");

#define COMPLEX___COMPLEX___METHODDEF    \
    {"__complex__", (PyCFunction)complex___complex__, METH_NOARGS, complex___complex____doc__},

static PyObject *
complex___complex___impl(PyComplexObject *self);

static PyObject *
complex___complex__(PyComplexObject *self, PyObject *Py_UNUSED(ignored))
{
    return complex___complex___impl(self);
}

PyDoc_STRVAR(complex_new__doc__,
"complex(real=0, imag=0)\n"
"--\n"
"\n"
"Create a complex number from a real part and an optional imaginary part.\n"
"\n"
"This is equivalent to (real + imag*1j) where imag defaults to 0.");

static PyObject *
complex_new_impl(PyTypeObject *type, PyObject *r, PyObject *i);

static PyObject *
complex_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"real", "imag", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "complex", 0};
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *r = NULL;
    PyObject *i = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 0, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        r = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    i = fastargs[1];
skip_optional_pos:
    return_value = complex_new_impl(type, r, i);

exit:
    return return_value;
}
/*[clinic end generated code: output=6d85094ace15677e input=a9049054013a1b77]*/
