/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(unicodedata_UCD_decimal__doc__,
"decimal($self, chr, default=<unrepresentable>, /)\n"
"--\n"
"\n"
"Converts a Unicode character into its equivalent decimal value.\n"
"\n"
"Returns the decimal value assigned to the character chr as integer.\n"
"If no such value is defined, default is returned, or, if not given,\n"
"ValueError is raised.");

#define UNICODEDATA_UCD_DECIMAL_METHODDEF    \
    {"decimal", (PyCFunction)(void(*)(void))unicodedata_UCD_decimal, METH_FASTCALL, unicodedata_UCD_decimal__doc__},

static PyObject *
unicodedata_UCD_decimal_impl(PyObject *self, int chr,
                             PyObject *default_value);

static PyObject *
unicodedata_UCD_decimal(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int chr;
    PyObject *default_value = NULL;

    if (!_PyArg_CheckPositional("decimal", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("decimal", "argument 1", "a unicode character", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0])) {
        goto exit;
    }
    if (PyUnicode_GET_LENGTH(args[0]) != 1) {
        _PyArg_BadArgument("decimal", "argument 1", "a unicode character", args[0]);
        goto exit;
    }
    chr = PyUnicode_READ_CHAR(args[0], 0);
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = args[1];
skip_optional:
    return_value = unicodedata_UCD_decimal_impl(self, chr, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_digit__doc__,
"digit($self, chr, default=<unrepresentable>, /)\n"
"--\n"
"\n"
"Converts a Unicode character into its equivalent digit value.\n"
"\n"
"Returns the digit value assigned to the character chr as integer.\n"
"If no such value is defined, default is returned, or, if not given,\n"
"ValueError is raised.");

#define UNICODEDATA_UCD_DIGIT_METHODDEF    \
    {"digit", (PyCFunction)(void(*)(void))unicodedata_UCD_digit, METH_FASTCALL, unicodedata_UCD_digit__doc__},

static PyObject *
unicodedata_UCD_digit_impl(PyObject *self, int chr, PyObject *default_value);

static PyObject *
unicodedata_UCD_digit(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int chr;
    PyObject *default_value = NULL;

    if (!_PyArg_CheckPositional("digit", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("digit", "argument 1", "a unicode character", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0])) {
        goto exit;
    }
    if (PyUnicode_GET_LENGTH(args[0]) != 1) {
        _PyArg_BadArgument("digit", "argument 1", "a unicode character", args[0]);
        goto exit;
    }
    chr = PyUnicode_READ_CHAR(args[0], 0);
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = args[1];
skip_optional:
    return_value = unicodedata_UCD_digit_impl(self, chr, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_numeric__doc__,
"numeric($self, chr, default=<unrepresentable>, /)\n"
"--\n"
"\n"
"Converts a Unicode character into its equivalent numeric value.\n"
"\n"
"Returns the numeric value assigned to the character chr as float.\n"
"If no such value is defined, default is returned, or, if not given,\n"
"ValueError is raised.");

#define UNICODEDATA_UCD_NUMERIC_METHODDEF    \
    {"numeric", (PyCFunction)(void(*)(void))unicodedata_UCD_numeric, METH_FASTCALL, unicodedata_UCD_numeric__doc__},

static PyObject *
unicodedata_UCD_numeric_impl(PyObject *self, int chr,
                             PyObject *default_value);

static PyObject *
unicodedata_UCD_numeric(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int chr;
    PyObject *default_value = NULL;

    if (!_PyArg_CheckPositional("numeric", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("numeric", "argument 1", "a unicode character", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0])) {
        goto exit;
    }
    if (PyUnicode_GET_LENGTH(args[0]) != 1) {
        _PyArg_BadArgument("numeric", "argument 1", "a unicode character", args[0]);
        goto exit;
    }
    chr = PyUnicode_READ_CHAR(args[0], 0);
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = args[1];
skip_optional:
    return_value = unicodedata_UCD_numeric_impl(self, chr, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_category__doc__,
"category($self, chr, /)\n"
"--\n"
"\n"
"Returns the general category assigned to the character chr as string.");

#define UNICODEDATA_UCD_CATEGORY_METHODDEF    \
    {"category", (PyCFunction)unicodedata_UCD_category, METH_O, unicodedata_UCD_category__doc__},

static PyObject *
unicodedata_UCD_category_impl(PyObject *self, int chr);

static PyObject *
unicodedata_UCD_category(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int chr;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("category", "argument", "a unicode character", arg);
        goto exit;
    }
    if (PyUnicode_READY(arg)) {
        goto exit;
    }
    if (PyUnicode_GET_LENGTH(arg) != 1) {
        _PyArg_BadArgument("category", "argument", "a unicode character", arg);
        goto exit;
    }
    chr = PyUnicode_READ_CHAR(arg, 0);
    return_value = unicodedata_UCD_category_impl(self, chr);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_bidirectional__doc__,
"bidirectional($self, chr, /)\n"
"--\n"
"\n"
"Returns the bidirectional class assigned to the character chr as string.\n"
"\n"
"If no such value is defined, an empty string is returned.");

#define UNICODEDATA_UCD_BIDIRECTIONAL_METHODDEF    \
    {"bidirectional", (PyCFunction)unicodedata_UCD_bidirectional, METH_O, unicodedata_UCD_bidirectional__doc__},

static PyObject *
unicodedata_UCD_bidirectional_impl(PyObject *self, int chr);

static PyObject *
unicodedata_UCD_bidirectional(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int chr;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("bidirectional", "argument", "a unicode character", arg);
        goto exit;
    }
    if (PyUnicode_READY(arg)) {
        goto exit;
    }
    if (PyUnicode_GET_LENGTH(arg) != 1) {
        _PyArg_BadArgument("bidirectional", "argument", "a unicode character", arg);
        goto exit;
    }
    chr = PyUnicode_READ_CHAR(arg, 0);
    return_value = unicodedata_UCD_bidirectional_impl(self, chr);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_combining__doc__,
"combining($self, chr, /)\n"
"--\n"
"\n"
"Returns the canonical combining class assigned to the character chr as integer.\n"
"\n"
"Returns 0 if no combining class is defined.");

#define UNICODEDATA_UCD_COMBINING_METHODDEF    \
    {"combining", (PyCFunction)unicodedata_UCD_combining, METH_O, unicodedata_UCD_combining__doc__},

static int
unicodedata_UCD_combining_impl(PyObject *self, int chr);

static PyObject *
unicodedata_UCD_combining(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int chr;
    int _return_value;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("combining", "argument", "a unicode character", arg);
        goto exit;
    }
    if (PyUnicode_READY(arg)) {
        goto exit;
    }
    if (PyUnicode_GET_LENGTH(arg) != 1) {
        _PyArg_BadArgument("combining", "argument", "a unicode character", arg);
        goto exit;
    }
    chr = PyUnicode_READ_CHAR(arg, 0);
    _return_value = unicodedata_UCD_combining_impl(self, chr);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_mirrored__doc__,
"mirrored($self, chr, /)\n"
"--\n"
"\n"
"Returns the mirrored property assigned to the character chr as integer.\n"
"\n"
"Returns 1 if the character has been identified as a \"mirrored\"\n"
"character in bidirectional text, 0 otherwise.");

#define UNICODEDATA_UCD_MIRRORED_METHODDEF    \
    {"mirrored", (PyCFunction)unicodedata_UCD_mirrored, METH_O, unicodedata_UCD_mirrored__doc__},

static int
unicodedata_UCD_mirrored_impl(PyObject *self, int chr);

static PyObject *
unicodedata_UCD_mirrored(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int chr;
    int _return_value;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("mirrored", "argument", "a unicode character", arg);
        goto exit;
    }
    if (PyUnicode_READY(arg)) {
        goto exit;
    }
    if (PyUnicode_GET_LENGTH(arg) != 1) {
        _PyArg_BadArgument("mirrored", "argument", "a unicode character", arg);
        goto exit;
    }
    chr = PyUnicode_READ_CHAR(arg, 0);
    _return_value = unicodedata_UCD_mirrored_impl(self, chr);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_east_asian_width__doc__,
"east_asian_width($self, chr, /)\n"
"--\n"
"\n"
"Returns the east asian width assigned to the character chr as string.");

#define UNICODEDATA_UCD_EAST_ASIAN_WIDTH_METHODDEF    \
    {"east_asian_width", (PyCFunction)unicodedata_UCD_east_asian_width, METH_O, unicodedata_UCD_east_asian_width__doc__},

static PyObject *
unicodedata_UCD_east_asian_width_impl(PyObject *self, int chr);

static PyObject *
unicodedata_UCD_east_asian_width(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int chr;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("east_asian_width", "argument", "a unicode character", arg);
        goto exit;
    }
    if (PyUnicode_READY(arg)) {
        goto exit;
    }
    if (PyUnicode_GET_LENGTH(arg) != 1) {
        _PyArg_BadArgument("east_asian_width", "argument", "a unicode character", arg);
        goto exit;
    }
    chr = PyUnicode_READ_CHAR(arg, 0);
    return_value = unicodedata_UCD_east_asian_width_impl(self, chr);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_decomposition__doc__,
"decomposition($self, chr, /)\n"
"--\n"
"\n"
"Returns the character decomposition mapping assigned to the character chr as string.\n"
"\n"
"An empty string is returned in case no such mapping is defined.");

#define UNICODEDATA_UCD_DECOMPOSITION_METHODDEF    \
    {"decomposition", (PyCFunction)unicodedata_UCD_decomposition, METH_O, unicodedata_UCD_decomposition__doc__},

static PyObject *
unicodedata_UCD_decomposition_impl(PyObject *self, int chr);

static PyObject *
unicodedata_UCD_decomposition(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int chr;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("decomposition", "argument", "a unicode character", arg);
        goto exit;
    }
    if (PyUnicode_READY(arg)) {
        goto exit;
    }
    if (PyUnicode_GET_LENGTH(arg) != 1) {
        _PyArg_BadArgument("decomposition", "argument", "a unicode character", arg);
        goto exit;
    }
    chr = PyUnicode_READ_CHAR(arg, 0);
    return_value = unicodedata_UCD_decomposition_impl(self, chr);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_is_normalized__doc__,
"is_normalized($self, form, unistr, /)\n"
"--\n"
"\n"
"Return whether the Unicode string unistr is in the normal form \'form\'.\n"
"\n"
"Valid values for form are \'NFC\', \'NFKC\', \'NFD\', and \'NFKD\'.");

#define UNICODEDATA_UCD_IS_NORMALIZED_METHODDEF    \
    {"is_normalized", (PyCFunction)(void(*)(void))unicodedata_UCD_is_normalized, METH_FASTCALL, unicodedata_UCD_is_normalized__doc__},

static PyObject *
unicodedata_UCD_is_normalized_impl(PyObject *self, PyObject *form,
                                   PyObject *input);

static PyObject *
unicodedata_UCD_is_normalized(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *form;
    PyObject *input;

    if (!_PyArg_CheckPositional("is_normalized", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("is_normalized", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    form = args[0];
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("is_normalized", "argument 2", "str", args[1]);
        goto exit;
    }
    if (PyUnicode_READY(args[1]) == -1) {
        goto exit;
    }
    input = args[1];
    return_value = unicodedata_UCD_is_normalized_impl(self, form, input);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_normalize__doc__,
"normalize($self, form, unistr, /)\n"
"--\n"
"\n"
"Return the normal form \'form\' for the Unicode string unistr.\n"
"\n"
"Valid values for form are \'NFC\', \'NFKC\', \'NFD\', and \'NFKD\'.");

#define UNICODEDATA_UCD_NORMALIZE_METHODDEF    \
    {"normalize", (PyCFunction)(void(*)(void))unicodedata_UCD_normalize, METH_FASTCALL, unicodedata_UCD_normalize__doc__},

static PyObject *
unicodedata_UCD_normalize_impl(PyObject *self, PyObject *form,
                               PyObject *input);

static PyObject *
unicodedata_UCD_normalize(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *form;
    PyObject *input;

    if (!_PyArg_CheckPositional("normalize", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("normalize", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    form = args[0];
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("normalize", "argument 2", "str", args[1]);
        goto exit;
    }
    if (PyUnicode_READY(args[1]) == -1) {
        goto exit;
    }
    input = args[1];
    return_value = unicodedata_UCD_normalize_impl(self, form, input);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_name__doc__,
"name($self, chr, default=<unrepresentable>, /)\n"
"--\n"
"\n"
"Returns the name assigned to the character chr as a string.\n"
"\n"
"If no name is defined, default is returned, or, if not given,\n"
"ValueError is raised.");

#define UNICODEDATA_UCD_NAME_METHODDEF    \
    {"name", (PyCFunction)(void(*)(void))unicodedata_UCD_name, METH_FASTCALL, unicodedata_UCD_name__doc__},

static PyObject *
unicodedata_UCD_name_impl(PyObject *self, int chr, PyObject *default_value);

static PyObject *
unicodedata_UCD_name(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int chr;
    PyObject *default_value = NULL;

    if (!_PyArg_CheckPositional("name", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("name", "argument 1", "a unicode character", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0])) {
        goto exit;
    }
    if (PyUnicode_GET_LENGTH(args[0]) != 1) {
        _PyArg_BadArgument("name", "argument 1", "a unicode character", args[0]);
        goto exit;
    }
    chr = PyUnicode_READ_CHAR(args[0], 0);
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = args[1];
skip_optional:
    return_value = unicodedata_UCD_name_impl(self, chr, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_lookup__doc__,
"lookup($self, name, /)\n"
"--\n"
"\n"
"Look up character by name.\n"
"\n"
"If a character with the given name is found, return the\n"
"corresponding character.  If not found, KeyError is raised.");

#define UNICODEDATA_UCD_LOOKUP_METHODDEF    \
    {"lookup", (PyCFunction)unicodedata_UCD_lookup, METH_O, unicodedata_UCD_lookup__doc__},

static PyObject *
unicodedata_UCD_lookup_impl(PyObject *self, const char *name,
                            Py_ssize_t name_length);

static PyObject *
unicodedata_UCD_lookup(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *name;
    Py_ssize_t name_length;

    if (!PyArg_Parse(arg, "s#:lookup", &name, &name_length)) {
        goto exit;
    }
    return_value = unicodedata_UCD_lookup_impl(self, name, name_length);

exit:
    return return_value;
}
/*[clinic end generated code: output=a3c0eb84eda47b2d input=a9049054013a1b77]*/
