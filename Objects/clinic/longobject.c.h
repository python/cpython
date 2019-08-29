/*[clinic input]
preserve
[clinic start generated code]*/

static PyObject *
long_new_impl(PyTypeObject *type, PyObject *x, PyObject *obase);

static PyObject *
long_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "base", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "int", 0};
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *x = NULL;
    PyObject *obase = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 0, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional_posonly;
    }
    noptargs--;
    x = fastargs[0];
skip_optional_posonly:
    if (!noptargs) {
        goto skip_optional_pos;
    }
    obase = fastargs[1];
skip_optional_pos:
    return_value = long_new_impl(type, x, obase);

exit:
    return return_value;
}

PyDoc_STRVAR(int___getnewargs____doc__,
"__getnewargs__($self, /)\n"
"--\n"
"\n");

#define INT___GETNEWARGS___METHODDEF    \
    {"__getnewargs__", (PyCFunction)int___getnewargs__, METH_NOARGS, int___getnewargs____doc__},

static PyObject *
int___getnewargs___impl(PyObject *self);

static PyObject *
int___getnewargs__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return int___getnewargs___impl(self);
}

PyDoc_STRVAR(int___format____doc__,
"__format__($self, format_spec, /)\n"
"--\n"
"\n");

#define INT___FORMAT___METHODDEF    \
    {"__format__", (PyCFunction)int___format__, METH_O, int___format____doc__},

static PyObject *
int___format___impl(PyObject *self, PyObject *format_spec);

static PyObject *
int___format__(PyObject *self, PyObject *arg)
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
    return_value = int___format___impl(self, format_spec);

exit:
    return return_value;
}

PyDoc_STRVAR(int___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Returns size in memory, in bytes.");

#define INT___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)int___sizeof__, METH_NOARGS, int___sizeof____doc__},

static Py_ssize_t
int___sizeof___impl(PyObject *self);

static PyObject *
int___sizeof__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    Py_ssize_t _return_value;

    _return_value = int___sizeof___impl(self);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(int_bit_length__doc__,
"bit_length($self, /)\n"
"--\n"
"\n"
"Number of bits necessary to represent self in binary.\n"
"\n"
">>> bin(37)\n"
"\'0b100101\'\n"
">>> (37).bit_length()\n"
"6");

#define INT_BIT_LENGTH_METHODDEF    \
    {"bit_length", (PyCFunction)int_bit_length, METH_NOARGS, int_bit_length__doc__},

static PyObject *
int_bit_length_impl(PyObject *self);

static PyObject *
int_bit_length(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return int_bit_length_impl(self);
}

PyDoc_STRVAR(int_as_integer_ratio__doc__,
"as_integer_ratio($self, /)\n"
"--\n"
"\n"
"Return integer ratio.\n"
"\n"
"Return a pair of integers, whose ratio is exactly equal to the original int\n"
"and with a positive denominator.\n"
"\n"
">>> (10).as_integer_ratio()\n"
"(10, 1)\n"
">>> (-10).as_integer_ratio()\n"
"(-10, 1)\n"
">>> (0).as_integer_ratio()\n"
"(0, 1)");

#define INT_AS_INTEGER_RATIO_METHODDEF    \
    {"as_integer_ratio", (PyCFunction)int_as_integer_ratio, METH_NOARGS, int_as_integer_ratio__doc__},

static PyObject *
int_as_integer_ratio_impl(PyObject *self);

static PyObject *
int_as_integer_ratio(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return int_as_integer_ratio_impl(self);
}

PyDoc_STRVAR(int_to_bytes__doc__,
"to_bytes($self, /, length, byteorder, *, signed=False)\n"
"--\n"
"\n"
"Return an array of bytes representing an integer.\n"
"\n"
"  length\n"
"    Length of bytes object to use.  An OverflowError is raised if the\n"
"    integer is not representable with the given number of bytes.\n"
"  byteorder\n"
"    The byte order used to represent the integer.  If byteorder is \'big\',\n"
"    the most significant byte is at the beginning of the byte array.  If\n"
"    byteorder is \'little\', the most significant byte is at the end of the\n"
"    byte array.  To request the native byte order of the host system, use\n"
"    `sys.byteorder\' as the byte order value.\n"
"  signed\n"
"    Determines whether two\'s complement is used to represent the integer.\n"
"    If signed is False and a negative integer is given, an OverflowError\n"
"    is raised.");

#define INT_TO_BYTES_METHODDEF    \
    {"to_bytes", (PyCFunction)(void(*)(void))int_to_bytes, METH_FASTCALL|METH_KEYWORDS, int_to_bytes__doc__},

static PyObject *
int_to_bytes_impl(PyObject *self, Py_ssize_t length, PyObject *byteorder,
                  int is_signed);

static PyObject *
int_to_bytes(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"length", "byteorder", "signed", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "to_bytes", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    Py_ssize_t length;
    PyObject *byteorder;
    int is_signed = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        length = ival;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("to_bytes", "argument 'byteorder'", "str", args[1]);
        goto exit;
    }
    if (PyUnicode_READY(args[1]) == -1) {
        goto exit;
    }
    byteorder = args[1];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    is_signed = PyObject_IsTrue(args[2]);
    if (is_signed < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = int_to_bytes_impl(self, length, byteorder, is_signed);

exit:
    return return_value;
}

PyDoc_STRVAR(int_from_bytes__doc__,
"from_bytes($type, /, bytes, byteorder, *, signed=False)\n"
"--\n"
"\n"
"Return the integer represented by the given array of bytes.\n"
"\n"
"  bytes\n"
"    Holds the array of bytes to convert.  The argument must either\n"
"    support the buffer protocol or be an iterable object producing bytes.\n"
"    Bytes and bytearray are examples of built-in objects that support the\n"
"    buffer protocol.\n"
"  byteorder\n"
"    The byte order used to represent the integer.  If byteorder is \'big\',\n"
"    the most significant byte is at the beginning of the byte array.  If\n"
"    byteorder is \'little\', the most significant byte is at the end of the\n"
"    byte array.  To request the native byte order of the host system, use\n"
"    `sys.byteorder\' as the byte order value.\n"
"  signed\n"
"    Indicates whether two\'s complement is used to represent the integer.");

#define INT_FROM_BYTES_METHODDEF    \
    {"from_bytes", (PyCFunction)(void(*)(void))int_from_bytes, METH_FASTCALL|METH_KEYWORDS|METH_CLASS, int_from_bytes__doc__},

static PyObject *
int_from_bytes_impl(PyTypeObject *type, PyObject *bytes_obj,
                    PyObject *byteorder, int is_signed);

static PyObject *
int_from_bytes(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"bytes", "byteorder", "signed", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "from_bytes", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *bytes_obj;
    PyObject *byteorder;
    int is_signed = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    bytes_obj = args[0];
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("from_bytes", "argument 'byteorder'", "str", args[1]);
        goto exit;
    }
    if (PyUnicode_READY(args[1]) == -1) {
        goto exit;
    }
    byteorder = args[1];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    is_signed = PyObject_IsTrue(args[2]);
    if (is_signed < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = int_from_bytes_impl(type, bytes_obj, byteorder, is_signed);

exit:
    return return_value;
}
/*[clinic end generated code: output=77bc3b2615822cb8 input=a9049054013a1b77]*/
