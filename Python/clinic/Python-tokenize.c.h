/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


static PyObject *
tokenizeriter_new_impl(PyTypeObject *type, const char *source);

static PyObject *
tokenizeriter_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #define NUM_KEYWORDS 1
    #if NUM_KEYWORDS == 0

    #  if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #    define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #  else
    #    define KWTUPLE NULL
    #  endif

    #else  // NUM_KEYWORDS != 0
    #  if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(source), },
    };
    #  define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #  else  // !Py_BUILD_CORE
    #    define KWTUPLE NULL
    #  endif  // !Py_BUILD_CORE
    #endif  // NUM_KEYWORDS != 0
    #undef NUM_KEYWORDS

    static const char * const _keywords[] = {"source", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "tokenizeriter",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    const char *source;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!PyUnicode_Check(fastargs[0])) {
        _PyArg_BadArgument("tokenizeriter", "argument 'source'", "str", fastargs[0]);
        goto exit;
    }
    Py_ssize_t source_length;
    source = PyUnicode_AsUTF8AndSize(fastargs[0], &source_length);
    if (source == NULL) {
        goto exit;
    }
    if (strlen(source) != (size_t)source_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = tokenizeriter_new_impl(type, source);

exit:
    return return_value;
}
/*[clinic end generated code: output=5664c98597aec79e input=a9049054013a1b77]*/
