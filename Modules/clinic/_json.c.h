/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(py_scanstring__doc__,
"scanstring($module, pystr, end, strict=True, /)\n"
"--\n"
"\n"
"Scan the string s for a JSON string.\n"
"\n"
"End is the index of the character in s after the quote that started the\n"
"JSON string. Unescapes all valid JSON string escape sequences and raises\n"
"ValueError on attempt to decode an invalid string. If strict is False\n"
"then literal control characters are allowed in the string.\n"
"\n"
"Returns a tuple of the decoded string and the index of the character in\n"
"s after the end quote.");

#define PY_SCANSTRING_METHODDEF    \
    {"scanstring", _PyCFunction_CAST(py_scanstring), METH_FASTCALL, py_scanstring__doc__},

static PyObject *
py_scanstring_impl(PyObject *module, PyObject *pystr, Py_ssize_t end,
                   int strict);

static PyObject *
py_scanstring(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *pystr;
    Py_ssize_t end;
    int strict = 1;

    if (!_PyArg_CheckPositional("scanstring", nargs, 2, 3)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("scanstring", "argument 1", "str", args[0]);
        goto exit;
    }
    pystr = args[0];
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
        end = ival;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    strict = PyObject_IsTrue(args[2]);
    if (strict < 0) {
        goto exit;
    }
skip_optional:
    return_value = py_scanstring_impl(module, pystr, end, strict);

exit:
    return return_value;
}

PyDoc_STRVAR(py_encode_basestring_ascii__doc__,
"encode_basestring_ascii($module, pystr, /)\n"
"--\n"
"\n"
"Return an ASCII-only JSON representation of a Python string");

#define PY_ENCODE_BASESTRING_ASCII_METHODDEF    \
    {"encode_basestring_ascii", (PyCFunction)py_encode_basestring_ascii, METH_O, py_encode_basestring_ascii__doc__},

static PyObject *
py_encode_basestring_ascii_impl(PyObject *module, PyObject *pystr);

static PyObject *
py_encode_basestring_ascii(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *pystr;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("encode_basestring_ascii", "argument", "str", arg);
        goto exit;
    }
    pystr = arg;
    return_value = py_encode_basestring_ascii_impl(module, pystr);

exit:
    return return_value;
}

PyDoc_STRVAR(py_encode_basestring__doc__,
"encode_basestring($module, pystr, /)\n"
"--\n"
"\n"
"Return a JSON representation of a Python string");

#define PY_ENCODE_BASESTRING_METHODDEF    \
    {"encode_basestring", (PyCFunction)py_encode_basestring, METH_O, py_encode_basestring__doc__},

static PyObject *
py_encode_basestring_impl(PyObject *module, PyObject *pystr);

static PyObject *
py_encode_basestring(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *pystr;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("encode_basestring", "argument", "str", arg);
        goto exit;
    }
    pystr = arg;
    return_value = py_encode_basestring_impl(module, pystr);

exit:
    return return_value;
}

static PyObject *
encoder_new_impl(PyTypeObject *type, PyObject *markers, PyObject *defaultfn,
                 PyObject *encoder, PyObject *indent,
                 PyObject *key_separator, PyObject *item_separator,
                 int sort_keys, int skipkeys, int allow_nan);

static PyObject *
encoder_new_helper(PyTypeObject *type, PyObject *const *args,
    Py_ssize_t nargs, Py_ssize_t nkw, PyObject *kwargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 9
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(markers), &_Py_ID(default), &_Py_ID(encoder), &_Py_ID(indent), &_Py_ID(key_separator), &_Py_ID(item_separator), &_Py_ID(sort_keys), &_Py_ID(skipkeys), &_Py_ID(allow_nan), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"markers", "default", "encoder", "indent", "key_separator", "item_separator", "sort_keys", "skipkeys", "allow_nan", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "make_encoder",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[9];
    PyObject * const *fastargs;
    PyObject *markers;
    PyObject *defaultfn;
    PyObject *encoder;
    PyObject *indent;
    PyObject *key_separator;
    PyObject *item_separator;
    int sort_keys;
    int skipkeys;
    int allow_nan;

    fastargs = _PyArg_UnpackKeywords(args, nargs, kwargs, kwnames, &_parser,
            /*minpos*/ 9, /*maxpos*/ 9, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    markers = fastargs[0];
    defaultfn = fastargs[1];
    encoder = fastargs[2];
    indent = fastargs[3];
    if (!PyUnicode_Check(fastargs[4])) {
        _PyArg_BadArgument("make_encoder", "argument 'key_separator'", "str", fastargs[4]);
        goto exit;
    }
    key_separator = fastargs[4];
    if (!PyUnicode_Check(fastargs[5])) {
        _PyArg_BadArgument("make_encoder", "argument 'item_separator'", "str", fastargs[5]);
        goto exit;
    }
    item_separator = fastargs[5];
    sort_keys = PyObject_IsTrue(fastargs[6]);
    if (sort_keys < 0) {
        goto exit;
    }
    skipkeys = PyObject_IsTrue(fastargs[7]);
    if (skipkeys < 0) {
        goto exit;
    }
    allow_nan = PyObject_IsTrue(fastargs[8]);
    if (allow_nan < 0) {
        goto exit;
    }
    return_value = encoder_new_impl(type, markers, defaultfn, encoder, indent, key_separator, item_separator, sort_keys, skipkeys, allow_nan);

exit:
    return return_value;
}

static PyObject *
encoder_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    return encoder_new_helper(type, _PyTuple_CAST(args)->ob_item,
        PyTuple_GET_SIZE(args),
        kwargs ? PyDict_GET_SIZE(kwargs) : 0,
        kwargs, NULL);
}

static PyObject *
encoder_vectorcall(PyObject *type, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *markers;
    PyObject *defaultfn;
    PyObject *encoder;
    PyObject *indent;
    PyObject *key_separator;
    PyObject *item_separator;
    int sort_keys;
    int skipkeys;
    int allow_nan;

    assert(_PyType_CAST(type)->tp_new == encoder_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (kwnames != NULL || nargs < 9 || nargs > 9) {
        return encoder_new_helper(_PyType_CAST(type), args, nargs,
            kwnames ? PyTuple_GET_SIZE(kwnames) : 0,
            NULL, kwnames);
    }
    markers = args[0];
    defaultfn = args[1];
    encoder = args[2];
    indent = args[3];
    if (!PyUnicode_Check(args[4])) {
        _PyArg_BadArgument("make_encoder", "argument 'key_separator'", "str", args[4]);
        goto exit;
    }
    key_separator = args[4];
    if (!PyUnicode_Check(args[5])) {
        _PyArg_BadArgument("make_encoder", "argument 'item_separator'", "str", args[5]);
        goto exit;
    }
    item_separator = args[5];
    sort_keys = PyObject_IsTrue(args[6]);
    if (sort_keys < 0) {
        goto exit;
    }
    skipkeys = PyObject_IsTrue(args[7]);
    if (skipkeys < 0) {
        goto exit;
    }
    allow_nan = PyObject_IsTrue(args[8]);
    if (allow_nan < 0) {
        goto exit;
    }
    return_value = encoder_new_impl(_PyType_CAST(type), markers, defaultfn, encoder, indent, key_separator, item_separator, sort_keys, skipkeys, allow_nan);

exit:
    return return_value;
}
/*[clinic end generated code: output=19476c429c7246c1 input=a9049054013a1b77]*/
