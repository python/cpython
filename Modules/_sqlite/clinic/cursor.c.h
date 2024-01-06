/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

static int
pysqlite_cursor_init_impl(pysqlite_Cursor *self,
                          pysqlite_Connection *connection);

static int
pysqlite_cursor_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    PyTypeObject *base_tp = clinic_state()->CursorType;
    pysqlite_Connection *connection;

    if ((Py_IS_TYPE(self, base_tp) ||
         Py_TYPE(self)->tp_new == base_tp->tp_new) &&
        !_PyArg_NoKeywords("Cursor", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("Cursor", PyTuple_GET_SIZE(args), 1, 1)) {
        goto exit;
    }
    if (!PyObject_TypeCheck(PyTuple_GET_ITEM(args, 0), clinic_state()->ConnectionType)) {
        _PyArg_BadArgument("Cursor", "argument 1", (clinic_state()->ConnectionType)->tp_name, PyTuple_GET_ITEM(args, 0));
        goto exit;
    }
    connection = (pysqlite_Connection *)PyTuple_GET_ITEM(args, 0);
    return_value = pysqlite_cursor_init_impl((pysqlite_Cursor *)self, connection);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_cursor_execute__doc__,
"execute($self, sql, parameters=(), /)\n"
"--\n"
"\n"
"Executes an SQL statement.");

#define PYSQLITE_CURSOR_EXECUTE_METHODDEF    \
    {"execute", _PyCFunction_CAST(pysqlite_cursor_execute), METH_FASTCALL, pysqlite_cursor_execute__doc__},

static PyObject *
pysqlite_cursor_execute_impl(pysqlite_Cursor *self, PyObject *sql,
                             PyObject *parameters);

static PyObject *
pysqlite_cursor_execute(pysqlite_Cursor *self, PyObject *const *args, Py_ssize_t nargs)
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
    sql = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    parameters = args[1];
skip_optional:
    return_value = pysqlite_cursor_execute_impl(self, sql, parameters);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_cursor_executemany__doc__,
"executemany($self, sql, seq_of_parameters, /)\n"
"--\n"
"\n"
"Repeatedly executes an SQL statement.");

#define PYSQLITE_CURSOR_EXECUTEMANY_METHODDEF    \
    {"executemany", _PyCFunction_CAST(pysqlite_cursor_executemany), METH_FASTCALL, pysqlite_cursor_executemany__doc__},

static PyObject *
pysqlite_cursor_executemany_impl(pysqlite_Cursor *self, PyObject *sql,
                                 PyObject *seq_of_parameters);

static PyObject *
pysqlite_cursor_executemany(pysqlite_Cursor *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *sql;
    PyObject *seq_of_parameters;

    if (!_PyArg_CheckPositional("executemany", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("executemany", "argument 1", "str", args[0]);
        goto exit;
    }
    sql = args[0];
    seq_of_parameters = args[1];
    return_value = pysqlite_cursor_executemany_impl(self, sql, seq_of_parameters);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_cursor_executescript__doc__,
"executescript($self, sql_script, /)\n"
"--\n"
"\n"
"Executes multiple SQL statements at once.");

#define PYSQLITE_CURSOR_EXECUTESCRIPT_METHODDEF    \
    {"executescript", (PyCFunction)pysqlite_cursor_executescript, METH_O, pysqlite_cursor_executescript__doc__},

static PyObject *
pysqlite_cursor_executescript_impl(pysqlite_Cursor *self,
                                   const char *sql_script);

static PyObject *
pysqlite_cursor_executescript(pysqlite_Cursor *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *sql_script;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("executescript", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t sql_script_length;
    sql_script = PyUnicode_AsUTF8AndSize(arg, &sql_script_length);
    if (sql_script == NULL) {
        goto exit;
    }
    if (strlen(sql_script) != (size_t)sql_script_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = pysqlite_cursor_executescript_impl(self, sql_script);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_cursor_fetchone__doc__,
"fetchone($self, /)\n"
"--\n"
"\n"
"Fetches one row from the resultset.");

#define PYSQLITE_CURSOR_FETCHONE_METHODDEF    \
    {"fetchone", (PyCFunction)pysqlite_cursor_fetchone, METH_NOARGS, pysqlite_cursor_fetchone__doc__},

static PyObject *
pysqlite_cursor_fetchone_impl(pysqlite_Cursor *self);

static PyObject *
pysqlite_cursor_fetchone(pysqlite_Cursor *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_cursor_fetchone_impl(self);
}

PyDoc_STRVAR(pysqlite_cursor_fetchmany__doc__,
"fetchmany($self, /, size=1)\n"
"--\n"
"\n"
"Fetches several rows from the resultset.\n"
"\n"
"  size\n"
"    The default value is set by the Cursor.arraysize attribute.");

#define PYSQLITE_CURSOR_FETCHMANY_METHODDEF    \
    {"fetchmany", _PyCFunction_CAST(pysqlite_cursor_fetchmany), METH_FASTCALL|METH_KEYWORDS, pysqlite_cursor_fetchmany__doc__},

static PyObject *
pysqlite_cursor_fetchmany_impl(pysqlite_Cursor *self, int maxrows);

static PyObject *
pysqlite_cursor_fetchmany(pysqlite_Cursor *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(size), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"size", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "fetchmany",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int maxrows = self->arraysize;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    maxrows = PyLong_AsInt(args[0]);
    if (maxrows == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = pysqlite_cursor_fetchmany_impl(self, maxrows);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_cursor_fetchall__doc__,
"fetchall($self, /)\n"
"--\n"
"\n"
"Fetches all rows from the resultset.");

#define PYSQLITE_CURSOR_FETCHALL_METHODDEF    \
    {"fetchall", (PyCFunction)pysqlite_cursor_fetchall, METH_NOARGS, pysqlite_cursor_fetchall__doc__},

static PyObject *
pysqlite_cursor_fetchall_impl(pysqlite_Cursor *self);

static PyObject *
pysqlite_cursor_fetchall(pysqlite_Cursor *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_cursor_fetchall_impl(self);
}

PyDoc_STRVAR(pysqlite_cursor_setinputsizes__doc__,
"setinputsizes($self, sizes, /)\n"
"--\n"
"\n"
"Required by DB-API. Does nothing in sqlite3.");

#define PYSQLITE_CURSOR_SETINPUTSIZES_METHODDEF    \
    {"setinputsizes", (PyCFunction)pysqlite_cursor_setinputsizes, METH_O, pysqlite_cursor_setinputsizes__doc__},

PyDoc_STRVAR(pysqlite_cursor_setoutputsize__doc__,
"setoutputsize($self, size, column=None, /)\n"
"--\n"
"\n"
"Required by DB-API. Does nothing in sqlite3.");

#define PYSQLITE_CURSOR_SETOUTPUTSIZE_METHODDEF    \
    {"setoutputsize", _PyCFunction_CAST(pysqlite_cursor_setoutputsize), METH_FASTCALL, pysqlite_cursor_setoutputsize__doc__},

static PyObject *
pysqlite_cursor_setoutputsize_impl(pysqlite_Cursor *self, PyObject *size,
                                   PyObject *column);

static PyObject *
pysqlite_cursor_setoutputsize(pysqlite_Cursor *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *size;
    PyObject *column = Py_None;

    if (!_PyArg_CheckPositional("setoutputsize", nargs, 1, 2)) {
        goto exit;
    }
    size = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    column = args[1];
skip_optional:
    return_value = pysqlite_cursor_setoutputsize_impl(self, size, column);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_cursor_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Closes the cursor.");

#define PYSQLITE_CURSOR_CLOSE_METHODDEF    \
    {"close", (PyCFunction)pysqlite_cursor_close, METH_NOARGS, pysqlite_cursor_close__doc__},

static PyObject *
pysqlite_cursor_close_impl(pysqlite_Cursor *self);

static PyObject *
pysqlite_cursor_close(pysqlite_Cursor *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_cursor_close_impl(self);
}
/*[clinic end generated code: output=a8ce095c3c80cf65 input=a9049054013a1b77]*/
