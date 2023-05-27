/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


static int
pysqlite_connection_init_impl(pysqlite_Connection *self, PyObject *database,
                              double timeout, int detect_types,
                              const char *isolation_level,
                              int check_same_thread, PyObject *factory,
                              int cache_size, int uri,
                              enum autocommit_mode autocommit);

static int
pysqlite_connection_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 9
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(database), &_Py_ID(timeout), &_Py_ID(detect_types), &_Py_ID(isolation_level), &_Py_ID(check_same_thread), &_Py_ID(factory), &_Py_ID(cached_statements), &_Py_ID(uri), &_Py_ID(autocommit), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"database", "timeout", "detect_types", "isolation_level", "check_same_thread", "factory", "cached_statements", "uri", "autocommit", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "Connection",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[9];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *database;
    double timeout = 5.0;
    int detect_types = 0;
    const char *isolation_level = "";
    int check_same_thread = 1;
    PyObject *factory = (PyObject*)clinic_state()->ConnectionType;
    int cache_size = 128;
    int uri = 0;
    enum autocommit_mode autocommit = LEGACY_TRANSACTION_CONTROL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 8, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    database = fastargs[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[1]) {
        if (PyFloat_CheckExact(fastargs[1])) {
            timeout = PyFloat_AS_DOUBLE(fastargs[1]);
        }
        else
        {
            timeout = PyFloat_AsDouble(fastargs[1]);
            if (timeout == -1.0 && PyErr_Occurred()) {
                goto exit;
            }
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[2]) {
        detect_types = _PyLong_AsInt(fastargs[2]);
        if (detect_types == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[3]) {
        if (!isolation_level_converter(fastargs[3], &isolation_level)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[4]) {
        check_same_thread = PyObject_IsTrue(fastargs[4]);
        if (check_same_thread < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[5]) {
        factory = fastargs[5];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[6]) {
        cache_size = _PyLong_AsInt(fastargs[6]);
        if (cache_size == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[7]) {
        uri = PyObject_IsTrue(fastargs[7]);
        if (uri < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!autocommit_converter(fastargs[8], &autocommit)) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = pysqlite_connection_init_impl((pysqlite_Connection *)self, database, timeout, detect_types, isolation_level, check_same_thread, factory, cache_size, uri, autocommit);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_cursor__doc__,
"cursor($self, /, factory=<unrepresentable>)\n"
"--\n"
"\n"
"Return a cursor for the connection.");

#define PYSQLITE_CONNECTION_CURSOR_METHODDEF    \
    {"cursor", _PyCFunction_CAST(pysqlite_connection_cursor), METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_cursor__doc__},

static PyObject *
pysqlite_connection_cursor_impl(pysqlite_Connection *self, PyObject *factory);

static PyObject *
pysqlite_connection_cursor(pysqlite_Connection *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(factory), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"factory", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "cursor",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *factory = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    factory = args[0];
skip_optional_pos:
    return_value = pysqlite_connection_cursor_impl(self, factory);

exit:
    return return_value;
}

PyDoc_STRVAR(blobopen__doc__,
"blobopen($self, table, column, row, /, *, readonly=False, name=\'main\')\n"
"--\n"
"\n"
"Open and return a BLOB object.\n"
"\n"
"  table\n"
"    Table name.\n"
"  column\n"
"    Column name.\n"
"  row\n"
"    Row index.\n"
"  readonly\n"
"    Open the BLOB without write permissions.\n"
"  name\n"
"    Database name.");

#define BLOBOPEN_METHODDEF    \
    {"blobopen", _PyCFunction_CAST(blobopen), METH_FASTCALL|METH_KEYWORDS, blobopen__doc__},

static PyObject *
blobopen_impl(pysqlite_Connection *self, const char *table, const char *col,
              sqlite3_int64 row, int readonly, const char *name);

static PyObject *
blobopen(pysqlite_Connection *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(readonly), &_Py_ID(name), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "", "readonly", "name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "blobopen",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 3;
    const char *table;
    const char *col;
    sqlite3_int64 row;
    int readonly = 0;
    const char *name = "main";

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("blobopen", "argument 1", "str", args[0]);
        goto exit;
    }
    Py_ssize_t table_length;
    table = PyUnicode_AsUTF8AndSize(args[0], &table_length);
    if (table == NULL) {
        goto exit;
    }
    if (strlen(table) != (size_t)table_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("blobopen", "argument 2", "str", args[1]);
        goto exit;
    }
    Py_ssize_t col_length;
    col = PyUnicode_AsUTF8AndSize(args[1], &col_length);
    if (col == NULL) {
        goto exit;
    }
    if (strlen(col) != (size_t)col_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    if (!sqlite3_int64_converter(args[2], &row)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[3]) {
        readonly = PyObject_IsTrue(args[3]);
        if (readonly < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyUnicode_Check(args[4])) {
        _PyArg_BadArgument("blobopen", "argument 'name'", "str", args[4]);
        goto exit;
    }
    Py_ssize_t name_length;
    name = PyUnicode_AsUTF8AndSize(args[4], &name_length);
    if (name == NULL) {
        goto exit;
    }
    if (strlen(name) != (size_t)name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
skip_optional_kwonly:
    return_value = blobopen_impl(self, table, col, row, readonly, name);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the database connection.\n"
"\n"
"Any pending transaction is not committed implicitly.");

#define PYSQLITE_CONNECTION_CLOSE_METHODDEF    \
    {"close", (PyCFunction)pysqlite_connection_close, METH_NOARGS, pysqlite_connection_close__doc__},

static PyObject *
pysqlite_connection_close_impl(pysqlite_Connection *self);

static PyObject *
pysqlite_connection_close(pysqlite_Connection *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_connection_close_impl(self);
}

PyDoc_STRVAR(pysqlite_connection_commit__doc__,
"commit($self, /)\n"
"--\n"
"\n"
"Commit any pending transaction to the database.\n"
"\n"
"If there is no open transaction, this method is a no-op.");

#define PYSQLITE_CONNECTION_COMMIT_METHODDEF    \
    {"commit", (PyCFunction)pysqlite_connection_commit, METH_NOARGS, pysqlite_connection_commit__doc__},

static PyObject *
pysqlite_connection_commit_impl(pysqlite_Connection *self);

static PyObject *
pysqlite_connection_commit(pysqlite_Connection *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_connection_commit_impl(self);
}

PyDoc_STRVAR(pysqlite_connection_rollback__doc__,
"rollback($self, /)\n"
"--\n"
"\n"
"Roll back to the start of any pending transaction.\n"
"\n"
"If there is no open transaction, this method is a no-op.");

#define PYSQLITE_CONNECTION_ROLLBACK_METHODDEF    \
    {"rollback", (PyCFunction)pysqlite_connection_rollback, METH_NOARGS, pysqlite_connection_rollback__doc__},

static PyObject *
pysqlite_connection_rollback_impl(pysqlite_Connection *self);

static PyObject *
pysqlite_connection_rollback(pysqlite_Connection *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_connection_rollback_impl(self);
}

PyDoc_STRVAR(pysqlite_connection_create_function__doc__,
"create_function($self, /, name, narg, func, *, deterministic=False)\n"
"--\n"
"\n"
"Creates a new function.");

#define PYSQLITE_CONNECTION_CREATE_FUNCTION_METHODDEF    \
    {"create_function", _PyCFunction_CAST(pysqlite_connection_create_function), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_create_function__doc__},

static PyObject *
pysqlite_connection_create_function_impl(pysqlite_Connection *self,
                                         PyTypeObject *cls, const char *name,
                                         int narg, PyObject *func,
                                         int deterministic);

static PyObject *
pysqlite_connection_create_function(pysqlite_Connection *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(name), &_Py_ID(narg), &_Py_ID(func), &_Py_ID(deterministic), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "narg", "func", "deterministic", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "create_function",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 3;
    const char *name;
    int narg;
    PyObject *func;
    int deterministic = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("create_function", "argument 'name'", "str", args[0]);
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
    narg = _PyLong_AsInt(args[1]);
    if (narg == -1 && PyErr_Occurred()) {
        goto exit;
    }
    func = args[2];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    deterministic = PyObject_IsTrue(args[3]);
    if (deterministic < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = pysqlite_connection_create_function_impl(self, cls, name, narg, func, deterministic);

exit:
    return return_value;
}

#if defined(HAVE_WINDOW_FUNCTIONS)

PyDoc_STRVAR(create_window_function__doc__,
"create_window_function($self, name, num_params, aggregate_class, /)\n"
"--\n"
"\n"
"Creates or redefines an aggregate window function. Non-standard.\n"
"\n"
"  name\n"
"    The name of the SQL aggregate window function to be created or\n"
"    redefined.\n"
"  num_params\n"
"    The number of arguments the step and inverse methods takes.\n"
"  aggregate_class\n"
"    A class with step(), finalize(), value(), and inverse() methods.\n"
"    Set to None to clear the window function.");

#define CREATE_WINDOW_FUNCTION_METHODDEF    \
    {"create_window_function", _PyCFunction_CAST(create_window_function), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, create_window_function__doc__},

static PyObject *
create_window_function_impl(pysqlite_Connection *self, PyTypeObject *cls,
                            const char *name, int num_params,
                            PyObject *aggregate_class);

static PyObject *
create_window_function(pysqlite_Connection *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "create_window_function",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    const char *name;
    int num_params;
    PyObject *aggregate_class;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("create_window_function", "argument 1", "str", args[0]);
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
    num_params = _PyLong_AsInt(args[1]);
    if (num_params == -1 && PyErr_Occurred()) {
        goto exit;
    }
    aggregate_class = args[2];
    return_value = create_window_function_impl(self, cls, name, num_params, aggregate_class);

exit:
    return return_value;
}

#endif /* defined(HAVE_WINDOW_FUNCTIONS) */

PyDoc_STRVAR(pysqlite_connection_create_aggregate__doc__,
"create_aggregate($self, /, name, n_arg, aggregate_class)\n"
"--\n"
"\n"
"Creates a new aggregate.");

#define PYSQLITE_CONNECTION_CREATE_AGGREGATE_METHODDEF    \
    {"create_aggregate", _PyCFunction_CAST(pysqlite_connection_create_aggregate), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_create_aggregate__doc__},

static PyObject *
pysqlite_connection_create_aggregate_impl(pysqlite_Connection *self,
                                          PyTypeObject *cls,
                                          const char *name, int n_arg,
                                          PyObject *aggregate_class);

static PyObject *
pysqlite_connection_create_aggregate(pysqlite_Connection *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(name), &_Py_ID(n_arg), &_Py_ID(aggregate_class), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "n_arg", "aggregate_class", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "create_aggregate",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    const char *name;
    int n_arg;
    PyObject *aggregate_class;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("create_aggregate", "argument 'name'", "str", args[0]);
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
    n_arg = _PyLong_AsInt(args[1]);
    if (n_arg == -1 && PyErr_Occurred()) {
        goto exit;
    }
    aggregate_class = args[2];
    return_value = pysqlite_connection_create_aggregate_impl(self, cls, name, n_arg, aggregate_class);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_set_authorizer__doc__,
"set_authorizer($self, /, authorizer_callback)\n"
"--\n"
"\n"
"Sets authorizer callback.");

#define PYSQLITE_CONNECTION_SET_AUTHORIZER_METHODDEF    \
    {"set_authorizer", _PyCFunction_CAST(pysqlite_connection_set_authorizer), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_set_authorizer__doc__},

static PyObject *
pysqlite_connection_set_authorizer_impl(pysqlite_Connection *self,
                                        PyTypeObject *cls,
                                        PyObject *callable);

static PyObject *
pysqlite_connection_set_authorizer(pysqlite_Connection *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(authorizer_callback), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"authorizer_callback", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_authorizer",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *callable;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    callable = args[0];
    return_value = pysqlite_connection_set_authorizer_impl(self, cls, callable);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_set_progress_handler__doc__,
"set_progress_handler($self, /, progress_handler, n)\n"
"--\n"
"\n"
"Sets progress handler callback.");

#define PYSQLITE_CONNECTION_SET_PROGRESS_HANDLER_METHODDEF    \
    {"set_progress_handler", _PyCFunction_CAST(pysqlite_connection_set_progress_handler), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_set_progress_handler__doc__},

static PyObject *
pysqlite_connection_set_progress_handler_impl(pysqlite_Connection *self,
                                              PyTypeObject *cls,
                                              PyObject *callable, int n);

static PyObject *
pysqlite_connection_set_progress_handler(pysqlite_Connection *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(progress_handler), &_Py_ID(n), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"progress_handler", "n", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_progress_handler",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *callable;
    int n;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    callable = args[0];
    n = _PyLong_AsInt(args[1]);
    if (n == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = pysqlite_connection_set_progress_handler_impl(self, cls, callable, n);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_set_trace_callback__doc__,
"set_trace_callback($self, /, trace_callback)\n"
"--\n"
"\n"
"Sets a trace callback called for each SQL statement (passed as unicode).");

#define PYSQLITE_CONNECTION_SET_TRACE_CALLBACK_METHODDEF    \
    {"set_trace_callback", _PyCFunction_CAST(pysqlite_connection_set_trace_callback), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_set_trace_callback__doc__},

static PyObject *
pysqlite_connection_set_trace_callback_impl(pysqlite_Connection *self,
                                            PyTypeObject *cls,
                                            PyObject *callable);

static PyObject *
pysqlite_connection_set_trace_callback(pysqlite_Connection *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(trace_callback), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"trace_callback", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_trace_callback",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *callable;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    callable = args[0];
    return_value = pysqlite_connection_set_trace_callback_impl(self, cls, callable);

exit:
    return return_value;
}

#if defined(PY_SQLITE_ENABLE_LOAD_EXTENSION)

PyDoc_STRVAR(pysqlite_connection_enable_load_extension__doc__,
"enable_load_extension($self, enable, /)\n"
"--\n"
"\n"
"Enable dynamic loading of SQLite extension modules.");

#define PYSQLITE_CONNECTION_ENABLE_LOAD_EXTENSION_METHODDEF    \
    {"enable_load_extension", (PyCFunction)pysqlite_connection_enable_load_extension, METH_O, pysqlite_connection_enable_load_extension__doc__},

static PyObject *
pysqlite_connection_enable_load_extension_impl(pysqlite_Connection *self,
                                               int onoff);

static PyObject *
pysqlite_connection_enable_load_extension(pysqlite_Connection *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int onoff;

    onoff = PyObject_IsTrue(arg);
    if (onoff < 0) {
        goto exit;
    }
    return_value = pysqlite_connection_enable_load_extension_impl(self, onoff);

exit:
    return return_value;
}

#endif /* defined(PY_SQLITE_ENABLE_LOAD_EXTENSION) */

#if defined(PY_SQLITE_ENABLE_LOAD_EXTENSION)

PyDoc_STRVAR(pysqlite_connection_load_extension__doc__,
"load_extension($self, name, /, *, entrypoint=None)\n"
"--\n"
"\n"
"Load SQLite extension module.");

#define PYSQLITE_CONNECTION_LOAD_EXTENSION_METHODDEF    \
    {"load_extension", _PyCFunction_CAST(pysqlite_connection_load_extension), METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_load_extension__doc__},

static PyObject *
pysqlite_connection_load_extension_impl(pysqlite_Connection *self,
                                        const char *extension_name,
                                        const char *entrypoint);

static PyObject *
pysqlite_connection_load_extension(pysqlite_Connection *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(entrypoint), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "entrypoint", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "load_extension",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    const char *extension_name;
    const char *entrypoint = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("load_extension", "argument 1", "str", args[0]);
        goto exit;
    }
    Py_ssize_t extension_name_length;
    extension_name = PyUnicode_AsUTF8AndSize(args[0], &extension_name_length);
    if (extension_name == NULL) {
        goto exit;
    }
    if (strlen(extension_name) != (size_t)extension_name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[1] == Py_None) {
        entrypoint = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t entrypoint_length;
        entrypoint = PyUnicode_AsUTF8AndSize(args[1], &entrypoint_length);
        if (entrypoint == NULL) {
            goto exit;
        }
        if (strlen(entrypoint) != (size_t)entrypoint_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("load_extension", "argument 'entrypoint'", "str or None", args[1]);
        goto exit;
    }
skip_optional_kwonly:
    return_value = pysqlite_connection_load_extension_impl(self, extension_name, entrypoint);

exit:
    return return_value;
}

#endif /* defined(PY_SQLITE_ENABLE_LOAD_EXTENSION) */

PyDoc_STRVAR(pysqlite_connection_execute__doc__,
"execute($self, sql, parameters=<unrepresentable>, /)\n"
"--\n"
"\n"
"Executes an SQL statement.");

#define PYSQLITE_CONNECTION_EXECUTE_METHODDEF    \
    {"execute", _PyCFunction_CAST(pysqlite_connection_execute), METH_FASTCALL, pysqlite_connection_execute__doc__},

static PyObject *
pysqlite_connection_execute_impl(pysqlite_Connection *self, PyObject *sql,
                                 PyObject *parameters);

static PyObject *
pysqlite_connection_execute(pysqlite_Connection *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *sql;
    PyObject *parameters = NULL;

    if (!_PyArg_CheckPositional("execute", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("execute", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    sql = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    parameters = args[1];
skip_optional:
    return_value = pysqlite_connection_execute_impl(self, sql, parameters);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_executemany__doc__,
"executemany($self, sql, parameters, /)\n"
"--\n"
"\n"
"Repeatedly executes an SQL statement.");

#define PYSQLITE_CONNECTION_EXECUTEMANY_METHODDEF    \
    {"executemany", _PyCFunction_CAST(pysqlite_connection_executemany), METH_FASTCALL, pysqlite_connection_executemany__doc__},

static PyObject *
pysqlite_connection_executemany_impl(pysqlite_Connection *self,
                                     PyObject *sql, PyObject *parameters);

static PyObject *
pysqlite_connection_executemany(pysqlite_Connection *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *sql;
    PyObject *parameters;

    if (!_PyArg_CheckPositional("executemany", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("executemany", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    sql = args[0];
    parameters = args[1];
    return_value = pysqlite_connection_executemany_impl(self, sql, parameters);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_executescript__doc__,
"executescript($self, sql_script, /)\n"
"--\n"
"\n"
"Executes multiple SQL statements at once.");

#define PYSQLITE_CONNECTION_EXECUTESCRIPT_METHODDEF    \
    {"executescript", (PyCFunction)pysqlite_connection_executescript, METH_O, pysqlite_connection_executescript__doc__},

PyDoc_STRVAR(pysqlite_connection_interrupt__doc__,
"interrupt($self, /)\n"
"--\n"
"\n"
"Abort any pending database operation.");

#define PYSQLITE_CONNECTION_INTERRUPT_METHODDEF    \
    {"interrupt", (PyCFunction)pysqlite_connection_interrupt, METH_NOARGS, pysqlite_connection_interrupt__doc__},

static PyObject *
pysqlite_connection_interrupt_impl(pysqlite_Connection *self);

static PyObject *
pysqlite_connection_interrupt(pysqlite_Connection *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_connection_interrupt_impl(self);
}

PyDoc_STRVAR(pysqlite_connection_iterdump__doc__,
"iterdump($self, /)\n"
"--\n"
"\n"
"Returns iterator to the dump of the database in an SQL text format.");

#define PYSQLITE_CONNECTION_ITERDUMP_METHODDEF    \
    {"iterdump", (PyCFunction)pysqlite_connection_iterdump, METH_NOARGS, pysqlite_connection_iterdump__doc__},

static PyObject *
pysqlite_connection_iterdump_impl(pysqlite_Connection *self);

static PyObject *
pysqlite_connection_iterdump(pysqlite_Connection *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_connection_iterdump_impl(self);
}

PyDoc_STRVAR(pysqlite_connection_backup__doc__,
"backup($self, /, target, *, pages=-1, progress=None, name=\'main\',\n"
"       sleep=0.25)\n"
"--\n"
"\n"
"Makes a backup of the database.");

#define PYSQLITE_CONNECTION_BACKUP_METHODDEF    \
    {"backup", _PyCFunction_CAST(pysqlite_connection_backup), METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_backup__doc__},

static PyObject *
pysqlite_connection_backup_impl(pysqlite_Connection *self,
                                pysqlite_Connection *target, int pages,
                                PyObject *progress, const char *name,
                                double sleep);

static PyObject *
pysqlite_connection_backup(pysqlite_Connection *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(target), &_Py_ID(pages), &_Py_ID(progress), &_Py_ID(name), &_Py_ID(sleep), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"target", "pages", "progress", "name", "sleep", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "backup",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    pysqlite_Connection *target;
    int pages = -1;
    PyObject *progress = Py_None;
    const char *name = "main";
    double sleep = 0.25;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyObject_TypeCheck(args[0], clinic_state()->ConnectionType)) {
        _PyArg_BadArgument("backup", "argument 'target'", (clinic_state()->ConnectionType)->tp_name, args[0]);
        goto exit;
    }
    target = (pysqlite_Connection *)args[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[1]) {
        pages = _PyLong_AsInt(args[1]);
        if (pages == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[2]) {
        progress = args[2];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[3]) {
        if (!PyUnicode_Check(args[3])) {
            _PyArg_BadArgument("backup", "argument 'name'", "str", args[3]);
            goto exit;
        }
        Py_ssize_t name_length;
        name = PyUnicode_AsUTF8AndSize(args[3], &name_length);
        if (name == NULL) {
            goto exit;
        }
        if (strlen(name) != (size_t)name_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (PyFloat_CheckExact(args[4])) {
        sleep = PyFloat_AS_DOUBLE(args[4]);
    }
    else
    {
        sleep = PyFloat_AsDouble(args[4]);
        if (sleep == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
skip_optional_kwonly:
    return_value = pysqlite_connection_backup_impl(self, target, pages, progress, name, sleep);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_create_collation__doc__,
"create_collation($self, name, callback, /)\n"
"--\n"
"\n"
"Creates a collation function.");

#define PYSQLITE_CONNECTION_CREATE_COLLATION_METHODDEF    \
    {"create_collation", _PyCFunction_CAST(pysqlite_connection_create_collation), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_create_collation__doc__},

static PyObject *
pysqlite_connection_create_collation_impl(pysqlite_Connection *self,
                                          PyTypeObject *cls,
                                          const char *name,
                                          PyObject *callable);

static PyObject *
pysqlite_connection_create_collation(pysqlite_Connection *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "create_collation",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    const char *name;
    PyObject *callable;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("create_collation", "argument 1", "str", args[0]);
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
    callable = args[1];
    return_value = pysqlite_connection_create_collation_impl(self, cls, name, callable);

exit:
    return return_value;
}

#if defined(PY_SQLITE_HAVE_SERIALIZE)

PyDoc_STRVAR(serialize__doc__,
"serialize($self, /, *, name=\'main\')\n"
"--\n"
"\n"
"Serialize a database into a byte string.\n"
"\n"
"  name\n"
"    Which database to serialize.\n"
"\n"
"For an ordinary on-disk database file, the serialization is just a copy of the\n"
"disk file. For an in-memory database or a \"temp\" database, the serialization is\n"
"the same sequence of bytes which would be written to disk if that database\n"
"were backed up to disk.");

#define SERIALIZE_METHODDEF    \
    {"serialize", _PyCFunction_CAST(serialize), METH_FASTCALL|METH_KEYWORDS, serialize__doc__},

static PyObject *
serialize_impl(pysqlite_Connection *self, const char *name);

static PyObject *
serialize(pysqlite_Connection *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "serialize",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    const char *name = "main";

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 0, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("serialize", "argument 'name'", "str", args[0]);
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
skip_optional_kwonly:
    return_value = serialize_impl(self, name);

exit:
    return return_value;
}

#endif /* defined(PY_SQLITE_HAVE_SERIALIZE) */

#if defined(PY_SQLITE_HAVE_SERIALIZE)

PyDoc_STRVAR(deserialize__doc__,
"deserialize($self, data, /, *, name=\'main\')\n"
"--\n"
"\n"
"Load a serialized database.\n"
"\n"
"  data\n"
"    The serialized database content.\n"
"  name\n"
"    Which database to reopen with the deserialization.\n"
"\n"
"The deserialize interface causes the database connection to disconnect from the\n"
"target database, and then reopen it as an in-memory database based on the given\n"
"serialized data.\n"
"\n"
"The deserialize interface will fail with SQLITE_BUSY if the database is\n"
"currently in a read transaction or is involved in a backup operation.");

#define DESERIALIZE_METHODDEF    \
    {"deserialize", _PyCFunction_CAST(deserialize), METH_FASTCALL|METH_KEYWORDS, deserialize__doc__},

static PyObject *
deserialize_impl(pysqlite_Connection *self, Py_buffer *data,
                 const char *name);

static PyObject *
deserialize(pysqlite_Connection *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(name), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "deserialize",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    const char *name = "main";

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyUnicode_Check(args[0])) {
        Py_ssize_t len;
        const char *ptr = PyUnicode_AsUTF8AndSize(args[0], &len);
        if (ptr == NULL) {
            goto exit;
        }
        PyBuffer_FillInfo(&data, args[0], (void *)ptr, len, 1, 0);
    }
    else { /* any bytes-like object */
        if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
        if (!PyBuffer_IsContiguous(&data, 'C')) {
            _PyArg_BadArgument("deserialize", "argument 1", "contiguous buffer", args[0]);
            goto exit;
        }
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("deserialize", "argument 'name'", "str", args[1]);
        goto exit;
    }
    Py_ssize_t name_length;
    name = PyUnicode_AsUTF8AndSize(args[1], &name_length);
    if (name == NULL) {
        goto exit;
    }
    if (strlen(name) != (size_t)name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
skip_optional_kwonly:
    return_value = deserialize_impl(self, &data, name);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

#endif /* defined(PY_SQLITE_HAVE_SERIALIZE) */

PyDoc_STRVAR(pysqlite_connection_enter__doc__,
"__enter__($self, /)\n"
"--\n"
"\n"
"Called when the connection is used as a context manager.\n"
"\n"
"Returns itself as a convenience to the caller.");

#define PYSQLITE_CONNECTION_ENTER_METHODDEF    \
    {"__enter__", (PyCFunction)pysqlite_connection_enter, METH_NOARGS, pysqlite_connection_enter__doc__},

static PyObject *
pysqlite_connection_enter_impl(pysqlite_Connection *self);

static PyObject *
pysqlite_connection_enter(pysqlite_Connection *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_connection_enter_impl(self);
}

PyDoc_STRVAR(pysqlite_connection_exit__doc__,
"__exit__($self, type, value, traceback, /)\n"
"--\n"
"\n"
"Called when the connection is used as a context manager.\n"
"\n"
"If there was any exception, a rollback takes place; otherwise we commit.");

#define PYSQLITE_CONNECTION_EXIT_METHODDEF    \
    {"__exit__", _PyCFunction_CAST(pysqlite_connection_exit), METH_FASTCALL, pysqlite_connection_exit__doc__},

static PyObject *
pysqlite_connection_exit_impl(pysqlite_Connection *self, PyObject *exc_type,
                              PyObject *exc_value, PyObject *exc_tb);

static PyObject *
pysqlite_connection_exit(pysqlite_Connection *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *exc_type;
    PyObject *exc_value;
    PyObject *exc_tb;

    if (!_PyArg_CheckPositional("__exit__", nargs, 3, 3)) {
        goto exit;
    }
    exc_type = args[0];
    exc_value = args[1];
    exc_tb = args[2];
    return_value = pysqlite_connection_exit_impl(self, exc_type, exc_value, exc_tb);

exit:
    return return_value;
}

PyDoc_STRVAR(setlimit__doc__,
"setlimit($self, category, limit, /)\n"
"--\n"
"\n"
"Set connection run-time limits.\n"
"\n"
"  category\n"
"    The limit category to be set.\n"
"  limit\n"
"    The new limit. If the new limit is a negative number, the limit is\n"
"    unchanged.\n"
"\n"
"Attempts to increase a limit above its hard upper bound are silently truncated\n"
"to the hard upper bound. Regardless of whether or not the limit was changed,\n"
"the prior value of the limit is returned.");

#define SETLIMIT_METHODDEF    \
    {"setlimit", _PyCFunction_CAST(setlimit), METH_FASTCALL, setlimit__doc__},

static PyObject *
setlimit_impl(pysqlite_Connection *self, int category, int limit);

static PyObject *
setlimit(pysqlite_Connection *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int category;
    int limit;

    if (!_PyArg_CheckPositional("setlimit", nargs, 2, 2)) {
        goto exit;
    }
    category = _PyLong_AsInt(args[0]);
    if (category == -1 && PyErr_Occurred()) {
        goto exit;
    }
    limit = _PyLong_AsInt(args[1]);
    if (limit == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = setlimit_impl(self, category, limit);

exit:
    return return_value;
}

PyDoc_STRVAR(getlimit__doc__,
"getlimit($self, category, /)\n"
"--\n"
"\n"
"Get connection run-time limits.\n"
"\n"
"  category\n"
"    The limit category to be queried.");

#define GETLIMIT_METHODDEF    \
    {"getlimit", (PyCFunction)getlimit, METH_O, getlimit__doc__},

static PyObject *
getlimit_impl(pysqlite_Connection *self, int category);

static PyObject *
getlimit(pysqlite_Connection *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int category;

    category = _PyLong_AsInt(arg);
    if (category == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = getlimit_impl(self, category);

exit:
    return return_value;
}

PyDoc_STRVAR(setconfig__doc__,
"setconfig($self, op, enable=True, /)\n"
"--\n"
"\n"
"Set a boolean connection configuration option.\n"
"\n"
"  op\n"
"    The configuration verb; one of the sqlite3.SQLITE_DBCONFIG codes.");

#define SETCONFIG_METHODDEF    \
    {"setconfig", _PyCFunction_CAST(setconfig), METH_FASTCALL, setconfig__doc__},

static PyObject *
setconfig_impl(pysqlite_Connection *self, int op, int enable);

static PyObject *
setconfig(pysqlite_Connection *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int op;
    int enable = 1;

    if (!_PyArg_CheckPositional("setconfig", nargs, 1, 2)) {
        goto exit;
    }
    op = _PyLong_AsInt(args[0]);
    if (op == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    enable = PyObject_IsTrue(args[1]);
    if (enable < 0) {
        goto exit;
    }
skip_optional:
    return_value = setconfig_impl(self, op, enable);

exit:
    return return_value;
}

PyDoc_STRVAR(getconfig__doc__,
"getconfig($self, op, /)\n"
"--\n"
"\n"
"Query a boolean connection configuration option.\n"
"\n"
"  op\n"
"    The configuration verb; one of the sqlite3.SQLITE_DBCONFIG codes.");

#define GETCONFIG_METHODDEF    \
    {"getconfig", (PyCFunction)getconfig, METH_O, getconfig__doc__},

static int
getconfig_impl(pysqlite_Connection *self, int op);

static PyObject *
getconfig(pysqlite_Connection *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int op;
    int _return_value;

    op = _PyLong_AsInt(arg);
    if (op == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = getconfig_impl(self, op);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

#ifndef CREATE_WINDOW_FUNCTION_METHODDEF
    #define CREATE_WINDOW_FUNCTION_METHODDEF
#endif /* !defined(CREATE_WINDOW_FUNCTION_METHODDEF) */

#ifndef PYSQLITE_CONNECTION_ENABLE_LOAD_EXTENSION_METHODDEF
    #define PYSQLITE_CONNECTION_ENABLE_LOAD_EXTENSION_METHODDEF
#endif /* !defined(PYSQLITE_CONNECTION_ENABLE_LOAD_EXTENSION_METHODDEF) */

#ifndef PYSQLITE_CONNECTION_LOAD_EXTENSION_METHODDEF
    #define PYSQLITE_CONNECTION_LOAD_EXTENSION_METHODDEF
#endif /* !defined(PYSQLITE_CONNECTION_LOAD_EXTENSION_METHODDEF) */

#ifndef SERIALIZE_METHODDEF
    #define SERIALIZE_METHODDEF
#endif /* !defined(SERIALIZE_METHODDEF) */

#ifndef DESERIALIZE_METHODDEF
    #define DESERIALIZE_METHODDEF
#endif /* !defined(DESERIALIZE_METHODDEF) */
/*[clinic end generated code: output=834a99827555bf1a input=a9049054013a1b77]*/
