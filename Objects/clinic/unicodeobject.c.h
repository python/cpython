/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(unicode_title__doc__,
"title($self, /)\n"
"--\n"
"\n"
"Return a version of the string where each word is titlecased.\n"
"\n"
"More specifically, words start with uppercased characters and all remaining\n"
"cased characters have lower case.");

#define UNICODE_TITLE_METHODDEF    \
    {"title", (PyCFunction)unicode_title, METH_NOARGS, unicode_title__doc__},

static PyObject *
unicode_title_impl(PyObject *self);

static PyObject *
unicode_title(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_title_impl(self);
}

PyDoc_STRVAR(unicode_capitalize__doc__,
"capitalize($self, /)\n"
"--\n"
"\n"
"Return a capitalized version of the string.\n"
"\n"
"More specifically, make the first character have upper case and the rest lower\n"
"case.");

#define UNICODE_CAPITALIZE_METHODDEF    \
    {"capitalize", (PyCFunction)unicode_capitalize, METH_NOARGS, unicode_capitalize__doc__},

static PyObject *
unicode_capitalize_impl(PyObject *self);

static PyObject *
unicode_capitalize(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_capitalize_impl(self);
}

PyDoc_STRVAR(unicode_casefold__doc__,
"casefold($self, /)\n"
"--\n"
"\n"
"Return a version of the string suitable for caseless comparisons.");

#define UNICODE_CASEFOLD_METHODDEF    \
    {"casefold", (PyCFunction)unicode_casefold, METH_NOARGS, unicode_casefold__doc__},

static PyObject *
unicode_casefold_impl(PyObject *self);

static PyObject *
unicode_casefold(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_casefold_impl(self);
}

PyDoc_STRVAR(unicode_center__doc__,
"center($self, width, fillchar=\' \', /)\n"
"--\n"
"\n"
"Return a centered string of length width.\n"
"\n"
"Padding is done using the specified fill character (default is a space).");

#define UNICODE_CENTER_METHODDEF    \
    {"center", (PyCFunction)(void(*)(void))unicode_center, METH_FASTCALL, unicode_center__doc__},

static PyObject *
unicode_center_impl(PyObject *self, Py_ssize_t width, Py_UCS4 fillchar);

static PyObject *
unicode_center(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t width;
    Py_UCS4 fillchar = ' ';

    if (!_PyArg_CheckPositional("center", nargs, 1, 2)) {
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
        width = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!convert_uc(args[1], &fillchar)) {
        goto exit;
    }
skip_optional:
    return_value = unicode_center_impl(self, width, fillchar);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_encode__doc__,
"encode($self, /, encoding=\'utf-8\', errors=\'strict\')\n"
"--\n"
"\n"
"Encode the string using the codec registered for encoding.\n"
"\n"
"  encoding\n"
"    The encoding in which to encode the string.\n"
"  errors\n"
"    The error handling scheme to use for encoding errors.\n"
"    The default is \'strict\' meaning that encoding errors raise a\n"
"    UnicodeEncodeError.  Other possible values are \'ignore\', \'replace\' and\n"
"    \'xmlcharrefreplace\' as well as any other name registered with\n"
"    codecs.register_error that can handle UnicodeEncodeErrors.");

#define UNICODE_ENCODE_METHODDEF    \
    {"encode", (PyCFunction)(void(*)(void))unicode_encode, METH_FASTCALL|METH_KEYWORDS, unicode_encode__doc__},

static PyObject *
unicode_encode_impl(PyObject *self, const char *encoding, const char *errors);

static PyObject *
unicode_encode(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"encoding", "errors", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "encode", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    const char *encoding = NULL;
    const char *errors = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        if (!PyUnicode_Check(args[0])) {
            _PyArg_BadArgument("encode", "argument 'encoding'", "str", args[0]);
            goto exit;
        }
        Py_ssize_t encoding_length;
        encoding = PyUnicode_AsUTF8AndSize(args[0], &encoding_length);
        if (encoding == NULL) {
            goto exit;
        }
        if (strlen(encoding) != (size_t)encoding_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("encode", "argument 'errors'", "str", args[1]);
        goto exit;
    }
    Py_ssize_t errors_length;
    errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
    if (errors == NULL) {
        goto exit;
    }
    if (strlen(errors) != (size_t)errors_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
skip_optional_pos:
    return_value = unicode_encode_impl(self, encoding, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_expandtabs__doc__,
"expandtabs($self, /, tabsize=8)\n"
"--\n"
"\n"
"Return a copy where all tab characters are expanded using spaces.\n"
"\n"
"If tabsize is not given, a tab size of 8 characters is assumed.");

#define UNICODE_EXPANDTABS_METHODDEF    \
    {"expandtabs", (PyCFunction)(void(*)(void))unicode_expandtabs, METH_FASTCALL|METH_KEYWORDS, unicode_expandtabs__doc__},

static PyObject *
unicode_expandtabs_impl(PyObject *self, int tabsize);

static PyObject *
unicode_expandtabs(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"tabsize", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "expandtabs", 0};
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int tabsize = 8;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    tabsize = _PyLong_AsInt(args[0]);
    if (tabsize == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = unicode_expandtabs_impl(self, tabsize);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_isascii__doc__,
"isascii($self, /)\n"
"--\n"
"\n"
"Return True if all characters in the string are ASCII, False otherwise.\n"
"\n"
"ASCII characters have code points in the range U+0000-U+007F.\n"
"Empty string is ASCII too.");

#define UNICODE_ISASCII_METHODDEF    \
    {"isascii", (PyCFunction)unicode_isascii, METH_NOARGS, unicode_isascii__doc__},

static PyObject *
unicode_isascii_impl(PyObject *self);

static PyObject *
unicode_isascii(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_isascii_impl(self);
}

PyDoc_STRVAR(unicode_islower__doc__,
"islower($self, /)\n"
"--\n"
"\n"
"Return True if the string is a lowercase string, False otherwise.\n"
"\n"
"A string is lowercase if all cased characters in the string are lowercase and\n"
"there is at least one cased character in the string.");

#define UNICODE_ISLOWER_METHODDEF    \
    {"islower", (PyCFunction)unicode_islower, METH_NOARGS, unicode_islower__doc__},

static PyObject *
unicode_islower_impl(PyObject *self);

static PyObject *
unicode_islower(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_islower_impl(self);
}

PyDoc_STRVAR(unicode_isupper__doc__,
"isupper($self, /)\n"
"--\n"
"\n"
"Return True if the string is an uppercase string, False otherwise.\n"
"\n"
"A string is uppercase if all cased characters in the string are uppercase and\n"
"there is at least one cased character in the string.");

#define UNICODE_ISUPPER_METHODDEF    \
    {"isupper", (PyCFunction)unicode_isupper, METH_NOARGS, unicode_isupper__doc__},

static PyObject *
unicode_isupper_impl(PyObject *self);

static PyObject *
unicode_isupper(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_isupper_impl(self);
}

PyDoc_STRVAR(unicode_istitle__doc__,
"istitle($self, /)\n"
"--\n"
"\n"
"Return True if the string is a title-cased string, False otherwise.\n"
"\n"
"In a title-cased string, upper- and title-case characters may only\n"
"follow uncased characters and lowercase characters only cased ones.");

#define UNICODE_ISTITLE_METHODDEF    \
    {"istitle", (PyCFunction)unicode_istitle, METH_NOARGS, unicode_istitle__doc__},

static PyObject *
unicode_istitle_impl(PyObject *self);

static PyObject *
unicode_istitle(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_istitle_impl(self);
}

PyDoc_STRVAR(unicode_isspace__doc__,
"isspace($self, /)\n"
"--\n"
"\n"
"Return True if the string is a whitespace string, False otherwise.\n"
"\n"
"A string is whitespace if all characters in the string are whitespace and there\n"
"is at least one character in the string.");

#define UNICODE_ISSPACE_METHODDEF    \
    {"isspace", (PyCFunction)unicode_isspace, METH_NOARGS, unicode_isspace__doc__},

static PyObject *
unicode_isspace_impl(PyObject *self);

static PyObject *
unicode_isspace(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_isspace_impl(self);
}

PyDoc_STRVAR(unicode_isalpha__doc__,
"isalpha($self, /)\n"
"--\n"
"\n"
"Return True if the string is an alphabetic string, False otherwise.\n"
"\n"
"A string is alphabetic if all characters in the string are alphabetic and there\n"
"is at least one character in the string.");

#define UNICODE_ISALPHA_METHODDEF    \
    {"isalpha", (PyCFunction)unicode_isalpha, METH_NOARGS, unicode_isalpha__doc__},

static PyObject *
unicode_isalpha_impl(PyObject *self);

static PyObject *
unicode_isalpha(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_isalpha_impl(self);
}

PyDoc_STRVAR(unicode_isalnum__doc__,
"isalnum($self, /)\n"
"--\n"
"\n"
"Return True if the string is an alpha-numeric string, False otherwise.\n"
"\n"
"A string is alpha-numeric if all characters in the string are alpha-numeric and\n"
"there is at least one character in the string.");

#define UNICODE_ISALNUM_METHODDEF    \
    {"isalnum", (PyCFunction)unicode_isalnum, METH_NOARGS, unicode_isalnum__doc__},

static PyObject *
unicode_isalnum_impl(PyObject *self);

static PyObject *
unicode_isalnum(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_isalnum_impl(self);
}

PyDoc_STRVAR(unicode_isdecimal__doc__,
"isdecimal($self, /)\n"
"--\n"
"\n"
"Return True if the string is a decimal string, False otherwise.\n"
"\n"
"A string is a decimal string if all characters in the string are decimal and\n"
"there is at least one character in the string.");

#define UNICODE_ISDECIMAL_METHODDEF    \
    {"isdecimal", (PyCFunction)unicode_isdecimal, METH_NOARGS, unicode_isdecimal__doc__},

static PyObject *
unicode_isdecimal_impl(PyObject *self);

static PyObject *
unicode_isdecimal(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_isdecimal_impl(self);
}

PyDoc_STRVAR(unicode_isdigit__doc__,
"isdigit($self, /)\n"
"--\n"
"\n"
"Return True if the string is a digit string, False otherwise.\n"
"\n"
"A string is a digit string if all characters in the string are digits and there\n"
"is at least one character in the string.");

#define UNICODE_ISDIGIT_METHODDEF    \
    {"isdigit", (PyCFunction)unicode_isdigit, METH_NOARGS, unicode_isdigit__doc__},

static PyObject *
unicode_isdigit_impl(PyObject *self);

static PyObject *
unicode_isdigit(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_isdigit_impl(self);
}

PyDoc_STRVAR(unicode_isnumeric__doc__,
"isnumeric($self, /)\n"
"--\n"
"\n"
"Return True if the string is a numeric string, False otherwise.\n"
"\n"
"A string is numeric if all characters in the string are numeric and there is at\n"
"least one character in the string.");

#define UNICODE_ISNUMERIC_METHODDEF    \
    {"isnumeric", (PyCFunction)unicode_isnumeric, METH_NOARGS, unicode_isnumeric__doc__},

static PyObject *
unicode_isnumeric_impl(PyObject *self);

static PyObject *
unicode_isnumeric(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_isnumeric_impl(self);
}

PyDoc_STRVAR(unicode_isidentifier__doc__,
"isidentifier($self, /)\n"
"--\n"
"\n"
"Return True if the string is a valid Python identifier, False otherwise.\n"
"\n"
"Call keyword.iskeyword(s) to test whether string s is a reserved identifier,\n"
"such as \"def\" or \"class\".");

#define UNICODE_ISIDENTIFIER_METHODDEF    \
    {"isidentifier", (PyCFunction)unicode_isidentifier, METH_NOARGS, unicode_isidentifier__doc__},

static PyObject *
unicode_isidentifier_impl(PyObject *self);

static PyObject *
unicode_isidentifier(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_isidentifier_impl(self);
}

PyDoc_STRVAR(unicode_isprintable__doc__,
"isprintable($self, /)\n"
"--\n"
"\n"
"Return True if the string is printable, False otherwise.\n"
"\n"
"A string is printable if all of its characters are considered printable in\n"
"repr() or if it is empty.");

#define UNICODE_ISPRINTABLE_METHODDEF    \
    {"isprintable", (PyCFunction)unicode_isprintable, METH_NOARGS, unicode_isprintable__doc__},

static PyObject *
unicode_isprintable_impl(PyObject *self);

static PyObject *
unicode_isprintable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_isprintable_impl(self);
}

PyDoc_STRVAR(unicode_join__doc__,
"join($self, iterable, /)\n"
"--\n"
"\n"
"Concatenate any number of strings.\n"
"\n"
"The string whose method is called is inserted in between each given string.\n"
"The result is returned as a new string.\n"
"\n"
"Example: \'.\'.join([\'ab\', \'pq\', \'rs\']) -> \'ab.pq.rs\'");

#define UNICODE_JOIN_METHODDEF    \
    {"join", (PyCFunction)unicode_join, METH_O, unicode_join__doc__},

PyDoc_STRVAR(unicode_ljust__doc__,
"ljust($self, width, fillchar=\' \', /)\n"
"--\n"
"\n"
"Return a left-justified string of length width.\n"
"\n"
"Padding is done using the specified fill character (default is a space).");

#define UNICODE_LJUST_METHODDEF    \
    {"ljust", (PyCFunction)(void(*)(void))unicode_ljust, METH_FASTCALL, unicode_ljust__doc__},

static PyObject *
unicode_ljust_impl(PyObject *self, Py_ssize_t width, Py_UCS4 fillchar);

static PyObject *
unicode_ljust(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t width;
    Py_UCS4 fillchar = ' ';

    if (!_PyArg_CheckPositional("ljust", nargs, 1, 2)) {
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
        width = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!convert_uc(args[1], &fillchar)) {
        goto exit;
    }
skip_optional:
    return_value = unicode_ljust_impl(self, width, fillchar);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_lower__doc__,
"lower($self, /)\n"
"--\n"
"\n"
"Return a copy of the string converted to lowercase.");

#define UNICODE_LOWER_METHODDEF    \
    {"lower", (PyCFunction)unicode_lower, METH_NOARGS, unicode_lower__doc__},

static PyObject *
unicode_lower_impl(PyObject *self);

static PyObject *
unicode_lower(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_lower_impl(self);
}

PyDoc_STRVAR(unicode_strip__doc__,
"strip($self, chars=None, /)\n"
"--\n"
"\n"
"Return a copy of the string with leading and trailing whitespace removed.\n"
"\n"
"If chars is given and not None, remove characters in chars instead.");

#define UNICODE_STRIP_METHODDEF    \
    {"strip", (PyCFunction)(void(*)(void))unicode_strip, METH_FASTCALL, unicode_strip__doc__},

static PyObject *
unicode_strip_impl(PyObject *self, PyObject *chars);

static PyObject *
unicode_strip(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *chars = Py_None;

    if (!_PyArg_CheckPositional("strip", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    chars = args[0];
skip_optional:
    return_value = unicode_strip_impl(self, chars);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_lstrip__doc__,
"lstrip($self, chars=None, /)\n"
"--\n"
"\n"
"Return a copy of the string with leading whitespace removed.\n"
"\n"
"If chars is given and not None, remove characters in chars instead.");

#define UNICODE_LSTRIP_METHODDEF    \
    {"lstrip", (PyCFunction)(void(*)(void))unicode_lstrip, METH_FASTCALL, unicode_lstrip__doc__},

static PyObject *
unicode_lstrip_impl(PyObject *self, PyObject *chars);

static PyObject *
unicode_lstrip(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *chars = Py_None;

    if (!_PyArg_CheckPositional("lstrip", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    chars = args[0];
skip_optional:
    return_value = unicode_lstrip_impl(self, chars);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_rstrip__doc__,
"rstrip($self, chars=None, /)\n"
"--\n"
"\n"
"Return a copy of the string with trailing whitespace removed.\n"
"\n"
"If chars is given and not None, remove characters in chars instead.");

#define UNICODE_RSTRIP_METHODDEF    \
    {"rstrip", (PyCFunction)(void(*)(void))unicode_rstrip, METH_FASTCALL, unicode_rstrip__doc__},

static PyObject *
unicode_rstrip_impl(PyObject *self, PyObject *chars);

static PyObject *
unicode_rstrip(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *chars = Py_None;

    if (!_PyArg_CheckPositional("rstrip", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    chars = args[0];
skip_optional:
    return_value = unicode_rstrip_impl(self, chars);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_replace__doc__,
"replace($self, old, new, count=-1, /)\n"
"--\n"
"\n"
"Return a copy with all occurrences of substring old replaced by new.\n"
"\n"
"  count\n"
"    Maximum number of occurrences to replace.\n"
"    -1 (the default value) means replace all occurrences.\n"
"\n"
"If the optional argument count is given, only the first count occurrences are\n"
"replaced.");

#define UNICODE_REPLACE_METHODDEF    \
    {"replace", (PyCFunction)(void(*)(void))unicode_replace, METH_FASTCALL, unicode_replace__doc__},

static PyObject *
unicode_replace_impl(PyObject *self, PyObject *old, PyObject *new,
                     Py_ssize_t count);

static PyObject *
unicode_replace(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *old;
    PyObject *new;
    Py_ssize_t count = -1;

    if (!_PyArg_CheckPositional("replace", nargs, 2, 3)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("replace", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    old = args[0];
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("replace", "argument 2", "str", args[1]);
        goto exit;
    }
    if (PyUnicode_READY(args[1]) == -1) {
        goto exit;
    }
    new = args[1];
    if (nargs < 3) {
        goto skip_optional;
    }
    if (PyFloat_Check(args[2])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        count = ival;
    }
skip_optional:
    return_value = unicode_replace_impl(self, old, new, count);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_removeprefix__doc__,
"removeprefix($self, prefix, /)\n"
"--\n"
"\n"
"Return a str with the given prefix string removed if present.\n"
"\n"
"If the string starts with the prefix string, return string[len(prefix):].\n"
"Otherwise, return a copy of the original string.");

#define UNICODE_REMOVEPREFIX_METHODDEF    \
    {"removeprefix", (PyCFunction)unicode_removeprefix, METH_O, unicode_removeprefix__doc__},

static PyObject *
unicode_removeprefix_impl(PyObject *self, PyObject *prefix);

static PyObject *
unicode_removeprefix(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *prefix;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("removeprefix", "argument", "str", arg);
        goto exit;
    }
    if (PyUnicode_READY(arg) == -1) {
        goto exit;
    }
    prefix = arg;
    return_value = unicode_removeprefix_impl(self, prefix);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_removesuffix__doc__,
"removesuffix($self, suffix, /)\n"
"--\n"
"\n"
"Return a str with the given suffix string removed if present.\n"
"\n"
"If the string ends with the suffix string and that suffix is not empty,\n"
"return string[:-len(suffix)]. Otherwise, return a copy of the original\n"
"string.");

#define UNICODE_REMOVESUFFIX_METHODDEF    \
    {"removesuffix", (PyCFunction)unicode_removesuffix, METH_O, unicode_removesuffix__doc__},

static PyObject *
unicode_removesuffix_impl(PyObject *self, PyObject *suffix);

static PyObject *
unicode_removesuffix(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *suffix;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("removesuffix", "argument", "str", arg);
        goto exit;
    }
    if (PyUnicode_READY(arg) == -1) {
        goto exit;
    }
    suffix = arg;
    return_value = unicode_removesuffix_impl(self, suffix);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_rjust__doc__,
"rjust($self, width, fillchar=\' \', /)\n"
"--\n"
"\n"
"Return a right-justified string of length width.\n"
"\n"
"Padding is done using the specified fill character (default is a space).");

#define UNICODE_RJUST_METHODDEF    \
    {"rjust", (PyCFunction)(void(*)(void))unicode_rjust, METH_FASTCALL, unicode_rjust__doc__},

static PyObject *
unicode_rjust_impl(PyObject *self, Py_ssize_t width, Py_UCS4 fillchar);

static PyObject *
unicode_rjust(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t width;
    Py_UCS4 fillchar = ' ';

    if (!_PyArg_CheckPositional("rjust", nargs, 1, 2)) {
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
        width = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!convert_uc(args[1], &fillchar)) {
        goto exit;
    }
skip_optional:
    return_value = unicode_rjust_impl(self, width, fillchar);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_split__doc__,
"split($self, /, sep=None, maxsplit=-1)\n"
"--\n"
"\n"
"Return a list of the words in the string, using sep as the delimiter string.\n"
"\n"
"  sep\n"
"    The delimiter according which to split the string.\n"
"    None (the default value) means split according to any whitespace,\n"
"    and discard empty strings from the result.\n"
"  maxsplit\n"
"    Maximum number of splits to do.\n"
"    -1 (the default value) means no limit.");

#define UNICODE_SPLIT_METHODDEF    \
    {"split", (PyCFunction)(void(*)(void))unicode_split, METH_FASTCALL|METH_KEYWORDS, unicode_split__doc__},

static PyObject *
unicode_split_impl(PyObject *self, PyObject *sep, Py_ssize_t maxsplit);

static PyObject *
unicode_split(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"sep", "maxsplit", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "split", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *sep = Py_None;
    Py_ssize_t maxsplit = -1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        sep = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (PyFloat_Check(args[1])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        maxsplit = ival;
    }
skip_optional_pos:
    return_value = unicode_split_impl(self, sep, maxsplit);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_partition__doc__,
"partition($self, sep, /)\n"
"--\n"
"\n"
"Partition the string into three parts using the given separator.\n"
"\n"
"This will search for the separator in the string.  If the separator is found,\n"
"returns a 3-tuple containing the part before the separator, the separator\n"
"itself, and the part after it.\n"
"\n"
"If the separator is not found, returns a 3-tuple containing the original string\n"
"and two empty strings.");

#define UNICODE_PARTITION_METHODDEF    \
    {"partition", (PyCFunction)unicode_partition, METH_O, unicode_partition__doc__},

PyDoc_STRVAR(unicode_rpartition__doc__,
"rpartition($self, sep, /)\n"
"--\n"
"\n"
"Partition the string into three parts using the given separator.\n"
"\n"
"This will search for the separator in the string, starting at the end. If\n"
"the separator is found, returns a 3-tuple containing the part before the\n"
"separator, the separator itself, and the part after it.\n"
"\n"
"If the separator is not found, returns a 3-tuple containing two empty strings\n"
"and the original string.");

#define UNICODE_RPARTITION_METHODDEF    \
    {"rpartition", (PyCFunction)unicode_rpartition, METH_O, unicode_rpartition__doc__},

PyDoc_STRVAR(unicode_rsplit__doc__,
"rsplit($self, /, sep=None, maxsplit=-1)\n"
"--\n"
"\n"
"Return a list of the words in the string, using sep as the delimiter string.\n"
"\n"
"  sep\n"
"    The delimiter according which to split the string.\n"
"    None (the default value) means split according to any whitespace,\n"
"    and discard empty strings from the result.\n"
"  maxsplit\n"
"    Maximum number of splits to do.\n"
"    -1 (the default value) means no limit.\n"
"\n"
"Splits are done starting at the end of the string and working to the front.");

#define UNICODE_RSPLIT_METHODDEF    \
    {"rsplit", (PyCFunction)(void(*)(void))unicode_rsplit, METH_FASTCALL|METH_KEYWORDS, unicode_rsplit__doc__},

static PyObject *
unicode_rsplit_impl(PyObject *self, PyObject *sep, Py_ssize_t maxsplit);

static PyObject *
unicode_rsplit(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"sep", "maxsplit", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "rsplit", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *sep = Py_None;
    Py_ssize_t maxsplit = -1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        sep = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (PyFloat_Check(args[1])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        maxsplit = ival;
    }
skip_optional_pos:
    return_value = unicode_rsplit_impl(self, sep, maxsplit);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_splitlines__doc__,
"splitlines($self, /, keepends=False)\n"
"--\n"
"\n"
"Return a list of the lines in the string, breaking at line boundaries.\n"
"\n"
"Line breaks are not included in the resulting list unless keepends is given and\n"
"true.");

#define UNICODE_SPLITLINES_METHODDEF    \
    {"splitlines", (PyCFunction)(void(*)(void))unicode_splitlines, METH_FASTCALL|METH_KEYWORDS, unicode_splitlines__doc__},

static PyObject *
unicode_splitlines_impl(PyObject *self, int keepends);

static PyObject *
unicode_splitlines(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"keepends", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "splitlines", 0};
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int keepends = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    keepends = _PyLong_AsInt(args[0]);
    if (keepends == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = unicode_splitlines_impl(self, keepends);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_swapcase__doc__,
"swapcase($self, /)\n"
"--\n"
"\n"
"Convert uppercase characters to lowercase and lowercase characters to uppercase.");

#define UNICODE_SWAPCASE_METHODDEF    \
    {"swapcase", (PyCFunction)unicode_swapcase, METH_NOARGS, unicode_swapcase__doc__},

static PyObject *
unicode_swapcase_impl(PyObject *self);

static PyObject *
unicode_swapcase(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_swapcase_impl(self);
}

PyDoc_STRVAR(unicode_maketrans__doc__,
"maketrans(x, y=<unrepresentable>, z=<unrepresentable>, /)\n"
"--\n"
"\n"
"Return a translation table usable for str.translate().\n"
"\n"
"If there is only one argument, it must be a dictionary mapping Unicode\n"
"ordinals (integers) or characters to Unicode ordinals, strings or None.\n"
"Character keys will be then converted to ordinals.\n"
"If there are two arguments, they must be strings of equal length, and\n"
"in the resulting dictionary, each character in x will be mapped to the\n"
"character at the same position in y. If there is a third argument, it\n"
"must be a string, whose characters will be mapped to None in the result.");

#define UNICODE_MAKETRANS_METHODDEF    \
    {"maketrans", (PyCFunction)(void(*)(void))unicode_maketrans, METH_FASTCALL|METH_STATIC, unicode_maketrans__doc__},

static PyObject *
unicode_maketrans_impl(PyObject *x, PyObject *y, PyObject *z);

static PyObject *
unicode_maketrans(void *null, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y = NULL;
    PyObject *z = NULL;

    if (!_PyArg_CheckPositional("maketrans", nargs, 1, 3)) {
        goto exit;
    }
    x = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("maketrans", "argument 2", "str", args[1]);
        goto exit;
    }
    if (PyUnicode_READY(args[1]) == -1) {
        goto exit;
    }
    y = args[1];
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!PyUnicode_Check(args[2])) {
        _PyArg_BadArgument("maketrans", "argument 3", "str", args[2]);
        goto exit;
    }
    if (PyUnicode_READY(args[2]) == -1) {
        goto exit;
    }
    z = args[2];
skip_optional:
    return_value = unicode_maketrans_impl(x, y, z);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_translate__doc__,
"translate($self, table, /)\n"
"--\n"
"\n"
"Replace each character in the string using the given translation table.\n"
"\n"
"  table\n"
"    Translation table, which must be a mapping of Unicode ordinals to\n"
"    Unicode ordinals, strings, or None.\n"
"\n"
"The table must implement lookup/indexing via __getitem__, for instance a\n"
"dictionary or list.  If this operation raises LookupError, the character is\n"
"left untouched.  Characters mapped to None are deleted.");

#define UNICODE_TRANSLATE_METHODDEF    \
    {"translate", (PyCFunction)unicode_translate, METH_O, unicode_translate__doc__},

PyDoc_STRVAR(unicode_upper__doc__,
"upper($self, /)\n"
"--\n"
"\n"
"Return a copy of the string converted to uppercase.");

#define UNICODE_UPPER_METHODDEF    \
    {"upper", (PyCFunction)unicode_upper, METH_NOARGS, unicode_upper__doc__},

static PyObject *
unicode_upper_impl(PyObject *self);

static PyObject *
unicode_upper(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_upper_impl(self);
}

PyDoc_STRVAR(unicode_zfill__doc__,
"zfill($self, width, /)\n"
"--\n"
"\n"
"Pad a numeric string with zeros on the left, to fill a field of the given width.\n"
"\n"
"The string is never truncated.");

#define UNICODE_ZFILL_METHODDEF    \
    {"zfill", (PyCFunction)unicode_zfill, METH_O, unicode_zfill__doc__},

static PyObject *
unicode_zfill_impl(PyObject *self, Py_ssize_t width);

static PyObject *
unicode_zfill(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_ssize_t width;

    if (PyFloat_Check(arg)) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(arg);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        width = ival;
    }
    return_value = unicode_zfill_impl(self, width);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode___format____doc__,
"__format__($self, format_spec, /)\n"
"--\n"
"\n"
"Return a formatted version of the string as described by format_spec.");

#define UNICODE___FORMAT___METHODDEF    \
    {"__format__", (PyCFunction)unicode___format__, METH_O, unicode___format____doc__},

static PyObject *
unicode___format___impl(PyObject *self, PyObject *format_spec);

static PyObject *
unicode___format__(PyObject *self, PyObject *arg)
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
    return_value = unicode___format___impl(self, format_spec);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_sizeof__doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Return the size of the string in memory, in bytes.");

#define UNICODE_SIZEOF_METHODDEF    \
    {"__sizeof__", (PyCFunction)unicode_sizeof, METH_NOARGS, unicode_sizeof__doc__},

static PyObject *
unicode_sizeof_impl(PyObject *self);

static PyObject *
unicode_sizeof(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return unicode_sizeof_impl(self);
}
/*[clinic end generated code: output=b91233f3722643be input=a9049054013a1b77]*/
