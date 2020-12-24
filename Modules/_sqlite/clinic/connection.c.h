/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(pysqlite_connection_cursor__doc__,
"cursor($self, /, factory=<unrepresentable>)\n"
"--\n"
"\n"
"Return a cursor for the connection.");

#define PYSQLITE_CONNECTION_CURSOR_METHODDEF    \
    {"cursor", (PyCFunction)(void(*)(void))pysqlite_connection_cursor, METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_cursor__doc__},

static PyObject *
pysqlite_connection_cursor_impl(pysqlite_Connection *self, PyObject *factory);

static PyObject *
pysqlite_connection_cursor(pysqlite_Connection *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"factory", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "cursor", 0};
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

PyDoc_STRVAR(pysqlite_connection_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Closes the connection.");

#define PYSQLITE_CONNECTION_CLOSE_METHODDEF    \
    {"close", (PyCFunction)pysqlite_connection_close, METH_NOARGS, pysqlite_connection_close__doc__},

static PyObject *
pysqlite_connection_close_impl(pysqlite_Connection *self);

static PyObject *
pysqlite_connection_close(pysqlite_Connection *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_connection_close_impl(self);
}

PyDoc_STRVAR(pysqlite_connection_rollback__doc__,
"rollback($self, /)\n"
"--\n"
"\n"
"Roll back the current transaction.");

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
"Creates a new function. Non-standard.");

#define PYSQLITE_CONNECTION_CREATE_FUNCTION_METHODDEF    \
    {"create_function", (PyCFunction)(void(*)(void))pysqlite_connection_create_function, METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_create_function__doc__},

static PyObject *
pysqlite_connection_create_function_impl(pysqlite_Connection *self,
                                         const char *name, int narg,
                                         PyObject *func, int deterministic);

static PyObject *
pysqlite_connection_create_function(pysqlite_Connection *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"name", "narg", "func", "deterministic", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "create_function", 0};
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
    return_value = pysqlite_connection_create_function_impl(self, name, narg, func, deterministic);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_create_aggregate__doc__,
"create_aggregate($self, /, name, n_arg, aggregate_class)\n"
"--\n"
"\n"
"Creates a new aggregate. Non-standard.");

#define PYSQLITE_CONNECTION_CREATE_AGGREGATE_METHODDEF    \
    {"create_aggregate", (PyCFunction)(void(*)(void))pysqlite_connection_create_aggregate, METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_create_aggregate__doc__},

static PyObject *
pysqlite_connection_create_aggregate_impl(pysqlite_Connection *self,
                                          const char *name, int n_arg,
                                          PyObject *aggregate_class);

static PyObject *
pysqlite_connection_create_aggregate(pysqlite_Connection *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"name", "n_arg", "aggregate_class", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "create_aggregate", 0};
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
    return_value = pysqlite_connection_create_aggregate_impl(self, name, n_arg, aggregate_class);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_set_authorizer__doc__,
"set_authorizer($self, /, authorizer_callback)\n"
"--\n"
"\n"
"Sets authorizer callback. Non-standard.");

#define PYSQLITE_CONNECTION_SET_AUTHORIZER_METHODDEF    \
    {"set_authorizer", (PyCFunction)(void(*)(void))pysqlite_connection_set_authorizer, METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_set_authorizer__doc__},

static PyObject *
pysqlite_connection_set_authorizer_impl(pysqlite_Connection *self,
                                        PyObject *authorizer_cb);

static PyObject *
pysqlite_connection_set_authorizer(pysqlite_Connection *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"authorizer_callback", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "set_authorizer", 0};
    PyObject *argsbuf[1];
    PyObject *authorizer_cb;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    authorizer_cb = args[0];
    return_value = pysqlite_connection_set_authorizer_impl(self, authorizer_cb);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_set_progress_handler__doc__,
"set_progress_handler($self, /, progress_handler, n)\n"
"--\n"
"\n"
"Sets progress handler callback. Non-standard.");

#define PYSQLITE_CONNECTION_SET_PROGRESS_HANDLER_METHODDEF    \
    {"set_progress_handler", (PyCFunction)(void(*)(void))pysqlite_connection_set_progress_handler, METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_set_progress_handler__doc__},

static PyObject *
pysqlite_connection_set_progress_handler_impl(pysqlite_Connection *self,
                                              PyObject *progress_handler,
                                              int n);

static PyObject *
pysqlite_connection_set_progress_handler(pysqlite_Connection *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"progress_handler", "n", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "set_progress_handler", 0};
    PyObject *argsbuf[2];
    PyObject *progress_handler;
    int n;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    progress_handler = args[0];
    n = _PyLong_AsInt(args[1]);
    if (n == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = pysqlite_connection_set_progress_handler_impl(self, progress_handler, n);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_set_trace_callback__doc__,
"set_trace_callback($self, /, trace_callback)\n"
"--\n"
"\n"
"Sets a trace callback called for each SQL statement (passed as unicode).\n"
"\n"
"Non-standard.");

#define PYSQLITE_CONNECTION_SET_TRACE_CALLBACK_METHODDEF    \
    {"set_trace_callback", (PyCFunction)(void(*)(void))pysqlite_connection_set_trace_callback, METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_set_trace_callback__doc__},

static PyObject *
pysqlite_connection_set_trace_callback_impl(pysqlite_Connection *self,
                                            PyObject *trace_callback);

static PyObject *
pysqlite_connection_set_trace_callback(pysqlite_Connection *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"trace_callback", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "set_trace_callback", 0};
    PyObject *argsbuf[1];
    PyObject *trace_callback;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    trace_callback = args[0];
    return_value = pysqlite_connection_set_trace_callback_impl(self, trace_callback);

exit:
    return return_value;
}

#if !defined(SQLITE_OMIT_LOAD_EXTENSION)

PyDoc_STRVAR(pysqlite_connection_enable_load_extension__doc__,
"enable_load_extension($self, enable, /)\n"
"--\n"
"\n"
"Enable dynamic loading of SQLite extension modules. Non-standard.");

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

    onoff = _PyLong_AsInt(arg);
    if (onoff == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = pysqlite_connection_enable_load_extension_impl(self, onoff);

exit:
    return return_value;
}

#endif /* !defined(SQLITE_OMIT_LOAD_EXTENSION) */

#if !defined(SQLITE_OMIT_LOAD_EXTENSION)

PyDoc_STRVAR(pysqlite_connection_load_extension__doc__,
"load_extension($self, name, /)\n"
"--\n"
"\n"
"Load SQLite extension module. Non-standard.");

#define PYSQLITE_CONNECTION_LOAD_EXTENSION_METHODDEF    \
    {"load_extension", (PyCFunction)pysqlite_connection_load_extension, METH_O, pysqlite_connection_load_extension__doc__},

static PyObject *
pysqlite_connection_load_extension_impl(pysqlite_Connection *self,
                                        const char *extension_name);

static PyObject *
pysqlite_connection_load_extension(pysqlite_Connection *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *extension_name;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("load_extension", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t extension_name_length;
    extension_name = PyUnicode_AsUTF8AndSize(arg, &extension_name_length);
    if (extension_name == NULL) {
        goto exit;
    }
    if (strlen(extension_name) != (size_t)extension_name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = pysqlite_connection_load_extension_impl(self, extension_name);

exit:
    return return_value;
}

#endif /* !defined(SQLITE_OMIT_LOAD_EXTENSION) */

PyDoc_STRVAR(pysqlite_connection_interrupt__doc__,
"interrupt($self, /)\n"
"--\n"
"\n"
"Abort any pending database operation. Non-standard.");

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
"Returns iterator to the dump of the database in an SQL text format.\n"
"\n"
"Non-standard.");

#define PYSQLITE_CONNECTION_ITERDUMP_METHODDEF    \
    {"iterdump", (PyCFunction)pysqlite_connection_iterdump, METH_NOARGS, pysqlite_connection_iterdump__doc__},

static PyObject *
pysqlite_connection_iterdump_impl(pysqlite_Connection *self);

static PyObject *
pysqlite_connection_iterdump(pysqlite_Connection *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_connection_iterdump_impl(self);
}

PyDoc_STRVAR(pysqlite_connection_create_collation__doc__,
"create_collation($self, name, callback, /)\n"
"--\n"
"\n"
"Creates a collation function. Non-standard.");

#define PYSQLITE_CONNECTION_CREATE_COLLATION_METHODDEF    \
    {"create_collation", (PyCFunction)(void(*)(void))pysqlite_connection_create_collation, METH_FASTCALL, pysqlite_connection_create_collation__doc__},

static PyObject *
pysqlite_connection_create_collation_impl(pysqlite_Connection *self,
                                          PyObject *name, PyObject *callable);

static PyObject *
pysqlite_connection_create_collation(pysqlite_Connection *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *name;
    PyObject *callable;

    if (!_PyArg_CheckPositional("create_collation", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("create_collation", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    name = args[0];
    callable = args[1];
    return_value = pysqlite_connection_create_collation_impl(self, name, callable);

exit:
    return return_value;
}

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
    {"__exit__", (PyCFunction)(void(*)(void))pysqlite_connection_exit, METH_FASTCALL, pysqlite_connection_exit__doc__},

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

#ifndef PYSQLITE_CONNECTION_ENABLE_LOAD_EXTENSION_METHODDEF
    #define PYSQLITE_CONNECTION_ENABLE_LOAD_EXTENSION_METHODDEF
#endif /* !defined(PYSQLITE_CONNECTION_ENABLE_LOAD_EXTENSION_METHODDEF) */

#ifndef PYSQLITE_CONNECTION_LOAD_EXTENSION_METHODDEF
    #define PYSQLITE_CONNECTION_LOAD_EXTENSION_METHODDEF
#endif /* !defined(PYSQLITE_CONNECTION_LOAD_EXTENSION_METHODDEF) */
/*[clinic end generated code: output=eb14a52e4c682f3b input=a9049054013a1b77]*/
