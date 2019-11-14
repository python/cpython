/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(float_is_integer__doc__,
"is_integer($self, /)\n"
"--\n"
"\n"
"Return True if the float is an integer.");

#define FLOAT_IS_INTEGER_METHODDEF    \
    {"is_integer", (PyCFunction)float_is_integer, METH_NOARGS, float_is_integer__doc__},

static PyObject *
float_is_integer_impl(PyObject *self);

static PyObject *
float_is_integer(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return float_is_integer_impl(self);
}

PyDoc_STRVAR(float___trunc____doc__,
"__trunc__($self, /)\n"
"--\n"
"\n"
"Return the Integral closest to x between 0 and x.");

#define FLOAT___TRUNC___METHODDEF    \
    {"__trunc__", (PyCFunction)float___trunc__, METH_NOARGS, float___trunc____doc__},

static PyObject *
float___trunc___impl(PyObject *self);

static PyObject *
float___trunc__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return float___trunc___impl(self);
}

PyDoc_STRVAR(float___round____doc__,
"__round__($self, ndigits=None, /)\n"
"--\n"
"\n"
"Return the Integral closest to x, rounding half toward even.\n"
"\n"
"When an argument is passed, work like built-in round(x, ndigits).");

#define FLOAT___ROUND___METHODDEF    \
    {"__round__", (PyCFunction)(void(*)(void))float___round__, METH_FASTCALL, float___round____doc__},

static PyObject *
float___round___impl(PyObject *self, PyObject *o_ndigits);

static PyObject *
float___round__(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *o_ndigits = Py_None;

    if (!_PyArg_CheckPositional("__round__", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    o_ndigits = args[0];
skip_optional:
    return_value = float___round___impl(self, o_ndigits);

exit:
    return return_value;
}

PyDoc_STRVAR(float_conjugate__doc__,
"conjugate($self, /)\n"
"--\n"
"\n"
"Return self, the complex conjugate of any float.");

#define FLOAT_CONJUGATE_METHODDEF    \
    {"conjugate", (PyCFunction)float_conjugate, METH_NOARGS, float_conjugate__doc__},

static PyObject *
float_conjugate_impl(PyObject *self);

static PyObject *
float_conjugate(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return float_conjugate_impl(self);
}

PyDoc_STRVAR(float_hex__doc__,
"hex($self, /)\n"
"--\n"
"\n"
"Return a hexadecimal representation of a floating-point number.\n"
"\n"
">>> (-0.1).hex()\n"
"\'-0x1.999999999999ap-4\'\n"
">>> 3.14159.hex()\n"
"\'0x1.921f9f01b866ep+1\'");

#define FLOAT_HEX_METHODDEF    \
    {"hex", (PyCFunction)float_hex, METH_NOARGS, float_hex__doc__},

static PyObject *
float_hex_impl(PyObject *self);

static PyObject *
float_hex(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return float_hex_impl(self);
}

PyDoc_STRVAR(float_fromhex__doc__,
"fromhex($type, string, /)\n"
"--\n"
"\n"
"Create a floating-point number from a hexadecimal string.\n"
"\n"
">>> float.fromhex(\'0x1.ffffp10\')\n"
"2047.984375\n"
">>> float.fromhex(\'-0x1p-1074\')\n"
"-5e-324");

#define FLOAT_FROMHEX_METHODDEF    \
    {"fromhex", (PyCFunction)float_fromhex, METH_O|METH_CLASS, float_fromhex__doc__},

PyDoc_STRVAR(float_as_integer_ratio__doc__,
"as_integer_ratio($self, /)\n"
"--\n"
"\n"
"Return integer ratio.\n"
"\n"
"Return a pair of integers, whose ratio is exactly equal to the original float\n"
"and with a positive denominator.\n"
"\n"
"Raise OverflowError on infinities and a ValueError on NaNs.\n"
"\n"
">>> (10.0).as_integer_ratio()\n"
"(10, 1)\n"
">>> (0.0).as_integer_ratio()\n"
"(0, 1)\n"
">>> (-.25).as_integer_ratio()\n"
"(-1, 4)");

#define FLOAT_AS_INTEGER_RATIO_METHODDEF    \
    {"as_integer_ratio", (PyCFunction)float_as_integer_ratio, METH_NOARGS, float_as_integer_ratio__doc__},

static PyObject *
float_as_integer_ratio_impl(PyObject *self);

static PyObject *
float_as_integer_ratio(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return float_as_integer_ratio_impl(self);
}

PyDoc_STRVAR(float_new__doc__,
"float(x=0, /)\n"
"--\n"
"\n"
"Convert a string or number to a floating point number, if possible.");

static PyObject *
float_new_impl(PyTypeObject *type, PyObject *x);

static PyObject *
float_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *x = _PyLong_Zero;

    if ((type == &PyFloat_Type) &&
        !_PyArg_NoKeywords("float", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("float", PyTuple_GET_SIZE(args), 0, 1)) {
        goto exit;
    }
    if (PyTuple_GET_SIZE(args) < 1) {
        goto skip_optional;
    }
    x = PyTuple_GET_ITEM(args, 0);
skip_optional:
    return_value = float_new_impl(type, x);

exit:
    return return_value;
}

PyDoc_STRVAR(float___getnewargs____doc__,
"__getnewargs__($self, /)\n"
"--\n"
"\n");

#define FLOAT___GETNEWARGS___METHODDEF    \
    {"__getnewargs__", (PyCFunction)float___getnewargs__, METH_NOARGS, float___getnewargs____doc__},

static PyObject *
float___getnewargs___impl(PyObject *self);

static PyObject *
float___getnewargs__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return float___getnewargs___impl(self);
}

PyDoc_STRVAR(float___getformat____doc__,
"__getformat__($type, typestr, /)\n"
"--\n"
"\n"
"You probably don\'t want to use this function.\n"
"\n"
"  typestr\n"
"    Must be \'double\' or \'float\'.\n"
"\n"
"It exists mainly to be used in Python\'s test suite.\n"
"\n"
"This function returns whichever of \'unknown\', \'IEEE, big-endian\' or \'IEEE,\n"
"little-endian\' best describes the format of floating point numbers used by the\n"
"C type named by typestr.");

#define FLOAT___GETFORMAT___METHODDEF    \
    {"__getformat__", (PyCFunction)float___getformat__, METH_O|METH_CLASS, float___getformat____doc__},

static PyObject *
float___getformat___impl(PyTypeObject *type, const char *typestr);

static PyObject *
float___getformat__(PyTypeObject *type, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *typestr;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("__getformat__", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t typestr_length;
    typestr = PyUnicode_AsUTF8AndSize(arg, &typestr_length);
    if (typestr == NULL) {
        goto exit;
    }
    if (strlen(typestr) != (size_t)typestr_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = float___getformat___impl(type, typestr);

exit:
    return return_value;
}

PyDoc_STRVAR(float___set_format____doc__,
"__set_format__($type, typestr, fmt, /)\n"
"--\n"
"\n"
"You probably don\'t want to use this function.\n"
"\n"
"  typestr\n"
"    Must be \'double\' or \'float\'.\n"
"  fmt\n"
"    Must be one of \'unknown\', \'IEEE, big-endian\' or \'IEEE, little-endian\',\n"
"    and in addition can only be one of the latter two if it appears to\n"
"    match the underlying C reality.\n"
"\n"
"It exists mainly to be used in Python\'s test suite.\n"
"\n"
"Override the automatic determination of C-level floating point type.\n"
"This affects how floats are converted to and from binary strings.");

#define FLOAT___SET_FORMAT___METHODDEF    \
    {"__set_format__", (PyCFunction)(void(*)(void))float___set_format__, METH_FASTCALL|METH_CLASS, float___set_format____doc__},

static PyObject *
float___set_format___impl(PyTypeObject *type, const char *typestr,
                          const char *fmt);

static PyObject *
float___set_format__(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *typestr;
    const char *fmt;

    if (!_PyArg_CheckPositional("__set_format__", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("__set_format__", "argument 1", "str", args[0]);
        goto exit;
    }
    Py_ssize_t typestr_length;
    typestr = PyUnicode_AsUTF8AndSize(args[0], &typestr_length);
    if (typestr == NULL) {
        goto exit;
    }
    if (strlen(typestr) != (size_t)typestr_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("__set_format__", "argument 2", "str", args[1]);
        goto exit;
    }
    Py_ssize_t fmt_length;
    fmt = PyUnicode_AsUTF8AndSize(args[1], &fmt_length);
    if (fmt == NULL) {
        goto exit;
    }
    if (strlen(fmt) != (size_t)fmt_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = float___set_format___impl(type, typestr, fmt);

exit:
    return return_value;
}

PyDoc_STRVAR(float___format____doc__,
"__format__($self, format_spec, /)\n"
"--\n"
"\n"
"Formats the float according to format_spec.");

#define FLOAT___FORMAT___METHODDEF    \
    {"__format__", (PyCFunction)float___format__, METH_O, float___format____doc__},

static PyObject *
float___format___impl(PyObject *self, PyObject *format_spec);

static PyObject *
float___format__(PyObject *self, PyObject *arg)
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
    return_value = float___format___impl(self, format_spec);

exit:
    return return_value;
}
/*[clinic end generated code: output=1676433b9f04fbc9 input=a9049054013a1b77]*/
