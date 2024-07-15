/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(bytes___bytes____doc__,
"__bytes__($self, /)\n"
"--\n"
"\n"
"Convert this value to exact type bytes.");

#define BYTES___BYTES___METHODDEF    \
    {"__bytes__", (PyCFunction)bytes___bytes__, METH_NOARGS, bytes___bytes____doc__},

static PyObject *
bytes___bytes___impl(PyBytesObject *self);

static PyObject *
bytes___bytes__(PyBytesObject *self, PyObject *Py_UNUSED(ignored))
{
    return bytes___bytes___impl(self);
}

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
    {"split", _PyCFunction_CAST(bytes_split), METH_FASTCALL|METH_KEYWORDS, bytes_split__doc__},

static PyObject *
bytes_split_impl(PyBytesObject *self, PyObject *sep, Py_ssize_t maxsplit);

static PyObject *
bytes_split(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(sep), &_Py_ID(maxsplit), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"sep", "maxsplit", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "split",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
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
    {"rsplit", _PyCFunction_CAST(bytes_rsplit), METH_FASTCALL|METH_KEYWORDS, bytes_rsplit__doc__},

static PyObject *
bytes_rsplit_impl(PyBytesObject *self, PyObject *sep, Py_ssize_t maxsplit);

static PyObject *
bytes_rsplit(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(sep), &_Py_ID(maxsplit), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"sep", "maxsplit", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "rsplit",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
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

PyDoc_STRVAR(bytes_find__doc__,
"find($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the lowest index in B where subsection \'sub\' is found, such that \'sub\' is contained within B[start,end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.\n"
"\n"
"Return -1 on failure.");

#define BYTES_FIND_METHODDEF    \
    {"find", _PyCFunction_CAST(bytes_find), METH_FASTCALL, bytes_find__doc__},

static PyObject *
bytes_find_impl(PyBytesObject *self, PyObject *sub, Py_ssize_t start,
                Py_ssize_t end);

static PyObject *
bytes_find(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_CheckPositional("find", nargs, 1, 3)) {
        goto exit;
    }
    sub = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndex(args[1], &start)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndex(args[2], &end)) {
        goto exit;
    }
skip_optional:
    return_value = bytes_find_impl(self, sub, start, end);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_index__doc__,
"index($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the lowest index in B where subsection \'sub\' is found, such that \'sub\' is contained within B[start,end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.\n"
"\n"
"Raise ValueError if the subsection is not found.");

#define BYTES_INDEX_METHODDEF    \
    {"index", _PyCFunction_CAST(bytes_index), METH_FASTCALL, bytes_index__doc__},

static PyObject *
bytes_index_impl(PyBytesObject *self, PyObject *sub, Py_ssize_t start,
                 Py_ssize_t end);

static PyObject *
bytes_index(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_CheckPositional("index", nargs, 1, 3)) {
        goto exit;
    }
    sub = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndex(args[1], &start)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndex(args[2], &end)) {
        goto exit;
    }
skip_optional:
    return_value = bytes_index_impl(self, sub, start, end);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_rfind__doc__,
"rfind($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the highest index in B where subsection \'sub\' is found, such that \'sub\' is contained within B[start,end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.\n"
"\n"
"Return -1 on failure.");

#define BYTES_RFIND_METHODDEF    \
    {"rfind", _PyCFunction_CAST(bytes_rfind), METH_FASTCALL, bytes_rfind__doc__},

static PyObject *
bytes_rfind_impl(PyBytesObject *self, PyObject *sub, Py_ssize_t start,
                 Py_ssize_t end);

static PyObject *
bytes_rfind(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_CheckPositional("rfind", nargs, 1, 3)) {
        goto exit;
    }
    sub = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndex(args[1], &start)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndex(args[2], &end)) {
        goto exit;
    }
skip_optional:
    return_value = bytes_rfind_impl(self, sub, start, end);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_rindex__doc__,
"rindex($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the highest index in B where subsection \'sub\' is found, such that \'sub\' is contained within B[start,end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.\n"
"\n"
"Raise ValueError if the subsection is not found.");

#define BYTES_RINDEX_METHODDEF    \
    {"rindex", _PyCFunction_CAST(bytes_rindex), METH_FASTCALL, bytes_rindex__doc__},

static PyObject *
bytes_rindex_impl(PyBytesObject *self, PyObject *sub, Py_ssize_t start,
                  Py_ssize_t end);

static PyObject *
bytes_rindex(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_CheckPositional("rindex", nargs, 1, 3)) {
        goto exit;
    }
    sub = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndex(args[1], &start)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndex(args[2], &end)) {
        goto exit;
    }
skip_optional:
    return_value = bytes_rindex_impl(self, sub, start, end);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_strip__doc__,
"strip($self, bytes=None, /)\n"
"--\n"
"\n"
"Strip leading and trailing bytes contained in the argument.\n"
"\n"
"If the argument is omitted or None, strip leading and trailing ASCII whitespace.");

#define BYTES_STRIP_METHODDEF    \
    {"strip", _PyCFunction_CAST(bytes_strip), METH_FASTCALL, bytes_strip__doc__},

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
    {"lstrip", _PyCFunction_CAST(bytes_lstrip), METH_FASTCALL, bytes_lstrip__doc__},

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
    {"rstrip", _PyCFunction_CAST(bytes_rstrip), METH_FASTCALL, bytes_rstrip__doc__},

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

PyDoc_STRVAR(bytes_count__doc__,
"count($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the number of non-overlapping occurrences of subsection \'sub\' in bytes B[start:end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.");

#define BYTES_COUNT_METHODDEF    \
    {"count", _PyCFunction_CAST(bytes_count), METH_FASTCALL, bytes_count__doc__},

static PyObject *
bytes_count_impl(PyBytesObject *self, PyObject *sub, Py_ssize_t start,
                 Py_ssize_t end);

static PyObject *
bytes_count(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_CheckPositional("count", nargs, 1, 3)) {
        goto exit;
    }
    sub = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndex(args[1], &start)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndex(args[2], &end)) {
        goto exit;
    }
skip_optional:
    return_value = bytes_count_impl(self, sub, start, end);

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
    {"translate", _PyCFunction_CAST(bytes_translate), METH_FASTCALL|METH_KEYWORDS, bytes_translate__doc__},

static PyObject *
bytes_translate_impl(PyBytesObject *self, PyObject *table,
                     PyObject *deletechars);

static PyObject *
bytes_translate(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(delete), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "delete", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "translate",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
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
"Return a translation table usable for the bytes or bytearray translate method.\n"
"\n"
"The returned table will be one where each byte in frm is mapped to the byte at\n"
"the same position in to.\n"
"\n"
"The bytes objects frm and to must be of the same length.");

#define BYTES_MAKETRANS_METHODDEF    \
    {"maketrans", _PyCFunction_CAST(bytes_maketrans), METH_FASTCALL|METH_STATIC, bytes_maketrans__doc__},

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
    if (PyObject_GetBuffer(args[1], &to, PyBUF_SIMPLE) != 0) {
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
    {"replace", _PyCFunction_CAST(bytes_replace), METH_FASTCALL, bytes_replace__doc__},

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
    if (PyObject_GetBuffer(args[1], &new, PyBUF_SIMPLE) != 0) {
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
    return_value = bytes_removesuffix_impl(self, &suffix);

exit:
    /* Cleanup for suffix */
    if (suffix.obj) {
       PyBuffer_Release(&suffix);
    }

    return return_value;
}

PyDoc_STRVAR(bytes_startswith__doc__,
"startswith($self, prefix[, start[, end]], /)\n"
"--\n"
"\n"
"Return True if the bytes starts with the specified prefix, False otherwise.\n"
"\n"
"  prefix\n"
"    A bytes or a tuple of bytes to try.\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.");

#define BYTES_STARTSWITH_METHODDEF    \
    {"startswith", _PyCFunction_CAST(bytes_startswith), METH_FASTCALL, bytes_startswith__doc__},

static PyObject *
bytes_startswith_impl(PyBytesObject *self, PyObject *subobj,
                      Py_ssize_t start, Py_ssize_t end);

static PyObject *
bytes_startswith(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *subobj;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_CheckPositional("startswith", nargs, 1, 3)) {
        goto exit;
    }
    subobj = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndex(args[1], &start)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndex(args[2], &end)) {
        goto exit;
    }
skip_optional:
    return_value = bytes_startswith_impl(self, subobj, start, end);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_endswith__doc__,
"endswith($self, suffix[, start[, end]], /)\n"
"--\n"
"\n"
"Return True if the bytes ends with the specified suffix, False otherwise.\n"
"\n"
"  suffix\n"
"    A bytes or a tuple of bytes to try.\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.");

#define BYTES_ENDSWITH_METHODDEF    \
    {"endswith", _PyCFunction_CAST(bytes_endswith), METH_FASTCALL, bytes_endswith__doc__},

static PyObject *
bytes_endswith_impl(PyBytesObject *self, PyObject *subobj, Py_ssize_t start,
                    Py_ssize_t end);

static PyObject *
bytes_endswith(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *subobj;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_CheckPositional("endswith", nargs, 1, 3)) {
        goto exit;
    }
    subobj = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndex(args[1], &start)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndex(args[2], &end)) {
        goto exit;
    }
skip_optional:
    return_value = bytes_endswith_impl(self, subobj, start, end);

exit:
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
    {"decode", _PyCFunction_CAST(bytes_decode), METH_FASTCALL|METH_KEYWORDS, bytes_decode__doc__},

static PyObject *
bytes_decode_impl(PyBytesObject *self, const char *encoding,
                  const char *errors);

static PyObject *
bytes_decode(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(encoding), &_Py_ID(errors), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"encoding", "errors", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "decode",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
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
    {"splitlines", _PyCFunction_CAST(bytes_splitlines), METH_FASTCALL|METH_KEYWORDS, bytes_splitlines__doc__},

static PyObject *
bytes_splitlines_impl(PyBytesObject *self, int keepends);

static PyObject *
bytes_splitlines(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(keepends), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"keepends", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "splitlines",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
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
    keepends = PyObject_IsTrue(args[0]);
    if (keepends < 0) {
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
    string = arg;
    return_value = bytes_fromhex_impl(type, string);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_hex__doc__,
"hex($self, /, sep=<unrepresentable>, bytes_per_sep=1)\n"
"--\n"
"\n"
"Create a string of hexadecimal numbers from a bytes object.\n"
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
    {"hex", _PyCFunction_CAST(bytes_hex), METH_FASTCALL|METH_KEYWORDS, bytes_hex__doc__},

static PyObject *
bytes_hex_impl(PyBytesObject *self, PyObject *sep, int bytes_per_sep);

static PyObject *
bytes_hex(PyBytesObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(sep), &_Py_ID(bytes_per_sep), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"sep", "bytes_per_sep", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "hex",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
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
    bytes_per_sep = PyLong_AsInt(args[1]);
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
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(source), &_Py_ID(encoding), &_Py_ID(errors), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"source", "encoding", "errors", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "bytes",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
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
/*[clinic end generated code: output=d6801c6001e57f91 input=a9049054013a1b77]*/
