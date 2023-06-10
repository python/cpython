/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


static PyObject *
tokenizeriter_new_impl(PyTypeObject *type, PyObject *readline,
                       int extra_tokens, const char *encoding);

static PyObject *
tokenizeriter_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
        .ob_item = { &_Py_ID(extra_tokens), &_Py_ID(encoding), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "extra_tokens", "encoding", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "tokenizeriter",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 2;
    PyObject *readline;
    int extra_tokens;
    const char *encoding = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    readline = fastargs[0];
    extra_tokens = PyObject_IsTrue(fastargs[1]);
    if (extra_tokens < 0) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyUnicode_Check(fastargs[2])) {
        _PyArg_BadArgument("tokenizeriter", "argument 'encoding'", "str", fastargs[2]);
        goto exit;
    }
    Py_ssize_t encoding_length;
    encoding = PyUnicode_AsUTF8AndSize(fastargs[2], &encoding_length);
    if (encoding == NULL) {
        goto exit;
    }
    if (strlen(encoding) != (size_t)encoding_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
skip_optional_kwonly:
    return_value = tokenizeriter_new_impl(type, readline, extra_tokens, encoding);

exit:
    return return_value;
}
/*[clinic end generated code: output=48be65a2808bdfa6 input=a9049054013a1b77]*/
