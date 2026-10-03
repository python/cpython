/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_NoPositional()

PyDoc_STRVAR(_csv_Dialect___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Raise an exception to avoid pickling.");

#define _CSV_DIALECT___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)_csv_Dialect___reduce__, METH_NOARGS, _csv_Dialect___reduce____doc__},

static PyObject *
_csv_Dialect___reduce___impl(DialectObj *self);

static PyObject *
_csv_Dialect___reduce__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _csv_Dialect___reduce___impl((DialectObj *)self);
}

PyDoc_STRVAR(_csv_Dialect___reduce_ex____doc__,
"__reduce_ex__($self, protocol, /)\n"
"--\n"
"\n"
"Raise an exception to avoid pickling.");

#define _CSV_DIALECT___REDUCE_EX___METHODDEF    \
    {"__reduce_ex__", (PyCFunction)_csv_Dialect___reduce_ex__, METH_O, _csv_Dialect___reduce_ex____doc__},

static PyObject *
_csv_Dialect___reduce_ex___impl(DialectObj *self, PyObject *protocol);

static PyObject *
_csv_Dialect___reduce_ex__(PyObject *self, PyObject *protocol)
{
    PyObject *return_value = NULL;

    return_value = _csv_Dialect___reduce_ex___impl((DialectObj *)self, protocol);

    return return_value;
}

PyDoc_STRVAR(_csv_Dialect___replace____doc__,
"__replace__($self, /, **changes)\n"
"--\n"
"\n"
"Return a copy of the dialect with the specified options replaced.");

#define _CSV_DIALECT___REPLACE___METHODDEF    \
    {"__replace__", _PyCFunction_CAST(_csv_Dialect___replace__), METH_VARARGS|METH_KEYWORDS, _csv_Dialect___replace____doc__},

static PyObject *
_csv_Dialect___replace___impl(DialectObj *self, PyObject *changes);

static PyObject *
_csv_Dialect___replace__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *changes = NULL;

    if (!_PyArg_NoPositional("__replace__", args)) {
        goto exit;
    }
    if (kwargs == NULL) {
        changes = PyDict_New();
        if (changes == NULL) {
            goto exit;
        }
    }
    else {
        changes = Py_NewRef(kwargs);
    }
    return_value = _csv_Dialect___replace___impl((DialectObj *)self, changes);

exit:
    /* Cleanup for changes */
    Py_XDECREF(changes);

    return return_value;
}

PyDoc_STRVAR(_csv_reader__doc__,
"reader($module, iterable, dialect=\'excel\', /, **fmtparams)\n"
"--\n"
"\n"
"Return a reader object that will process lines from the given iterable.\n"
"\n"
"The \"iterable\" argument can be any object that returns a line\n"
"of input for each iteration, such as a file object or a list.  The\n"
"optional \"dialect\" argument defines a CSV dialect.  The function\n"
"also accepts optional keyword arguments which override settings\n"
"provided by the dialect.\n"
"\n"
"The returned object is an iterator.  Each iteration returns a row\n"
"of the CSV file (which can span multiple input lines).");

#define _CSV_READER_METHODDEF    \
    {"reader", _PyCFunction_CAST(_csv_reader), METH_VARARGS|METH_KEYWORDS, _csv_reader__doc__},

static PyObject *
_csv_reader_impl(PyObject *module, PyObject *iterable, PyObject *dialect,
                 PyObject *fmtparams);

static PyObject *
_csv_reader(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *iterable;
    PyObject *dialect = NULL;
    PyObject *fmtparams = NULL;

    if (!_PyArg_CheckPositional("reader", PyTuple_GET_SIZE(args), 1, 2)) {
        goto exit;
    }
    iterable = PyTuple_GET_ITEM(args, 0);
    if (PyTuple_GET_SIZE(args) < 2) {
        goto skip_optional;
    }
    dialect = PyTuple_GET_ITEM(args, 1);
skip_optional:
    if (kwargs == NULL) {
        fmtparams = PyDict_New();
        if (fmtparams == NULL) {
            goto exit;
        }
    }
    else {
        fmtparams = Py_NewRef(kwargs);
    }
    return_value = _csv_reader_impl(module, iterable, dialect, fmtparams);

exit:
    /* Cleanup for fmtparams */
    Py_XDECREF(fmtparams);

    return return_value;
}

PyDoc_STRVAR(_csv_writer__doc__,
"writer($module, fileobj, dialect=\'excel\', /, **fmtparams)\n"
"--\n"
"\n"
"Return a writer object writing user data to the given file object.\n"
"\n"
"The \"fileobj\" argument can be any object that supports the file API.\n"
"The optional \"dialect\" argument defines a CSV dialect.  The function\n"
"also accepts optional keyword arguments which override settings\n"
"provided by the dialect.");

#define _CSV_WRITER_METHODDEF    \
    {"writer", _PyCFunction_CAST(_csv_writer), METH_VARARGS|METH_KEYWORDS, _csv_writer__doc__},

static PyObject *
_csv_writer_impl(PyObject *module, PyObject *output_file, PyObject *dialect,
                 PyObject *fmtparams);

static PyObject *
_csv_writer(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *output_file;
    PyObject *dialect = NULL;
    PyObject *fmtparams = NULL;

    if (!_PyArg_CheckPositional("writer", PyTuple_GET_SIZE(args), 1, 2)) {
        goto exit;
    }
    output_file = PyTuple_GET_ITEM(args, 0);
    if (PyTuple_GET_SIZE(args) < 2) {
        goto skip_optional;
    }
    dialect = PyTuple_GET_ITEM(args, 1);
skip_optional:
    if (kwargs == NULL) {
        fmtparams = PyDict_New();
        if (fmtparams == NULL) {
            goto exit;
        }
    }
    else {
        fmtparams = Py_NewRef(kwargs);
    }
    return_value = _csv_writer_impl(module, output_file, dialect, fmtparams);

exit:
    /* Cleanup for fmtparams */
    Py_XDECREF(fmtparams);

    return return_value;
}

PyDoc_STRVAR(_csv_list_dialects__doc__,
"list_dialects($module, /)\n"
"--\n"
"\n"
"Return a list of all known dialect names.");

#define _CSV_LIST_DIALECTS_METHODDEF    \
    {"list_dialects", (PyCFunction)_csv_list_dialects, METH_NOARGS, _csv_list_dialects__doc__},

static PyObject *
_csv_list_dialects_impl(PyObject *module);

static PyObject *
_csv_list_dialects(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _csv_list_dialects_impl(module);
}

PyDoc_STRVAR(_csv_register_dialect__doc__,
"register_dialect($module, name, dialect=\'excel\', /, **fmtparams)\n"
"--\n"
"\n"
"Create a mapping from a string name to a CVS dialect.\n"
"\n"
"The optional \"dialect\" argument specifies the base dialect instance\n"
"or the name of the registered dialect.  The function also accepts\n"
"optional keyword arguments which override settings provided by the\n"
"dialect.");

#define _CSV_REGISTER_DIALECT_METHODDEF    \
    {"register_dialect", _PyCFunction_CAST(_csv_register_dialect), METH_VARARGS|METH_KEYWORDS, _csv_register_dialect__doc__},

static PyObject *
_csv_register_dialect_impl(PyObject *module, PyObject *name_obj,
                           PyObject *dialect_obj, PyObject *fmtparams);

static PyObject *
_csv_register_dialect(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *name_obj;
    PyObject *dialect_obj = NULL;
    PyObject *fmtparams = NULL;

    if (!_PyArg_CheckPositional("register_dialect", PyTuple_GET_SIZE(args), 1, 2)) {
        goto exit;
    }
    name_obj = PyTuple_GET_ITEM(args, 0);
    if (PyTuple_GET_SIZE(args) < 2) {
        goto skip_optional;
    }
    dialect_obj = PyTuple_GET_ITEM(args, 1);
skip_optional:
    if (kwargs == NULL) {
        fmtparams = PyDict_New();
        if (fmtparams == NULL) {
            goto exit;
        }
    }
    else {
        fmtparams = Py_NewRef(kwargs);
    }
    return_value = _csv_register_dialect_impl(module, name_obj, dialect_obj, fmtparams);

exit:
    /* Cleanup for fmtparams */
    Py_XDECREF(fmtparams);

    return return_value;
}

PyDoc_STRVAR(_csv_unregister_dialect__doc__,
"unregister_dialect($module, /, name)\n"
"--\n"
"\n"
"Delete the name/dialect mapping associated with a string name.");

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
"Return the dialect instance associated with name.");

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
/*[clinic end generated code: output=1a90b8a8ed82497d input=a9049054013a1b77]*/
