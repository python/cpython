/*[clinic input]
preserve
[clinic start generated code]*/

static int
bytearray___init___impl(PyByteArrayObject *self, PyObject *arg,
                        const char *encoding, const char *errors);

static int
bytearray___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"source", "encoding", "errors", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "bytearray", 0};
    PyObject *argsbuf[3];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *arg = NULL;
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
        arg = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[1]) {
        if (!PyUnicode_Check(fastargs[1])) {
            _PyArg_BadArgument("bytearray", "argument 'encoding'", "str", fastargs[1]);
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
        _PyArg_BadArgument("bytearray", "argument 'errors'", "str", fastargs[2]);
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
    return_value = bytearray___init___impl((PyByteArrayObject *)self, arg, encoding, errors);

exit:
    return return_value;
}

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

PyDoc_STRVAR(bytearray_removeprefix__doc__,
"removeprefix($self, prefix, /)\n"
"--\n"
"\n"
"Return a bytearray with the given prefix string removed if present.\n"
"\n"
"If the bytearray starts with the prefix string, return\n"
"bytearray[len(prefix):].  Otherwise, return a copy of the original\n"
"bytearray.");

#define BYTEARRAY_REMOVEPREFIX_METHODDEF    \
    {"removeprefix", (PyCFunction)bytearray_removeprefix, METH_O, bytearray_removeprefix__doc__},

static PyObject *
bytearray_removeprefix_impl(PyByteArrayObject *self, Py_buffer *prefix);

static PyObject *
bytearray_removeprefix(PyByteArrayObject *self, PyObject *arg)
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
    return_value = bytearray_removeprefix_impl(self, &prefix);

exit:
    /* Cleanup for prefix */
    if (prefix.obj) {
       PyBuffer_Release(&prefix);
    }

    return return_value;
}

PyDoc_STRVAR(bytearray_removesuffix__doc__,
"removesuffix($self, suffix, /)\n"
"--\n"
"\n"
"Return a bytearray with the given suffix string removed if present.\n"
"\n"
"If the bytearray ends with the suffix string and that suffix is not\n"
"empty, return bytearray[:-len(suffix)].  Otherwise, return a copy of\n"
"the original bytearray.");

#define BYTEARRAY_REMOVESUFFIX_METHODDEF    \
    {"removesuffix", (PyCFunction)bytearray_removesuffix, METH_O, bytearray_removesuffix__doc__},

static PyObject *
bytearray_removesuffix_impl(PyByteArrayObject *self, Py_buffer *suffix);

static PyObject *
bytearray_removesuffix(PyByteArrayObject *self, PyObject *arg)
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
    return_value = bytearray_removesuffix_impl(self, &suffix);

exit:
    /* Cleanup for suffix */
    if (suffix.obj) {
       PyBuffer_Release(&suffix);
    }

    return return_value;
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
    {"translate", (PyCFunction)(void(*)(void))bytearray_translate, METH_FASTCALL|METH_KEYWORDS, bytearray_translate__doc__},

static PyObject *
bytearray_translate_impl(PyByteArrayObject *self, PyObject *table,
                         PyObject *deletechars);

static PyObject *
bytearray_translate(PyByteArrayObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
    {"maketrans", (PyCFunction)(void(*)(void))bytearray_maketrans, METH_FASTCALL|METH_STATIC, bytearray_maketrans__doc__},

static PyObject *
bytearray_maketrans_impl(Py_buffer *frm, Py_buffer *to);

static PyObject *
bytearray_maketrans(void *null, PyObject *const *args, Py_ssize_t nargs)
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
    {"replace", (PyCFunction)(void(*)(void))bytearray_replace, METH_FASTCALL, bytearray_replace__doc__},

static PyObject *
bytearray_replace_impl(PyByteArrayObject *self, Py_buffer *old,
                       Py_buffer *new, Py_ssize_t count);

static PyObject *
bytearray_replace(PyByteArrayObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    {"split", (PyCFunction)(void(*)(void))bytearray_split, METH_FASTCALL|METH_KEYWORDS, bytearray_split__doc__},

static PyObject *
bytearray_split_impl(PyByteArrayObject *self, PyObject *sep,
                     Py_ssize_t maxsplit);

static PyObject *
bytearray_split(PyByteArrayObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
    {"rsplit", (PyCFunction)(void(*)(void))bytearray_rsplit, METH_FASTCALL|METH_KEYWORDS, bytearray_rsplit__doc__},

static PyObject *
bytearray_rsplit_impl(PyByteArrayObject *self, PyObject *sep,
                      Py_ssize_t maxsplit);

static PyObject *
bytearray_rsplit(PyByteArrayObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
    {"insert", (PyCFunction)(void(*)(void))bytearray_insert, METH_FASTCALL, bytearray_insert__doc__},

static PyObject *
bytearray_insert_impl(PyByteArrayObject *self, Py_ssize_t index, int item);

static PyObject *
bytearray_insert(PyByteArrayObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t index;
    int item;

    if (!_PyArg_CheckPositional("insert", nargs, 2, 2)) {
        goto exit;
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
        index = ival;
    }
    if (!_getbytevalue(args[1], &item)) {
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

    if (!_getbytevalue(arg, &item)) {
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
    {"pop", (PyCFunction)(void(*)(void))bytearray_pop, METH_FASTCALL, bytearray_pop__doc__},

static PyObject *
bytearray_pop_impl(PyByteArrayObject *self, Py_ssize_t index);

static PyObject *
bytearray_pop(PyByteArrayObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t index = -1;

    if (!_PyArg_CheckPositional("pop", nargs, 0, 1)) {
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
        index = ival;
    }
skip_optional:
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

    if (!_getbytevalue(arg, &value)) {
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
    {"strip", (PyCFunction)(void(*)(void))bytearray_strip, METH_FASTCALL, bytearray_strip__doc__},

static PyObject *
bytearray_strip_impl(PyByteArrayObject *self, PyObject *bytes);

static PyObject *
bytearray_strip(PyByteArrayObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    {"lstrip", (PyCFunction)(void(*)(void))bytearray_lstrip, METH_FASTCALL, bytearray_lstrip__doc__},

static PyObject *
bytearray_lstrip_impl(PyByteArrayObject *self, PyObject *bytes);

static PyObject *
bytearray_lstrip(PyByteArrayObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    {"rstrip", (PyCFunction)(void(*)(void))bytearray_rstrip, METH_FASTCALL, bytearray_rstrip__doc__},

static PyObject *
bytearray_rstrip_impl(PyByteArrayObject *self, PyObject *bytes);

static PyObject *
bytearray_rstrip(PyByteArrayObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    {"decode", (PyCFunction)(void(*)(void))bytearray_decode, METH_FASTCALL|METH_KEYWORDS, bytearray_decode__doc__},

static PyObject *
bytearray_decode_impl(PyByteArrayObject *self, const char *encoding,
                      const char *errors);

static PyObject *
bytearray_decode(PyByteArrayObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
    {"splitlines", (PyCFunction)(void(*)(void))bytearray_splitlines, METH_FASTCALL|METH_KEYWORDS, bytearray_splitlines__doc__},

static PyObject *
bytearray_splitlines_impl(PyByteArrayObject *self, int keepends);

static PyObject *
bytearray_splitlines(PyByteArrayObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("fromhex", "argument", "str", arg);
        goto exit;
    }
    if (PyUnicode_READY(arg) == -1) {
        goto exit;
    }
    string = arg;
    return_value = bytearray_fromhex_impl(type, string);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_hex__doc__,
"hex($self, /, sep=<unrepresentable>, bytes_per_sep=1)\n"
"--\n"
"\n"
"Create a string of hexadecimal numbers from a bytearray object.\n"
"\n"
"  sep\n"
"    An optional single character or byte to separate hex bytes.\n"
"  bytes_per_sep\n"
"    How many bytes between separators.  Positive values count from the\n"
"    right, negative values count from the left.\n"
"\n"
"Example:\n"
">>> value = bytearray([0xb9, 0x01, 0xef])\n"
">>> value.hex()\n"
"\'b901ef\'\n"
">>> value.hex(\':\')\n"
"\'b9:01:ef\'\n"
">>> value.hex(\':\', 2)\n"
"\'b9:01ef\'\n"
">>> value.hex(\':\', -2)\n"
"\'b901:ef\'");

#define BYTEARRAY_HEX_METHODDEF    \
    {"hex", (PyCFunction)(void(*)(void))bytearray_hex, METH_FASTCALL|METH_KEYWORDS, bytearray_hex__doc__},

static PyObject *
bytearray_hex_impl(PyByteArrayObject *self, PyObject *sep, int bytes_per_sep);

static PyObject *
bytearray_hex(PyByteArrayObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
    return_value = bytearray_hex_impl(self, sep, bytes_per_sep);

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
    {"__reduce_ex__", (PyCFunction)(void(*)(void))bytearray_reduce_ex, METH_FASTCALL, bytearray_reduce_ex__doc__},

static PyObject *
bytearray_reduce_ex_impl(PyByteArrayObject *self, int proto);

static PyObject *
bytearray_reduce_ex(PyByteArrayObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int proto = 0;

    if (!_PyArg_CheckPositional("__reduce_ex__", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    proto = _PyLong_AsInt(args[0]);
    if (proto == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
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
/*[clinic end generated code: output=a82659f581e55629 input=a9049054013a1b77]*/
