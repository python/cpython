/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(bytes_split__doc__,
"split($self, /, sep=None, maxsplit=-1)\n"
"--\n"
"\n"
"Return a list of the sections in the bytes, using sep as the delimiter.\n"
"\n"
"  sep\n"
"    The delimiter according which to split the bytes.\n"
"    None (the default value) means split on ASCII whitespace characters\n"
"    (space, tab, return, newline, formfeed, vertical tab).\n"
"  maxsplit\n"
"    Maximum number of splits to do.\n"
"    -1 (the default value) means no limit.");

#define BYTES_SPLIT_METHODDEF    \
    {"split", (PyCFunction)(void(*)(void))bytes_split, METH_FASTCALL|METH_KEYWORDS, bytes_split__doc__},

static PyObject *
bytes_split_impl(PyBytesObject *self, PyObject *sep, Py_ssize_t maxsplit);

static PyObject *
bytes_split(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        maxsplit = ival;
    }
skip_optional_pos:
    return_value = bytes_split_impl(self, sep, maxsplit);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_partition__doc__,
"partition($self, sep, /)\n"
"--\n"
"\n"
"Partition the bytes into three parts using the given separator.\n"
"\n"
"This will search for the separator sep in the bytes. If the separator is found,\n"
"returns a 3-tuple containing the part before the separator, the separator\n"
"itself, and the part after it.\n"
"\n"
"If the separator is not found, returns a 3-tuple containing the original bytes\n"
"object and two empty bytes objects.");

#define BYTES_PARTITION_METHODDEF    \
    {"partition", (PyCFunction)bytes_partition, METH_O, bytes_partition__doc__},

static PyObject *
bytes_partition_impl(PyBytesObject *self, Py_buffer *sep);

static PyObject *
bytes_partition(PyBytesObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer sep = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &sep, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&sep, 'C')) {
        _PyArg_BadArgument("partition", "argument", "contiguous buffer", arg);
        goto exit;
    }
    return_value = bytes_partition_impl(self, &sep);

exit:
    /* Cleanup for sep */
    if (sep.obj) {
       PyBuffer_Release(&sep);
    }

    return return_value;
}

PyDoc_STRVAR(bytes_rpartition__doc__,
"rpartition($self, sep, /)\n"
"--\n"
"\n"
"Partition the bytes into three parts using the given separator.\n"
"\n"
"This will search for the separator sep in the bytes, starting at the end. If\n"
"the separator is found, returns a 3-tuple containing the part before the\n"
"separator, the separator itself, and the part after it.\n"
"\n"
"If the separator is not found, returns a 3-tuple containing two empty bytes\n"
"objects and the original bytes object.");

#define BYTES_RPARTITION_METHODDEF    \
    {"rpartition", (PyCFunction)bytes_rpartition, METH_O, bytes_rpartition__doc__},

static PyObject *
bytes_rpartition_impl(PyBytesObject *self, Py_buffer *sep);

static PyObject *
bytes_rpartition(PyBytesObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer sep = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &sep, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&sep, 'C')) {
        _PyArg_BadArgument("rpartition", "argument", "contiguous buffer", arg);
        goto exit;
    }
    return_value = bytes_rpartition_impl(self, &sep);

exit:
    /* Cleanup for sep */
    if (sep.obj) {
       PyBuffer_Release(&sep);
    }

    return return_value;
}

PyDoc_STRVAR(bytes_rsplit__doc__,
"rsplit($self, /, sep=None, maxsplit=-1)\n"
"--\n"
"\n"
"Return a list of the sections in the bytes, using sep as the delimiter.\n"
"\n"
"  sep\n"
"    The delimiter according which to split the bytes.\n"
"    None (the default value) means split on ASCII whitespace characters\n"
"    (space, tab, return, newline, formfeed, vertical tab).\n"
"  maxsplit\n"
"    Maximum number of splits to do.\n"
"    -1 (the default value) means no limit.\n"
"\n"
"Splitting is done starting at the end of the bytes and working to the front.");

#define BYTES_RSPLIT_METHODDEF    \
    {"rsplit", (PyCFunction)(void(*)(void))bytes_rsplit, METH_FASTCALL|METH_KEYWORDS, bytes_rsplit__doc__},

static PyObject *
bytes_rsplit_impl(PyBytesObject *self, PyObject *sep, Py_ssize_t maxsplit);

static PyObject *
bytes_rsplit(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        maxsplit = ival;
    }
skip_optional_pos:
    return_value = bytes_rsplit_impl(self, sep, maxsplit);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_join__doc__,
"join($self, iterable_of_bytes, /)\n"
"--\n"
"\n"
"Concatenate any number of bytes objects.\n"
"\n"
"The bytes whose method is called is inserted in between each pair.\n"
"\n"
"The result is returned as a new bytes object.\n"
"\n"
"Example: b\'.\'.join([b\'ab\', b\'pq\', b\'rs\']) -> b\'ab.pq.rs\'.");

#define BYTES_JOIN_METHODDEF    \
    {"join", (PyCFunction)bytes_join, METH_O, bytes_join__doc__},

PyDoc_STRVAR(bytes_strip__doc__,
"strip($self, bytes=None, /)\n"
"--\n"
"\n"
"Strip leading and trailing bytes contained in the argument.\n"
"\n"
"If the argument is omitted or None, strip leading and trailing ASCII whitespace.");

#define BYTES_STRIP_METHODDEF    \
    {"strip", (PyCFunction)(void(*)(void))bytes_strip, METH_FASTCALL, bytes_strip__doc__},

static PyObject *
bytes_strip_impl(PyBytesObject *self, PyObject *bytes);

static PyObject *
bytes_strip(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *bytes = Py_None;

    if (!_PyArg_CheckPositional("strip", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    bytes = args[0];
skip_optional:
    return_value = bytes_strip_impl(self, bytes);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_lstrip__doc__,
"lstrip($self, bytes=None, /)\n"
"--\n"
"\n"
"Strip leading bytes contained in the argument.\n"
"\n"
"If the argument is omitted or None, strip leading  ASCII whitespace.");

#define BYTES_LSTRIP_METHODDEF    \
    {"lstrip", (PyCFunction)(void(*)(void))bytes_lstrip, METH_FASTCALL, bytes_lstrip__doc__},

static PyObject *
bytes_lstrip_impl(PyBytesObject *self, PyObject *bytes);

static PyObject *
bytes_lstrip(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *bytes = Py_None;

    if (!_PyArg_CheckPositional("lstrip", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    bytes = args[0];
skip_optional:
    return_value = bytes_lstrip_impl(self, bytes);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_rstrip__doc__,
"rstrip($self, bytes=None, /)\n"
"--\n"
"\n"
"Strip trailing bytes contained in the argument.\n"
"\n"
"If the argument is omitted or None, strip trailing ASCII whitespace.");

#define BYTES_RSTRIP_METHODDEF    \
    {"rstrip", (PyCFunction)(void(*)(void))bytes_rstrip, METH_FASTCALL, bytes_rstrip__doc__},

static PyObject *
bytes_rstrip_impl(PyBytesObject *self, PyObject *bytes);

static PyObject *
bytes_rstrip(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *bytes = Py_None;

    if (!_PyArg_CheckPositional("rstrip", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    bytes = args[0];
skip_optional:
    return_value = bytes_rstrip_impl(self, bytes);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_translate__doc__,
"translate($self, table, /, delete=b\'\')\n"
"--\n"
"\n"
"Return a copy with each character mapped by the given translation table.\n"
"\n"
"  table\n"
"    Translation table, which must be a bytes object of length 256.\n"
"\n"
"All characters occurring in the optional argument delete are removed.\n"
"The remaining characters are mapped through the given translation table.");

#define BYTES_TRANSLATE_METHODDEF    \
    {"translate", (PyCFunction)(void(*)(void))bytes_translate, METH_FASTCALL|METH_KEYWORDS, bytes_translate__doc__},

static PyObject *
bytes_translate_impl(PyBytesObject *self, PyObject *table,
                     PyObject *deletechars);

static PyObject *
bytes_translate(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "delete", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "translate", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *table;
    PyObject *deletechars = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    table = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    deletechars = args[1];
skip_optional_pos:
    return_value = bytes_translate_impl(self, table, deletechars);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_maketrans__doc__,
"maketrans(frm, to, /)\n"
"--\n"
"\n"
"Return a translation table useable for the bytes or bytearray translate method.\n"
"\n"
"The returned table will be one where each byte in frm is mapped to the byte at\n"
"the same position in to.\n"
"\n"
"The bytes objects frm and to must be of the same length.");

#define BYTES_MAKETRANS_METHODDEF    \
    {"maketrans", (PyCFunction)(void(*)(void))bytes_maketrans, METH_FASTCALL|METH_STATIC, bytes_maketrans__doc__},

static PyObject *
bytes_maketrans_impl(Py_buffer *frm, Py_buffer *to);

static PyObject *
bytes_maketrans(void *null, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer frm = {NULL, NULL};
    Py_buffer to = {NULL, NULL};

    if (!_PyArg_CheckPositional("maketrans", nargs, 2, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &frm, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&frm, 'C')) {
        _PyArg_BadArgument("maketrans", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &to, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&to, 'C')) {
        _PyArg_BadArgument("maketrans", "argument 2", "contiguous buffer", args[1]);
        goto exit;
    }
    return_value = bytes_maketrans_impl(&frm, &to);

exit:
    /* Cleanup for frm */
    if (frm.obj) {
       PyBuffer_Release(&frm);
    }
    /* Cleanup for to */
    if (to.obj) {
       PyBuffer_Release(&to);
    }

    return return_value;
}

PyDoc_STRVAR(bytes_replace__doc__,
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

#define BYTES_REPLACE_METHODDEF    \
    {"replace", (PyCFunction)(void(*)(void))bytes_replace, METH_FASTCALL, bytes_replace__doc__},

static PyObject *
bytes_replace_impl(PyBytesObject *self, Py_buffer *old, Py_buffer *new,
                   Py_ssize_t count);

static PyObject *
bytes_replace(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer old = {NULL, NULL};
    Py_buffer new = {NULL, NULL};
    Py_ssize_t count = -1;

    if (!_PyArg_CheckPositional("replace", nargs, 2, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &old, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&old, 'C')) {
        _PyArg_BadArgument("replace", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &new, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&new, 'C')) {
        _PyArg_BadArgument("replace", "argument 2", "contiguous buffer", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[2]);
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
    return_value = bytes_replace_impl(self, &old, &new, count);

exit:
    /* Cleanup for old */
    if (old.obj) {
       PyBuffer_Release(&old);
    }
    /* Cleanup for new */
    if (new.obj) {
       PyBuffer_Release(&new);
    }

    return return_value;
}

PyDoc_STRVAR(bytes_removeprefix__doc__,
"removeprefix($self, prefix, /)\n"
"--\n"
"\n"
"Return a bytes object with the given prefix string removed if present.\n"
"\n"
"If the bytes starts with the prefix string, return bytes[len(prefix):].\n"
"Otherwise, return a copy of the original bytes.");

#define BYTES_REMOVEPREFIX_METHODDEF    \
    {"removeprefix", (PyCFunction)bytes_removeprefix, METH_O, bytes_removeprefix__doc__},

static PyObject *
bytes_removeprefix_impl(PyBytesObject *self, Py_buffer *prefix);

static PyObject *
bytes_removeprefix(PyBytesObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer prefix = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &prefix, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&prefix, 'C')) {
        _PyArg_BadArgument("removeprefix", "argument", "contiguous buffer", arg);
        goto exit;
    }
    return_value = bytes_removeprefix_impl(self, &prefix);

exit:
    /* Cleanup for prefix */
    if (prefix.obj) {
       PyBuffer_Release(&prefix);
    }

    return return_value;
}

PyDoc_STRVAR(bytes_removesuffix__doc__,
"removesuffix($self, suffix, /)\n"
"--\n"
"\n"
"Return a bytes object with the given suffix string removed if present.\n"
"\n"
"If the bytes ends with the suffix string and that suffix is not empty,\n"
"return bytes[:-len(prefix)].  Otherwise, return a copy of the original\n"
"bytes.");

#define BYTES_REMOVESUFFIX_METHODDEF    \
    {"removesuffix", (PyCFunction)bytes_removesuffix, METH_O, bytes_removesuffix__doc__},

static PyObject *
bytes_removesuffix_impl(PyBytesObject *self, Py_buffer *suffix);

static PyObject *
bytes_removesuffix(PyBytesObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer suffix = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &suffix, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&suffix, 'C')) {
        _PyArg_BadArgument("removesuffix", "argument", "contiguous buffer", arg);
        goto exit;
    }
    return_value = bytes_removesuffix_impl(self, &suffix);

exit:
    /* Cleanup for suffix */
    if (suffix.obj) {
       PyBuffer_Release(&suffix);
    }

    return return_value;
}

PyDoc_STRVAR(bytes_decode__doc__,
"decode($self, /, encoding=\'utf-8\', errors=\'strict\')\n"
"--\n"
"\n"
"Decode the bytes using the codec registered for encoding.\n"
"\n"
"  encoding\n"
"    The encoding with which to decode the bytes.\n"
"  errors\n"
"    The error handling scheme to use for the handling of decoding errors.\n"
"    The default is \'strict\' meaning that decoding errors raise a\n"
"    UnicodeDecodeError. Other possible values are \'ignore\' and \'replace\'\n"
"    as well as any other name registered with codecs.register_error that\n"
"    can handle UnicodeDecodeErrors.");

#define BYTES_DECODE_METHODDEF    \
    {"decode", (PyCFunction)(void(*)(void))bytes_decode, METH_FASTCALL|METH_KEYWORDS, bytes_decode__doc__},

static PyObject *
bytes_decode_impl(PyBytesObject *self, const char *encoding,
                  const char *errors);

static PyObject *
bytes_decode(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"encoding", "errors", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "decode", 0};
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
            _PyArg_BadArgument("decode", "argument 'encoding'", "str", args[0]);
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
        _PyArg_BadArgument("decode", "argument 'errors'", "str", args[1]);
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
    return_value = bytes_decode_impl(self, encoding, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_splitlines__doc__,
"splitlines($self, /, keepends=False)\n"
"--\n"
"\n"
"Return a list of the lines in the bytes, breaking at line boundaries.\n"
"\n"
"Line breaks are not included in the resulting list unless keepends is given and\n"
"true.");

#define BYTES_SPLITLINES_METHODDEF    \
    {"splitlines", (PyCFunction)(void(*)(void))bytes_splitlines, METH_FASTCALL|METH_KEYWORDS, bytes_splitlines__doc__},

static PyObject *
bytes_splitlines_impl(PyBytesObject *self, int keepends);

static PyObject *
bytes_splitlines(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
    keepends = _PyLong_AsInt(args[0]);
    if (keepends == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = bytes_splitlines_impl(self, keepends);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_fromhex__doc__,
"fromhex($type, string, /)\n"
"--\n"
"\n"
"Create a bytes object from a string of hexadecimal numbers.\n"
"\n"
"Spaces between two numbers are accepted.\n"
"Example: bytes.fromhex(\'B9 01EF\') -> b\'\\\\xb9\\\\x01\\\\xef\'.");

#define BYTES_FROMHEX_METHODDEF    \
    {"fromhex", (PyCFunction)bytes_fromhex, METH_O|METH_CLASS, bytes_fromhex__doc__},

static PyObject *
bytes_fromhex_impl(PyTypeObject *type, PyObject *string);

static PyObject *
bytes_fromhex(PyTypeObject *type, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *string;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("fromhex", "argument", "str", arg);
        goto exit;
    }
    if (PyUnicode_READY(arg) == -1) {
        goto exit;
    }
    string = arg;
    return_value = bytes_fromhex_impl(type, string);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_hex__doc__,
"hex($self, /, sep=<unrepresentable>, bytes_per_sep=1)\n"
"--\n"
"\n"
"Create a str of hexadecimal numbers from a bytes object.\n"
"\n"
"  sep\n"
"    An optional single character or byte to separate hex bytes.\n"
"  bytes_per_sep\n"
"    How many bytes between separators.  Positive values count from the\n"
"    right, negative values count from the left.\n"
"\n"
"Example:\n"
">>> value = b\'\\xb9\\x01\\xef\'\n"
">>> value.hex()\n"
"\'b901ef\'\n"
">>> value.hex(\':\')\n"
"\'b9:01:ef\'\n"
">>> value.hex(\':\', 2)\n"
"\'b9:01ef\'\n"
">>> value.hex(\':\', -2)\n"
"\'b901:ef\'");

#define BYTES_HEX_METHODDEF    \
    {"hex", (PyCFunction)(void(*)(void))bytes_hex, METH_FASTCALL|METH_KEYWORDS, bytes_hex__doc__},

static PyObject *
bytes_hex_impl(PyBytesObject *self, PyObject *sep, int bytes_per_sep);

static PyObject *
bytes_hex(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"sep", "bytes_per_sep", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "hex", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *sep = NULL;
    int bytes_per_sep = 1;

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
    bytes_per_sep = _PyLong_AsInt(args[1]);
    if (bytes_per_sep == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = bytes_hex_impl(self, sep, bytes_per_sep);

exit:
    return return_value;
}

static PyObject *
bytes_new_impl(PyTypeObject *type, PyObject *x, const char *encoding,
               const char *errors);

static PyObject *
bytes_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"source", "encoding", "errors", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "bytes", 0};
    PyObject *argsbuf[3];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *x = NULL;
    const char *encoding = NULL;
    const char *errors = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 0, 3, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        x = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[1]) {
        if (!PyUnicode_Check(fastargs[1])) {
            _PyArg_BadArgument("bytes", "argument 'encoding'", "str", fastargs[1]);
            goto exit;
        }
        Py_ssize_t encoding_length;
        encoding = PyUnicode_AsUTF8AndSize(fastargs[1], &encoding_length);
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
    if (!PyUnicode_Check(fastargs[2])) {
        _PyArg_BadArgument("bytes", "argument 'errors'", "str", fastargs[2]);
        goto exit;
    }
    Py_ssize_t errors_length;
    errors = PyUnicode_AsUTF8AndSize(fastargs[2], &errors_length);
    if (errors == NULL) {
        goto exit;
    }
    if (strlen(errors) != (size_t)errors_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
skip_optional_pos:
    return_value = bytes_new_impl(type, x, encoding, errors);

exit:
    return return_value;
}
/*[clinic end generated code: output=6101b417d6a6a717 input=a9049054013a1b77]*/
