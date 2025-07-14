/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_testcapi_err_set_raised__doc__,
"err_set_raised($module, exception, /)\n"
"--\n"
"\n");

#define _TESTCAPI_ERR_SET_RAISED_METHODDEF    \
    {"err_set_raised", (PyCFunction)_testcapi_err_set_raised, METH_O, _testcapi_err_set_raised__doc__},

PyDoc_STRVAR(_testcapi_exception_print__doc__,
"exception_print($module, exception, legacy=False, /)\n"
"--\n"
"\n"
"To test the format of exceptions as printed out.");

#define _TESTCAPI_EXCEPTION_PRINT_METHODDEF    \
    {"exception_print", _PyCFunction_CAST(_testcapi_exception_print), METH_FASTCALL, _testcapi_exception_print__doc__},

static PyObject *
_testcapi_exception_print_impl(PyObject *module, PyObject *exc, int legacy);

static PyObject *
_testcapi_exception_print(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *exc;
    int legacy = 0;

    if (!_PyArg_CheckPositional("exception_print", nargs, 1, 2)) {
        goto exit;
    }
    exc = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    legacy = PyObject_IsTrue(args[1]);
    if (legacy < 0) {
        goto exit;
    }
skip_optional:
    return_value = _testcapi_exception_print_impl(module, exc, legacy);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_make_exception_with_doc__doc__,
"make_exception_with_doc($module, /, name, doc=<unrepresentable>,\n"
"                        base=<unrepresentable>, dict=<unrepresentable>)\n"
"--\n"
"\n"
"Test PyErr_NewExceptionWithDoc (also exercise PyErr_NewException). Run via Lib/test/test_exceptions.py");

#define _TESTCAPI_MAKE_EXCEPTION_WITH_DOC_METHODDEF    \
    {"make_exception_with_doc", _PyCFunction_CAST(_testcapi_make_exception_with_doc), METH_FASTCALL|METH_KEYWORDS, _testcapi_make_exception_with_doc__doc__},

static PyObject *
_testcapi_make_exception_with_doc_impl(PyObject *module, const char *name,
                                       const char *doc, PyObject *base,
                                       PyObject *dict);

static PyObject *
_testcapi_make_exception_with_doc(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(name), &_Py_ID(doc), &_Py_ID(base), &_Py_ID(dict), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "doc", "base", "dict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "make_exception_with_doc",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    const char *name;
    const char *doc = NULL;
    PyObject *base = NULL;
    PyObject *dict = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("make_exception_with_doc", "argument 'name'", "str", args[0]);
        goto exit;
    }
    Py_ssize_t name_length;
    name = PyUnicode_AsUTF8AndSize(args[0], &name_length);
    if (name == NULL) {
        goto exit;
    }
    if (strlen(name) != (size_t)name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        if (!PyUnicode_Check(args[1])) {
            _PyArg_BadArgument("make_exception_with_doc", "argument 'doc'", "str", args[1]);
            goto exit;
        }
        Py_ssize_t doc_length;
        doc = PyUnicode_AsUTF8AndSize(args[1], &doc_length);
        if (doc == NULL) {
            goto exit;
        }
        if (strlen(doc) != (size_t)doc_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[2]) {
        base = args[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    dict = args[3];
skip_optional_pos:
    return_value = _testcapi_make_exception_with_doc_impl(module, name, doc, base, dict);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_exc_set_object__doc__,
"exc_set_object($module, exception, obj, /)\n"
"--\n"
"\n");

#define _TESTCAPI_EXC_SET_OBJECT_METHODDEF    \
    {"exc_set_object", _PyCFunction_CAST(_testcapi_exc_set_object), METH_FASTCALL, _testcapi_exc_set_object__doc__},

static PyObject *
_testcapi_exc_set_object_impl(PyObject *module, PyObject *exc, PyObject *obj);

static PyObject *
_testcapi_exc_set_object(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *exc;
    PyObject *obj;

    if (!_PyArg_CheckPositional("exc_set_object", nargs, 2, 2)) {
        goto exit;
    }
    exc = args[0];
    obj = args[1];
    return_value = _testcapi_exc_set_object_impl(module, exc, obj);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_exc_set_object_fetch__doc__,
"exc_set_object_fetch($module, exception, obj, /)\n"
"--\n"
"\n");

#define _TESTCAPI_EXC_SET_OBJECT_FETCH_METHODDEF    \
    {"exc_set_object_fetch", _PyCFunction_CAST(_testcapi_exc_set_object_fetch), METH_FASTCALL, _testcapi_exc_set_object_fetch__doc__},

static PyObject *
_testcapi_exc_set_object_fetch_impl(PyObject *module, PyObject *exc,
                                    PyObject *obj);

static PyObject *
_testcapi_exc_set_object_fetch(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *exc;
    PyObject *obj;

    if (!_PyArg_CheckPositional("exc_set_object_fetch", nargs, 2, 2)) {
        goto exit;
    }
    exc = args[0];
    obj = args[1];
    return_value = _testcapi_exc_set_object_fetch_impl(module, exc, obj);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_err_setstring__doc__,
"err_setstring($module, exc, value, /)\n"
"--\n"
"\n");

#define _TESTCAPI_ERR_SETSTRING_METHODDEF    \
    {"err_setstring", _PyCFunction_CAST(_testcapi_err_setstring), METH_FASTCALL, _testcapi_err_setstring__doc__},

static PyObject *
_testcapi_err_setstring_impl(PyObject *module, PyObject *exc,
                             const char *value, Py_ssize_t value_length);

static PyObject *
_testcapi_err_setstring(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *exc;
    const char *value;
    Py_ssize_t value_length;

    if (!_PyArg_ParseStack(args, nargs, "Oz#:err_setstring",
        &exc, &value, &value_length)) {
        goto exit;
    }
    return_value = _testcapi_err_setstring_impl(module, exc, value, value_length);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_err_setfromerrnowithfilename__doc__,
"err_setfromerrnowithfilename($module, error, exc, value, /)\n"
"--\n"
"\n");

#define _TESTCAPI_ERR_SETFROMERRNOWITHFILENAME_METHODDEF    \
    {"err_setfromerrnowithfilename", _PyCFunction_CAST(_testcapi_err_setfromerrnowithfilename), METH_FASTCALL, _testcapi_err_setfromerrnowithfilename__doc__},

static PyObject *
_testcapi_err_setfromerrnowithfilename_impl(PyObject *module, int error,
                                            PyObject *exc, const char *value,
                                            Py_ssize_t value_length);

static PyObject *
_testcapi_err_setfromerrnowithfilename(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int error;
    PyObject *exc;
    const char *value;
    Py_ssize_t value_length;

    if (!_PyArg_ParseStack(args, nargs, "iOz#:err_setfromerrnowithfilename",
        &error, &exc, &value, &value_length)) {
        goto exit;
    }
    return_value = _testcapi_err_setfromerrnowithfilename_impl(module, error, exc, value, value_length);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_raise_exception__doc__,
"raise_exception($module, exception, num_args, /)\n"
"--\n"
"\n");

#define _TESTCAPI_RAISE_EXCEPTION_METHODDEF    \
    {"raise_exception", _PyCFunction_CAST(_testcapi_raise_exception), METH_FASTCALL, _testcapi_raise_exception__doc__},

static PyObject *
_testcapi_raise_exception_impl(PyObject *module, PyObject *exc, int num_args);

static PyObject *
_testcapi_raise_exception(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *exc;
    int num_args;

    if (!_PyArg_CheckPositional("raise_exception", nargs, 2, 2)) {
        goto exit;
    }
    exc = args[0];
    num_args = PyLong_AsInt(args[1]);
    if (num_args == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _testcapi_raise_exception_impl(module, exc, num_args);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_raise_memoryerror__doc__,
"raise_memoryerror($module, /)\n"
"--\n"
"\n");

#define _TESTCAPI_RAISE_MEMORYERROR_METHODDEF    \
    {"raise_memoryerror", (PyCFunction)_testcapi_raise_memoryerror, METH_NOARGS, _testcapi_raise_memoryerror__doc__},

static PyObject *
_testcapi_raise_memoryerror_impl(PyObject *module);

static PyObject *
_testcapi_raise_memoryerror(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _testcapi_raise_memoryerror_impl(module);
}

PyDoc_STRVAR(_testcapi_fatal_error__doc__,
"fatal_error($module, message, release_gil=False, /)\n"
"--\n"
"\n");

#define _TESTCAPI_FATAL_ERROR_METHODDEF    \
    {"fatal_error", _PyCFunction_CAST(_testcapi_fatal_error), METH_FASTCALL, _testcapi_fatal_error__doc__},

static PyObject *
_testcapi_fatal_error_impl(PyObject *module, const char *message,
                           int release_gil);

static PyObject *
_testcapi_fatal_error(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *message;
    int release_gil = 0;

    if (!_PyArg_ParseStack(args, nargs, "y|p:fatal_error",
        &message, &release_gil)) {
        goto exit;
    }
    return_value = _testcapi_fatal_error_impl(module, message, release_gil);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_set_exc_info__doc__,
"set_exc_info($module, new_type, new_value, new_tb, /)\n"
"--\n"
"\n");

#define _TESTCAPI_SET_EXC_INFO_METHODDEF    \
    {"set_exc_info", _PyCFunction_CAST(_testcapi_set_exc_info), METH_FASTCALL, _testcapi_set_exc_info__doc__},

static PyObject *
_testcapi_set_exc_info_impl(PyObject *module, PyObject *new_type,
                            PyObject *new_value, PyObject *new_tb);

static PyObject *
_testcapi_set_exc_info(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *new_type;
    PyObject *new_value;
    PyObject *new_tb;

    if (!_PyArg_CheckPositional("set_exc_info", nargs, 3, 3)) {
        goto exit;
    }
    new_type = args[0];
    new_value = args[1];
    new_tb = args[2];
    return_value = _testcapi_set_exc_info_impl(module, new_type, new_value, new_tb);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_set_exception__doc__,
"set_exception($module, new_exc, /)\n"
"--\n"
"\n");

#define _TESTCAPI_SET_EXCEPTION_METHODDEF    \
    {"set_exception", (PyCFunction)_testcapi_set_exception, METH_O, _testcapi_set_exception__doc__},

PyDoc_STRVAR(_testcapi_traceback_print__doc__,
"traceback_print($module, traceback, file, /)\n"
"--\n"
"\n"
"To test the format of tracebacks as printed out.");

#define _TESTCAPI_TRACEBACK_PRINT_METHODDEF    \
    {"traceback_print", _PyCFunction_CAST(_testcapi_traceback_print), METH_FASTCALL, _testcapi_traceback_print__doc__},

static PyObject *
_testcapi_traceback_print_impl(PyObject *module, PyObject *traceback,
                               PyObject *file);

static PyObject *
_testcapi_traceback_print(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *traceback;
    PyObject *file;

    if (!_PyArg_CheckPositional("traceback_print", nargs, 2, 2)) {
        goto exit;
    }
    traceback = args[0];
    file = args[1];
    return_value = _testcapi_traceback_print_impl(module, traceback, file);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_unstable_exc_prep_reraise_star__doc__,
"unstable_exc_prep_reraise_star($module, orig, excs, /)\n"
"--\n"
"\n"
"To test PyUnstable_Exc_PrepReraiseStar.");

#define _TESTCAPI_UNSTABLE_EXC_PREP_RERAISE_STAR_METHODDEF    \
    {"unstable_exc_prep_reraise_star", _PyCFunction_CAST(_testcapi_unstable_exc_prep_reraise_star), METH_FASTCALL, _testcapi_unstable_exc_prep_reraise_star__doc__},

static PyObject *
_testcapi_unstable_exc_prep_reraise_star_impl(PyObject *module,
                                              PyObject *orig, PyObject *excs);

static PyObject *
_testcapi_unstable_exc_prep_reraise_star(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *orig;
    PyObject *excs;

    if (!_PyArg_CheckPositional("unstable_exc_prep_reraise_star", nargs, 2, 2)) {
        goto exit;
    }
    orig = args[0];
    excs = args[1];
    return_value = _testcapi_unstable_exc_prep_reraise_star_impl(module, orig, excs);

exit:
    return return_value;
}
/*[clinic end generated code: output=357caea020348789 input=a9049054013a1b77]*/
