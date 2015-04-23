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
    {"split", (PyCFunction)bytes_split, METH_VARARGS|METH_KEYWORDS, bytes_split__doc__},

static PyObject *
bytes_split_impl(PyBytesObject*self, PyObject *sep, Py_ssize_t maxsplit);

static PyObject *
bytes_split(PyBytesObject*self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"sep", "maxsplit", NULL};
    PyObject *sep = Py_None;
    Py_ssize_t maxsplit = -1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|On:split", _keywords,
        &sep, &maxsplit))
        goto exit;
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

    if (!PyArg_Parse(arg, "y*:partition", &sep))
        goto exit;
    return_value = bytes_partition_impl(self, &sep);

exit:
    /* Cleanup for sep */
    if (sep.obj)
       PyBuffer_Release(&sep);

    return return_value;
}

PyDoc_STRVAR(bytes_rpartition__doc__,
"rpartition($self, sep, /)\n"
"--\n"
"\n"
"Partition the bytes into three parts using the given separator.\n"
"\n"
"This will search for the separator sep in the bytes, starting and the end. If\n"
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

    if (!PyArg_Parse(arg, "y*:rpartition", &sep))
        goto exit;
    return_value = bytes_rpartition_impl(self, &sep);

exit:
    /* Cleanup for sep */
    if (sep.obj)
       PyBuffer_Release(&sep);

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
    {"rsplit", (PyCFunction)bytes_rsplit, METH_VARARGS|METH_KEYWORDS, bytes_rsplit__doc__},

static PyObject *
bytes_rsplit_impl(PyBytesObject*self, PyObject *sep, Py_ssize_t maxsplit);

static PyObject *
bytes_rsplit(PyBytesObject*self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"sep", "maxsplit", NULL};
    PyObject *sep = Py_None;
    Py_ssize_t maxsplit = -1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|On:rsplit", _keywords,
        &sep, &maxsplit))
        goto exit;
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
    {"strip", (PyCFunction)bytes_strip, METH_VARARGS, bytes_strip__doc__},

static PyObject *
bytes_strip_impl(PyBytesObject *self, PyObject *bytes);

static PyObject *
bytes_strip(PyBytesObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *bytes = Py_None;

    if (!PyArg_UnpackTuple(args, "strip",
        0, 1,
        &bytes))
        goto exit;
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
    {"lstrip", (PyCFunction)bytes_lstrip, METH_VARARGS, bytes_lstrip__doc__},

static PyObject *
bytes_lstrip_impl(PyBytesObject *self, PyObject *bytes);

static PyObject *
bytes_lstrip(PyBytesObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *bytes = Py_None;

    if (!PyArg_UnpackTuple(args, "lstrip",
        0, 1,
        &bytes))
        goto exit;
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
    {"rstrip", (PyCFunction)bytes_rstrip, METH_VARARGS, bytes_rstrip__doc__},

static PyObject *
bytes_rstrip_impl(PyBytesObject *self, PyObject *bytes);

static PyObject *
bytes_rstrip(PyBytesObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *bytes = Py_None;

    if (!PyArg_UnpackTuple(args, "rstrip",
        0, 1,
        &bytes))
        goto exit;
    return_value = bytes_rstrip_impl(self, bytes);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_translate__doc__,
"translate(table, [deletechars])\n"
"Return a copy with each character mapped by the given translation table.\n"
"\n"
"  table\n"
"    Translation table, which must be a bytes object of length 256.\n"
"\n"
"All characters occurring in the optional argument deletechars are removed.\n"
"The remaining characters are mapped through the given translation table.");

#define BYTES_TRANSLATE_METHODDEF    \
    {"translate", (PyCFunction)bytes_translate, METH_VARARGS, bytes_translate__doc__},

static PyObject *
bytes_translate_impl(PyBytesObject *self, PyObject *table, int group_right_1,
                     PyObject *deletechars);

static PyObject *
bytes_translate(PyBytesObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *table;
    int group_right_1 = 0;
    PyObject *deletechars = NULL;

    switch (PyTuple_GET_SIZE(args)) {
        case 1:
            if (!PyArg_ParseTuple(args, "O:translate", &table))
                goto exit;
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "OO:translate", &table, &deletechars))
                goto exit;
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "bytes.translate requires 1 to 2 arguments");
            goto exit;
    }
    return_value = bytes_translate_impl(self, table, group_right_1, deletechars);

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
    {"maketrans", (PyCFunction)bytes_maketrans, METH_VARARGS|METH_STATIC, bytes_maketrans__doc__},

static PyObject *
bytes_maketrans_impl(Py_buffer *frm, Py_buffer *to);

static PyObject *
bytes_maketrans(void *null, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer frm = {NULL, NULL};
    Py_buffer to = {NULL, NULL};

    if (!PyArg_ParseTuple(args, "y*y*:maketrans",
        &frm, &to))
        goto exit;
    return_value = bytes_maketrans_impl(&frm, &to);

exit:
    /* Cleanup for frm */
    if (frm.obj)
       PyBuffer_Release(&frm);
    /* Cleanup for to */
    if (to.obj)
       PyBuffer_Release(&to);

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
    {"replace", (PyCFunction)bytes_replace, METH_VARARGS, bytes_replace__doc__},

static PyObject *
bytes_replace_impl(PyBytesObject*self, Py_buffer *old, Py_buffer *new,
                   Py_ssize_t count);

static PyObject *
bytes_replace(PyBytesObject*self, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer old = {NULL, NULL};
    Py_buffer new = {NULL, NULL};
    Py_ssize_t count = -1;

    if (!PyArg_ParseTuple(args, "y*y*|n:replace",
        &old, &new, &count))
        goto exit;
    return_value = bytes_replace_impl(self, &old, &new, count);

exit:
    /* Cleanup for old */
    if (old.obj)
       PyBuffer_Release(&old);
    /* Cleanup for new */
    if (new.obj)
       PyBuffer_Release(&new);

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
    {"decode", (PyCFunction)bytes_decode, METH_VARARGS|METH_KEYWORDS, bytes_decode__doc__},

static PyObject *
bytes_decode_impl(PyBytesObject*self, const char *encoding,
                  const char *errors);

static PyObject *
bytes_decode(PyBytesObject*self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"encoding", "errors", NULL};
    const char *encoding = NULL;
    const char *errors = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ss:decode", _keywords,
        &encoding, &errors))
        goto exit;
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
    {"splitlines", (PyCFunction)bytes_splitlines, METH_VARARGS|METH_KEYWORDS, bytes_splitlines__doc__},

static PyObject *
bytes_splitlines_impl(PyBytesObject*self, int keepends);

static PyObject *
bytes_splitlines(PyBytesObject*self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"keepends", NULL};
    int keepends = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:splitlines", _keywords,
        &keepends))
        goto exit;
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

    if (!PyArg_Parse(arg, "U:fromhex", &string))
        goto exit;
    return_value = bytes_fromhex_impl(type, string);

exit:
    return return_value;
}
/*[clinic end generated code: output=bd0ce8f25d7e18f4 input=a9049054013a1b77]*/
