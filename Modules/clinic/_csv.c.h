/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_csv_list_dialects__doc__,
"list_dialects($module, /)\n"
"--\n"
"\n"
"Return a list of all known dialect names.\n"
"\n"
"    names = csv.list_dialects()");

#define _CSV_LIST_DIALECTS_METHODDEF    \
    {"list_dialects", (PyCFunction)_csv_list_dialects, METH_NOARGS, _csv_list_dialects__doc__},

static PyObject *
_csv_list_dialects_impl(PyObject *module);

static PyObject *
_csv_list_dialects(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _csv_list_dialects_impl(module);
}

PyDoc_STRVAR(_csv_unregister_dialect__doc__,
"unregister_dialect($module, /, name)\n"
"--\n"
"\n"
"Delete the name/dialect mapping associated with a string name.\n"
"\n"
"    csv.unregister_dialect(name)");

#define _CSV_UNREGISTER_DIALECT_METHODDEF    \
    {"unregister_dialect", _PyCFunction_CAST(_csv_unregister_dialect), METH_FASTCALL|METH_KEYWORDS, _csv_unregister_dialect__doc__},

static PyObject *
_csv_unregister_dialect_impl(PyObject *module, PyObject *name);

static PyObject *
_csv_unregister_dialect(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(name), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "unregister_dialect",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *name;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    name = args[0];
    return_value = _csv_unregister_dialect_impl(module, name);

exit:
    return return_value;
}

PyDoc_STRVAR(_csv_get_dialect__doc__,
"get_dialect($module, /, name)\n"
"--\n"
"\n"
"Return the dialect instance associated with name.\n"
"\n"
"    dialect = csv.get_dialect(name)");

#define _CSV_GET_DIALECT_METHODDEF    \
    {"get_dialect", _PyCFunction_CAST(_csv_get_dialect), METH_FASTCALL|METH_KEYWORDS, _csv_get_dialect__doc__},

static PyObject *
_csv_get_dialect_impl(PyObject *module, PyObject *name);

static PyObject *
_csv_get_dialect(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(name), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_dialect",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *name;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    name = args[0];
    return_value = _csv_get_dialect_impl(module, name);

exit:
    return return_value;
}

PyDoc_STRVAR(_csv_field_size_limit__doc__,
"field_size_limit($module, /, new_limit=<unrepresentable>)\n"
"--\n"
"\n"
"Sets an upper limit on parsed fields.\n"
"\n"
"    csv.field_size_limit([limit])\n"
"\n"
"Returns old limit. If limit is not given, no new limit is set and\n"
"the old limit is returned");

#define _CSV_FIELD_SIZE_LIMIT_METHODDEF    \
    {"field_size_limit", _PyCFunction_CAST(_csv_field_size_limit), METH_FASTCALL|METH_KEYWORDS, _csv_field_size_limit__doc__},

static PyObject *
_csv_field_size_limit_impl(PyObject *module, PyObject *new_limit);

static PyObject *
_csv_field_size_limit(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(new_limit), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"new_limit", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "field_size_limit",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *new_limit = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    new_limit = args[0];
skip_optional_pos:
    return_value = _csv_field_size_limit_impl(module, new_limit);

exit:
    return return_value;
}
/*[clinic end generated code: output=1fb09d5e7667ad0d input=a9049054013a1b77]*/
