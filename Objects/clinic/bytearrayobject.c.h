/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

static int
bytearray___init___impl(PyByteArrayObject *self, PyObject *arg,
                        const char *encoding, const char *errors);

static int
bytearray___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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
        .fname = "bytearray",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *arg = NULL;
    const char *encoding = NULL;
    const char *errors = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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

PyDoc_STRVAR(bytearray_find__doc__,
"find($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the lowest index in B where subsection \'sub\' is found, such that \'sub\' is contained within B[start:end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.\n"
"\n"
"Return -1 on failure.");

#define BYTEARRAY_FIND_METHODDEF    \
    {"find", _PyCFunction_CAST(bytearray_find), METH_FASTCALL, bytearray_find__doc__},

static PyObject *
bytearray_find_impl(PyByteArrayObject *self, PyObject *sub, Py_ssize_t start,
                    Py_ssize_t end);

static PyObject *
bytearray_find(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_find_impl((PyByteArrayObject *)self, sub, start, end);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_count__doc__,
"count($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the number of non-overlapping occurrences of subsection \'sub\' in bytes B[start:end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.");

#define BYTEARRAY_COUNT_METHODDEF    \
    {"count", _PyCFunction_CAST(bytearray_count), METH_FASTCALL, bytearray_count__doc__},

static PyObject *
bytearray_count_impl(PyByteArrayObject *self, PyObject *sub,
                     Py_ssize_t start, Py_ssize_t end);

static PyObject *
bytearray_count(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_count_impl((PyByteArrayObject *)self, sub, start, end);
    Py_END_CRITICAL_SECTION();

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
bytearray_clear(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return bytearray_clear_impl((PyByteArrayObject *)self);
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
bytearray_copy(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_copy_impl((PyByteArrayObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(bytearray_index__doc__,
"index($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the lowest index in B where subsection \'sub\' is found, such that \'sub\' is contained within B[start:end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.\n"
"\n"
"Raise ValueError if the subsection is not found.");

#define BYTEARRAY_INDEX_METHODDEF    \
    {"index", _PyCFunction_CAST(bytearray_index), METH_FASTCALL, bytearray_index__doc__},

static PyObject *
bytearray_index_impl(PyByteArrayObject *self, PyObject *sub,
                     Py_ssize_t start, Py_ssize_t end);

static PyObject *
bytearray_index(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_index_impl((PyByteArrayObject *)self, sub, start, end);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_rfind__doc__,
"rfind($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the highest index in B where subsection \'sub\' is found, such that \'sub\' is contained within B[start:end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.\n"
"\n"
"Return -1 on failure.");

#define BYTEARRAY_RFIND_METHODDEF    \
    {"rfind", _PyCFunction_CAST(bytearray_rfind), METH_FASTCALL, bytearray_rfind__doc__},

static PyObject *
bytearray_rfind_impl(PyByteArrayObject *self, PyObject *sub,
                     Py_ssize_t start, Py_ssize_t end);

static PyObject *
bytearray_rfind(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_rfind_impl((PyByteArrayObject *)self, sub, start, end);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_rindex__doc__,
"rindex($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the highest index in B where subsection \'sub\' is found, such that \'sub\' is contained within B[start:end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.\n"
"\n"
"Raise ValueError if the subsection is not found.");

#define BYTEARRAY_RINDEX_METHODDEF    \
    {"rindex", _PyCFunction_CAST(bytearray_rindex), METH_FASTCALL, bytearray_rindex__doc__},

static PyObject *
bytearray_rindex_impl(PyByteArrayObject *self, PyObject *sub,
                      Py_ssize_t start, Py_ssize_t end);

static PyObject *
bytearray_rindex(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_rindex_impl((PyByteArrayObject *)self, sub, start, end);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_startswith__doc__,
"startswith($self, prefix[, start[, end]], /)\n"
"--\n"
"\n"
"Return True if the bytearray starts with the specified prefix, False otherwise.\n"
"\n"
"  prefix\n"
"    A bytes or a tuple of bytes to try.\n"
"  start\n"
"    Optional start position. Default: start of the bytearray.\n"
"  end\n"
"    Optional stop position. Default: end of the bytearray.");

#define BYTEARRAY_STARTSWITH_METHODDEF    \
    {"startswith", _PyCFunction_CAST(bytearray_startswith), METH_FASTCALL, bytearray_startswith__doc__},

static PyObject *
bytearray_startswith_impl(PyByteArrayObject *self, PyObject *subobj,
                          Py_ssize_t start, Py_ssize_t end);

static PyObject *
bytearray_startswith(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_startswith_impl((PyByteArrayObject *)self, subobj, start, end);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_endswith__doc__,
"endswith($self, suffix[, start[, end]], /)\n"
"--\n"
"\n"
"Return True if the bytearray ends with the specified suffix, False otherwise.\n"
"\n"
"  suffix\n"
"    A bytes or a tuple of bytes to try.\n"
"  start\n"
"    Optional start position. Default: start of the bytearray.\n"
"  end\n"
"    Optional stop position. Default: end of the bytearray.");

#define BYTEARRAY_ENDSWITH_METHODDEF    \
    {"endswith", _PyCFunction_CAST(bytearray_endswith), METH_FASTCALL, bytearray_endswith__doc__},

static PyObject *
bytearray_endswith_impl(PyByteArrayObject *self, PyObject *subobj,
                        Py_ssize_t start, Py_ssize_t end);

static PyObject *
bytearray_endswith(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_endswith_impl((PyByteArrayObject *)self, subobj, start, end);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
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
bytearray_removeprefix(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer prefix = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &prefix, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_removeprefix_impl((PyByteArrayObject *)self, &prefix);
    Py_END_CRITICAL_SECTION();

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
bytearray_removesuffix(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer suffix = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &suffix, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_removesuffix_impl((PyByteArrayObject *)self, &suffix);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for suffix */
    if (suffix.obj) {
       PyBuffer_Release(&suffix);
    }

    return return_value;
}

PyDoc_STRVAR(bytearray_resize__doc__,
"resize($self, size, /)\n"
"--\n"
"\n"
"Resize the internal buffer of bytearray to len.\n"
"\n"
"  size\n"
"    New size to resize to..");

#define BYTEARRAY_RESIZE_METHODDEF    \
    {"resize", (PyCFunction)bytearray_resize, METH_O, bytearray_resize__doc__},

static PyObject *
bytearray_resize_impl(PyByteArrayObject *self, Py_ssize_t size);

static PyObject *
bytearray_resize(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_ssize_t size;

    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(arg);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        size = ival;
    }
    return_value = bytearray_resize_impl((PyByteArrayObject *)self, size);

exit:
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
    {"translate", _PyCFunction_CAST(bytearray_translate), METH_FASTCALL|METH_KEYWORDS, bytearray_translate__doc__},

static PyObject *
bytearray_translate_impl(PyByteArrayObject *self, PyObject *table,
                         PyObject *deletechars);

static PyObject *
bytearray_translate(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    table = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    deletechars = args[1];
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_translate_impl((PyByteArrayObject *)self, table, deletechars);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_maketrans__doc__,
"maketrans(frm, to, /)\n"
"--\n"
"\n"
"Return a translation table usable for the bytes or bytearray translate method.\n"
"\n"
"The returned table will be one where each byte in frm is mapped to the byte at\n"
"the same position in to.\n"
"\n"
"The bytes objects frm and to must be of the same length.");

#define BYTEARRAY_MAKETRANS_METHODDEF    \
    {"maketrans", _PyCFunction_CAST(bytearray_maketrans), METH_FASTCALL|METH_STATIC, bytearray_maketrans__doc__},

static PyObject *
bytearray_maketrans_impl(Py_buffer *frm, Py_buffer *to);

static PyObject *
bytearray_maketrans(PyObject *null, PyObject *const *args, Py_ssize_t nargs)
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
    {"replace", _PyCFunction_CAST(bytearray_replace), METH_FASTCALL, bytearray_replace__doc__},

static PyObject *
bytearray_replace_impl(PyByteArrayObject *self, Py_buffer *old,
                       Py_buffer *new, Py_ssize_t count);

static PyObject *
bytearray_replace(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_replace_impl((PyByteArrayObject *)self, &old, &new, count);
    Py_END_CRITICAL_SECTION();

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
    {"split", _PyCFunction_CAST(bytearray_split), METH_FASTCALL|METH_KEYWORDS, bytearray_split__doc__},

static PyObject *
bytearray_split_impl(PyByteArrayObject *self, PyObject *sep,
                     Py_ssize_t maxsplit);

static PyObject *
bytearray_split(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_split_impl((PyByteArrayObject *)self, sep, maxsplit);
    Py_END_CRITICAL_SECTION();

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

static PyObject *
bytearray_partition_impl(PyByteArrayObject *self, PyObject *sep);

static PyObject *
bytearray_partition(PyObject *self, PyObject *sep)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_partition_impl((PyByteArrayObject *)self, sep);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

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

static PyObject *
bytearray_rpartition_impl(PyByteArrayObject *self, PyObject *sep);

static PyObject *
bytearray_rpartition(PyObject *self, PyObject *sep)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_rpartition_impl((PyByteArrayObject *)self, sep);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

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
    {"rsplit", _PyCFunction_CAST(bytearray_rsplit), METH_FASTCALL|METH_KEYWORDS, bytearray_rsplit__doc__},

static PyObject *
bytearray_rsplit_impl(PyByteArrayObject *self, PyObject *sep,
                      Py_ssize_t maxsplit);

static PyObject *
bytearray_rsplit(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_rsplit_impl((PyByteArrayObject *)self, sep, maxsplit);
    Py_END_CRITICAL_SECTION();

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
bytearray_reverse(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_reverse_impl((PyByteArrayObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
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
    {"insert", _PyCFunction_CAST(bytearray_insert), METH_FASTCALL, bytearray_insert__doc__},

static PyObject *
bytearray_insert_impl(PyByteArrayObject *self, Py_ssize_t index, int item);

static PyObject *
bytearray_insert(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_insert_impl((PyByteArrayObject *)self, index, item);
    Py_END_CRITICAL_SECTION();

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
bytearray_append(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int item;

    if (!_getbytevalue(arg, &item)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_append_impl((PyByteArrayObject *)self, item);
    Py_END_CRITICAL_SECTION();

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

static PyObject *
bytearray_extend_impl(PyByteArrayObject *self, PyObject *iterable_of_ints);

static PyObject *
bytearray_extend(PyObject *self, PyObject *iterable_of_ints)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_extend_impl((PyByteArrayObject *)self, iterable_of_ints);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

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
    {"pop", _PyCFunction_CAST(bytearray_pop), METH_FASTCALL, bytearray_pop__doc__},

static PyObject *
bytearray_pop_impl(PyByteArrayObject *self, Py_ssize_t index);

static PyObject *
bytearray_pop(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_pop_impl((PyByteArrayObject *)self, index);
    Py_END_CRITICAL_SECTION();

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
bytearray_remove(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int value;

    if (!_getbytevalue(arg, &value)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_remove_impl((PyByteArrayObject *)self, value);
    Py_END_CRITICAL_SECTION();

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
    {"strip", _PyCFunction_CAST(bytearray_strip), METH_FASTCALL, bytearray_strip__doc__},

static PyObject *
bytearray_strip_impl(PyByteArrayObject *self, PyObject *bytes);

static PyObject *
bytearray_strip(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_strip_impl((PyByteArrayObject *)self, bytes);
    Py_END_CRITICAL_SECTION();

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
    {"lstrip", _PyCFunction_CAST(bytearray_lstrip), METH_FASTCALL, bytearray_lstrip__doc__},

static PyObject *
bytearray_lstrip_impl(PyByteArrayObject *self, PyObject *bytes);

static PyObject *
bytearray_lstrip(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_lstrip_impl((PyByteArrayObject *)self, bytes);
    Py_END_CRITICAL_SECTION();

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
    {"rstrip", _PyCFunction_CAST(bytearray_rstrip), METH_FASTCALL, bytearray_rstrip__doc__},

static PyObject *
bytearray_rstrip_impl(PyByteArrayObject *self, PyObject *bytes);

static PyObject *
bytearray_rstrip(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_rstrip_impl((PyByteArrayObject *)self, bytes);
    Py_END_CRITICAL_SECTION();

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
    {"decode", _PyCFunction_CAST(bytearray_decode), METH_FASTCALL|METH_KEYWORDS, bytearray_decode__doc__},

static PyObject *
bytearray_decode_impl(PyByteArrayObject *self, const char *encoding,
                      const char *errors);

static PyObject *
bytearray_decode(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_decode_impl((PyByteArrayObject *)self, encoding, errors);
    Py_END_CRITICAL_SECTION();

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

static PyObject *
bytearray_join_impl(PyByteArrayObject *self, PyObject *iterable_of_bytes);

static PyObject *
bytearray_join(PyObject *self, PyObject *iterable_of_bytes)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_join_impl((PyByteArrayObject *)self, iterable_of_bytes);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(bytearray_splitlines__doc__,
"splitlines($self, /, keepends=False)\n"
"--\n"
"\n"
"Return a list of the lines in the bytearray, breaking at line boundaries.\n"
"\n"
"Line breaks are not included in the resulting list unless keepends is given and\n"
"true.");

#define BYTEARRAY_SPLITLINES_METHODDEF    \
    {"splitlines", _PyCFunction_CAST(bytearray_splitlines), METH_FASTCALL|METH_KEYWORDS, bytearray_splitlines__doc__},

static PyObject *
bytearray_splitlines_impl(PyByteArrayObject *self, int keepends);

static PyObject *
bytearray_splitlines(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_splitlines_impl((PyByteArrayObject *)self, keepends);
    Py_END_CRITICAL_SECTION();

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
bytearray_fromhex(PyObject *type, PyObject *string)
{
    PyObject *return_value = NULL;

    return_value = bytearray_fromhex_impl((PyTypeObject *)type, string);

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
    {"hex", _PyCFunction_CAST(bytearray_hex), METH_FASTCALL|METH_KEYWORDS, bytearray_hex__doc__},

static PyObject *
bytearray_hex_impl(PyByteArrayObject *self, PyObject *sep, int bytes_per_sep);

static PyObject *
bytearray_hex(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_hex_impl((PyByteArrayObject *)self, sep, bytes_per_sep);
    Py_END_CRITICAL_SECTION();

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
bytearray_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_reduce_impl((PyByteArrayObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(bytearray_reduce_ex__doc__,
"__reduce_ex__($self, proto=0, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define BYTEARRAY_REDUCE_EX_METHODDEF    \
    {"__reduce_ex__", _PyCFunction_CAST(bytearray_reduce_ex), METH_FASTCALL, bytearray_reduce_ex__doc__},

static PyObject *
bytearray_reduce_ex_impl(PyByteArrayObject *self, int proto);

static PyObject *
bytearray_reduce_ex(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int proto = 0;

    if (!_PyArg_CheckPositional("__reduce_ex__", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    proto = PyLong_AsInt(args[0]);
    if (proto == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_reduce_ex_impl((PyByteArrayObject *)self, proto);
    Py_END_CRITICAL_SECTION();

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
bytearray_sizeof(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return bytearray_sizeof_impl((PyByteArrayObject *)self);
}
/*[clinic end generated code: output=be6d28193bc96a2c input=a9049054013a1b77]*/
