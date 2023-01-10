/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(test_empty_function__doc__,
"test_empty_function($module, /)\n"
"--\n"
"\n");

#define TEST_EMPTY_FUNCTION_METHODDEF    \
    {"test_empty_function", (PyCFunction)test_empty_function, METH_NOARGS, test_empty_function__doc__},

static PyObject *
test_empty_function_impl(PyObject *module);

static PyObject *
test_empty_function(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return test_empty_function_impl(module);
}

PyDoc_STRVAR(objects_converter__doc__,
"objects_converter($module, a, b=<unrepresentable>, /)\n"
"--\n"
"\n");

#define OBJECTS_CONVERTER_METHODDEF    \
    {"objects_converter", _PyCFunction_CAST(objects_converter), METH_FASTCALL, objects_converter__doc__},

static PyObject *
objects_converter_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
objects_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b = NULL;

    if (!_PyArg_CheckPositional("objects_converter", nargs, 1, 2)) {
        goto exit;
    }
    a = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    b = args[1];
skip_optional:
    return_value = objects_converter_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_object_converter__doc__,
"bytes_object_converter($module, a, /)\n"
"--\n"
"\n");

#define BYTES_OBJECT_CONVERTER_METHODDEF    \
    {"bytes_object_converter", (PyCFunction)bytes_object_converter, METH_O, bytes_object_converter__doc__},

static PyObject *
bytes_object_converter_impl(PyObject *module, PyBytesObject *a);

static PyObject *
bytes_object_converter(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyBytesObject *a;

    if (!PyBytes_Check(arg)) {
        _PyArg_BadArgument("bytes_object_converter", "argument", "bytes", arg);
        goto exit;
    }
    a = (PyBytesObject *)arg;
    return_value = bytes_object_converter_impl(module, a);

exit:
    return return_value;
}

PyDoc_STRVAR(byte_array_object_converter__doc__,
"byte_array_object_converter($module, a, /)\n"
"--\n"
"\n");

#define BYTE_ARRAY_OBJECT_CONVERTER_METHODDEF    \
    {"byte_array_object_converter", (PyCFunction)byte_array_object_converter, METH_O, byte_array_object_converter__doc__},

static PyObject *
byte_array_object_converter_impl(PyObject *module, PyByteArrayObject *a);

static PyObject *
byte_array_object_converter(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyByteArrayObject *a;

    if (!PyByteArray_Check(arg)) {
        _PyArg_BadArgument("byte_array_object_converter", "argument", "bytearray", arg);
        goto exit;
    }
    a = (PyByteArrayObject *)arg;
    return_value = byte_array_object_converter_impl(module, a);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_converter__doc__,
"unicode_converter($module, a, /)\n"
"--\n"
"\n");

#define UNICODE_CONVERTER_METHODDEF    \
    {"unicode_converter", (PyCFunction)unicode_converter, METH_O, unicode_converter__doc__},

static PyObject *
unicode_converter_impl(PyObject *module, PyObject *a);

static PyObject *
unicode_converter(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *a;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("unicode_converter", "argument", "str", arg);
        goto exit;
    }
    if (PyUnicode_READY(arg) == -1) {
        goto exit;
    }
    a = arg;
    return_value = unicode_converter_impl(module, a);

exit:
    return return_value;
}

PyDoc_STRVAR(bool_converter__doc__,
"bool_converter($module, a=True, b=True, c=True, /)\n"
"--\n"
"\n");

#define BOOL_CONVERTER_METHODDEF    \
    {"bool_converter", _PyCFunction_CAST(bool_converter), METH_FASTCALL, bool_converter__doc__},

static PyObject *
bool_converter_impl(PyObject *module, int a, int b, int c);

static PyObject *
bool_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int a = 1;
    int b = 1;
    int c = 1;

    if (!_PyArg_CheckPositional("bool_converter", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    a = PyObject_IsTrue(args[0]);
    if (a < 0) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    b = PyObject_IsTrue(args[1]);
    if (b < 0) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    c = _PyLong_AsInt(args[2]);
    if (c == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = bool_converter_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(char_converter__doc__,
"char_converter($module, a=b\'A\', b=b\'\\x07\', c=b\'\\x08\', d=b\'\\t\', e=b\'\\n\',\n"
"               f=b\'\\x0b\', g=b\'\\x0c\', h=b\'\\r\', i=b\'\"\', j=b\"\'\", k=b\'?\',\n"
"               l=b\'\\\\\', m=b\'\\x00\', n=b\'\\xff\', /)\n"
"--\n"
"\n");

#define CHAR_CONVERTER_METHODDEF    \
    {"char_converter", _PyCFunction_CAST(char_converter), METH_FASTCALL, char_converter__doc__},

static PyObject *
char_converter_impl(PyObject *module, char a, char b, char c, char d, char e,
                    char f, char g, char h, char i, char j, char k, char l,
                    char m, char n);

static PyObject *
char_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    char a = 'A';
    char b = '\x07';
    char c = '\x08';
    char d = '\t';
    char e = '\n';
    char f = '\x0b';
    char g = '\x0c';
    char h = '\r';
    char i = '"';
    char j = '\'';
    char k = '?';
    char l = '\\';
    char m = '\x00';
    char n = '\xff';

    if (!_PyArg_CheckPositional("char_converter", nargs, 0, 14)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (PyBytes_Check(args[0]) && PyBytes_GET_SIZE(args[0]) == 1) {
        a = PyBytes_AS_STRING(args[0])[0];
    }
    else if (PyByteArray_Check(args[0]) && PyByteArray_GET_SIZE(args[0]) == 1) {
        a = PyByteArray_AS_STRING(args[0])[0];
    }
    else {
        _PyArg_BadArgument("char_converter", "argument 1", "a byte string of length 1", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (PyBytes_Check(args[1]) && PyBytes_GET_SIZE(args[1]) == 1) {
        b = PyBytes_AS_STRING(args[1])[0];
    }
    else if (PyByteArray_Check(args[1]) && PyByteArray_GET_SIZE(args[1]) == 1) {
        b = PyByteArray_AS_STRING(args[1])[0];
    }
    else {
        _PyArg_BadArgument("char_converter", "argument 2", "a byte string of length 1", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (PyBytes_Check(args[2]) && PyBytes_GET_SIZE(args[2]) == 1) {
        c = PyBytes_AS_STRING(args[2])[0];
    }
    else if (PyByteArray_Check(args[2]) && PyByteArray_GET_SIZE(args[2]) == 1) {
        c = PyByteArray_AS_STRING(args[2])[0];
    }
    else {
        _PyArg_BadArgument("char_converter", "argument 3", "a byte string of length 1", args[2]);
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    if (PyBytes_Check(args[3]) && PyBytes_GET_SIZE(args[3]) == 1) {
        d = PyBytes_AS_STRING(args[3])[0];
    }
    else if (PyByteArray_Check(args[3]) && PyByteArray_GET_SIZE(args[3]) == 1) {
        d = PyByteArray_AS_STRING(args[3])[0];
    }
    else {
        _PyArg_BadArgument("char_converter", "argument 4", "a byte string of length 1", args[3]);
        goto exit;
    }
    if (nargs < 5) {
        goto skip_optional;
    }
    if (PyBytes_Check(args[4]) && PyBytes_GET_SIZE(args[4]) == 1) {
        e = PyBytes_AS_STRING(args[4])[0];
    }
    else if (PyByteArray_Check(args[4]) && PyByteArray_GET_SIZE(args[4]) == 1) {
        e = PyByteArray_AS_STRING(args[4])[0];
    }
    else {
        _PyArg_BadArgument("char_converter", "argument 5", "a byte string of length 1", args[4]);
        goto exit;
    }
    if (nargs < 6) {
        goto skip_optional;
    }
    if (PyBytes_Check(args[5]) && PyBytes_GET_SIZE(args[5]) == 1) {
        f = PyBytes_AS_STRING(args[5])[0];
    }
    else if (PyByteArray_Check(args[5]) && PyByteArray_GET_SIZE(args[5]) == 1) {
        f = PyByteArray_AS_STRING(args[5])[0];
    }
    else {
        _PyArg_BadArgument("char_converter", "argument 6", "a byte string of length 1", args[5]);
        goto exit;
    }
    if (nargs < 7) {
        goto skip_optional;
    }
    if (PyBytes_Check(args[6]) && PyBytes_GET_SIZE(args[6]) == 1) {
        g = PyBytes_AS_STRING(args[6])[0];
    }
    else if (PyByteArray_Check(args[6]) && PyByteArray_GET_SIZE(args[6]) == 1) {
        g = PyByteArray_AS_STRING(args[6])[0];
    }
    else {
        _PyArg_BadArgument("char_converter", "argument 7", "a byte string of length 1", args[6]);
        goto exit;
    }
    if (nargs < 8) {
        goto skip_optional;
    }
    if (PyBytes_Check(args[7]) && PyBytes_GET_SIZE(args[7]) == 1) {
        h = PyBytes_AS_STRING(args[7])[0];
    }
    else if (PyByteArray_Check(args[7]) && PyByteArray_GET_SIZE(args[7]) == 1) {
        h = PyByteArray_AS_STRING(args[7])[0];
    }
    else {
        _PyArg_BadArgument("char_converter", "argument 8", "a byte string of length 1", args[7]);
        goto exit;
    }
    if (nargs < 9) {
        goto skip_optional;
    }
    if (PyBytes_Check(args[8]) && PyBytes_GET_SIZE(args[8]) == 1) {
        i = PyBytes_AS_STRING(args[8])[0];
    }
    else if (PyByteArray_Check(args[8]) && PyByteArray_GET_SIZE(args[8]) == 1) {
        i = PyByteArray_AS_STRING(args[8])[0];
    }
    else {
        _PyArg_BadArgument("char_converter", "argument 9", "a byte string of length 1", args[8]);
        goto exit;
    }
    if (nargs < 10) {
        goto skip_optional;
    }
    if (PyBytes_Check(args[9]) && PyBytes_GET_SIZE(args[9]) == 1) {
        j = PyBytes_AS_STRING(args[9])[0];
    }
    else if (PyByteArray_Check(args[9]) && PyByteArray_GET_SIZE(args[9]) == 1) {
        j = PyByteArray_AS_STRING(args[9])[0];
    }
    else {
        _PyArg_BadArgument("char_converter", "argument 10", "a byte string of length 1", args[9]);
        goto exit;
    }
    if (nargs < 11) {
        goto skip_optional;
    }
    if (PyBytes_Check(args[10]) && PyBytes_GET_SIZE(args[10]) == 1) {
        k = PyBytes_AS_STRING(args[10])[0];
    }
    else if (PyByteArray_Check(args[10]) && PyByteArray_GET_SIZE(args[10]) == 1) {
        k = PyByteArray_AS_STRING(args[10])[0];
    }
    else {
        _PyArg_BadArgument("char_converter", "argument 11", "a byte string of length 1", args[10]);
        goto exit;
    }
    if (nargs < 12) {
        goto skip_optional;
    }
    if (PyBytes_Check(args[11]) && PyBytes_GET_SIZE(args[11]) == 1) {
        l = PyBytes_AS_STRING(args[11])[0];
    }
    else if (PyByteArray_Check(args[11]) && PyByteArray_GET_SIZE(args[11]) == 1) {
        l = PyByteArray_AS_STRING(args[11])[0];
    }
    else {
        _PyArg_BadArgument("char_converter", "argument 12", "a byte string of length 1", args[11]);
        goto exit;
    }
    if (nargs < 13) {
        goto skip_optional;
    }
    if (PyBytes_Check(args[12]) && PyBytes_GET_SIZE(args[12]) == 1) {
        m = PyBytes_AS_STRING(args[12])[0];
    }
    else if (PyByteArray_Check(args[12]) && PyByteArray_GET_SIZE(args[12]) == 1) {
        m = PyByteArray_AS_STRING(args[12])[0];
    }
    else {
        _PyArg_BadArgument("char_converter", "argument 13", "a byte string of length 1", args[12]);
        goto exit;
    }
    if (nargs < 14) {
        goto skip_optional;
    }
    if (PyBytes_Check(args[13]) && PyBytes_GET_SIZE(args[13]) == 1) {
        n = PyBytes_AS_STRING(args[13])[0];
    }
    else if (PyByteArray_Check(args[13]) && PyByteArray_GET_SIZE(args[13]) == 1) {
        n = PyByteArray_AS_STRING(args[13])[0];
    }
    else {
        _PyArg_BadArgument("char_converter", "argument 14", "a byte string of length 1", args[13]);
        goto exit;
    }
skip_optional:
    return_value = char_converter_impl(module, a, b, c, d, e, f, g, h, i, j, k, l, m, n);

exit:
    return return_value;
}

PyDoc_STRVAR(unsigned_char_converter__doc__,
"unsigned_char_converter($module, a=12, b=34, c=56, /)\n"
"--\n"
"\n");

#define UNSIGNED_CHAR_CONVERTER_METHODDEF    \
    {"unsigned_char_converter", _PyCFunction_CAST(unsigned_char_converter), METH_FASTCALL, unsigned_char_converter__doc__},

static PyObject *
unsigned_char_converter_impl(PyObject *module, unsigned char a,
                             unsigned char b, unsigned char c);

static PyObject *
unsigned_char_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    unsigned char a = 12;
    unsigned char b = 34;
    unsigned char c = 56;

    if (!_PyArg_CheckPositional("unsigned_char_converter", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        long ival = PyLong_AsLong(args[0]);
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        else if (ival < 0) {
            PyErr_SetString(PyExc_OverflowError,
                            "unsigned byte integer is less than minimum");
            goto exit;
        }
        else if (ival > UCHAR_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "unsigned byte integer is greater than maximum");
            goto exit;
        }
        else {
            a = (unsigned char) ival;
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        long ival = PyLong_AsLong(args[1]);
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        else if (ival < 0) {
            PyErr_SetString(PyExc_OverflowError,
                            "unsigned byte integer is less than minimum");
            goto exit;
        }
        else if (ival > UCHAR_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "unsigned byte integer is greater than maximum");
            goto exit;
        }
        else {
            b = (unsigned char) ival;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        unsigned long ival = PyLong_AsUnsignedLongMask(args[2]);
        if (ival == (unsigned long)-1 && PyErr_Occurred()) {
            goto exit;
        }
        else {
            c = (unsigned char) ival;
        }
    }
skip_optional:
    return_value = unsigned_char_converter_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(short_converter__doc__,
"short_converter($module, a=12, /)\n"
"--\n"
"\n");

#define SHORT_CONVERTER_METHODDEF    \
    {"short_converter", _PyCFunction_CAST(short_converter), METH_FASTCALL, short_converter__doc__},

static PyObject *
short_converter_impl(PyObject *module, short a);

static PyObject *
short_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    short a = 12;

    if (!_PyArg_CheckPositional("short_converter", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        long ival = PyLong_AsLong(args[0]);
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        else if (ival < SHRT_MIN) {
            PyErr_SetString(PyExc_OverflowError,
                            "signed short integer is less than minimum");
            goto exit;
        }
        else if (ival > SHRT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "signed short integer is greater than maximum");
            goto exit;
        }
        else {
            a = (short) ival;
        }
    }
skip_optional:
    return_value = short_converter_impl(module, a);

exit:
    return return_value;
}

PyDoc_STRVAR(unsigned_short_converter__doc__,
"unsigned_short_converter($module, a=12, b=34, c=56, /)\n"
"--\n"
"\n");

#define UNSIGNED_SHORT_CONVERTER_METHODDEF    \
    {"unsigned_short_converter", _PyCFunction_CAST(unsigned_short_converter), METH_FASTCALL, unsigned_short_converter__doc__},

static PyObject *
unsigned_short_converter_impl(PyObject *module, unsigned short a,
                              unsigned short b, unsigned short c);

static PyObject *
unsigned_short_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    unsigned short a = 12;
    unsigned short b = 34;
    unsigned short c = 56;

    if (!_PyArg_CheckPositional("unsigned_short_converter", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_PyLong_UnsignedShort_Converter(args[0], &a)) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyLong_UnsignedShort_Converter(args[1], &b)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    c = (unsigned short)PyLong_AsUnsignedLongMask(args[2]);
    if (c == (unsigned short)-1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = unsigned_short_converter_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(int_converter__doc__,
"int_converter($module, a=12, b=34, c=45, /)\n"
"--\n"
"\n");

#define INT_CONVERTER_METHODDEF    \
    {"int_converter", _PyCFunction_CAST(int_converter), METH_FASTCALL, int_converter__doc__},

static PyObject *
int_converter_impl(PyObject *module, int a, int b, int c);

static PyObject *
int_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int a = 12;
    int b = 34;
    int c = 45;

    if (!_PyArg_CheckPositional("int_converter", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    a = _PyLong_AsInt(args[0]);
    if (a == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    b = _PyLong_AsInt(args[1]);
    if (b == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!PyUnicode_Check(args[2])) {
        _PyArg_BadArgument("int_converter", "argument 3", "a unicode character", args[2]);
        goto exit;
    }
    if (PyUnicode_READY(args[2])) {
        goto exit;
    }
    if (PyUnicode_GET_LENGTH(args[2]) != 1) {
        _PyArg_BadArgument("int_converter", "argument 3", "a unicode character", args[2]);
        goto exit;
    }
    c = PyUnicode_READ_CHAR(args[2], 0);
skip_optional:
    return_value = int_converter_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(unsigned_int_converter__doc__,
"unsigned_int_converter($module, a=12, b=34, c=56, /)\n"
"--\n"
"\n");

#define UNSIGNED_INT_CONVERTER_METHODDEF    \
    {"unsigned_int_converter", _PyCFunction_CAST(unsigned_int_converter), METH_FASTCALL, unsigned_int_converter__doc__},

static PyObject *
unsigned_int_converter_impl(PyObject *module, unsigned int a, unsigned int b,
                            unsigned int c);

static PyObject *
unsigned_int_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    unsigned int a = 12;
    unsigned int b = 34;
    unsigned int c = 56;

    if (!_PyArg_CheckPositional("unsigned_int_converter", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_PyLong_UnsignedInt_Converter(args[0], &a)) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyLong_UnsignedInt_Converter(args[1], &b)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    c = (unsigned int)PyLong_AsUnsignedLongMask(args[2]);
    if (c == (unsigned int)-1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = unsigned_int_converter_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(long_converter__doc__,
"long_converter($module, a=12, /)\n"
"--\n"
"\n");

#define LONG_CONVERTER_METHODDEF    \
    {"long_converter", _PyCFunction_CAST(long_converter), METH_FASTCALL, long_converter__doc__},

static PyObject *
long_converter_impl(PyObject *module, long a);

static PyObject *
long_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    long a = 12;

    if (!_PyArg_CheckPositional("long_converter", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    a = PyLong_AsLong(args[0]);
    if (a == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = long_converter_impl(module, a);

exit:
    return return_value;
}

PyDoc_STRVAR(unsigned_long_converter__doc__,
"unsigned_long_converter($module, a=12, b=34, c=56, /)\n"
"--\n"
"\n");

#define UNSIGNED_LONG_CONVERTER_METHODDEF    \
    {"unsigned_long_converter", _PyCFunction_CAST(unsigned_long_converter), METH_FASTCALL, unsigned_long_converter__doc__},

static PyObject *
unsigned_long_converter_impl(PyObject *module, unsigned long a,
                             unsigned long b, unsigned long c);

static PyObject *
unsigned_long_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    unsigned long a = 12;
    unsigned long b = 34;
    unsigned long c = 56;

    if (!_PyArg_CheckPositional("unsigned_long_converter", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_PyLong_UnsignedLong_Converter(args[0], &a)) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyLong_UnsignedLong_Converter(args[1], &b)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!PyLong_Check(args[2])) {
        _PyArg_BadArgument("unsigned_long_converter", "argument 3", "int", args[2]);
        goto exit;
    }
    c = PyLong_AsUnsignedLongMask(args[2]);
skip_optional:
    return_value = unsigned_long_converter_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(long_long_converter__doc__,
"long_long_converter($module, a=12, /)\n"
"--\n"
"\n");

#define LONG_LONG_CONVERTER_METHODDEF    \
    {"long_long_converter", _PyCFunction_CAST(long_long_converter), METH_FASTCALL, long_long_converter__doc__},

static PyObject *
long_long_converter_impl(PyObject *module, long long a);

static PyObject *
long_long_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    long long a = 12;

    if (!_PyArg_CheckPositional("long_long_converter", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    a = PyLong_AsLongLong(args[0]);
    if (a == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = long_long_converter_impl(module, a);

exit:
    return return_value;
}

PyDoc_STRVAR(unsigned_long_long_converter__doc__,
"unsigned_long_long_converter($module, a=12, b=34, c=56, /)\n"
"--\n"
"\n");

#define UNSIGNED_LONG_LONG_CONVERTER_METHODDEF    \
    {"unsigned_long_long_converter", _PyCFunction_CAST(unsigned_long_long_converter), METH_FASTCALL, unsigned_long_long_converter__doc__},

static PyObject *
unsigned_long_long_converter_impl(PyObject *module, unsigned long long a,
                                  unsigned long long b, unsigned long long c);

static PyObject *
unsigned_long_long_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    unsigned long long a = 12;
    unsigned long long b = 34;
    unsigned long long c = 56;

    if (!_PyArg_CheckPositional("unsigned_long_long_converter", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_PyLong_UnsignedLongLong_Converter(args[0], &a)) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyLong_UnsignedLongLong_Converter(args[1], &b)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!PyLong_Check(args[2])) {
        _PyArg_BadArgument("unsigned_long_long_converter", "argument 3", "int", args[2]);
        goto exit;
    }
    c = PyLong_AsUnsignedLongLongMask(args[2]);
skip_optional:
    return_value = unsigned_long_long_converter_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(py_ssize_t_converter__doc__,
"py_ssize_t_converter($module, a=12, b=34, c=56, /)\n"
"--\n"
"\n");

#define PY_SSIZE_T_CONVERTER_METHODDEF    \
    {"py_ssize_t_converter", _PyCFunction_CAST(py_ssize_t_converter), METH_FASTCALL, py_ssize_t_converter__doc__},

static PyObject *
py_ssize_t_converter_impl(PyObject *module, Py_ssize_t a, Py_ssize_t b,
                          Py_ssize_t c);

static PyObject *
py_ssize_t_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t a = 12;
    Py_ssize_t b = 34;
    Py_ssize_t c = 56;

    if (!_PyArg_CheckPositional("py_ssize_t_converter", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        a = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        b = ival;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_Py_convert_optional_to_ssize_t(args[2], &c)) {
        goto exit;
    }
skip_optional:
    return_value = py_ssize_t_converter_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(slice_index_converter__doc__,
"slice_index_converter($module, a=12, b=34, c=56, /)\n"
"--\n"
"\n");

#define SLICE_INDEX_CONVERTER_METHODDEF    \
    {"slice_index_converter", _PyCFunction_CAST(slice_index_converter), METH_FASTCALL, slice_index_converter__doc__},

static PyObject *
slice_index_converter_impl(PyObject *module, Py_ssize_t a, Py_ssize_t b,
                           Py_ssize_t c);

static PyObject *
slice_index_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t a = 12;
    Py_ssize_t b = 34;
    Py_ssize_t c = 56;

    if (!_PyArg_CheckPositional("slice_index_converter", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndex(args[0], &a)) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndexNotNone(args[1], &b)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndex(args[2], &c)) {
        goto exit;
    }
skip_optional:
    return_value = slice_index_converter_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(size_t_converter__doc__,
"size_t_converter($module, a=12, /)\n"
"--\n"
"\n");

#define SIZE_T_CONVERTER_METHODDEF    \
    {"size_t_converter", _PyCFunction_CAST(size_t_converter), METH_FASTCALL, size_t_converter__doc__},

static PyObject *
size_t_converter_impl(PyObject *module, size_t a);

static PyObject *
size_t_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    size_t a = 12;

    if (!_PyArg_CheckPositional("size_t_converter", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_PyLong_Size_t_Converter(args[0], &a)) {
        goto exit;
    }
skip_optional:
    return_value = size_t_converter_impl(module, a);

exit:
    return return_value;
}

PyDoc_STRVAR(float_converter__doc__,
"float_converter($module, a=12.5, /)\n"
"--\n"
"\n");

#define FLOAT_CONVERTER_METHODDEF    \
    {"float_converter", _PyCFunction_CAST(float_converter), METH_FASTCALL, float_converter__doc__},

static PyObject *
float_converter_impl(PyObject *module, float a);

static PyObject *
float_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    float a = 12.5;

    if (!_PyArg_CheckPositional("float_converter", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (PyFloat_CheckExact(args[0])) {
        a = (float) (PyFloat_AS_DOUBLE(args[0]));
    }
    else
    {
        a = (float) PyFloat_AsDouble(args[0]);
        if (a == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
skip_optional:
    return_value = float_converter_impl(module, a);

exit:
    return return_value;
}

PyDoc_STRVAR(double_converter__doc__,
"double_converter($module, a=12.5, /)\n"
"--\n"
"\n");

#define DOUBLE_CONVERTER_METHODDEF    \
    {"double_converter", _PyCFunction_CAST(double_converter), METH_FASTCALL, double_converter__doc__},

static PyObject *
double_converter_impl(PyObject *module, double a);

static PyObject *
double_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    double a = 12.5;

    if (!_PyArg_CheckPositional("double_converter", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (PyFloat_CheckExact(args[0])) {
        a = PyFloat_AS_DOUBLE(args[0]);
    }
    else
    {
        a = PyFloat_AsDouble(args[0]);
        if (a == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
skip_optional:
    return_value = double_converter_impl(module, a);

exit:
    return return_value;
}

PyDoc_STRVAR(py_complex_converter__doc__,
"py_complex_converter($module, a, /)\n"
"--\n"
"\n");

#define PY_COMPLEX_CONVERTER_METHODDEF    \
    {"py_complex_converter", (PyCFunction)py_complex_converter, METH_O, py_complex_converter__doc__},

static PyObject *
py_complex_converter_impl(PyObject *module, Py_complex a);

static PyObject *
py_complex_converter(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex a;

    a = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    return_value = py_complex_converter_impl(module, a);

exit:
    return return_value;
}

PyDoc_STRVAR(str_converter__doc__,
"str_converter($module, a=\'a\', b=\'b\', c=\'c\', /)\n"
"--\n"
"\n");

#define STR_CONVERTER_METHODDEF    \
    {"str_converter", _PyCFunction_CAST(str_converter), METH_FASTCALL, str_converter__doc__},

static PyObject *
str_converter_impl(PyObject *module, const char *a, const char *b,
                   const char *c, Py_ssize_t c_length);

static PyObject *
str_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *a = "a";
    const char *b = "b";
    const char *c = "c";
    Py_ssize_t c_length;

    if (!_PyArg_ParseStack(args, nargs, "|sys#:str_converter",
        &a, &b, &c, &c_length)) {
        goto exit;
    }
    return_value = str_converter_impl(module, a, b, c, c_length);

exit:
    return return_value;
}

PyDoc_STRVAR(str_converter_encoding__doc__,
"str_converter_encoding($module, a, b, c, /)\n"
"--\n"
"\n");

#define STR_CONVERTER_ENCODING_METHODDEF    \
    {"str_converter_encoding", _PyCFunction_CAST(str_converter_encoding), METH_FASTCALL, str_converter_encoding__doc__},

static PyObject *
str_converter_encoding_impl(PyObject *module, char *a, char *b, char *c,
                            Py_ssize_t c_length);

static PyObject *
str_converter_encoding(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    char *a = NULL;
    char *b = NULL;
    char *c = NULL;
    Py_ssize_t c_length;

    if (!_PyArg_ParseStack(args, nargs, "esetet#:str_converter_encoding",
        "idna", &a, "idna", &b, "idna", &c, &c_length)) {
        goto exit;
    }
    return_value = str_converter_encoding_impl(module, a, b, c, c_length);
    /* Post parse cleanup for a */
    PyMem_FREE(a);
    /* Post parse cleanup for b */
    PyMem_FREE(b);
    /* Post parse cleanup for c */
    PyMem_FREE(c);

exit:
    return return_value;
}

PyDoc_STRVAR(py_buffer_converter__doc__,
"py_buffer_converter($module, a, b, /)\n"
"--\n"
"\n");

#define PY_BUFFER_CONVERTER_METHODDEF    \
    {"py_buffer_converter", _PyCFunction_CAST(py_buffer_converter), METH_FASTCALL, py_buffer_converter__doc__},

static PyObject *
py_buffer_converter_impl(PyObject *module, Py_buffer *a, Py_buffer *b);

static PyObject *
py_buffer_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer a = {NULL, NULL};
    Py_buffer b = {NULL, NULL};

    if (!_PyArg_ParseStack(args, nargs, "z*w*:py_buffer_converter",
        &a, &b)) {
        goto exit;
    }
    return_value = py_buffer_converter_impl(module, &a, &b);

exit:
    /* Cleanup for a */
    if (a.obj) {
       PyBuffer_Release(&a);
    }
    /* Cleanup for b */
    if (b.obj) {
       PyBuffer_Release(&b);
    }

    return return_value;
}

PyDoc_STRVAR(keywords__doc__,
"keywords($module, /, a, b)\n"
"--\n"
"\n");

#define KEYWORDS_METHODDEF    \
    {"keywords", _PyCFunction_CAST(keywords), METH_FASTCALL|METH_KEYWORDS, keywords__doc__},

static PyObject *
keywords_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
keywords(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"a", "b", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "keywords", 0};
    PyObject *argsbuf[2];
    PyObject *a;
    PyObject *b;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = keywords_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(keywords_kwonly__doc__,
"keywords_kwonly($module, /, a, *, b)\n"
"--\n"
"\n");

#define KEYWORDS_KWONLY_METHODDEF    \
    {"keywords_kwonly", _PyCFunction_CAST(keywords_kwonly), METH_FASTCALL|METH_KEYWORDS, keywords_kwonly__doc__},

static PyObject *
keywords_kwonly_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
keywords_kwonly(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"a", "b", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "keywords_kwonly", 0};
    PyObject *argsbuf[2];
    PyObject *a;
    PyObject *b;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 1, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = keywords_kwonly_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(keywords_opt__doc__,
"keywords_opt($module, /, a, b=None, c=None)\n"
"--\n"
"\n");

#define KEYWORDS_OPT_METHODDEF    \
    {"keywords_opt", _PyCFunction_CAST(keywords_opt), METH_FASTCALL|METH_KEYWORDS, keywords_opt__doc__},

static PyObject *
keywords_opt_impl(PyObject *module, PyObject *a, PyObject *b, PyObject *c);

static PyObject *
keywords_opt(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"a", "b", "c", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "keywords_opt", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *b = Py_None;
    PyObject *c = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        b = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    c = args[2];
skip_optional_pos:
    return_value = keywords_opt_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(keywords_opt_kwonly__doc__,
"keywords_opt_kwonly($module, /, a, b=None, *, c=None, d=None)\n"
"--\n"
"\n");

#define KEYWORDS_OPT_KWONLY_METHODDEF    \
    {"keywords_opt_kwonly", _PyCFunction_CAST(keywords_opt_kwonly), METH_FASTCALL|METH_KEYWORDS, keywords_opt_kwonly__doc__},

static PyObject *
keywords_opt_kwonly_impl(PyObject *module, PyObject *a, PyObject *b,
                         PyObject *c, PyObject *d);

static PyObject *
keywords_opt_kwonly(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"a", "b", "c", "d", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "keywords_opt_kwonly", 0};
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *b = Py_None;
    PyObject *c = Py_None;
    PyObject *d = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        b = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[2]) {
        c = args[2];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    d = args[3];
skip_optional_kwonly:
    return_value = keywords_opt_kwonly_impl(module, a, b, c, d);

exit:
    return return_value;
}

PyDoc_STRVAR(keywords_kwonly_opt__doc__,
"keywords_kwonly_opt($module, /, a, *, b=None, c=None)\n"
"--\n"
"\n");

#define KEYWORDS_KWONLY_OPT_METHODDEF    \
    {"keywords_kwonly_opt", _PyCFunction_CAST(keywords_kwonly_opt), METH_FASTCALL|METH_KEYWORDS, keywords_kwonly_opt__doc__},

static PyObject *
keywords_kwonly_opt_impl(PyObject *module, PyObject *a, PyObject *b,
                         PyObject *c);

static PyObject *
keywords_kwonly_opt(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"a", "b", "c", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "keywords_kwonly_opt", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *b = Py_None;
    PyObject *c = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[1]) {
        b = args[1];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    c = args[2];
skip_optional_kwonly:
    return_value = keywords_kwonly_opt_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_keywords__doc__,
"posonly_keywords($module, a, /, b)\n"
"--\n"
"\n");

#define POSONLY_KEYWORDS_METHODDEF    \
    {"posonly_keywords", _PyCFunction_CAST(posonly_keywords), METH_FASTCALL|METH_KEYWORDS, posonly_keywords__doc__},

static PyObject *
posonly_keywords_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
posonly_keywords(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "b", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "posonly_keywords", 0};
    PyObject *argsbuf[2];
    PyObject *a;
    PyObject *b;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = posonly_keywords_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_kwonly__doc__,
"posonly_kwonly($module, a, /, *, b)\n"
"--\n"
"\n");

#define POSONLY_KWONLY_METHODDEF    \
    {"posonly_kwonly", _PyCFunction_CAST(posonly_kwonly), METH_FASTCALL|METH_KEYWORDS, posonly_kwonly__doc__},

static PyObject *
posonly_kwonly_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
posonly_kwonly(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "b", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "posonly_kwonly", 0};
    PyObject *argsbuf[2];
    PyObject *a;
    PyObject *b;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 1, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = posonly_kwonly_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_keywords_kwonly__doc__,
"posonly_keywords_kwonly($module, a, /, b, *, c)\n"
"--\n"
"\n");

#define POSONLY_KEYWORDS_KWONLY_METHODDEF    \
    {"posonly_keywords_kwonly", _PyCFunction_CAST(posonly_keywords_kwonly), METH_FASTCALL|METH_KEYWORDS, posonly_keywords_kwonly__doc__},

static PyObject *
posonly_keywords_kwonly_impl(PyObject *module, PyObject *a, PyObject *b,
                             PyObject *c);

static PyObject *
posonly_keywords_kwonly(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "b", "c", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "posonly_keywords_kwonly", 0};
    PyObject *argsbuf[3];
    PyObject *a;
    PyObject *b;
    PyObject *c;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 1, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    c = args[2];
    return_value = posonly_keywords_kwonly_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_keywords_opt__doc__,
"posonly_keywords_opt($module, a, /, b, c=None, d=None)\n"
"--\n"
"\n");

#define POSONLY_KEYWORDS_OPT_METHODDEF    \
    {"posonly_keywords_opt", _PyCFunction_CAST(posonly_keywords_opt), METH_FASTCALL|METH_KEYWORDS, posonly_keywords_opt__doc__},

static PyObject *
posonly_keywords_opt_impl(PyObject *module, PyObject *a, PyObject *b,
                          PyObject *c, PyObject *d);

static PyObject *
posonly_keywords_opt(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "b", "c", "d", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "posonly_keywords_opt", 0};
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *a;
    PyObject *b;
    PyObject *c = Py_None;
    PyObject *d = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 4, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        c = args[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    d = args[3];
skip_optional_pos:
    return_value = posonly_keywords_opt_impl(module, a, b, c, d);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_opt_keywords_opt__doc__,
"posonly_opt_keywords_opt($module, a, b=None, /, c=None, d=None)\n"
"--\n"
"\n");

#define POSONLY_OPT_KEYWORDS_OPT_METHODDEF    \
    {"posonly_opt_keywords_opt", _PyCFunction_CAST(posonly_opt_keywords_opt), METH_FASTCALL|METH_KEYWORDS, posonly_opt_keywords_opt__doc__},

static PyObject *
posonly_opt_keywords_opt_impl(PyObject *module, PyObject *a, PyObject *b,
                              PyObject *c, PyObject *d);

static PyObject *
posonly_opt_keywords_opt(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "", "c", "d", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "posonly_opt_keywords_opt", 0};
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *b = Py_None;
    PyObject *c = Py_None;
    PyObject *d = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 4, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    if (nargs < 2) {
        goto skip_optional_posonly;
    }
    noptargs--;
    b = args[1];
skip_optional_posonly:
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        c = args[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    d = args[3];
skip_optional_pos:
    return_value = posonly_opt_keywords_opt_impl(module, a, b, c, d);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_kwonly_opt__doc__,
"posonly_kwonly_opt($module, a, /, *, b, c=None, d=None)\n"
"--\n"
"\n");

#define POSONLY_KWONLY_OPT_METHODDEF    \
    {"posonly_kwonly_opt", _PyCFunction_CAST(posonly_kwonly_opt), METH_FASTCALL|METH_KEYWORDS, posonly_kwonly_opt__doc__},

static PyObject *
posonly_kwonly_opt_impl(PyObject *module, PyObject *a, PyObject *b,
                        PyObject *c, PyObject *d);

static PyObject *
posonly_kwonly_opt(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "b", "c", "d", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "posonly_kwonly_opt", 0};
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *a;
    PyObject *b;
    PyObject *c = Py_None;
    PyObject *d = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 1, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[2]) {
        c = args[2];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    d = args[3];
skip_optional_kwonly:
    return_value = posonly_kwonly_opt_impl(module, a, b, c, d);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_opt_kwonly_opt__doc__,
"posonly_opt_kwonly_opt($module, a, b=None, /, *, c=None, d=None)\n"
"--\n"
"\n");

#define POSONLY_OPT_KWONLY_OPT_METHODDEF    \
    {"posonly_opt_kwonly_opt", _PyCFunction_CAST(posonly_opt_kwonly_opt), METH_FASTCALL|METH_KEYWORDS, posonly_opt_kwonly_opt__doc__},

static PyObject *
posonly_opt_kwonly_opt_impl(PyObject *module, PyObject *a, PyObject *b,
                            PyObject *c, PyObject *d);

static PyObject *
posonly_opt_kwonly_opt(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "", "c", "d", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "posonly_opt_kwonly_opt", 0};
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *b = Py_None;
    PyObject *c = Py_None;
    PyObject *d = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    if (nargs < 2) {
        goto skip_optional_posonly;
    }
    noptargs--;
    b = args[1];
skip_optional_posonly:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[2]) {
        c = args[2];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    d = args[3];
skip_optional_kwonly:
    return_value = posonly_opt_kwonly_opt_impl(module, a, b, c, d);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_keywords_kwonly_opt__doc__,
"posonly_keywords_kwonly_opt($module, a, /, b, *, c, d=None, e=None)\n"
"--\n"
"\n");

#define POSONLY_KEYWORDS_KWONLY_OPT_METHODDEF    \
    {"posonly_keywords_kwonly_opt", _PyCFunction_CAST(posonly_keywords_kwonly_opt), METH_FASTCALL|METH_KEYWORDS, posonly_keywords_kwonly_opt__doc__},

static PyObject *
posonly_keywords_kwonly_opt_impl(PyObject *module, PyObject *a, PyObject *b,
                                 PyObject *c, PyObject *d, PyObject *e);

static PyObject *
posonly_keywords_kwonly_opt(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "b", "c", "d", "e", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "posonly_keywords_kwonly_opt", 0};
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 3;
    PyObject *a;
    PyObject *b;
    PyObject *c;
    PyObject *d = Py_None;
    PyObject *e = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 1, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    c = args[2];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[3]) {
        d = args[3];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    e = args[4];
skip_optional_kwonly:
    return_value = posonly_keywords_kwonly_opt_impl(module, a, b, c, d, e);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_keywords_opt_kwonly_opt__doc__,
"posonly_keywords_opt_kwonly_opt($module, a, /, b, c=None, *, d=None,\n"
"                                e=None)\n"
"--\n"
"\n");

#define POSONLY_KEYWORDS_OPT_KWONLY_OPT_METHODDEF    \
    {"posonly_keywords_opt_kwonly_opt", _PyCFunction_CAST(posonly_keywords_opt_kwonly_opt), METH_FASTCALL|METH_KEYWORDS, posonly_keywords_opt_kwonly_opt__doc__},

static PyObject *
posonly_keywords_opt_kwonly_opt_impl(PyObject *module, PyObject *a,
                                     PyObject *b, PyObject *c, PyObject *d,
                                     PyObject *e);

static PyObject *
posonly_keywords_opt_kwonly_opt(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "b", "c", "d", "e", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "posonly_keywords_opt_kwonly_opt", 0};
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *a;
    PyObject *b;
    PyObject *c = Py_None;
    PyObject *d = Py_None;
    PyObject *e = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        c = args[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[3]) {
        d = args[3];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    e = args[4];
skip_optional_kwonly:
    return_value = posonly_keywords_opt_kwonly_opt_impl(module, a, b, c, d, e);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_opt_keywords_opt_kwonly_opt__doc__,
"posonly_opt_keywords_opt_kwonly_opt($module, a, b=None, /, c=None, *,\n"
"                                    d=None)\n"
"--\n"
"\n");

#define POSONLY_OPT_KEYWORDS_OPT_KWONLY_OPT_METHODDEF    \
    {"posonly_opt_keywords_opt_kwonly_opt", _PyCFunction_CAST(posonly_opt_keywords_opt_kwonly_opt), METH_FASTCALL|METH_KEYWORDS, posonly_opt_keywords_opt_kwonly_opt__doc__},

static PyObject *
posonly_opt_keywords_opt_kwonly_opt_impl(PyObject *module, PyObject *a,
                                         PyObject *b, PyObject *c,
                                         PyObject *d);

static PyObject *
posonly_opt_keywords_opt_kwonly_opt(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "", "c", "d", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "posonly_opt_keywords_opt_kwonly_opt", 0};
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *b = Py_None;
    PyObject *c = Py_None;
    PyObject *d = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    if (nargs < 2) {
        goto skip_optional_posonly;
    }
    noptargs--;
    b = args[1];
skip_optional_posonly:
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        c = args[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    d = args[3];
skip_optional_kwonly:
    return_value = posonly_opt_keywords_opt_kwonly_opt_impl(module, a, b, c, d);

exit:
    return return_value;
}

PyDoc_STRVAR(keyword_only_parameter__doc__,
"keyword_only_parameter($module, /, *, a)\n"
"--\n"
"\n");

#define KEYWORD_ONLY_PARAMETER_METHODDEF    \
    {"keyword_only_parameter", _PyCFunction_CAST(keyword_only_parameter), METH_FASTCALL|METH_KEYWORDS, keyword_only_parameter__doc__},

static PyObject *
keyword_only_parameter_impl(PyObject *module, PyObject *a);

static PyObject *
keyword_only_parameter(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"a", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "keyword_only_parameter", 0};
    PyObject *argsbuf[1];
    PyObject *a;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 0, 1, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    return_value = keyword_only_parameter_impl(module, a);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_vararg__doc__,
"posonly_vararg($module, a, /, b, *args)\n"
"--\n"
"\n");

#define POSONLY_VARARG_METHODDEF    \
    {"posonly_vararg", _PyCFunction_CAST(posonly_vararg), METH_FASTCALL|METH_KEYWORDS, posonly_vararg__doc__},

static PyObject *
posonly_vararg_impl(PyObject *module, PyObject *a, PyObject *b,
                    PyObject *args);

static PyObject *
posonly_vararg(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "b", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "posonly_vararg", 0};
    PyObject *argsbuf[3];
    PyObject *a;
    PyObject *b;
    PyObject *__clinic_args = NULL;

    args = _PyArg_UnpackKeywordsWithVararg(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, 2, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    __clinic_args = args[2];
    return_value = posonly_vararg_impl(module, a, b, __clinic_args);

exit:
    Py_XDECREF(__clinic_args);
    return return_value;
}

PyDoc_STRVAR(vararg_and_posonly__doc__,
"vararg_and_posonly($module, a, /, *args)\n"
"--\n"
"\n");

#define VARARG_AND_POSONLY_METHODDEF    \
    {"vararg_and_posonly", _PyCFunction_CAST(vararg_and_posonly), METH_FASTCALL, vararg_and_posonly__doc__},

static PyObject *
vararg_and_posonly_impl(PyObject *module, PyObject *a, PyObject *args);

static PyObject *
vararg_and_posonly(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *__clinic_args = NULL;

    if (!_PyArg_CheckPositional("vararg_and_posonly", nargs, 1, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    a = args[0];
    __clinic_args = PyTuple_New(nargs - 1);
    if (!__clinic_args) {
        goto exit;
    }
    for (Py_ssize_t i = 0; i < nargs - 1; ++i) {
        PyTuple_SET_ITEM(__clinic_args, i, Py_NewRef(args[1 + i]));
    }
    return_value = vararg_and_posonly_impl(module, a, __clinic_args);

exit:
    Py_XDECREF(__clinic_args);
    return return_value;
}

PyDoc_STRVAR(vararg__doc__,
"vararg($module, /, a, *args)\n"
"--\n"
"\n");

#define VARARG_METHODDEF    \
    {"vararg", _PyCFunction_CAST(vararg), METH_FASTCALL|METH_KEYWORDS, vararg__doc__},

static PyObject *
vararg_impl(PyObject *module, PyObject *a, PyObject *args);

static PyObject *
vararg(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"a", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "vararg", 0};
    PyObject *argsbuf[2];
    PyObject *a;
    PyObject *__clinic_args = NULL;

    args = _PyArg_UnpackKeywordsWithVararg(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, 1, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    __clinic_args = args[1];
    return_value = vararg_impl(module, a, __clinic_args);

exit:
    Py_XDECREF(__clinic_args);
    return return_value;
}

PyDoc_STRVAR(vararg_with_default__doc__,
"vararg_with_default($module, /, a, *args, b=False)\n"
"--\n"
"\n");

#define VARARG_WITH_DEFAULT_METHODDEF    \
    {"vararg_with_default", _PyCFunction_CAST(vararg_with_default), METH_FASTCALL|METH_KEYWORDS, vararg_with_default__doc__},

static PyObject *
vararg_with_default_impl(PyObject *module, PyObject *a, PyObject *args,
                         int b);

static PyObject *
vararg_with_default(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"a", "b", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "vararg_with_default", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = Py_MIN(nargs, 1) + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *__clinic_args = NULL;
    int b = 0;

    args = _PyArg_UnpackKeywordsWithVararg(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, 1, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    __clinic_args = args[1];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    b = PyObject_IsTrue(args[2]);
    if (b < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = vararg_with_default_impl(module, a, __clinic_args, b);

exit:
    Py_XDECREF(__clinic_args);
    return return_value;
}

PyDoc_STRVAR(vararg_with_only_defaults__doc__,
"vararg_with_only_defaults($module, /, *args, b=None)\n"
"--\n"
"\n");

#define VARARG_WITH_ONLY_DEFAULTS_METHODDEF    \
    {"vararg_with_only_defaults", _PyCFunction_CAST(vararg_with_only_defaults), METH_FASTCALL|METH_KEYWORDS, vararg_with_only_defaults__doc__},

static PyObject *
vararg_with_only_defaults_impl(PyObject *module, PyObject *args, PyObject *b);

static PyObject *
vararg_with_only_defaults(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"b", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "vararg_with_only_defaults", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = 0 + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *__clinic_args = NULL;
    PyObject *b = Py_None;

    args = _PyArg_UnpackKeywordsWithVararg(args, nargs, NULL, kwnames, &_parser, 0, 0, 0, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    __clinic_args = args[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    b = args[1];
skip_optional_kwonly:
    return_value = vararg_with_only_defaults_impl(module, __clinic_args, b);

exit:
    Py_XDECREF(__clinic_args);
    return return_value;
}

PyDoc_STRVAR(gh_32092_oob__doc__,
"gh_32092_oob($module, /, pos1, pos2, *varargs, kw1=None, kw2=None)\n"
"--\n"
"\n"
"Proof-of-concept of GH-32092 OOB bug.");

#define GH_32092_OOB_METHODDEF    \
    {"gh_32092_oob", _PyCFunction_CAST(gh_32092_oob), METH_FASTCALL|METH_KEYWORDS, gh_32092_oob__doc__},

static PyObject *
gh_32092_oob_impl(PyObject *module, PyObject *pos1, PyObject *pos2,
                  PyObject *varargs, PyObject *kw1, PyObject *kw2);

static PyObject *
gh_32092_oob(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"pos1", "pos2", "kw1", "kw2", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "gh_32092_oob", 0};
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = Py_MIN(nargs, 2) + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *pos1;
    PyObject *pos2;
    PyObject *varargs = NULL;
    PyObject *kw1 = Py_None;
    PyObject *kw2 = Py_None;

    args = _PyArg_UnpackKeywordsWithVararg(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, 2, argsbuf);
    if (!args) {
        goto exit;
    }
    pos1 = args[0];
    pos2 = args[1];
    varargs = args[2];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[3]) {
        kw1 = args[3];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    kw2 = args[4];
skip_optional_kwonly:
    return_value = gh_32092_oob_impl(module, pos1, pos2, varargs, kw1, kw2);

exit:
    Py_XDECREF(varargs);
    return return_value;
}

PyDoc_STRVAR(gh_32092_kw_pass__doc__,
"gh_32092_kw_pass($module, /, pos, *args, kw=None)\n"
"--\n"
"\n"
"Proof-of-concept of GH-32092 keyword args passing bug.");

#define GH_32092_KW_PASS_METHODDEF    \
    {"gh_32092_kw_pass", _PyCFunction_CAST(gh_32092_kw_pass), METH_FASTCALL|METH_KEYWORDS, gh_32092_kw_pass__doc__},

static PyObject *
gh_32092_kw_pass_impl(PyObject *module, PyObject *pos, PyObject *args,
                      PyObject *kw);

static PyObject *
gh_32092_kw_pass(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"pos", "kw", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "gh_32092_kw_pass", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = Py_MIN(nargs, 1) + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *pos;
    PyObject *__clinic_args = NULL;
    PyObject *kw = Py_None;

    args = _PyArg_UnpackKeywordsWithVararg(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, 1, argsbuf);
    if (!args) {
        goto exit;
    }
    pos = args[0];
    __clinic_args = args[1];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    kw = args[2];
skip_optional_kwonly:
    return_value = gh_32092_kw_pass_impl(module, pos, __clinic_args, kw);

exit:
    Py_XDECREF(__clinic_args);
    return return_value;
}

PyDoc_STRVAR(gh_99233_refcount__doc__,
"gh_99233_refcount($module, /, *args)\n"
"--\n"
"\n"
"Proof-of-concept of GH-99233 refcount error bug.");

#define GH_99233_REFCOUNT_METHODDEF    \
    {"gh_99233_refcount", _PyCFunction_CAST(gh_99233_refcount), METH_FASTCALL, gh_99233_refcount__doc__},

static PyObject *
gh_99233_refcount_impl(PyObject *module, PyObject *args);

static PyObject *
gh_99233_refcount(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *__clinic_args = NULL;

    if (!_PyArg_CheckPositional("gh_99233_refcount", nargs, 0, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    __clinic_args = PyTuple_New(nargs - 0);
    if (!__clinic_args) {
        goto exit;
    }
    for (Py_ssize_t i = 0; i < nargs - 0; ++i) {
        PyTuple_SET_ITEM(__clinic_args, i, Py_NewRef(args[0 + i]));
    }
    return_value = gh_99233_refcount_impl(module, __clinic_args);

exit:
    Py_XDECREF(__clinic_args);
    return return_value;
}

PyDoc_STRVAR(gh_99240_double_free__doc__,
"gh_99240_double_free($module, a, b, /)\n"
"--\n"
"\n"
"Proof-of-concept of GH-99240 double-free bug.");

#define GH_99240_DOUBLE_FREE_METHODDEF    \
    {"gh_99240_double_free", _PyCFunction_CAST(gh_99240_double_free), METH_FASTCALL, gh_99240_double_free__doc__},

static PyObject *
gh_99240_double_free_impl(PyObject *module, char *a, char *b);

static PyObject *
gh_99240_double_free(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    char *a = NULL;
    char *b = NULL;

    if (!_PyArg_ParseStack(args, nargs, "eses:gh_99240_double_free",
        "idna", &a, "idna", &b)) {
        goto exit;
    }
    return_value = gh_99240_double_free_impl(module, a, b);
    /* Post parse cleanup for a */
    PyMem_FREE(a);
    /* Post parse cleanup for b */
    PyMem_FREE(b);

exit:
    return return_value;
}
/*[clinic end generated code: output=6b719efc1b8bd2c8 input=a9049054013a1b77]*/
