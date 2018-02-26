/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(bytearray_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all items from the bytearray.");

#define BYTEARRAY_CLEAR_METHODDEF    \
    {"clear", (PyCFunction)bytearray_clear, METH_NOARGS, bytearray_clear__doc__},

static PyObject *
bytearray_clear_impl(PyByteArrayObject *self);

static PyObject *
bytearray_clear(PyByteArrayObject *self, PyObject *Py_UNUSED(ignored))
{
    return bytearray_clear_impl(self);
}

PyDoc_STRVAR(bytearray_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of B.");

#define BYTEARRAY_COPY_METHODDEF    \
    {"copy", (PyCFunction)bytearray_copy, METH_NOARGS, bytearray_copy__doc__},

static PyObject *
bytearray_copy_impl(PyByteArrayObject *self);

static PyObject *
bytearray_copy(PyByteArrayObject *self, PyObject *Py_UNUSED(ignored))
{
    return bytearray_copy_impl(self);
}

PyDoc_STRVAR(bytearray_translate__doc__,
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

#define BYTEARRAY_TRANSLATE_METHODDEF    \
    {"translate", (PyCFunction)bytearray_translate, METH_FASTCALL, bytearray_translate__doc__},

static PyObject *
bytearray_translate_impl(PyByteArrayObject *self, PyObject *table,
                         PyObject *deletechars);

static PyObject *
bytearray_translate(PyByteArrayObject *self, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "delete", NULL};
    static _PyArg_Parser _parser = {"O|O:translate", _keywords, 0};
    PyObject *table;
    PyObject *deletechars = NULL;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &table, &deletechars)) {
        goto exit;
    }
    return_value = bytearray_translate_impl(self, table, deletechars);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_maketrans__doc__,
"maketrans(frm, to, /)\n"
"--\n"
"\n"
"Return a translation table useable for the bytes or bytearray translate method.\n"
"\n"
"The returned table will be one where each byte in frm is mapped to the byte at\n"
"the same position in to.\n"
"\n"
"The bytes objects frm and to must be of the same length.");

#define BYTEARRAY_MAKETRANS_METHODDEF    \
    {"maketrans", (PyCFunction)bytearray_maketrans, METH_VARARGS|METH_STATIC, bytearray_maketrans__doc__},

static PyObject *
bytearray_maketrans_impl(Py_buffer *frm, Py_buffer *to);

static PyObject *
bytearray_maketrans(void *null, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer frm = {NULL, NULL};
    Py_buffer to = {NULL, NULL};

    if (!PyArg_ParseTuple(args, "y*y*:maketrans",
        &frm, &to)) {
        goto exit;
    }
    return_value = bytearray_maketrans_impl(&frm, &to);

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

PyDoc_STRVAR(bytearray_replace__doc__,
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

#define BYTEARRAY_REPLACE_METHODDEF    \
    {"replace", (PyCFunction)bytearray_replace, METH_VARARGS, bytearray_replace__doc__},

static PyObject *
bytearray_replace_impl(PyByteArrayObject *self, Py_buffer *old,
                       Py_buffer *new, Py_ssize_t count);

static PyObject *
bytearray_replace(PyByteArrayObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer old = {NULL, NULL};
    Py_buffer new = {NULL, NULL};
    Py_ssize_t count = -1;

    if (!PyArg_ParseTuple(args, "y*y*|n:replace",
        &old, &new, &count)) {
        goto exit;
    }
    return_value = bytearray_replace_impl(self, &old, &new, count);

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

PyDoc_STRVAR(bytearray_split__doc__,
"split($self, /, sep=None, maxsplit=-1)\n"
"--\n"
"\n"
"Return a list of the sections in the bytearray, using sep as the delimiter.\n"
"\n"
"  sep\n"
"    The delimiter according which to split the bytearray.\n"
"    None (the default value) means split on ASCII whitespace characters\n"
"    (space, tab, return, newline, formfeed, vertical tab).\n"
"  maxsplit\n"
"    Maximum number of splits to do.\n"
"    -1 (the default value) means no limit.");

#define BYTEARRAY_SPLIT_METHODDEF    \
    {"split", (PyCFunction)bytearray_split, METH_FASTCALL, bytearray_split__doc__},

static PyObject *
bytearray_split_impl(PyByteArrayObject *self, PyObject *sep,
                     Py_ssize_t maxsplit);

static PyObject *
bytearray_split(PyByteArrayObject *self, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"sep", "maxsplit", NULL};
    static _PyArg_Parser _parser = {"|On:split", _keywords, 0};
    PyObject *sep = Py_None;
    Py_ssize_t maxsplit = -1;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &sep, &maxsplit)) {
        goto exit;
    }
    return_value = bytearray_split_impl(self, sep, maxsplit);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_partition__doc__,
"partition($self, sep, /)\n"
"--\n"
"\n"
"Partition the bytearray into three parts using the given separator.\n"
"\n"
"This will search for the separator sep in the bytearray. If the separator is\n"
"found, returns a 3-tuple containing the part before the separator, the\n"
"separator itself, and the part after it as new bytearray objects.\n"
"\n"
"If the separator is not found, returns a 3-tuple containing the copy of the\n"
"original bytearray object and two empty bytearray objects.");

#define BYTEARRAY_PARTITION_METHODDEF    \
    {"partition", (PyCFunction)bytearray_partition, METH_O, bytearray_partition__doc__},

PyDoc_STRVAR(bytearray_rpartition__doc__,
"rpartition($self, sep, /)\n"
"--\n"
"\n"
"Partition the bytearray into three parts using the given separator.\n"
"\n"
"This will search for the separator sep in the bytearray, starting at the end.\n"
"If the separator is found, returns a 3-tuple containing the part before the\n"
"separator, the separator itself, and the part after it as new bytearray\n"
"objects.\n"
"\n"
"If the separator is not found, returns a 3-tuple containing two empty bytearray\n"
"objects and the copy of the original bytearray object.");

#define BYTEARRAY_RPARTITION_METHODDEF    \
    {"rpartition", (PyCFunction)bytearray_rpartition, METH_O, bytearray_rpartition__doc__},

PyDoc_STRVAR(bytearray_rsplit__doc__,
"rsplit($self, /, sep=None, maxsplit=-1)\n"
"--\n"
"\n"
"Return a list of the sections in the bytearray, using sep as the delimiter.\n"
"\n"
"  sep\n"
"    The delimiter according which to split the bytearray.\n"
"    None (the default value) means split on ASCII whitespace characters\n"
"    (space, tab, return, newline, formfeed, vertical tab).\n"
"  maxsplit\n"
"    Maximum number of splits to do.\n"
"    -1 (the default value) means no limit.\n"
"\n"
"Splitting is done starting at the end of the bytearray and working to the front.");

#define BYTEARRAY_RSPLIT_METHODDEF    \
    {"rsplit", (PyCFunction)bytearray_rsplit, METH_FASTCALL, bytearray_rsplit__doc__},

static PyObject *
bytearray_rsplit_impl(PyByteArrayObject *self, PyObject *sep,
                      Py_ssize_t maxsplit);

static PyObject *
bytearray_rsplit(PyByteArrayObject *self, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"sep", "maxsplit", NULL};
    static _PyArg_Parser _parser = {"|On:rsplit", _keywords, 0};
    PyObject *sep = Py_None;
    Py_ssize_t maxsplit = -1;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &sep, &maxsplit)) {
        goto exit;
    }
    return_value = bytearray_rsplit_impl(self, sep, maxsplit);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_reverse__doc__,
"reverse($self, /)\n"
"--\n"
"\n"
"Reverse the order of the values in B in place.");

#define BYTEARRAY_REVERSE_METHODDEF    \
    {"reverse", (PyCFunction)bytearray_reverse, METH_NOARGS, bytearray_reverse__doc__},

static PyObject *
bytearray_reverse_impl(PyByteArrayObject *self);

static PyObject *
bytearray_reverse(PyByteArrayObject *self, PyObject *Py_UNUSED(ignored))
{
    return bytearray_reverse_impl(self);
}

PyDoc_STRVAR(bytearray_insert__doc__,
"insert($self, index, item, /)\n"
"--\n"
"\n"
"Insert a single item into the bytearray before the given index.\n"
"\n"
"  index\n"
"    The index where the value is to be inserted.\n"
"  item\n"
"    The item to be inserted.");

#define BYTEARRAY_INSERT_METHODDEF    \
    {"insert", (PyCFunction)bytearray_insert, METH_VARARGS, bytearray_insert__doc__},

static PyObject *
bytearray_insert_impl(PyByteArrayObject *self, Py_ssize_t index, int item);

static PyObject *
bytearray_insert(PyByteArrayObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_ssize_t index;
    int item;

    if (!PyArg_ParseTuple(args, "nO&:insert",
        &index, _getbytevalue, &item)) {
        goto exit;
    }
    return_value = bytearray_insert_impl(self, index, item);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_append__doc__,
"append($self, item, /)\n"
"--\n"
"\n"
"Append a single item to the end of the bytearray.\n"
"\n"
"  item\n"
"    The item to be appended.");

#define BYTEARRAY_APPEND_METHODDEF    \
    {"append", (PyCFunction)bytearray_append, METH_O, bytearray_append__doc__},

static PyObject *
bytearray_append_impl(PyByteArrayObject *self, int item);

static PyObject *
bytearray_append(PyByteArrayObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int item;

    if (!PyArg_Parse(arg, "O&:append", _getbytevalue, &item)) {
        goto exit;
    }
    return_value = bytearray_append_impl(self, item);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_extend__doc__,
"extend($self, iterable_of_ints, /)\n"
"--\n"
"\n"
"Append all the items from the iterator or sequence to the end of the bytearray.\n"
"\n"
"  iterable_of_ints\n"
"    The iterable of items to append.");

#define BYTEARRAY_EXTEND_METHODDEF    \
    {"extend", (PyCFunction)bytearray_extend, METH_O, bytearray_extend__doc__},

PyDoc_STRVAR(bytearray_pop__doc__,
"pop($self, index=-1, /)\n"
"--\n"
"\n"
"Remove and return a single item from B.\n"
"\n"
"  index\n"
"    The index from where to remove the item.\n"
"    -1 (the default value) means remove the last item.\n"
"\n"
"If no index argument is given, will pop the last item.");

#define BYTEARRAY_POP_METHODDEF    \
    {"pop", (PyCFunction)bytearray_pop, METH_VARARGS, bytearray_pop__doc__},

static PyObject *
bytearray_pop_impl(PyByteArrayObject *self, Py_ssize_t index);

static PyObject *
bytearray_pop(PyByteArrayObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_ssize_t index = -1;

    if (!PyArg_ParseTuple(args, "|n:pop",
        &index)) {
        goto exit;
    }
    return_value = bytearray_pop_impl(self, index);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_remove__doc__,
"remove($self, value, /)\n"
"--\n"
"\n"
"Remove the first occurrence of a value in the bytearray.\n"
"\n"
"  value\n"
"    The value to remove.");

#define BYTEARRAY_REMOVE_METHODDEF    \
    {"remove", (PyCFunction)bytearray_remove, METH_O, bytearray_remove__doc__},

static PyObject *
bytearray_remove_impl(PyByteArrayObject *self, int value);

static PyObject *
bytearray_remove(PyByteArrayObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int value;

    if (!PyArg_Parse(arg, "O&:remove", _getbytevalue, &value)) {
        goto exit;
    }
    return_value = bytearray_remove_impl(self, value);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_strip__doc__,
"strip($self, bytes=None, /)\n"
"--\n"
"\n"
"Strip leading and trailing bytes contained in the argument.\n"
"\n"
"If the argument is omitted or None, strip leading and trailing ASCII whitespace.");

#define BYTEARRAY_STRIP_METHODDEF    \
    {"strip", (PyCFunction)bytearray_strip, METH_VARARGS, bytearray_strip__doc__},

static PyObject *
bytearray_strip_impl(PyByteArrayObject *self, PyObject *bytes);

static PyObject *
bytearray_strip(PyByteArrayObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *bytes = Py_None;

    if (!PyArg_UnpackTuple(args, "strip",
        0, 1,
        &bytes)) {
        goto exit;
    }
    return_value = bytearray_strip_impl(self, bytes);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_lstrip__doc__,
"lstrip($self, bytes=None, /)\n"
"--\n"
"\n"
"Strip leading bytes contained in the argument.\n"
"\n"
"If the argument is omitted or None, strip leading ASCII whitespace.");

#define BYTEARRAY_LSTRIP_METHODDEF    \
    {"lstrip", (PyCFunction)bytearray_lstrip, METH_VARARGS, bytearray_lstrip__doc__},

static PyObject *
bytearray_lstrip_impl(PyByteArrayObject *self, PyObject *bytes);

static PyObject *
bytearray_lstrip(PyByteArrayObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *bytes = Py_None;

    if (!PyArg_UnpackTuple(args, "lstrip",
        0, 1,
        &bytes)) {
        goto exit;
    }
    return_value = bytearray_lstrip_impl(self, bytes);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_rstrip__doc__,
"rstrip($self, bytes=None, /)\n"
"--\n"
"\n"
"Strip trailing bytes contained in the argument.\n"
"\n"
"If the argument is omitted or None, strip trailing ASCII whitespace.");

#define BYTEARRAY_RSTRIP_METHODDEF    \
    {"rstrip", (PyCFunction)bytearray_rstrip, METH_VARARGS, bytearray_rstrip__doc__},

static PyObject *
bytearray_rstrip_impl(PyByteArrayObject *self, PyObject *bytes);

static PyObject *
bytearray_rstrip(PyByteArrayObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *bytes = Py_None;

    if (!PyArg_UnpackTuple(args, "rstrip",
        0, 1,
        &bytes)) {
        goto exit;
    }
    return_value = bytearray_rstrip_impl(self, bytes);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_decode__doc__,
"decode($self, /, encoding=\'utf-8\', errors=\'strict\')\n"
"--\n"
"\n"
"Decode the bytearray using the codec registered for encoding.\n"
"\n"
"  encoding\n"
"    The encoding with which to decode the bytearray.\n"
"  errors\n"
"    The error handling scheme to use for the handling of decoding errors.\n"
"    The default is \'strict\' meaning that decoding errors raise a\n"
"    UnicodeDecodeError. Other possible values are \'ignore\' and \'replace\'\n"
"    as well as any other name registered with codecs.register_error that\n"
"    can handle UnicodeDecodeErrors.");

#define BYTEARRAY_DECODE_METHODDEF    \
    {"decode", (PyCFunction)bytearray_decode, METH_FASTCALL, bytearray_decode__doc__},

static PyObject *
bytearray_decode_impl(PyByteArrayObject *self, const char *encoding,
                      const char *errors);

static PyObject *
bytearray_decode(PyByteArrayObject *self, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"encoding", "errors", NULL};
    static _PyArg_Parser _parser = {"|ss:decode", _keywords, 0};
    const char *encoding = NULL;
    const char *errors = NULL;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &encoding, &errors)) {
        goto exit;
    }
    return_value = bytearray_decode_impl(self, encoding, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_join__doc__,
"join($self, iterable_of_bytes, /)\n"
"--\n"
"\n"
"Concatenate any number of bytes/bytearray objects.\n"
"\n"
"The bytearray whose method is called is inserted in between each pair.\n"
"\n"
"The result is returned as a new bytearray object.");

#define BYTEARRAY_JOIN_METHODDEF    \
    {"join", (PyCFunction)bytearray_join, METH_O, bytearray_join__doc__},

PyDoc_STRVAR(bytearray_splitlines__doc__,
"splitlines($self, /, keepends=False)\n"
"--\n"
"\n"
"Return a list of the lines in the bytearray, breaking at line boundaries.\n"
"\n"
"Line breaks are not included in the resulting list unless keepends is given and\n"
"true.");

#define BYTEARRAY_SPLITLINES_METHODDEF    \
    {"splitlines", (PyCFunction)bytearray_splitlines, METH_FASTCALL, bytearray_splitlines__doc__},

static PyObject *
bytearray_splitlines_impl(PyByteArrayObject *self, int keepends);

static PyObject *
bytearray_splitlines(PyByteArrayObject *self, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"keepends", NULL};
    static _PyArg_Parser _parser = {"|i:splitlines", _keywords, 0};
    int keepends = 0;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &keepends)) {
        goto exit;
    }
    return_value = bytearray_splitlines_impl(self, keepends);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_fromhex__doc__,
"fromhex($type, string, /)\n"
"--\n"
"\n"
"Create a bytearray object from a string of hexadecimal numbers.\n"
"\n"
"Spaces between two numbers are accepted.\n"
"Example: bytearray.fromhex(\'B9 01EF\') -> bytearray(b\'\\\\xb9\\\\x01\\\\xef\')");

#define BYTEARRAY_FROMHEX_METHODDEF    \
    {"fromhex", (PyCFunction)bytearray_fromhex, METH_O|METH_CLASS, bytearray_fromhex__doc__},

static PyObject *
bytearray_fromhex_impl(PyTypeObject *type, PyObject *string);

static PyObject *
bytearray_fromhex(PyTypeObject *type, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *string;

    if (!PyArg_Parse(arg, "U:fromhex", &string)) {
        goto exit;
    }
    return_value = bytearray_fromhex_impl(type, string);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define BYTEARRAY_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)bytearray_reduce, METH_NOARGS, bytearray_reduce__doc__},

static PyObject *
bytearray_reduce_impl(PyByteArrayObject *self);

static PyObject *
bytearray_reduce(PyByteArrayObject *self, PyObject *Py_UNUSED(ignored))
{
    return bytearray_reduce_impl(self);
}

PyDoc_STRVAR(bytearray_reduce_ex__doc__,
"__reduce_ex__($self, proto=0, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define BYTEARRAY_REDUCE_EX_METHODDEF    \
    {"__reduce_ex__", (PyCFunction)bytearray_reduce_ex, METH_VARARGS, bytearray_reduce_ex__doc__},

static PyObject *
bytearray_reduce_ex_impl(PyByteArrayObject *self, int proto);

static PyObject *
bytearray_reduce_ex(PyByteArrayObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int proto = 0;

    if (!PyArg_ParseTuple(args, "|i:__reduce_ex__",
        &proto)) {
        goto exit;
    }
    return_value = bytearray_reduce_ex_impl(self, proto);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_sizeof__doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Returns the size of the bytearray object in memory, in bytes.");

#define BYTEARRAY_SIZEOF_METHODDEF    \
    {"__sizeof__", (PyCFunction)bytearray_sizeof, METH_NOARGS, bytearray_sizeof__doc__},

static PyObject *
bytearray_sizeof_impl(PyByteArrayObject *self);

static PyObject *
bytearray_sizeof(PyByteArrayObject *self, PyObject *Py_UNUSED(ignored))
{
    return bytearray_sizeof_impl(self);
}
/*[clinic end generated code: output=8f022100f059226c input=a9049054013a1b77]*/
