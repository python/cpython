/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(pysqlite_complete_statement__doc__,
"complete_statement($module, /, statement)\n"
"--\n"
"\n"
"Checks if a string contains a complete SQL statement.");

#define PYSQLITE_COMPLETE_STATEMENT_METHODDEF    \
    {"complete_statement", _PyCFunction_CAST(pysqlite_complete_statement), METH_FASTCALL|METH_KEYWORDS, pysqlite_complete_statement__doc__},

static PyObject *
pysqlite_complete_statement_impl(PyObject *module, const char *statement);

static PyObject *
pysqlite_complete_statement(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(statement), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"statement", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "complete_statement",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    const char *statement;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("complete_statement", "argument 'statement'", "str", args[0]);
        goto exit;
    }
    Py_ssize_t statement_length;
    statement = PyUnicode_AsUTF8AndSize(args[0], &statement_length);
    if (statement == NULL) {
        goto exit;
    }
    if (strlen(statement) != (size_t)statement_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = pysqlite_complete_statement_impl(module, statement);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_register_adapter__doc__,
"register_adapter($module, type, adapter, /)\n"
"--\n"
"\n"
"Register a function to adapt Python objects to SQLite values.");

#define PYSQLITE_REGISTER_ADAPTER_METHODDEF    \
    {"register_adapter", _PyCFunction_CAST(pysqlite_register_adapter), METH_FASTCALL, pysqlite_register_adapter__doc__},

static PyObject *
pysqlite_register_adapter_impl(PyObject *module, PyTypeObject *type,
                               PyObject *caster);

static PyObject *
pysqlite_register_adapter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *type;
    PyObject *caster;

    if (!_PyArg_CheckPositional("register_adapter", nargs, 2, 2)) {
        goto exit;
    }
    type = (PyTypeObject *)args[0];
    caster = args[1];
    return_value = pysqlite_register_adapter_impl(module, type, caster);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_register_converter__doc__,
"register_converter($module, typename, converter, /)\n"
"--\n"
"\n"
"Register a function to convert SQLite values to Python objects.");

#define PYSQLITE_REGISTER_CONVERTER_METHODDEF    \
    {"register_converter", _PyCFunction_CAST(pysqlite_register_converter), METH_FASTCALL, pysqlite_register_converter__doc__},

static PyObject *
pysqlite_register_converter_impl(PyObject *module, PyObject *orig_name,
                                 PyObject *callable);

static PyObject *
pysqlite_register_converter(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *orig_name;
    PyObject *callable;

    if (!_PyArg_CheckPositional("register_converter", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("register_converter", "argument 1", "str", args[0]);
        goto exit;
    }
    orig_name = args[0];
    callable = args[1];
    return_value = pysqlite_register_converter_impl(module, orig_name, callable);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_enable_callback_trace__doc__,
"enable_callback_tracebacks($module, enable, /)\n"
"--\n"
"\n"
"Enable or disable callback functions throwing errors to stderr.");

#define PYSQLITE_ENABLE_CALLBACK_TRACE_METHODDEF    \
    {"enable_callback_tracebacks", (PyCFunction)pysqlite_enable_callback_trace, METH_O, pysqlite_enable_callback_trace__doc__},

static PyObject *
pysqlite_enable_callback_trace_impl(PyObject *module, int enable);

static PyObject *
pysqlite_enable_callback_trace(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int enable;

    enable = PyLong_AsInt(arg);
    if (enable == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = pysqlite_enable_callback_trace_impl(module, enable);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_adapt__doc__,
"adapt($module, obj, proto=PrepareProtocolType, alt=<unrepresentable>, /)\n"
"--\n"
"\n"
"Adapt given object to given protocol.");

#define PYSQLITE_ADAPT_METHODDEF    \
    {"adapt", _PyCFunction_CAST(pysqlite_adapt), METH_FASTCALL, pysqlite_adapt__doc__},

static PyObject *
pysqlite_adapt_impl(PyObject *module, PyObject *obj, PyObject *proto,
                    PyObject *alt);

static PyObject *
pysqlite_adapt(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    PyObject *proto = (PyObject *)clinic_state()->PrepareProtocolType;
    PyObject *alt = NULL;

    if (!_PyArg_CheckPositional("adapt", nargs, 1, 3)) {
        goto exit;
    }
    obj = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    proto = args[1];
    if (nargs < 3) {
        goto skip_optional;
    }
    alt = args[2];
skip_optional:
    return_value = pysqlite_adapt_impl(module, obj, proto, alt);

exit:
    return return_value;
}
/*[clinic end generated code: output=17c4e031680a5168 input=a9049054013a1b77]*/
