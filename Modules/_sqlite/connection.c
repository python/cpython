/* connection.c - the connection type
 *
 * Copyright (C) 2004-2010 Gerhard Häring <gh@ghaering.de>
 *
 * This file is part of pysqlite.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "module.h"
#include "structmember.h"         // PyMemberDef
#include "connection.h"
#include "statement.h"
#include "cursor.h"
#include "prepare_protocol.h"
#include "util.h"

#define ACTION_FINALIZE 1
#define ACTION_RESET 2

#if SQLITE_VERSION_NUMBER >= 3014000
#define HAVE_TRACE_V2
#endif

#define clinic_state() (pysqlite_get_state(NULL))
#include "clinic/connection.c.h"
#undef clinic_state

/*[clinic input]
module _sqlite3
class _sqlite3.Connection "pysqlite_Connection *" "clinic_state()->ConnectionType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=67369db2faf80891]*/

_Py_IDENTIFIER(cursor);

static const char * const begin_statements[] = {
    "BEGIN ",
    "BEGIN DEFERRED",
    "BEGIN IMMEDIATE",
    "BEGIN EXCLUSIVE",
    NULL
};

static int pysqlite_connection_set_isolation_level(pysqlite_Connection* self, PyObject* isolation_level, void *Py_UNUSED(ignored));
static void _pysqlite_drop_unused_cursor_references(pysqlite_Connection* self);

static PyObject *
new_statement_cache(pysqlite_Connection *self, int maxsize)
{
    PyObject *args[] = { PyLong_FromLong(maxsize), };
    if (args[0] == NULL) {
        return NULL;
    }
    pysqlite_state *state = pysqlite_get_state(NULL);
    PyObject *inner = PyObject_Vectorcall(state->lru_cache, args, 1, NULL);
    Py_DECREF(args[0]);
    if (inner == NULL) {
        return NULL;
    }

    args[0] = (PyObject *)self;  // Borrowed ref.
    PyObject *res = PyObject_Vectorcall(inner, args, 1, NULL);
    Py_DECREF(inner);
    return res;
}

/*[clinic input]
_sqlite3.Connection.__init__ as pysqlite_connection_init

    database as database_obj: object(converter='PyUnicode_FSConverter')
    timeout: double = 5.0
    detect_types: int = 0
    isolation_level: object = NULL
    check_same_thread: bool(accept={int}) = True
    factory: object(c_default='(PyObject*)clinic_state()->ConnectionType') = ConnectionType
    cached_statements: int = 128
    uri: bool = False
[clinic start generated code]*/

static int
pysqlite_connection_init_impl(pysqlite_Connection *self,
                              PyObject *database_obj, double timeout,
                              int detect_types, PyObject *isolation_level,
                              int check_same_thread, PyObject *factory,
                              int cached_statements, int uri)
/*[clinic end generated code: output=dc19df1c0e2b7b77 input=aa1f21bf12fe907a]*/
{
    int rc;

    if (PySys_Audit("sqlite3.connect", "O", database_obj) < 0) {
        return -1;
    }

    const char *database = PyBytes_AsString(database_obj);

    self->initialized = 1;

    self->begin_statement = NULL;

    Py_CLEAR(self->statement_cache);
    Py_CLEAR(self->statements);
    Py_CLEAR(self->cursors);

    Py_INCREF(Py_None);
    Py_XSETREF(self->row_factory, Py_None);

    Py_INCREF(&PyUnicode_Type);
    Py_XSETREF(self->text_factory, (PyObject*)&PyUnicode_Type);

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_open_v2(database, &self->db,
                         SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                         (uri ? SQLITE_OPEN_URI : 0), NULL);
    Py_END_ALLOW_THREADS

    Py_DECREF(database_obj);  // needed bco. the AC FSConverter

    if (rc != SQLITE_OK) {
        _pysqlite_seterror(self->db);
        return -1;
    }

    if (!isolation_level) {
        isolation_level = PyUnicode_FromString("");
        if (!isolation_level) {
            return -1;
        }
    } else {
        Py_INCREF(isolation_level);
    }
    Py_CLEAR(self->isolation_level);
    if (pysqlite_connection_set_isolation_level(self, isolation_level, NULL) < 0) {
        Py_DECREF(isolation_level);
        return -1;
    }
    Py_DECREF(isolation_level);

    self->statement_cache = new_statement_cache(self, cached_statements);
    if (self->statement_cache == NULL) {
        return -1;
    }
    if (PyErr_Occurred()) {
        return -1;
    }

    self->created_statements = 0;
    self->created_cursors = 0;

    /* Create lists of weak references to statements/cursors */
    self->statements = PyList_New(0);
    self->cursors = PyList_New(0);
    if (!self->statements || !self->cursors) {
        return -1;
    }

    self->detect_types = detect_types;
    (void)sqlite3_busy_timeout(self->db, (int)(timeout*1000));
    self->thread_ident = PyThread_get_thread_ident();
    self->check_same_thread = check_same_thread;

    self->function_pinboard_trace_callback = NULL;
    self->function_pinboard_progress_handler = NULL;
    self->function_pinboard_authorizer_cb = NULL;

    Py_XSETREF(self->collations, PyDict_New());
    if (!self->collations) {
        return -1;
    }

    pysqlite_state *state = pysqlite_get_state(NULL);
    self->Warning               = state->Warning;
    self->Error                 = state->Error;
    self->InterfaceError        = state->InterfaceError;
    self->DatabaseError         = state->DatabaseError;
    self->DataError             = pysqlite_DataError;
    self->OperationalError      = pysqlite_OperationalError;
    self->IntegrityError        = pysqlite_IntegrityError;
    self->InternalError         = state->InternalError;
    self->ProgrammingError      = pysqlite_ProgrammingError;
    self->NotSupportedError     = pysqlite_NotSupportedError;

    if (PySys_Audit("sqlite3.connect/handle", "O", self) < 0) {
        return -1;
    }

    return 0;
}

/* action in (ACTION_RESET, ACTION_FINALIZE) */
static void
pysqlite_do_all_statements(pysqlite_Connection *self, int action,
                           int reset_cursors)
{
    int i;
    PyObject* weakref;
    PyObject* statement;
    pysqlite_Cursor* cursor;

    for (i = 0; i < PyList_Size(self->statements); i++) {
        weakref = PyList_GetItem(self->statements, i);
        statement = PyWeakref_GetObject(weakref);
        if (statement != Py_None) {
            Py_INCREF(statement);
            if (action == ACTION_RESET) {
                (void)pysqlite_statement_reset((pysqlite_Statement*)statement);
            } else {
                (void)pysqlite_statement_finalize((pysqlite_Statement*)statement);
            }
            Py_DECREF(statement);
        }
    }

    if (reset_cursors) {
        for (i = 0; i < PyList_Size(self->cursors); i++) {
            weakref = PyList_GetItem(self->cursors, i);
            cursor = (pysqlite_Cursor*)PyWeakref_GetObject(weakref);
            if ((PyObject*)cursor != Py_None) {
                cursor->reset = 1;
            }
        }
    }
}

static int
connection_traverse(pysqlite_Connection *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->isolation_level);
    Py_VISIT(self->statement_cache);
    Py_VISIT(self->statements);
    Py_VISIT(self->cursors);
    Py_VISIT(self->row_factory);
    Py_VISIT(self->text_factory);
    Py_VISIT(self->function_pinboard_trace_callback);
    Py_VISIT(self->function_pinboard_progress_handler);
    Py_VISIT(self->function_pinboard_authorizer_cb);
    Py_VISIT(self->collations);
    return 0;
}

static int
connection_clear(pysqlite_Connection *self)
{
    Py_CLEAR(self->isolation_level);
    Py_CLEAR(self->statement_cache);
    Py_CLEAR(self->statements);
    Py_CLEAR(self->cursors);
    Py_CLEAR(self->row_factory);
    Py_CLEAR(self->text_factory);
    Py_CLEAR(self->function_pinboard_trace_callback);
    Py_CLEAR(self->function_pinboard_progress_handler);
    Py_CLEAR(self->function_pinboard_authorizer_cb);
    Py_CLEAR(self->collations);
    return 0;
}

static void
connection_close(pysqlite_Connection *self)
{
    if (self->db) {
        int rc = sqlite3_close_v2(self->db);
        assert(rc == SQLITE_OK), (void)rc;
        self->db = NULL;
    }
}

static void
connection_dealloc(pysqlite_Connection *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    tp->tp_clear((PyObject *)self);

    /* Clean up if user has not called .close() explicitly. */
    connection_close(self);

    tp->tp_free(self);
    Py_DECREF(tp);
}

/*
 * Registers a cursor with the connection.
 *
 * 0 => error; 1 => ok
 */
int pysqlite_connection_register_cursor(pysqlite_Connection* connection, PyObject* cursor)
{
    PyObject* weakref;

    weakref = PyWeakref_NewRef((PyObject*)cursor, NULL);
    if (!weakref) {
        goto error;
    }

    if (PyList_Append(connection->cursors, weakref) != 0) {
        Py_CLEAR(weakref);
        goto error;
    }

    Py_DECREF(weakref);

    return 1;
error:
    return 0;
}

/*[clinic input]
_sqlite3.Connection.cursor as pysqlite_connection_cursor

    factory: object = NULL

Return a cursor for the connection.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_cursor_impl(pysqlite_Connection *self, PyObject *factory)
/*[clinic end generated code: output=562432a9e6af2aa1 input=4127345aa091b650]*/
{
    PyObject* cursor;

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    pysqlite_state *state = pysqlite_get_state(NULL);
    if (factory == NULL) {
        factory = (PyObject *)state->CursorType;
    }

    cursor = PyObject_CallOneArg(factory, (PyObject *)self);
    if (cursor == NULL)
        return NULL;
    if (!PyObject_TypeCheck(cursor, state->CursorType)) {
        PyErr_Format(PyExc_TypeError,
                     "factory must return a cursor, not %.100s",
                     Py_TYPE(cursor)->tp_name);
        Py_DECREF(cursor);
        return NULL;
    }

    _pysqlite_drop_unused_cursor_references(self);

    if (cursor && self->row_factory != Py_None) {
        Py_INCREF(self->row_factory);
        Py_XSETREF(((pysqlite_Cursor *)cursor)->row_factory, self->row_factory);
    }

    return cursor;
}

/*[clinic input]
_sqlite3.Connection.close as pysqlite_connection_close

Closes the connection.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_close_impl(pysqlite_Connection *self)
/*[clinic end generated code: output=a546a0da212c9b97 input=3d58064bbffaa3d3]*/
{
    if (!pysqlite_check_thread(self)) {
        return NULL;
    }

    pysqlite_do_all_statements(self, ACTION_FINALIZE, 1);
    connection_close(self);

    Py_RETURN_NONE;
}

/*
 * Checks if a connection object is usable (i. e. not closed).
 *
 * 0 => error; 1 => ok
 */
int pysqlite_check_connection(pysqlite_Connection* con)
{
    if (!con->initialized) {
        PyErr_SetString(pysqlite_ProgrammingError, "Base Connection.__init__ not called.");
        return 0;
    }

    if (!con->db) {
        PyErr_SetString(pysqlite_ProgrammingError, "Cannot operate on a closed database.");
        return 0;
    } else {
        return 1;
    }
}

/*[clinic input]
_sqlite3.Connection.commit as pysqlite_connection_commit

Commit the current transaction.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_commit_impl(pysqlite_Connection *self)
/*[clinic end generated code: output=3da45579e89407f2 input=39c12c04dda276a8]*/
{
    int rc;
    sqlite3_stmt* statement;

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    if (!sqlite3_get_autocommit(self->db)) {

        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_prepare_v2(self->db, "COMMIT", 7, &statement, NULL);
        Py_END_ALLOW_THREADS
        if (rc != SQLITE_OK) {
            _pysqlite_seterror(self->db);
            goto error;
        }

        rc = pysqlite_step(statement);
        if (rc != SQLITE_DONE) {
            _pysqlite_seterror(self->db);
        }

        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_finalize(statement);
        Py_END_ALLOW_THREADS
        if (rc != SQLITE_OK && !PyErr_Occurred()) {
            _pysqlite_seterror(self->db);
        }

    }

error:
    if (PyErr_Occurred()) {
        return NULL;
    } else {
        Py_RETURN_NONE;
    }
}

/*[clinic input]
_sqlite3.Connection.rollback as pysqlite_connection_rollback

Roll back the current transaction.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_rollback_impl(pysqlite_Connection *self)
/*[clinic end generated code: output=b66fa0d43e7ef305 input=12d4e8d068942830]*/
{
    int rc;
    sqlite3_stmt* statement;

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    if (!sqlite3_get_autocommit(self->db)) {
        pysqlite_do_all_statements(self, ACTION_RESET, 1);

        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_prepare_v2(self->db, "ROLLBACK", 9, &statement, NULL);
        Py_END_ALLOW_THREADS
        if (rc != SQLITE_OK) {
            _pysqlite_seterror(self->db);
            goto error;
        }

        rc = pysqlite_step(statement);
        if (rc != SQLITE_DONE) {
            _pysqlite_seterror(self->db);
        }

        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_finalize(statement);
        Py_END_ALLOW_THREADS
        if (rc != SQLITE_OK && !PyErr_Occurred()) {
            _pysqlite_seterror(self->db);
        }

    }

error:
    if (PyErr_Occurred()) {
        return NULL;
    } else {
        Py_RETURN_NONE;
    }
}

static int
_pysqlite_set_result(sqlite3_context* context, PyObject* py_val)
{
    if (py_val == Py_None) {
        sqlite3_result_null(context);
    } else if (PyLong_Check(py_val)) {
        sqlite_int64 value = _pysqlite_long_as_int64(py_val);
        if (value == -1 && PyErr_Occurred())
            return -1;
        sqlite3_result_int64(context, value);
    } else if (PyFloat_Check(py_val)) {
        sqlite3_result_double(context, PyFloat_AsDouble(py_val));
    } else if (PyUnicode_Check(py_val)) {
        const char *str = PyUnicode_AsUTF8(py_val);
        if (str == NULL)
            return -1;
        sqlite3_result_text(context, str, -1, SQLITE_TRANSIENT);
    } else if (PyObject_CheckBuffer(py_val)) {
        Py_buffer view;
        if (PyObject_GetBuffer(py_val, &view, PyBUF_SIMPLE) != 0) {
            PyErr_SetString(PyExc_ValueError,
                            "could not convert BLOB to buffer");
            return -1;
        }
        if (view.len > INT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "BLOB longer than INT_MAX bytes");
            PyBuffer_Release(&view);
            return -1;
        }
        sqlite3_result_blob(context, view.buf, (int)view.len, SQLITE_TRANSIENT);
        PyBuffer_Release(&view);
    } else {
        return -1;
    }
    return 0;
}

static PyObject *
_pysqlite_build_py_params(sqlite3_context *context, int argc,
                          sqlite3_value **argv)
{
    PyObject* args;
    int i;
    sqlite3_value* cur_value;
    PyObject* cur_py_value;

    args = PyTuple_New(argc);
    if (!args) {
        return NULL;
    }

    for (i = 0; i < argc; i++) {
        cur_value = argv[i];
        switch (sqlite3_value_type(argv[i])) {
            case SQLITE_INTEGER:
                cur_py_value = PyLong_FromLongLong(sqlite3_value_int64(cur_value));
                break;
            case SQLITE_FLOAT:
                cur_py_value = PyFloat_FromDouble(sqlite3_value_double(cur_value));
                break;
            case SQLITE_TEXT: {
                sqlite3 *db = sqlite3_context_db_handle(context);
                const char *text = (const char *)sqlite3_value_text(cur_value);

                if (text == NULL && sqlite3_errcode(db) == SQLITE_NOMEM) {
                    PyErr_NoMemory();
                    goto error;
                }

                Py_ssize_t size = sqlite3_value_bytes(cur_value);
                cur_py_value = PyUnicode_FromStringAndSize(text, size);
                break;
            }
            case SQLITE_BLOB: {
                sqlite3 *db = sqlite3_context_db_handle(context);
                const void *blob = sqlite3_value_blob(cur_value);

                if (blob == NULL && sqlite3_errcode(db) == SQLITE_NOMEM) {
                    PyErr_NoMemory();
                    goto error;
                }

                Py_ssize_t size = sqlite3_value_bytes(cur_value);
                cur_py_value = PyBytes_FromStringAndSize(blob, size);
                break;
            }
            case SQLITE_NULL:
            default:
                cur_py_value = Py_NewRef(Py_None);
        }

        if (!cur_py_value) {
            goto error;
        }

        PyTuple_SET_ITEM(args, i, cur_py_value);
    }

    return args;

error:
    Py_DECREF(args);
    return NULL;
}

static void
_pysqlite_func_callback(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    PyObject* args;
    PyObject* py_func;
    PyObject* py_retval = NULL;
    int ok;

    PyGILState_STATE threadstate;

    threadstate = PyGILState_Ensure();

    py_func = (PyObject*)sqlite3_user_data(context);

    args = _pysqlite_build_py_params(context, argc, argv);
    if (args) {
        py_retval = PyObject_CallObject(py_func, args);
        Py_DECREF(args);
    }

    ok = 0;
    if (py_retval) {
        ok = _pysqlite_set_result(context, py_retval) == 0;
        Py_DECREF(py_retval);
    }
    if (!ok) {
        if (_pysqlite_enable_callback_tracebacks) {
            PyErr_Print();
        } else {
            PyErr_Clear();
        }
        sqlite3_result_error(context, "user-defined function raised exception", -1);
    }

    PyGILState_Release(threadstate);
}

static void _pysqlite_step_callback(sqlite3_context *context, int argc, sqlite3_value** params)
{
    PyObject* args;
    PyObject* function_result = NULL;
    PyObject* aggregate_class;
    PyObject** aggregate_instance;
    PyObject* stepmethod = NULL;

    PyGILState_STATE threadstate;

    threadstate = PyGILState_Ensure();

    aggregate_class = (PyObject*)sqlite3_user_data(context);

    aggregate_instance = (PyObject**)sqlite3_aggregate_context(context, sizeof(PyObject*));

    if (*aggregate_instance == NULL) {
        *aggregate_instance = _PyObject_CallNoArg(aggregate_class);

        if (PyErr_Occurred()) {
            *aggregate_instance = 0;
            if (_pysqlite_enable_callback_tracebacks) {
                PyErr_Print();
            } else {
                PyErr_Clear();
            }
            sqlite3_result_error(context, "user-defined aggregate's '__init__' method raised error", -1);
            goto error;
        }
    }

    stepmethod = PyObject_GetAttrString(*aggregate_instance, "step");
    if (!stepmethod) {
        goto error;
    }

    args = _pysqlite_build_py_params(context, argc, params);
    if (!args) {
        goto error;
    }

    function_result = PyObject_CallObject(stepmethod, args);
    Py_DECREF(args);

    if (!function_result) {
        if (_pysqlite_enable_callback_tracebacks) {
            PyErr_Print();
        } else {
            PyErr_Clear();
        }
        sqlite3_result_error(context, "user-defined aggregate's 'step' method raised error", -1);
    }

error:
    Py_XDECREF(stepmethod);
    Py_XDECREF(function_result);

    PyGILState_Release(threadstate);
}

static void
_pysqlite_final_callback(sqlite3_context *context)
{
    PyObject* function_result;
    PyObject** aggregate_instance;
    _Py_IDENTIFIER(finalize);
    int ok;
    PyObject *exception, *value, *tb;

    PyGILState_STATE threadstate;

    threadstate = PyGILState_Ensure();

    aggregate_instance = (PyObject**)sqlite3_aggregate_context(context, 0);
    if (aggregate_instance == NULL) {
        /* No rows matched the query; the step handler was never called. */
        goto error;
    }
    else if (!*aggregate_instance) {
        /* this branch is executed if there was an exception in the aggregate's
         * __init__ */

        goto error;
    }

    /* Keep the exception (if any) of the last call to step() */
    PyErr_Fetch(&exception, &value, &tb);

    function_result = _PyObject_CallMethodIdNoArgs(*aggregate_instance, &PyId_finalize);

    Py_DECREF(*aggregate_instance);

    ok = 0;
    if (function_result) {
        ok = _pysqlite_set_result(context, function_result) == 0;
        Py_DECREF(function_result);
    }
    if (!ok) {
        if (_pysqlite_enable_callback_tracebacks) {
            PyErr_Print();
        } else {
            PyErr_Clear();
        }
        sqlite3_result_error(context, "user-defined aggregate's 'finalize' method raised error", -1);
    }

    /* Restore the exception (if any) of the last call to step(),
       but clear also the current exception if finalize() failed */
    PyErr_Restore(exception, value, tb);

error:
    PyGILState_Release(threadstate);
}

static void _pysqlite_drop_unused_statement_references(pysqlite_Connection* self)
{
    PyObject* new_list;
    PyObject* weakref;
    int i;

    /* we only need to do this once in a while */
    if (self->created_statements++ < 200) {
        return;
    }

    self->created_statements = 0;

    new_list = PyList_New(0);
    if (!new_list) {
        return;
    }

    for (i = 0; i < PyList_Size(self->statements); i++) {
        weakref = PyList_GetItem(self->statements, i);
        if (PyWeakref_GetObject(weakref) != Py_None) {
            if (PyList_Append(new_list, weakref) != 0) {
                Py_DECREF(new_list);
                return;
            }
        }
    }

    Py_SETREF(self->statements, new_list);
}

static void _pysqlite_drop_unused_cursor_references(pysqlite_Connection* self)
{
    PyObject* new_list;
    PyObject* weakref;
    int i;

    /* we only need to do this once in a while */
    if (self->created_cursors++ < 200) {
        return;
    }

    self->created_cursors = 0;

    new_list = PyList_New(0);
    if (!new_list) {
        return;
    }

    for (i = 0; i < PyList_Size(self->cursors); i++) {
        weakref = PyList_GetItem(self->cursors, i);
        if (PyWeakref_GetObject(weakref) != Py_None) {
            if (PyList_Append(new_list, weakref) != 0) {
                Py_DECREF(new_list);
                return;
            }
        }
    }

    Py_SETREF(self->cursors, new_list);
}

static void _destructor(void* args)
{
    // This function may be called without the GIL held, so we need to ensure
    // that we destroy 'args' with the GIL
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    Py_DECREF((PyObject*)args);
    PyGILState_Release(gstate);
}

/*[clinic input]
_sqlite3.Connection.create_function as pysqlite_connection_create_function

    name: str
    narg: int
    func: object
    *
    deterministic: bool = False

Creates a new function. Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_create_function_impl(pysqlite_Connection *self,
                                         const char *name, int narg,
                                         PyObject *func, int deterministic)
/*[clinic end generated code: output=07d1877dd98c0308 input=f2edcf073e815beb]*/
{
    int rc;
    int flags = SQLITE_UTF8;

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    if (deterministic) {
#if SQLITE_VERSION_NUMBER < 3008003
        PyErr_SetString(pysqlite_NotSupportedError,
                        "deterministic=True requires SQLite 3.8.3 or higher");
        return NULL;
#else
        if (sqlite3_libversion_number() < 3008003) {
            PyErr_SetString(pysqlite_NotSupportedError,
                            "deterministic=True requires SQLite 3.8.3 or higher");
            return NULL;
        }
        flags |= SQLITE_DETERMINISTIC;
#endif
    }
    rc = sqlite3_create_function_v2(self->db,
                                    name,
                                    narg,
                                    flags,
                                    (void*)Py_NewRef(func),
                                    _pysqlite_func_callback,
                                    NULL,
                                    NULL,
                                    &_destructor);  // will decref func

    if (rc != SQLITE_OK) {
        /* Workaround for SQLite bug: no error code or string is available here */
        PyErr_SetString(pysqlite_OperationalError, "Error creating function");
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_sqlite3.Connection.create_aggregate as pysqlite_connection_create_aggregate

    name: str
    n_arg: int
    aggregate_class: object

Creates a new aggregate. Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_create_aggregate_impl(pysqlite_Connection *self,
                                          const char *name, int n_arg,
                                          PyObject *aggregate_class)
/*[clinic end generated code: output=fbb2f858cfa4d8db input=c2e13bbf234500a5]*/
{
    int rc;

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    rc = sqlite3_create_function_v2(self->db,
                                    name,
                                    n_arg,
                                    SQLITE_UTF8,
                                    (void*)Py_NewRef(aggregate_class),
                                    0,
                                    &_pysqlite_step_callback,
                                    &_pysqlite_final_callback,
                                    &_destructor); // will decref func
    if (rc != SQLITE_OK) {
        /* Workaround for SQLite bug: no error code or string is available here */
        PyErr_SetString(pysqlite_OperationalError, "Error creating aggregate");
        return NULL;
    }
    Py_RETURN_NONE;
}

static int _authorizer_callback(void* user_arg, int action, const char* arg1, const char* arg2 , const char* dbname, const char* access_attempt_source)
{
    PyObject *ret;
    int rc;
    PyGILState_STATE gilstate;

    gilstate = PyGILState_Ensure();

    ret = PyObject_CallFunction((PyObject*)user_arg, "issss", action, arg1, arg2, dbname, access_attempt_source);

    if (ret == NULL) {
        if (_pysqlite_enable_callback_tracebacks)
            PyErr_Print();
        else
            PyErr_Clear();

        rc = SQLITE_DENY;
    }
    else {
        if (PyLong_Check(ret)) {
            rc = _PyLong_AsInt(ret);
            if (rc == -1 && PyErr_Occurred()) {
                if (_pysqlite_enable_callback_tracebacks)
                    PyErr_Print();
                else
                    PyErr_Clear();
                rc = SQLITE_DENY;
            }
        }
        else {
            rc = SQLITE_DENY;
        }
        Py_DECREF(ret);
    }

    PyGILState_Release(gilstate);
    return rc;
}

static int _progress_handler(void* user_arg)
{
    int rc;
    PyObject *ret;
    PyGILState_STATE gilstate;

    gilstate = PyGILState_Ensure();
    ret = _PyObject_CallNoArg((PyObject*)user_arg);

    if (!ret) {
        if (_pysqlite_enable_callback_tracebacks) {
            PyErr_Print();
        } else {
            PyErr_Clear();
        }

        /* abort query if error occurred */
        rc = 1;
    } else {
        rc = (int)PyObject_IsTrue(ret);
        Py_DECREF(ret);
    }

    PyGILState_Release(gilstate);
    return rc;
}

#ifdef HAVE_TRACE_V2
/*
 * From https://sqlite.org/c3ref/trace_v2.html:
 * The integer return value from the callback is currently ignored, though this
 * may change in future releases. Callback implementations should return zero
 * to ensure future compatibility.
 */
static int _trace_callback(unsigned int type, void* user_arg, void* prepared_statement, void* statement_string)
#else
static void _trace_callback(void* user_arg, const char* statement_string)
#endif
{
    PyObject *py_statement = NULL;
    PyObject *ret = NULL;

    PyGILState_STATE gilstate;

#ifdef HAVE_TRACE_V2
    if (type != SQLITE_TRACE_STMT) {
        return 0;
    }
#endif

    gilstate = PyGILState_Ensure();
    py_statement = PyUnicode_DecodeUTF8(statement_string,
            strlen(statement_string), "replace");
    if (py_statement) {
        ret = PyObject_CallOneArg((PyObject*)user_arg, py_statement);
        Py_DECREF(py_statement);
    }

    if (ret) {
        Py_DECREF(ret);
    } else {
        if (_pysqlite_enable_callback_tracebacks) {
            PyErr_Print();
        } else {
            PyErr_Clear();
        }
    }

    PyGILState_Release(gilstate);
#ifdef HAVE_TRACE_V2
    return 0;
#endif
}

/*[clinic input]
_sqlite3.Connection.set_authorizer as pysqlite_connection_set_authorizer

    authorizer_callback as authorizer_cb: object

Sets authorizer callback. Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_set_authorizer_impl(pysqlite_Connection *self,
                                        PyObject *authorizer_cb)
/*[clinic end generated code: output=f18ba575d788b35c input=df079724c020d2f2]*/
{
    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    int rc;
    if (authorizer_cb == Py_None) {
        rc = sqlite3_set_authorizer(self->db, NULL, NULL);
        Py_XSETREF(self->function_pinboard_authorizer_cb, NULL);
    }
    else {
        Py_INCREF(authorizer_cb);
        Py_XSETREF(self->function_pinboard_authorizer_cb, authorizer_cb);
        rc = sqlite3_set_authorizer(self->db, _authorizer_callback, authorizer_cb);
    }
    if (rc != SQLITE_OK) {
        PyErr_SetString(pysqlite_OperationalError, "Error setting authorizer callback");
        Py_XSETREF(self->function_pinboard_authorizer_cb, NULL);
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_sqlite3.Connection.set_progress_handler as pysqlite_connection_set_progress_handler

    progress_handler: object
    n: int

Sets progress handler callback. Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_set_progress_handler_impl(pysqlite_Connection *self,
                                              PyObject *progress_handler,
                                              int n)
/*[clinic end generated code: output=35a7c10364cb1b04 input=857696c25f964c64]*/
{
    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    if (progress_handler == Py_None) {
        /* None clears the progress handler previously set */
        sqlite3_progress_handler(self->db, 0, 0, (void*)0);
        Py_XSETREF(self->function_pinboard_progress_handler, NULL);
    } else {
        sqlite3_progress_handler(self->db, n, _progress_handler, progress_handler);
        Py_INCREF(progress_handler);
        Py_XSETREF(self->function_pinboard_progress_handler, progress_handler);
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_sqlite3.Connection.set_trace_callback as pysqlite_connection_set_trace_callback

    trace_callback: object

Sets a trace callback called for each SQL statement (passed as unicode).

Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_set_trace_callback_impl(pysqlite_Connection *self,
                                            PyObject *trace_callback)
/*[clinic end generated code: output=fb0e307b9924d454 input=56d60fd38d763679]*/
{
    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    if (trace_callback == Py_None) {
        /*
         * None clears the trace callback previously set
         *
         * Ref.
         * - https://sqlite.org/c3ref/c_trace.html
         * - https://sqlite.org/c3ref/trace_v2.html
         */
#ifdef HAVE_TRACE_V2
        sqlite3_trace_v2(self->db, SQLITE_TRACE_STMT, 0, 0);
#else
        sqlite3_trace(self->db, 0, (void*)0);
#endif
        Py_XSETREF(self->function_pinboard_trace_callback, NULL);
    } else {
#ifdef HAVE_TRACE_V2
        sqlite3_trace_v2(self->db, SQLITE_TRACE_STMT, _trace_callback, trace_callback);
#else
        sqlite3_trace(self->db, _trace_callback, trace_callback);
#endif
        Py_INCREF(trace_callback);
        Py_XSETREF(self->function_pinboard_trace_callback, trace_callback);
    }

    Py_RETURN_NONE;
}

#ifndef SQLITE_OMIT_LOAD_EXTENSION
/*[clinic input]
_sqlite3.Connection.enable_load_extension as pysqlite_connection_enable_load_extension

    enable as onoff: bool(accept={int})
    /

Enable dynamic loading of SQLite extension modules. Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_enable_load_extension_impl(pysqlite_Connection *self,
                                               int onoff)
/*[clinic end generated code: output=9cac37190d388baf input=5c0da5b121121cbc]*/
{
    int rc;

    if (PySys_Audit("sqlite3.enable_load_extension",
                    "OO", self, onoff ? Py_True : Py_False) < 0) {
        return NULL;
    }

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    rc = sqlite3_enable_load_extension(self->db, onoff);

    if (rc != SQLITE_OK) {
        PyErr_SetString(pysqlite_OperationalError, "Error enabling load extension");
        return NULL;
    } else {
        Py_RETURN_NONE;
    }
}

/*[clinic input]
_sqlite3.Connection.load_extension as pysqlite_connection_load_extension

    name as extension_name: str
    /

Load SQLite extension module. Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_load_extension_impl(pysqlite_Connection *self,
                                        const char *extension_name)
/*[clinic end generated code: output=47eb1d7312bc97a7 input=0b711574560db9fc]*/
{
    int rc;
    char* errmsg;

    if (PySys_Audit("sqlite3.load_extension", "Os", self, extension_name) < 0) {
        return NULL;
    }

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    rc = sqlite3_load_extension(self->db, extension_name, 0, &errmsg);
    if (rc != 0) {
        PyErr_SetString(pysqlite_OperationalError, errmsg);
        return NULL;
    } else {
        Py_RETURN_NONE;
    }
}
#endif

int pysqlite_check_thread(pysqlite_Connection* self)
{
    if (self->check_same_thread) {
        if (PyThread_get_thread_ident() != self->thread_ident) {
            PyErr_Format(pysqlite_ProgrammingError,
                        "SQLite objects created in a thread can only be used in that same thread. "
                        "The object was created in thread id %lu and this is thread id %lu.",
                        self->thread_ident, PyThread_get_thread_ident());
            return 0;
        }

    }
    return 1;
}

static PyObject* pysqlite_connection_get_isolation_level(pysqlite_Connection* self, void* unused)
{
    return Py_NewRef(self->isolation_level);
}

static PyObject* pysqlite_connection_get_total_changes(pysqlite_Connection* self, void* unused)
{
    if (!pysqlite_check_connection(self)) {
        return NULL;
    } else {
        return Py_BuildValue("i", sqlite3_total_changes(self->db));
    }
}

static PyObject* pysqlite_connection_get_in_transaction(pysqlite_Connection* self, void* unused)
{
    if (!pysqlite_check_connection(self)) {
        return NULL;
    }
    if (!sqlite3_get_autocommit(self->db)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static int
pysqlite_connection_set_isolation_level(pysqlite_Connection* self, PyObject* isolation_level, void *Py_UNUSED(ignored))
{
    if (isolation_level == NULL) {
        PyErr_SetString(PyExc_AttributeError, "cannot delete attribute");
        return -1;
    }
    if (isolation_level == Py_None) {
        PyObject *res = pysqlite_connection_commit(self, NULL);
        if (!res) {
            return -1;
        }
        Py_DECREF(res);

        self->begin_statement = NULL;
    } else {
        const char * const *candidate;
        PyObject *uppercase_level;
        _Py_IDENTIFIER(upper);

        if (!PyUnicode_Check(isolation_level)) {
            PyErr_Format(PyExc_TypeError,
                         "isolation_level must be a string or None, not %.100s",
                         Py_TYPE(isolation_level)->tp_name);
            return -1;
        }

        uppercase_level = _PyObject_CallMethodIdOneArg(
                        (PyObject *)&PyUnicode_Type, &PyId_upper,
                        isolation_level);
        if (!uppercase_level) {
            return -1;
        }
        for (candidate = begin_statements; *candidate; candidate++) {
            if (_PyUnicode_EqualToASCIIString(uppercase_level, *candidate + 6))
                break;
        }
        Py_DECREF(uppercase_level);
        if (!*candidate) {
            PyErr_SetString(PyExc_ValueError,
                            "invalid value for isolation_level");
            return -1;
        }
        self->begin_statement = *candidate;
    }

    Py_INCREF(isolation_level);
    Py_XSETREF(self->isolation_level, isolation_level);
    return 0;
}

static PyObject *
pysqlite_connection_call(pysqlite_Connection *self, PyObject *args,
                         PyObject *kwargs)
{
    PyObject* sql;
    pysqlite_Statement* statement;
    PyObject* weakref;

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    if (!_PyArg_NoKeywords(MODULE_NAME ".Connection", kwargs))
        return NULL;

    if (!PyArg_ParseTuple(args, "U", &sql))
        return NULL;

    _pysqlite_drop_unused_statement_references(self);

    statement = pysqlite_statement_create(self, sql);
    if (statement == NULL) {
        return NULL;
    }

    weakref = PyWeakref_NewRef((PyObject*)statement, NULL);
    if (weakref == NULL)
        goto error;
    if (PyList_Append(self->statements, weakref) != 0) {
        Py_DECREF(weakref);
        goto error;
    }
    Py_DECREF(weakref);

    return (PyObject*)statement;

error:
    Py_DECREF(statement);
    return NULL;
}

/*[clinic input]
_sqlite3.Connection.execute as pysqlite_connection_execute

    sql: unicode
    parameters: object = NULL
    /

Executes a SQL statement. Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_execute_impl(pysqlite_Connection *self, PyObject *sql,
                                 PyObject *parameters)
/*[clinic end generated code: output=5be05ae01ee17ee4 input=fbd17c75c7140271]*/
{
    _Py_IDENTIFIER(execute);
    PyObject* cursor = 0;
    PyObject* result = 0;

    cursor = _PyObject_CallMethodIdNoArgs((PyObject*)self, &PyId_cursor);
    if (!cursor) {
        goto error;
    }

    result = _PyObject_CallMethodIdObjArgs(cursor, &PyId_execute, sql, parameters, NULL);
    if (!result) {
        Py_CLEAR(cursor);
    }

error:
    Py_XDECREF(result);

    return cursor;
}

/*[clinic input]
_sqlite3.Connection.executemany as pysqlite_connection_executemany

    sql: unicode
    parameters: object
    /

Repeatedly executes a SQL statement. Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_executemany_impl(pysqlite_Connection *self,
                                     PyObject *sql, PyObject *parameters)
/*[clinic end generated code: output=776cd2fd20bfe71f input=4feab80659ffc82b]*/
{
    _Py_IDENTIFIER(executemany);
    PyObject* cursor = 0;
    PyObject* result = 0;

    cursor = _PyObject_CallMethodIdNoArgs((PyObject*)self, &PyId_cursor);
    if (!cursor) {
        goto error;
    }

    result = _PyObject_CallMethodIdObjArgs(cursor, &PyId_executemany, sql,
                                           parameters, NULL);
    if (!result) {
        Py_CLEAR(cursor);
    }

error:
    Py_XDECREF(result);

    return cursor;
}

/*[clinic input]
_sqlite3.Connection.executescript as pysqlite_connection_executescript

    sql_script as script_obj: object
    /

Executes a multiple SQL statements at once. Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_executescript(pysqlite_Connection *self,
                                  PyObject *script_obj)
/*[clinic end generated code: output=4c4f9d77aa0ae37d input=c0b14695aa6c81d9]*/
{
    _Py_IDENTIFIER(executescript);
    PyObject* cursor = 0;
    PyObject* result = 0;

    cursor = _PyObject_CallMethodIdNoArgs((PyObject*)self, &PyId_cursor);
    if (!cursor) {
        goto error;
    }

    result = _PyObject_CallMethodIdObjArgs(cursor, &PyId_executescript,
                                           script_obj, NULL);
    if (!result) {
        Py_CLEAR(cursor);
    }

error:
    Py_XDECREF(result);

    return cursor;
}

/* ------------------------- COLLATION CODE ------------------------ */

static int
pysqlite_collation_callback(
        void* context,
        int text1_length, const void* text1_data,
        int text2_length, const void* text2_data)
{
    PyObject* callback = (PyObject*)context;
    PyObject* string1 = 0;
    PyObject* string2 = 0;
    PyGILState_STATE gilstate;
    PyObject* retval = NULL;
    long longval;
    int result = 0;
    gilstate = PyGILState_Ensure();

    if (PyErr_Occurred()) {
        goto finally;
    }

    string1 = PyUnicode_FromStringAndSize((const char*)text1_data, text1_length);
    string2 = PyUnicode_FromStringAndSize((const char*)text2_data, text2_length);

    if (!string1 || !string2) {
        goto finally; /* failed to allocate strings */
    }

    retval = PyObject_CallFunctionObjArgs(callback, string1, string2, NULL);

    if (!retval) {
        /* execution failed */
        goto finally;
    }

    longval = PyLong_AsLongAndOverflow(retval, &result);
    if (longval == -1 && PyErr_Occurred()) {
        PyErr_Clear();
        result = 0;
    }
    else if (!result) {
        if (longval > 0)
            result = 1;
        else if (longval < 0)
            result = -1;
    }

finally:
    Py_XDECREF(string1);
    Py_XDECREF(string2);
    Py_XDECREF(retval);
    PyGILState_Release(gilstate);
    return result;
}

/*[clinic input]
_sqlite3.Connection.interrupt as pysqlite_connection_interrupt

Abort any pending database operation. Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_interrupt_impl(pysqlite_Connection *self)
/*[clinic end generated code: output=f193204bc9e70b47 input=4bd0ad083cf93aa7]*/
{
    PyObject* retval = NULL;

    if (!pysqlite_check_connection(self)) {
        goto finally;
    }

    sqlite3_interrupt(self->db);

    retval = Py_NewRef(Py_None);

finally:
    return retval;
}

/* Function author: Paul Kippes <kippesp@gmail.com>
 * Class method of Connection to call the Python function _iterdump
 * of the sqlite3 module.
 */
/*[clinic input]
_sqlite3.Connection.iterdump as pysqlite_connection_iterdump

Returns iterator to the dump of the database in an SQL text format.

Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_iterdump_impl(pysqlite_Connection *self)
/*[clinic end generated code: output=586997aaf9808768 input=53bc907cb5eedb85]*/
{
    _Py_IDENTIFIER(_iterdump);
    PyObject* retval = NULL;
    PyObject* module = NULL;
    PyObject* module_dict;
    PyObject* pyfn_iterdump;

    if (!pysqlite_check_connection(self)) {
        goto finally;
    }

    module = PyImport_ImportModule(MODULE_NAME ".dump");
    if (!module) {
        goto finally;
    }

    module_dict = PyModule_GetDict(module);
    if (!module_dict) {
        goto finally;
    }

    pyfn_iterdump = _PyDict_GetItemIdWithError(module_dict, &PyId__iterdump);
    if (!pyfn_iterdump) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(pysqlite_OperationalError,
                            "Failed to obtain _iterdump() reference");
        }
        goto finally;
    }

    retval = PyObject_CallOneArg(pyfn_iterdump, (PyObject *)self);

finally:
    Py_XDECREF(module);
    return retval;
}

/*[clinic input]
_sqlite3.Connection.backup as pysqlite_connection_backup

    target: object(type='pysqlite_Connection *', subclass_of='clinic_state()->ConnectionType')
    *
    pages: int = -1
    progress: object = None
    name: str = "main"
    sleep: double = 0.250

Makes a backup of the database. Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_backup_impl(pysqlite_Connection *self,
                                pysqlite_Connection *target, int pages,
                                PyObject *progress, const char *name,
                                double sleep)
/*[clinic end generated code: output=306a3e6a38c36334 input=c759627ab1ad46ff]*/
{
    int rc;
    int sleep_ms = (int)(sleep * 1000.0);
    sqlite3 *bck_conn;
    sqlite3_backup *bck_handle;

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    if (!pysqlite_check_connection(target)) {
        return NULL;
    }

    if (target == self) {
        PyErr_SetString(PyExc_ValueError, "target cannot be the same connection instance");
        return NULL;
    }

#if SQLITE_VERSION_NUMBER < 3008008
    /* Since 3.8.8 this is already done, per commit
       https://www.sqlite.org/src/info/169b5505498c0a7e */
    if (!sqlite3_get_autocommit(target->db)) {
        PyErr_SetString(pysqlite_OperationalError, "target is in transaction");
        return NULL;
    }
#endif

    if (progress != Py_None && !PyCallable_Check(progress)) {
        PyErr_SetString(PyExc_TypeError, "progress argument must be a callable");
        return NULL;
    }

    if (pages == 0) {
        pages = -1;
    }

    bck_conn = target->db;

    Py_BEGIN_ALLOW_THREADS
    bck_handle = sqlite3_backup_init(bck_conn, "main", self->db, name);
    Py_END_ALLOW_THREADS

    if (bck_handle == NULL) {
        _pysqlite_seterror(bck_conn);
        return NULL;
    }

    do {
        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_backup_step(bck_handle, pages);
        Py_END_ALLOW_THREADS

        if (progress != Py_None) {
            int remaining = sqlite3_backup_remaining(bck_handle);
            int pagecount = sqlite3_backup_pagecount(bck_handle);
            PyObject *res = PyObject_CallFunction(progress, "iii", rc,
                                                  remaining, pagecount);
            if (res == NULL) {
                /* Callback failed: abort backup and bail. */
                Py_BEGIN_ALLOW_THREADS
                sqlite3_backup_finish(bck_handle);
                Py_END_ALLOW_THREADS
                return NULL;
            }
            Py_DECREF(res);
        }

        /* Sleep for a while if there are still further pages to copy and
           the engine could not make any progress */
        if (rc == SQLITE_BUSY || rc == SQLITE_LOCKED) {
            Py_BEGIN_ALLOW_THREADS
            sqlite3_sleep(sleep_ms);
            Py_END_ALLOW_THREADS
        }
    } while (rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED);

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_backup_finish(bck_handle);
    Py_END_ALLOW_THREADS

    if (rc != SQLITE_OK) {
        _pysqlite_seterror(bck_conn);
        return NULL;
    }

    Py_RETURN_NONE;
}

/*[clinic input]
_sqlite3.Connection.create_collation as pysqlite_connection_create_collation

    name: unicode
    callback as callable: object
    /

Creates a collation function. Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_create_collation_impl(pysqlite_Connection *self,
                                          PyObject *name, PyObject *callable)
/*[clinic end generated code: output=0f63b8995565ae22 input=5c3898813a776cf2]*/
{
    PyObject* uppercase_name = 0;
    Py_ssize_t i, len;
    _Py_IDENTIFIER(upper);
    const char *uppercase_name_str;
    int rc;
    unsigned int kind;
    const void *data;

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        goto finally;
    }

    uppercase_name = _PyObject_CallMethodIdOneArg((PyObject *)&PyUnicode_Type,
                                                  &PyId_upper, name);
    if (!uppercase_name) {
        goto finally;
    }

    if (PyUnicode_READY(uppercase_name))
        goto finally;
    len = PyUnicode_GET_LENGTH(uppercase_name);
    kind = PyUnicode_KIND(uppercase_name);
    data = PyUnicode_DATA(uppercase_name);
    for (i=0; i<len; i++) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        if ((ch >= '0' && ch <= '9')
         || (ch >= 'A' && ch <= 'Z')
         || (ch == '_'))
        {
            continue;
        } else {
            PyErr_SetString(pysqlite_ProgrammingError, "invalid character in collation name");
            goto finally;
        }
    }

    uppercase_name_str = PyUnicode_AsUTF8(uppercase_name);
    if (!uppercase_name_str)
        goto finally;

    if (callable != Py_None && !PyCallable_Check(callable)) {
        PyErr_SetString(PyExc_TypeError, "parameter must be callable");
        goto finally;
    }

    if (callable != Py_None) {
        if (PyDict_SetItem(self->collations, uppercase_name, callable) == -1)
            goto finally;
    } else {
        if (PyDict_DelItem(self->collations, uppercase_name) == -1)
            goto finally;
    }

    rc = sqlite3_create_collation(self->db,
                                  uppercase_name_str,
                                  SQLITE_UTF8,
                                  (callable != Py_None) ? callable : NULL,
                                  (callable != Py_None) ? pysqlite_collation_callback : NULL);
    if (rc != SQLITE_OK) {
        if (callable != Py_None) {
            if (PyDict_DelItem(self->collations, uppercase_name) < 0) {
                PyErr_Clear();
            }
        }
        _pysqlite_seterror(self->db);
        goto finally;
    }

finally:
    Py_XDECREF(uppercase_name);

    if (PyErr_Occurred()) {
        return NULL;
    }
    return Py_NewRef(Py_None);
}

/*[clinic input]
_sqlite3.Connection.__enter__ as pysqlite_connection_enter

Called when the connection is used as a context manager.

Returns itself as a convenience to the caller.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_enter_impl(pysqlite_Connection *self)
/*[clinic end generated code: output=457b09726d3e9dcd input=127d7a4f17e86d8f]*/
{
    if (!pysqlite_check_connection(self)) {
        return NULL;
    }
    return Py_NewRef((PyObject *)self);
}

/*[clinic input]
_sqlite3.Connection.__exit__ as pysqlite_connection_exit

    type as exc_type: object
    value as exc_value: object
    traceback as exc_tb: object
    /

Called when the connection is used as a context manager.

If there was any exception, a rollback takes place; otherwise we commit.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_exit_impl(pysqlite_Connection *self, PyObject *exc_type,
                              PyObject *exc_value, PyObject *exc_tb)
/*[clinic end generated code: output=0705200e9321202a input=bd66f1532c9c54a7]*/
{
    const char* method_name;
    PyObject* result;

    if (exc_type == Py_None && exc_value == Py_None && exc_tb == Py_None) {
        method_name = "commit";
    } else {
        method_name = "rollback";
    }

    result = PyObject_CallMethod((PyObject*)self, method_name, NULL);
    if (!result) {
        return NULL;
    }
    Py_DECREF(result);

    Py_RETURN_FALSE;
}

static const char connection_doc[] =
PyDoc_STR("SQLite database connection object.");

static PyGetSetDef connection_getset[] = {
    {"isolation_level",  (getter)pysqlite_connection_get_isolation_level, (setter)pysqlite_connection_set_isolation_level},
    {"total_changes",  (getter)pysqlite_connection_get_total_changes, (setter)0},
    {"in_transaction",  (getter)pysqlite_connection_get_in_transaction, (setter)0},
    {NULL}
};

static PyMethodDef connection_methods[] = {
    PYSQLITE_CONNECTION_BACKUP_METHODDEF
    PYSQLITE_CONNECTION_CLOSE_METHODDEF
    PYSQLITE_CONNECTION_COMMIT_METHODDEF
    PYSQLITE_CONNECTION_CREATE_AGGREGATE_METHODDEF
    PYSQLITE_CONNECTION_CREATE_COLLATION_METHODDEF
    PYSQLITE_CONNECTION_CREATE_FUNCTION_METHODDEF
    PYSQLITE_CONNECTION_CURSOR_METHODDEF
    PYSQLITE_CONNECTION_ENABLE_LOAD_EXTENSION_METHODDEF
    PYSQLITE_CONNECTION_ENTER_METHODDEF
    PYSQLITE_CONNECTION_EXECUTEMANY_METHODDEF
    PYSQLITE_CONNECTION_EXECUTESCRIPT_METHODDEF
    PYSQLITE_CONNECTION_EXECUTE_METHODDEF
    PYSQLITE_CONNECTION_EXIT_METHODDEF
    PYSQLITE_CONNECTION_INTERRUPT_METHODDEF
    PYSQLITE_CONNECTION_ITERDUMP_METHODDEF
    PYSQLITE_CONNECTION_LOAD_EXTENSION_METHODDEF
    PYSQLITE_CONNECTION_ROLLBACK_METHODDEF
    PYSQLITE_CONNECTION_SET_AUTHORIZER_METHODDEF
    PYSQLITE_CONNECTION_SET_PROGRESS_HANDLER_METHODDEF
    PYSQLITE_CONNECTION_SET_TRACE_CALLBACK_METHODDEF
    {NULL, NULL}
};

static struct PyMemberDef connection_members[] =
{
    {"Warning", T_OBJECT, offsetof(pysqlite_Connection, Warning), READONLY},
    {"Error", T_OBJECT, offsetof(pysqlite_Connection, Error), READONLY},
    {"InterfaceError", T_OBJECT, offsetof(pysqlite_Connection, InterfaceError), READONLY},
    {"DatabaseError", T_OBJECT, offsetof(pysqlite_Connection, DatabaseError), READONLY},
    {"DataError", T_OBJECT, offsetof(pysqlite_Connection, DataError), READONLY},
    {"OperationalError", T_OBJECT, offsetof(pysqlite_Connection, OperationalError), READONLY},
    {"IntegrityError", T_OBJECT, offsetof(pysqlite_Connection, IntegrityError), READONLY},
    {"InternalError", T_OBJECT, offsetof(pysqlite_Connection, InternalError), READONLY},
    {"ProgrammingError", T_OBJECT, offsetof(pysqlite_Connection, ProgrammingError), READONLY},
    {"NotSupportedError", T_OBJECT, offsetof(pysqlite_Connection, NotSupportedError), READONLY},
    {"row_factory", T_OBJECT, offsetof(pysqlite_Connection, row_factory)},
    {"text_factory", T_OBJECT, offsetof(pysqlite_Connection, text_factory)},
    {NULL}
};

static PyType_Slot connection_slots[] = {
    {Py_tp_dealloc, connection_dealloc},
    {Py_tp_doc, (void *)connection_doc},
    {Py_tp_methods, connection_methods},
    {Py_tp_members, connection_members},
    {Py_tp_getset, connection_getset},
    {Py_tp_init, pysqlite_connection_init},
    {Py_tp_call, pysqlite_connection_call},
    {Py_tp_traverse, connection_traverse},
    {Py_tp_clear, connection_clear},
    {0, NULL},
};

static PyType_Spec connection_spec = {
    .name = MODULE_NAME ".Connection",
    .basicsize = sizeof(pysqlite_Connection),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = connection_slots,
};

int
pysqlite_connection_setup_types(PyObject *module)
{
    PyObject *type = PyType_FromModuleAndSpec(module, &connection_spec, NULL);
    if (type == NULL) {
        return -1;
    }
    pysqlite_state *state = pysqlite_get_state(module);
    state->ConnectionType = (PyTypeObject *)type;
    return 0;
}
