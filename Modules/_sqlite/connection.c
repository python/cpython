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
#include "blob.h"
#include "prepare_protocol.h"
#include "util.h"

#include <stdbool.h>

#if SQLITE_VERSION_NUMBER >= 3014000
#define HAVE_TRACE_V2
#endif

#if SQLITE_VERSION_NUMBER >= 3025000
#define HAVE_WINDOW_FUNCTIONS
#endif

static const char *
get_isolation_level(const char *level)
{
    assert(level != NULL);
    static const char *const allowed_levels[] = {
        "",
        "DEFERRED",
        "IMMEDIATE",
        "EXCLUSIVE",
        NULL
    };
    for (int i = 0; allowed_levels[i] != NULL; i++) {
        const char *candidate = allowed_levels[i];
        if (sqlite3_stricmp(level, candidate) == 0) {
            return candidate;
        }
    }
    PyErr_SetString(PyExc_ValueError,
                    "isolation_level string must be '', 'DEFERRED', "
                    "'IMMEDIATE', or 'EXCLUSIVE'");
    return NULL;
}

static int
isolation_level_converter(PyObject *str_or_none, const char **result)
{
    if (Py_IsNone(str_or_none)) {
        *result = NULL;
    }
    else if (PyUnicode_Check(str_or_none)) {
        Py_ssize_t sz;
        const char *str = PyUnicode_AsUTF8AndSize(str_or_none, &sz);
        if (str == NULL) {
            return 0;
        }
        if (strlen(str) != (size_t)sz) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            return 0;
        }

        const char *level = get_isolation_level(str);
        if (level == NULL) {
            return 0;
        }
        *result = level;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "isolation_level must be str or None");
        return 0;
    }
    return 1;
}

static int
autocommit_converter(PyObject *val, enum autocommit_mode *result)
{
    if (Py_IsTrue(val)) {
        *result = AUTOCOMMIT_ENABLED;
        return 1;
    }
    if (Py_IsFalse(val)) {
        *result = AUTOCOMMIT_DISABLED;
        return 1;
    }
    if (PyLong_Check(val) &&
        PyLong_AsLong(val) == LEGACY_TRANSACTION_CONTROL)
    {
        *result = AUTOCOMMIT_LEGACY;
        return 1;
    }

    PyErr_SetString(PyExc_ValueError,
        "autocommit must be True, False, or "
        "sqlite3.LEGACY_TRANSACTION_CONTROL");
    return 0;
}

static int
sqlite3_int64_converter(PyObject *obj, sqlite3_int64 *result)
{
    if (!PyLong_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "expected 'int'");
        return 0;
    }
    *result = _pysqlite_long_as_int64(obj);
    if (PyErr_Occurred()) {
        return 0;
    }
    return 1;
}

#define clinic_state() (pysqlite_get_state_by_type(Py_TYPE(self)))
#include "clinic/connection.c.h"
#undef clinic_state

/*[clinic input]
module _sqlite3
class _sqlite3.Connection "pysqlite_Connection *" "clinic_state()->ConnectionType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=67369db2faf80891]*/

static void _pysqlite_drop_unused_cursor_references(pysqlite_Connection* self);
static void free_callback_context(callback_context *ctx);
static void set_callback_context(callback_context **ctx_pp,
                                 callback_context *ctx);
static int connection_close(pysqlite_Connection *self);
PyObject *_pysqlite_query_execute(pysqlite_Cursor *, int, PyObject *, PyObject *);

static PyObject *
new_statement_cache(pysqlite_Connection *self, pysqlite_state *state,
                    int maxsize)
{
    PyObject *args[] = { NULL, PyLong_FromLong(maxsize), };
    if (args[1] == NULL) {
        return NULL;
    }
    PyObject *lru_cache = state->lru_cache;
    size_t nargsf = 1 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    PyObject *inner = PyObject_Vectorcall(lru_cache, args + 1, nargsf, NULL);
    Py_DECREF(args[1]);
    if (inner == NULL) {
        return NULL;
    }

    args[1] = (PyObject *)self;  // Borrowed ref.
    nargsf = 1 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    PyObject *res = PyObject_Vectorcall(inner, args + 1, nargsf, NULL);
    Py_DECREF(inner);
    return res;
}

static inline int
connection_exec_stmt(pysqlite_Connection *self, const char *sql)
{
    int rc;
    Py_BEGIN_ALLOW_THREADS
    int len = (int)strlen(sql) + 1;
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(self->db, sql, len, &stmt, NULL);
    if (rc == SQLITE_OK) {
        (void)sqlite3_step(stmt);
        rc = sqlite3_finalize(stmt);
    }
    Py_END_ALLOW_THREADS

    if (rc != SQLITE_OK) {
        (void)_pysqlite_seterror(self->state, self->db);
        return -1;
    }
    return 0;
}

/*[python input]
class IsolationLevel_converter(CConverter):
    type = "const char *"
    converter = "isolation_level_converter"

class Autocommit_converter(CConverter):
    type = "enum autocommit_mode"
    converter = "autocommit_converter"

class sqlite3_int64_converter(CConverter):
    type = "sqlite3_int64"
    converter = "sqlite3_int64_converter"

[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=dff8760fb1eba6a1]*/

// NB: This needs to be in sync with the sqlite3.connect docstring
/*[clinic input]
_sqlite3.Connection.__init__ as pysqlite_connection_init

    database: object
    timeout: double = 5.0
    detect_types: int = 0
    isolation_level: IsolationLevel = ""
    check_same_thread: bool = True
    factory: object(c_default='(PyObject*)clinic_state()->ConnectionType') = ConnectionType
    cached_statements as cache_size: int = 128
    uri: bool = False
    *
    autocommit: Autocommit(c_default='LEGACY_TRANSACTION_CONTROL') = sqlite3.LEGACY_TRANSACTION_CONTROL
[clinic start generated code]*/

static int
pysqlite_connection_init_impl(pysqlite_Connection *self, PyObject *database,
                              double timeout, int detect_types,
                              const char *isolation_level,
                              int check_same_thread, PyObject *factory,
                              int cache_size, int uri,
                              enum autocommit_mode autocommit)
/*[clinic end generated code: output=cba057313ea7712f input=9b0ab6c12f674fa3]*/
{
    if (PySys_Audit("sqlite3.connect", "O", database) < 0) {
        return -1;
    }

    PyObject *bytes;
    if (!PyUnicode_FSConverter(database, &bytes)) {
        return -1;
    }

    if (self->initialized) {
        self->initialized = 0;

        PyTypeObject *tp = Py_TYPE(self);
        tp->tp_clear((PyObject *)self);
        if (connection_close(self) < 0) {
            return -1;
        }
    }

    // Create and configure SQLite database object.
    sqlite3 *db;
    int rc;
    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_open_v2(PyBytes_AS_STRING(bytes), &db,
                         SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                         (uri ? SQLITE_OPEN_URI : 0), NULL);
    if (rc == SQLITE_OK) {
        (void)sqlite3_busy_timeout(db, (int)(timeout*1000));
    }
    Py_END_ALLOW_THREADS

    Py_DECREF(bytes);
    if (db == NULL && rc == SQLITE_NOMEM) {
        PyErr_NoMemory();
        return -1;
    }

    pysqlite_state *state = pysqlite_get_state_by_type(Py_TYPE(self));
    if (rc != SQLITE_OK) {
        _pysqlite_seterror(state, db);
        goto error;
    }

    // Create LRU statement cache; returns a new reference.
    PyObject *statement_cache = new_statement_cache(self, state, cache_size);
    if (statement_cache == NULL) {
        goto error;
    }

    /* Create lists of weak references to cursors and blobs */
    PyObject *cursors = PyList_New(0);
    if (cursors == NULL) {
        Py_DECREF(statement_cache);
        goto error;
    }

    PyObject *blobs = PyList_New(0);
    if (blobs == NULL) {
        Py_DECREF(statement_cache);
        Py_DECREF(cursors);
        goto error;
    }

    // Init connection state members.
    self->db = db;
    self->state = state;
    self->detect_types = detect_types;
    self->isolation_level = isolation_level;
    self->autocommit = autocommit;
    self->check_same_thread = check_same_thread;
    self->thread_ident = PyThread_get_thread_ident();
    self->statement_cache = statement_cache;
    self->cursors = cursors;
    self->blobs = blobs;
    self->created_cursors = 0;
    self->row_factory = Py_NewRef(Py_None);
    self->text_factory = Py_NewRef(&PyUnicode_Type);
    self->trace_ctx = NULL;
    self->progress_ctx = NULL;
    self->authorizer_ctx = NULL;

    // Borrowed refs
    self->Warning               = state->Warning;
    self->Error                 = state->Error;
    self->InterfaceError        = state->InterfaceError;
    self->DatabaseError         = state->DatabaseError;
    self->DataError             = state->DataError;
    self->OperationalError      = state->OperationalError;
    self->IntegrityError        = state->IntegrityError;
    self->InternalError         = state->InternalError;
    self->ProgrammingError      = state->ProgrammingError;
    self->NotSupportedError     = state->NotSupportedError;

    if (PySys_Audit("sqlite3.connect/handle", "O", self) < 0) {
        return -1;  // Don't goto error; at this point, dealloc will clean up.
    }

    self->initialized = 1;

    if (autocommit == AUTOCOMMIT_DISABLED) {
        if (connection_exec_stmt(self, "BEGIN") < 0) {
            return -1;
        }
    }
    return 0;

error:
    // There are no statements or other SQLite objects attached to the
    // database, so sqlite3_close() should always return SQLITE_OK.
    rc = sqlite3_close(db);
    assert(rc == SQLITE_OK);
    return -1;
}

#define VISIT_CALLBACK_CONTEXT(ctx) \
do {                                \
    if (ctx) {                      \
        Py_VISIT(ctx->callable);    \
        Py_VISIT(ctx->module);      \
    }                               \
} while (0)

static int
connection_traverse(pysqlite_Connection *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->statement_cache);
    Py_VISIT(self->cursors);
    Py_VISIT(self->blobs);
    Py_VISIT(self->row_factory);
    Py_VISIT(self->text_factory);
    VISIT_CALLBACK_CONTEXT(self->trace_ctx);
    VISIT_CALLBACK_CONTEXT(self->progress_ctx);
    VISIT_CALLBACK_CONTEXT(self->authorizer_ctx);
#undef VISIT_CALLBACK_CONTEXT
    return 0;
}

static inline void
clear_callback_context(callback_context *ctx)
{
    if (ctx != NULL) {
        Py_CLEAR(ctx->callable);
        Py_CLEAR(ctx->module);
    }
}

static int
connection_clear(pysqlite_Connection *self)
{
    Py_CLEAR(self->statement_cache);
    Py_CLEAR(self->cursors);
    Py_CLEAR(self->blobs);
    Py_CLEAR(self->row_factory);
    Py_CLEAR(self->text_factory);
    clear_callback_context(self->trace_ctx);
    clear_callback_context(self->progress_ctx);
    clear_callback_context(self->authorizer_ctx);
    return 0;
}

static void
free_callback_contexts(pysqlite_Connection *self)
{
    set_callback_context(&self->trace_ctx, NULL);
    set_callback_context(&self->progress_ctx, NULL);
    set_callback_context(&self->authorizer_ctx, NULL);
}

static void
remove_callbacks(sqlite3 *db)
{
    assert(db != NULL);
    /* None of these APIs can fail, as long as they are given a valid
     * database pointer. */
    int rc;
#ifdef HAVE_TRACE_V2
    rc = sqlite3_trace_v2(db, SQLITE_TRACE_STMT, 0, 0);
    assert(rc == SQLITE_OK), (void)rc;
#else
    sqlite3_trace(db, 0, (void*)0);
#endif

    sqlite3_progress_handler(db, 0, 0, (void *)0);

    rc = sqlite3_set_authorizer(db, NULL, NULL);
    assert(rc == SQLITE_OK), (void)rc;
}

static int
connection_close(pysqlite_Connection *self)
{
    if (self->db == NULL) {
        return 0;
    }

    int rc = 0;
    if (self->autocommit == AUTOCOMMIT_DISABLED &&
        !sqlite3_get_autocommit(self->db))
    {
        if (connection_exec_stmt(self, "ROLLBACK") < 0) {
            rc = -1;
        }
    }

    sqlite3 *db = self->db;
    self->db = NULL;

    Py_BEGIN_ALLOW_THREADS
    /* The v2 close call always returns SQLITE_OK if given a valid database
     * pointer (which we do), so we can safely ignore the return value */
    (void)sqlite3_close_v2(db);
    Py_END_ALLOW_THREADS

    free_callback_contexts(self);
    return rc;
}

static void
connection_finalize(PyObject *self)
{
    pysqlite_Connection *con = (pysqlite_Connection *)self;
    PyObject *exc = PyErr_GetRaisedException();

    /* If close is implicitly called as a result of interpreter
     * tear-down, we must not call back into Python. */
    PyInterpreterState *interp = PyInterpreterState_Get();
    int teardown = _Py_IsInterpreterFinalizing(interp);
    if (teardown && con->db) {
        remove_callbacks(con->db);
    }

    /* Clean up if user has not called .close() explicitly. */
    if (connection_close(con) < 0) {
        if (teardown) {
            PyErr_Clear();
        }
        else {
            PyErr_WriteUnraisable((PyObject *)self);
        }
    }

    PyErr_SetRaisedException(exc);
}

static void
connection_dealloc(PyObject *self)
{
    if (PyObject_CallFinalizerFromDealloc(self) < 0) {
        return;
    }
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    tp->tp_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
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

    if (factory == NULL) {
        factory = (PyObject *)self->state->CursorType;
    }

    cursor = PyObject_CallOneArg(factory, (PyObject *)self);
    if (cursor == NULL)
        return NULL;
    if (!PyObject_TypeCheck(cursor, self->state->CursorType)) {
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
_sqlite3.Connection.blobopen as blobopen

    table: str
        Table name.
    column as col: str
        Column name.
    row: sqlite3_int64
        Row index.
    /
    *
    readonly: bool = False
        Open the BLOB without write permissions.
    name: str = "main"
        Database name.

Open and return a BLOB object.
[clinic start generated code]*/

static PyObject *
blobopen_impl(pysqlite_Connection *self, const char *table, const char *col,
              sqlite3_int64 row, int readonly, const char *name)
/*[clinic end generated code: output=6a02d43efb885d1c input=23576bd1108d8774]*/
{
    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    int rc;
    sqlite3_blob *blob;

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_blob_open(self->db, name, table, col, row, !readonly, &blob);
    Py_END_ALLOW_THREADS

    if (rc == SQLITE_MISUSE) {
        PyErr_Format(self->state->InterfaceError, sqlite3_errstr(rc));
        return NULL;
    }
    else if (rc != SQLITE_OK) {
        _pysqlite_seterror(self->state, self->db);
        return NULL;
    }

    pysqlite_Blob *obj = PyObject_GC_New(pysqlite_Blob, self->state->BlobType);
    if (obj == NULL) {
        goto error;
    }

    obj->connection = (pysqlite_Connection *)Py_NewRef(self);
    obj->blob = blob;
    obj->offset = 0;
    obj->in_weakreflist = NULL;

    PyObject_GC_Track(obj);

    // Add our blob to connection blobs list
    PyObject *weakref = PyWeakref_NewRef((PyObject *)obj, NULL);
    if (weakref == NULL) {
        goto error;
    }
    rc = PyList_Append(self->blobs, weakref);
    Py_DECREF(weakref);
    if (rc < 0) {
        goto error;
    }

    return (PyObject *)obj;

error:
    Py_XDECREF(obj);
    return NULL;
}

/*[clinic input]
_sqlite3.Connection.close as pysqlite_connection_close

Close the database connection.

Any pending transaction is not committed implicitly.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_close_impl(pysqlite_Connection *self)
/*[clinic end generated code: output=a546a0da212c9b97 input=b3ed5b74f6fefc06]*/
{
    if (!pysqlite_check_thread(self)) {
        return NULL;
    }

    if (!self->initialized) {
        PyTypeObject *tp = Py_TYPE(self);
        pysqlite_state *state = pysqlite_get_state_by_type(tp);
        PyErr_SetString(state->ProgrammingError,
                        "Base Connection.__init__ not called.");
        return NULL;
    }

    pysqlite_close_all_blobs(self);
    Py_CLEAR(self->statement_cache);
    if (connection_close(self) < 0) {
        return NULL;
    }

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
        pysqlite_state *state = pysqlite_get_state_by_type(Py_TYPE(con));
        PyErr_SetString(state->ProgrammingError,
                        "Base Connection.__init__ not called.");
        return 0;
    }

    if (!con->db) {
        PyErr_SetString(con->state->ProgrammingError,
                        "Cannot operate on a closed database.");
        return 0;
    } else {
        return 1;
    }
}

/*[clinic input]
_sqlite3.Connection.commit as pysqlite_connection_commit

Commit any pending transaction to the database.

If there is no open transaction, this method is a no-op.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_commit_impl(pysqlite_Connection *self)
/*[clinic end generated code: output=3da45579e89407f2 input=c8793c97c3446065]*/
{
    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    if (self->autocommit == AUTOCOMMIT_LEGACY) {
        if (!sqlite3_get_autocommit(self->db)) {
            if (connection_exec_stmt(self, "COMMIT") < 0) {
                return NULL;
            }
        }
    }
    else if (self->autocommit == AUTOCOMMIT_DISABLED) {
        if (connection_exec_stmt(self, "COMMIT") < 0) {
            return NULL;
        }
        if (connection_exec_stmt(self, "BEGIN") < 0) {
            return NULL;
        }
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_sqlite3.Connection.rollback as pysqlite_connection_rollback

Roll back to the start of any pending transaction.

If there is no open transaction, this method is a no-op.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_rollback_impl(pysqlite_Connection *self)
/*[clinic end generated code: output=b66fa0d43e7ef305 input=7f60a2f1076f16b3]*/
{
    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    if (self->autocommit == AUTOCOMMIT_LEGACY) {
        if (!sqlite3_get_autocommit(self->db)) {
            if (connection_exec_stmt(self, "ROLLBACK") < 0) {
                return NULL;
            }
        }
    }
    else if (self->autocommit == AUTOCOMMIT_DISABLED) {
        if (connection_exec_stmt(self, "ROLLBACK") < 0) {
            return NULL;
        }
        if (connection_exec_stmt(self, "BEGIN") < 0) {
            return NULL;
        }
    }
    Py_RETURN_NONE;
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
        double value = PyFloat_AsDouble(py_val);
        if (value == -1 && PyErr_Occurred()) {
            return -1;
        }
        sqlite3_result_double(context, value);
    } else if (PyUnicode_Check(py_val)) {
        Py_ssize_t sz;
        const char *str = PyUnicode_AsUTF8AndSize(py_val, &sz);
        if (str == NULL) {
            return -1;
        }
        if (sz > INT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "string is longer than INT_MAX bytes");
            return -1;
        }
        sqlite3_result_text(context, str, (int)sz, SQLITE_TRANSIENT);
    } else if (PyObject_CheckBuffer(py_val)) {
        Py_buffer view;
        if (PyObject_GetBuffer(py_val, &view, PyBUF_SIMPLE) != 0) {
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
        callback_context *ctx = (callback_context *)sqlite3_user_data(context);
        PyErr_Format(ctx->state->ProgrammingError,
                     "User-defined functions cannot return '%s' values to "
                     "SQLite",
                     Py_TYPE(py_val)->tp_name);
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
print_or_clear_traceback(callback_context *ctx)
{
    assert(ctx != NULL);
    assert(ctx->state != NULL);
    if (ctx->state->enable_callback_tracebacks) {
        PyErr_WriteUnraisable(ctx->callable);
    }
    else {
        PyErr_Clear();
    }
}

// Checks the Python exception and sets the appropriate SQLite error code.
static void
set_sqlite_error(sqlite3_context *context, const char *msg)
{
    assert(PyErr_Occurred());
    if (PyErr_ExceptionMatches(PyExc_MemoryError)) {
        sqlite3_result_error_nomem(context);
    }
    else if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
        sqlite3_result_error_toobig(context);
    }
    else {
        sqlite3_result_error(context, msg, -1);
    }
    callback_context *ctx = (callback_context *)sqlite3_user_data(context);
    print_or_clear_traceback(ctx);
}

static void
func_callback(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    PyGILState_STATE threadstate = PyGILState_Ensure();

    PyObject* args;
    PyObject* py_retval = NULL;
    int ok;

    args = _pysqlite_build_py_params(context, argc, argv);
    if (args) {
        callback_context *ctx = (callback_context *)sqlite3_user_data(context);
        assert(ctx != NULL);
        py_retval = PyObject_CallObject(ctx->callable, args);
        Py_DECREF(args);
    }

    ok = 0;
    if (py_retval) {
        ok = _pysqlite_set_result(context, py_retval) == 0;
        Py_DECREF(py_retval);
    }
    if (!ok) {
        set_sqlite_error(context, "user-defined function raised exception");
    }

    PyGILState_Release(threadstate);
}

static void
step_callback(sqlite3_context *context, int argc, sqlite3_value **params)
{
    PyGILState_STATE threadstate = PyGILState_Ensure();

    PyObject* args;
    PyObject* function_result = NULL;
    PyObject** aggregate_instance;
    PyObject* stepmethod = NULL;

    callback_context *ctx = (callback_context *)sqlite3_user_data(context);
    assert(ctx != NULL);

    aggregate_instance = (PyObject**)sqlite3_aggregate_context(context, sizeof(PyObject*));
    if (*aggregate_instance == NULL) {
        *aggregate_instance = PyObject_CallNoArgs(ctx->callable);
        if (!*aggregate_instance) {
            set_sqlite_error(context,
                    "user-defined aggregate's '__init__' method raised error");
            goto error;
        }
    }

    stepmethod = PyObject_GetAttr(*aggregate_instance, ctx->state->str_step);
    if (!stepmethod) {
        set_sqlite_error(context,
                "user-defined aggregate's 'step' method not defined");
        goto error;
    }

    args = _pysqlite_build_py_params(context, argc, params);
    if (!args) {
        goto error;
    }

    function_result = PyObject_CallObject(stepmethod, args);
    Py_DECREF(args);

    if (!function_result) {
        set_sqlite_error(context,
                "user-defined aggregate's 'step' method raised error");
    }

error:
    Py_XDECREF(stepmethod);
    Py_XDECREF(function_result);

    PyGILState_Release(threadstate);
}

static void
final_callback(sqlite3_context *context)
{
    PyGILState_STATE threadstate = PyGILState_Ensure();

    PyObject* function_result;
    PyObject** aggregate_instance;
    int ok;

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

    // Keep the exception (if any) of the last call to step, value, or inverse
    PyObject *exc = PyErr_GetRaisedException();

    callback_context *ctx = (callback_context *)sqlite3_user_data(context);
    assert(ctx != NULL);
    function_result = PyObject_CallMethodNoArgs(*aggregate_instance,
                                                ctx->state->str_finalize);
    Py_DECREF(*aggregate_instance);

    ok = 0;
    if (function_result) {
        ok = _pysqlite_set_result(context, function_result) == 0;
        Py_DECREF(function_result);
    }
    if (!ok) {
        int attr_err = PyErr_ExceptionMatches(PyExc_AttributeError);
        _PyErr_ChainExceptions1(exc);

        /* Note: contrary to the step, value, and inverse callbacks, SQLite
         * does _not_, as of SQLite 3.38.0, propagate errors to sqlite3_step()
         * from the finalize callback. This implies that execute*() will not
         * raise OperationalError, as it normally would. */
        set_sqlite_error(context, attr_err
                ? "user-defined aggregate's 'finalize' method not defined"
                : "user-defined aggregate's 'finalize' method raised error");
    }
    else {
        PyErr_SetRaisedException(exc);
    }

error:
    PyGILState_Release(threadstate);
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

/* Allocate a UDF/callback context structure. In order to ensure that the state
 * pointer always outlives the callback context, we make sure it owns a
 * reference to the module itself. create_callback_context() is always called
 * from connection methods, so we use the defining class to fetch the module
 * pointer.
 */
static callback_context *
create_callback_context(PyTypeObject *cls, PyObject *callable)
{
    callback_context *ctx = PyMem_Malloc(sizeof(callback_context));
    if (ctx != NULL) {
        PyObject *module = PyType_GetModule(cls);
        ctx->callable = Py_NewRef(callable);
        ctx->module = Py_NewRef(module);
        ctx->state = pysqlite_get_state(module);
    }
    return ctx;
}

static void
free_callback_context(callback_context *ctx)
{
    assert(ctx != NULL);
    Py_XDECREF(ctx->callable);
    Py_XDECREF(ctx->module);
    PyMem_Free(ctx);
}

static void
set_callback_context(callback_context **ctx_pp, callback_context *ctx)
{
    assert(ctx_pp != NULL);
    callback_context *tmp = *ctx_pp;
    *ctx_pp = ctx;
    if (tmp != NULL) {
        free_callback_context(tmp);
    }
}

static void
destructor_callback(void *ctx)
{
    if (ctx != NULL) {
        // This function may be called without the GIL held, so we need to
        // ensure that we destroy 'ctx' with the GIL held.
        PyGILState_STATE gstate = PyGILState_Ensure();
        free_callback_context((callback_context *)ctx);
        PyGILState_Release(gstate);
    }
}

/*[clinic input]
_sqlite3.Connection.create_function as pysqlite_connection_create_function

    cls: defining_class
    /
    name: str
    narg: int
    func: object
    *
    deterministic: bool = False

Creates a new function.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_create_function_impl(pysqlite_Connection *self,
                                         PyTypeObject *cls, const char *name,
                                         int narg, PyObject *func,
                                         int deterministic)
/*[clinic end generated code: output=8a811529287ad240 input=b3e8e1d8ddaffbef]*/
{
    int rc;
    int flags = SQLITE_UTF8;

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    if (deterministic) {
#if SQLITE_VERSION_NUMBER < 3008003
        PyErr_SetString(self->NotSupportedError,
                        "deterministic=True requires SQLite 3.8.3 or higher");
        return NULL;
#else
        if (sqlite3_libversion_number() < 3008003) {
            PyErr_SetString(self->NotSupportedError,
                            "deterministic=True requires SQLite 3.8.3 or higher");
            return NULL;
        }
        flags |= SQLITE_DETERMINISTIC;
#endif
    }
    callback_context *ctx = create_callback_context(cls, func);
    if (ctx == NULL) {
        return NULL;
    }
    rc = sqlite3_create_function_v2(self->db, name, narg, flags, ctx,
                                    func_callback,
                                    NULL,
                                    NULL,
                                    &destructor_callback);  // will decref func

    if (rc != SQLITE_OK) {
        /* Workaround for SQLite bug: no error code or string is available here */
        PyErr_SetString(self->OperationalError, "Error creating function");
        return NULL;
    }
    Py_RETURN_NONE;
}

#ifdef HAVE_WINDOW_FUNCTIONS
/*
 * Regarding the 'inverse' aggregate callback:
 * This method is only required by window aggregate functions, not
 * ordinary aggregate function implementations.  It is invoked to remove
 * a row from the current window.  The function arguments, if any,
 * correspond to the row being removed.
 */
static void
inverse_callback(sqlite3_context *context, int argc, sqlite3_value **params)
{
    PyGILState_STATE gilstate = PyGILState_Ensure();

    callback_context *ctx = (callback_context *)sqlite3_user_data(context);
    assert(ctx != NULL);

    int size = sizeof(PyObject *);
    PyObject **cls = (PyObject **)sqlite3_aggregate_context(context, size);
    assert(cls != NULL);
    assert(*cls != NULL);

    PyObject *method = PyObject_GetAttr(*cls, ctx->state->str_inverse);
    if (method == NULL) {
        set_sqlite_error(context,
                "user-defined aggregate's 'inverse' method not defined");
        goto exit;
    }

    PyObject *args = _pysqlite_build_py_params(context, argc, params);
    if (args == NULL) {
        set_sqlite_error(context,
                "unable to build arguments for user-defined aggregate's "
                "'inverse' method");
        goto exit;
    }

    PyObject *res = PyObject_CallObject(method, args);
    Py_DECREF(args);
    if (res == NULL) {
        set_sqlite_error(context,
                "user-defined aggregate's 'inverse' method raised error");
        goto exit;
    }
    Py_DECREF(res);

exit:
    Py_XDECREF(method);
    PyGILState_Release(gilstate);
}

/*
 * Regarding the 'value' aggregate callback:
 * This method is only required by window aggregate functions, not
 * ordinary aggregate function implementations.  It is invoked to return
 * the current value of the aggregate.
 */
static void
value_callback(sqlite3_context *context)
{
    PyGILState_STATE gilstate = PyGILState_Ensure();

    callback_context *ctx = (callback_context *)sqlite3_user_data(context);
    assert(ctx != NULL);

    int size = sizeof(PyObject *);
    PyObject **cls = (PyObject **)sqlite3_aggregate_context(context, size);
    assert(cls != NULL);
    assert(*cls != NULL);

    PyObject *res = PyObject_CallMethodNoArgs(*cls, ctx->state->str_value);
    if (res == NULL) {
        int attr_err = PyErr_ExceptionMatches(PyExc_AttributeError);
        set_sqlite_error(context, attr_err
                ? "user-defined aggregate's 'value' method not defined"
                : "user-defined aggregate's 'value' method raised error");
    }
    else {
        int rc = _pysqlite_set_result(context, res);
        Py_DECREF(res);
        if (rc < 0) {
            set_sqlite_error(context,
                    "unable to set result from user-defined aggregate's "
                    "'value' method");
        }
    }

    PyGILState_Release(gilstate);
}

/*[clinic input]
_sqlite3.Connection.create_window_function as create_window_function

    cls: defining_class
    name: str
        The name of the SQL aggregate window function to be created or
        redefined.
    num_params: int
        The number of arguments the step and inverse methods takes.
    aggregate_class: object
        A class with step(), finalize(), value(), and inverse() methods.
        Set to None to clear the window function.
    /

Creates or redefines an aggregate window function. Non-standard.
[clinic start generated code]*/

static PyObject *
create_window_function_impl(pysqlite_Connection *self, PyTypeObject *cls,
                            const char *name, int num_params,
                            PyObject *aggregate_class)
/*[clinic end generated code: output=5332cd9464522235 input=46d57a54225b5228]*/
{
    if (sqlite3_libversion_number() < 3025000) {
        PyErr_SetString(self->NotSupportedError,
                        "create_window_function() requires "
                        "SQLite 3.25.0 or higher");
        return NULL;
    }

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    int flags = SQLITE_UTF8;
    int rc;
    if (Py_IsNone(aggregate_class)) {
        rc = sqlite3_create_window_function(self->db, name, num_params, flags,
                                            0, 0, 0, 0, 0, 0);
    }
    else {
        callback_context *ctx = create_callback_context(cls, aggregate_class);
        if (ctx == NULL) {
            return NULL;
        }
        rc = sqlite3_create_window_function(self->db, name, num_params, flags,
                                            ctx,
                                            &step_callback,
                                            &final_callback,
                                            &value_callback,
                                            &inverse_callback,
                                            &destructor_callback);
    }

    if (rc != SQLITE_OK) {
        // Errors are not set on the database connection, so we cannot
        // use _pysqlite_seterror().
        PyErr_SetString(self->ProgrammingError, sqlite3_errstr(rc));
        return NULL;
    }
    Py_RETURN_NONE;
}
#endif

/*[clinic input]
_sqlite3.Connection.create_aggregate as pysqlite_connection_create_aggregate

    cls: defining_class
    /
    name: str
    n_arg: int
    aggregate_class: object

Creates a new aggregate.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_create_aggregate_impl(pysqlite_Connection *self,
                                          PyTypeObject *cls,
                                          const char *name, int n_arg,
                                          PyObject *aggregate_class)
/*[clinic end generated code: output=1b02d0f0aec7ff96 input=68a2a26366d4c686]*/
{
    int rc;

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    callback_context *ctx = create_callback_context(cls, aggregate_class);
    if (ctx == NULL) {
        return NULL;
    }
    rc = sqlite3_create_function_v2(self->db, name, n_arg, SQLITE_UTF8, ctx,
                                    0,
                                    &step_callback,
                                    &final_callback,
                                    &destructor_callback); // will decref func
    if (rc != SQLITE_OK) {
        /* Workaround for SQLite bug: no error code or string is available here */
        PyErr_SetString(self->OperationalError, "Error creating aggregate");
        return NULL;
    }
    Py_RETURN_NONE;
}

static int
authorizer_callback(void *ctx, int action, const char *arg1,
                    const char *arg2 , const char *dbname,
                    const char *access_attempt_source)
{
    PyGILState_STATE gilstate = PyGILState_Ensure();

    PyObject *ret;
    int rc = SQLITE_DENY;

    assert(ctx != NULL);
    PyObject *callable = ((callback_context *)ctx)->callable;
    ret = PyObject_CallFunction(callable, "issss", action, arg1, arg2, dbname,
                                access_attempt_source);

    if (ret == NULL) {
        print_or_clear_traceback(ctx);
        rc = SQLITE_DENY;
    }
    else {
        if (PyLong_Check(ret)) {
            rc = _PyLong_AsInt(ret);
            if (rc == -1 && PyErr_Occurred()) {
                print_or_clear_traceback(ctx);
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

static int
progress_callback(void *ctx)
{
    PyGILState_STATE gilstate = PyGILState_Ensure();

    int rc;
    PyObject *ret;

    assert(ctx != NULL);
    PyObject *callable = ((callback_context *)ctx)->callable;
    ret = PyObject_CallNoArgs(callable);
    if (!ret) {
        /* abort query if error occurred */
        rc = -1;
    }
    else {
        rc = PyObject_IsTrue(ret);
        Py_DECREF(ret);
    }
    if (rc < 0) {
        print_or_clear_traceback(ctx);
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
static int
trace_callback(unsigned int type, void *ctx, void *stmt, void *sql)
#else
static void
trace_callback(void *ctx, const char *sql)
#endif
{
#ifdef HAVE_TRACE_V2
    if (type != SQLITE_TRACE_STMT) {
        return 0;
    }
#endif

    PyGILState_STATE gilstate = PyGILState_Ensure();

    assert(ctx != NULL);
    pysqlite_state *state = ((callback_context *)ctx)->state;
    assert(state != NULL);

    PyObject *py_statement = NULL;
#ifdef HAVE_TRACE_V2
    const char *expanded_sql = sqlite3_expanded_sql((sqlite3_stmt *)stmt);
    if (expanded_sql == NULL) {
        sqlite3 *db = sqlite3_db_handle((sqlite3_stmt *)stmt);
        if (sqlite3_errcode(db) == SQLITE_NOMEM) {
            (void)PyErr_NoMemory();
            goto exit;
        }

        PyErr_SetString(state->DataError,
                "Expanded SQL string exceeds the maximum string length");
        print_or_clear_traceback((callback_context *)ctx);

        // Fall back to unexpanded sql
        py_statement = PyUnicode_FromString((const char *)sql);
    }
    else {
        py_statement = PyUnicode_FromString(expanded_sql);
        sqlite3_free((void *)expanded_sql);
    }
#else
    if (sql == NULL) {
        PyErr_SetString(state->DataError,
                "Expanded SQL string exceeds the maximum string length");
        print_or_clear_traceback((callback_context *)ctx);
        goto exit;
    }
    py_statement = PyUnicode_FromString(sql);
#endif
    if (py_statement) {
        PyObject *callable = ((callback_context *)ctx)->callable;
        PyObject *ret = PyObject_CallOneArg(callable, py_statement);
        Py_DECREF(py_statement);
        Py_XDECREF(ret);
    }
    if (PyErr_Occurred()) {
        print_or_clear_traceback((callback_context *)ctx);
    }

exit:
    PyGILState_Release(gilstate);
#ifdef HAVE_TRACE_V2
    return 0;
#endif
}

/*[clinic input]
_sqlite3.Connection.set_authorizer as pysqlite_connection_set_authorizer

    cls: defining_class
    /
    authorizer_callback as callable: object

Sets authorizer callback.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_set_authorizer_impl(pysqlite_Connection *self,
                                        PyTypeObject *cls,
                                        PyObject *callable)
/*[clinic end generated code: output=75fa60114fc971c3 input=605d32ba92dd3eca]*/
{
    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    int rc;
    if (callable == Py_None) {
        rc = sqlite3_set_authorizer(self->db, NULL, NULL);
        set_callback_context(&self->authorizer_ctx, NULL);
    }
    else {
        callback_context *ctx = create_callback_context(cls, callable);
        if (ctx == NULL) {
            return NULL;
        }
        rc = sqlite3_set_authorizer(self->db, authorizer_callback, ctx);
        set_callback_context(&self->authorizer_ctx, ctx);
    }
    if (rc != SQLITE_OK) {
        PyErr_SetString(self->OperationalError,
                        "Error setting authorizer callback");
        set_callback_context(&self->authorizer_ctx, NULL);
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_sqlite3.Connection.set_progress_handler as pysqlite_connection_set_progress_handler

    cls: defining_class
    /
    progress_handler as callable: object
    n: int

Sets progress handler callback.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_set_progress_handler_impl(pysqlite_Connection *self,
                                              PyTypeObject *cls,
                                              PyObject *callable, int n)
/*[clinic end generated code: output=0739957fd8034a50 input=f7c1837984bd86db]*/
{
    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    if (callable == Py_None) {
        /* None clears the progress handler previously set */
        sqlite3_progress_handler(self->db, 0, 0, (void*)0);
        set_callback_context(&self->progress_ctx, NULL);
    }
    else {
        callback_context *ctx = create_callback_context(cls, callable);
        if (ctx == NULL) {
            return NULL;
        }
        sqlite3_progress_handler(self->db, n, progress_callback, ctx);
        set_callback_context(&self->progress_ctx, ctx);
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_sqlite3.Connection.set_trace_callback as pysqlite_connection_set_trace_callback

    cls: defining_class
    /
    trace_callback as callable: object

Sets a trace callback called for each SQL statement (passed as unicode).
[clinic start generated code]*/

static PyObject *
pysqlite_connection_set_trace_callback_impl(pysqlite_Connection *self,
                                            PyTypeObject *cls,
                                            PyObject *callable)
/*[clinic end generated code: output=d91048c03bfcee05 input=351a94210c5f81bb]*/
{
    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    if (callable == Py_None) {
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
        set_callback_context(&self->trace_ctx, NULL);
    }
    else {
        callback_context *ctx = create_callback_context(cls, callable);
        if (ctx == NULL) {
            return NULL;
        }
#ifdef HAVE_TRACE_V2
        sqlite3_trace_v2(self->db, SQLITE_TRACE_STMT, trace_callback, ctx);
#else
        sqlite3_trace(self->db, trace_callback, ctx);
#endif
        set_callback_context(&self->trace_ctx, ctx);
    }

    Py_RETURN_NONE;
}

#ifdef PY_SQLITE_ENABLE_LOAD_EXTENSION
/*[clinic input]
_sqlite3.Connection.enable_load_extension as pysqlite_connection_enable_load_extension

    enable as onoff: bool
    /

Enable dynamic loading of SQLite extension modules.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_enable_load_extension_impl(pysqlite_Connection *self,
                                               int onoff)
/*[clinic end generated code: output=9cac37190d388baf input=2a1e87931486380f]*/
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
        PyErr_SetString(self->OperationalError,
                        "Error enabling load extension");
        return NULL;
    } else {
        Py_RETURN_NONE;
    }
}

/*[clinic input]
_sqlite3.Connection.load_extension as pysqlite_connection_load_extension

    name as extension_name: str
    /
    *
    entrypoint: str(accept={str, NoneType}) = None

Load SQLite extension module.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_load_extension_impl(pysqlite_Connection *self,
                                        const char *extension_name,
                                        const char *entrypoint)
/*[clinic end generated code: output=7e61a7add9de0286 input=c36b14ea702e04f5]*/
{
    int rc;
    char* errmsg;

    if (PySys_Audit("sqlite3.load_extension", "Os", self, extension_name) < 0) {
        return NULL;
    }

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    rc = sqlite3_load_extension(self->db, extension_name, entrypoint, &errmsg);
    if (rc != 0) {
        PyErr_SetString(self->OperationalError, errmsg);
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
            PyErr_Format(self->ProgrammingError,
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
    if (!pysqlite_check_connection(self)) {
        return NULL;
    }
    if (self->isolation_level != NULL) {
        return PyUnicode_FromString(self->isolation_level);
    }
    Py_RETURN_NONE;
}

static PyObject* pysqlite_connection_get_total_changes(pysqlite_Connection* self, void* unused)
{
    if (!pysqlite_check_connection(self)) {
        return NULL;
    }
    return PyLong_FromLong(sqlite3_total_changes(self->db));
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
    if (Py_IsNone(isolation_level)) {
        self->isolation_level = NULL;

        // Execute a COMMIT to re-enable autocommit mode
        PyObject *res = pysqlite_connection_commit_impl(self);
        if (res == NULL) {
            return -1;
        }
        Py_DECREF(res);
        return 0;
    }
    if (!isolation_level_converter(isolation_level, &self->isolation_level)) {
        return -1;
    }
    return 0;
}

static PyObject *
pysqlite_connection_call(pysqlite_Connection *self, PyObject *args,
                         PyObject *kwargs)
{
    PyObject* sql;
    pysqlite_Statement* statement;

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    if (!_PyArg_NoKeywords(MODULE_NAME ".Connection", kwargs))
        return NULL;

    if (!PyArg_ParseTuple(args, "U", &sql))
        return NULL;

    statement = pysqlite_statement_create(self, sql);
    if (statement == NULL) {
        return NULL;
    }

    return (PyObject*)statement;
}

/*[clinic input]
_sqlite3.Connection.execute as pysqlite_connection_execute

    sql: unicode
    parameters: object = NULL
    /

Executes an SQL statement.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_execute_impl(pysqlite_Connection *self, PyObject *sql,
                                 PyObject *parameters)
/*[clinic end generated code: output=5be05ae01ee17ee4 input=27aa7792681ddba2]*/
{
    PyObject* result = 0;

    PyObject *cursor = pysqlite_connection_cursor_impl(self, NULL);
    if (!cursor) {
        goto error;
    }

    result = _pysqlite_query_execute((pysqlite_Cursor *)cursor, 0, sql, parameters);
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

Repeatedly executes an SQL statement.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_executemany_impl(pysqlite_Connection *self,
                                     PyObject *sql, PyObject *parameters)
/*[clinic end generated code: output=776cd2fd20bfe71f input=495be76551d525db]*/
{
    PyObject* result = 0;

    PyObject *cursor = pysqlite_connection_cursor_impl(self, NULL);
    if (!cursor) {
        goto error;
    }

    result = _pysqlite_query_execute((pysqlite_Cursor *)cursor, 1, sql, parameters);
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

Executes multiple SQL statements at once.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_executescript(pysqlite_Connection *self,
                                  PyObject *script_obj)
/*[clinic end generated code: output=4c4f9d77aa0ae37d input=f6e5f1ccfa313db4]*/
{
    PyObject* result = 0;

    PyObject *cursor = pysqlite_connection_cursor_impl(self, NULL);
    if (!cursor) {
        goto error;
    }

    PyObject *meth = self->state->str_executescript;  // borrowed ref.
    result = PyObject_CallMethodObjArgs(cursor, meth, script_obj, NULL);
    if (!result) {
        Py_CLEAR(cursor);
    }

error:
    Py_XDECREF(result);

    return cursor;
}

/* ------------------------- COLLATION CODE ------------------------ */

static int
collation_callback(void *context, int text1_length, const void *text1_data,
                   int text2_length, const void *text2_data)
{
    PyGILState_STATE gilstate = PyGILState_Ensure();

    PyObject* string1 = 0;
    PyObject* string2 = 0;
    PyObject* retval = NULL;
    long longval;
    int result = 0;

    /* This callback may be executed multiple times per sqlite3_step(). Bail if
     * the previous call failed */
    if (PyErr_Occurred()) {
        goto finally;
    }

    string1 = PyUnicode_FromStringAndSize((const char*)text1_data, text1_length);
    if (string1 == NULL) {
        goto finally;
    }
    string2 = PyUnicode_FromStringAndSize((const char*)text2_data, text2_length);
    if (string2 == NULL) {
        goto finally;
    }

    callback_context *ctx = (callback_context *)context;
    assert(ctx != NULL);
    PyObject *args[] = { NULL, string1, string2 };  // Borrowed refs.
    size_t nargsf = 2 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    retval = PyObject_Vectorcall(ctx->callable, args + 1, nargsf, NULL);
    if (retval == NULL) {
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

Abort any pending database operation.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_interrupt_impl(pysqlite_Connection *self)
/*[clinic end generated code: output=f193204bc9e70b47 input=75ad03ade7012859]*/
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
[clinic start generated code]*/

static PyObject *
pysqlite_connection_iterdump_impl(pysqlite_Connection *self)
/*[clinic end generated code: output=586997aaf9808768 input=1911ca756066da89]*/
{
    if (!pysqlite_check_connection(self)) {
        return NULL;
    }

    PyObject *iterdump = _PyImport_GetModuleAttrString(MODULE_NAME ".dump", "_iterdump");
    if (!iterdump) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(self->OperationalError,
                            "Failed to obtain _iterdump() reference");
        }
        return NULL;
    }

    PyObject *retval = PyObject_CallOneArg(iterdump, (PyObject *)self);
    Py_DECREF(iterdump);
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

Makes a backup of the database.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_backup_impl(pysqlite_Connection *self,
                                pysqlite_Connection *target, int pages,
                                PyObject *progress, const char *name,
                                double sleep)
/*[clinic end generated code: output=306a3e6a38c36334 input=c6519d0f59d0fd7f]*/
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
        PyErr_SetString(self->OperationalError, "target is in transaction");
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
        _pysqlite_seterror(self->state, bck_conn);
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
        _pysqlite_seterror(self->state, bck_conn);
        return NULL;
    }

    Py_RETURN_NONE;
}

/*[clinic input]
_sqlite3.Connection.create_collation as pysqlite_connection_create_collation

    cls: defining_class
    name: str
    callback as callable: object
    /

Creates a collation function.
[clinic start generated code]*/

static PyObject *
pysqlite_connection_create_collation_impl(pysqlite_Connection *self,
                                          PyTypeObject *cls,
                                          const char *name,
                                          PyObject *callable)
/*[clinic end generated code: output=32d339e97869c378 input=f67ecd2e31e61ad3]*/
{
    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    callback_context *ctx = NULL;
    int rc;
    int flags = SQLITE_UTF8;
    if (callable == Py_None) {
        rc = sqlite3_create_collation_v2(self->db, name, flags,
                                         NULL, NULL, NULL);
    }
    else {
        if (!PyCallable_Check(callable)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be callable");
            return NULL;
        }
        ctx = create_callback_context(cls, callable);
        if (ctx == NULL) {
            return NULL;
        }
        rc = sqlite3_create_collation_v2(self->db, name, flags, ctx,
                                         &collation_callback,
                                         &destructor_callback);
    }

    if (rc != SQLITE_OK) {
        /* Unlike other sqlite3_* functions, the destructor callback is _not_
         * called if sqlite3_create_collation_v2() fails, so we have to free
         * the context before returning.
         */
        if (callable != Py_None) {
            free_callback_context(ctx);
        }
        _pysqlite_seterror(self->state, self->db);
        return NULL;
    }

    Py_RETURN_NONE;
}

#ifdef PY_SQLITE_HAVE_SERIALIZE
/*[clinic input]
_sqlite3.Connection.serialize as serialize

    *
    name: str = "main"
        Which database to serialize.

Serialize a database into a byte string.

For an ordinary on-disk database file, the serialization is just a copy of the
disk file. For an in-memory database or a "temp" database, the serialization is
the same sequence of bytes which would be written to disk if that database
were backed up to disk.
[clinic start generated code]*/

static PyObject *
serialize_impl(pysqlite_Connection *self, const char *name)
/*[clinic end generated code: output=97342b0e55239dd3 input=d2eb5194a65abe2b]*/
{
    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    /* If SQLite has a contiguous memory representation of the database, we can
     * avoid memory allocations, so we try with the no-copy flag first.
     */
    sqlite3_int64 size;
    unsigned int flags = SQLITE_SERIALIZE_NOCOPY;
    const char *data;

    Py_BEGIN_ALLOW_THREADS
    data = (const char *)sqlite3_serialize(self->db, name, &size, flags);
    if (data == NULL) {
        flags &= ~SQLITE_SERIALIZE_NOCOPY;
        data = (const char *)sqlite3_serialize(self->db, name, &size, flags);
    }
    Py_END_ALLOW_THREADS

    if (data == NULL) {
        PyErr_Format(self->OperationalError, "unable to serialize '%s'",
                     name);
        return NULL;
    }
    PyObject *res = PyBytes_FromStringAndSize(data, (Py_ssize_t)size);
    if (!(flags & SQLITE_SERIALIZE_NOCOPY)) {
        sqlite3_free((void *)data);
    }
    return res;
}

/*[clinic input]
_sqlite3.Connection.deserialize as deserialize

    data: Py_buffer(accept={buffer, str})
        The serialized database content.
    /
    *
    name: str = "main"
        Which database to reopen with the deserialization.

Load a serialized database.

The deserialize interface causes the database connection to disconnect from the
target database, and then reopen it as an in-memory database based on the given
serialized data.

The deserialize interface will fail with SQLITE_BUSY if the database is
currently in a read transaction or is involved in a backup operation.
[clinic start generated code]*/

static PyObject *
deserialize_impl(pysqlite_Connection *self, Py_buffer *data,
                 const char *name)
/*[clinic end generated code: output=e394c798b98bad89 input=1be4ca1faacf28f2]*/
{
    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    /* Transfer ownership of the buffer to SQLite:
     * - Move buffer from Py to SQLite
     * - Tell SQLite to free buffer memory
     * - Tell SQLite that it is permitted to grow the resulting database
     *
     * Make sure we don't overflow sqlite3_deserialize(); it accepts a signed
     * 64-bit int as its data size argument.
     *
     * We can safely use sqlite3_malloc64 here, since it was introduced before
     * the serialize APIs.
     */
    if (data->len > 9223372036854775807) {  // (1 << 63) - 1
        PyErr_SetString(PyExc_OverflowError, "'data' is too large");
        return NULL;
    }

    sqlite3_int64 size = (sqlite3_int64)data->len;
    unsigned char *buf = sqlite3_malloc64(size);
    if (buf == NULL) {
        return PyErr_NoMemory();
    }

    const unsigned int flags = SQLITE_DESERIALIZE_FREEONCLOSE |
                               SQLITE_DESERIALIZE_RESIZEABLE;
    int rc;
    Py_BEGIN_ALLOW_THREADS
    (void)memcpy(buf, data->buf, data->len);
    rc = sqlite3_deserialize(self->db, name, buf, size, size, flags);
    Py_END_ALLOW_THREADS

    if (rc != SQLITE_OK) {
        (void)_pysqlite_seterror(self->state, self->db);
        return NULL;
    }
    Py_RETURN_NONE;
}
#endif  // PY_SQLITE_HAVE_SERIALIZE


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
    int commit = 0;
    PyObject* result;

    if (exc_type == Py_None && exc_value == Py_None && exc_tb == Py_None) {
        commit = 1;
        result = pysqlite_connection_commit_impl(self);
    }
    else {
        result = pysqlite_connection_rollback_impl(self);
    }

    if (result == NULL) {
        if (commit) {
            /* Commit failed; try to rollback in order to unlock the database.
             * If rollback also fails, chain the exceptions. */
            PyObject *exc = PyErr_GetRaisedException();
            result = pysqlite_connection_rollback_impl(self);
            if (result == NULL) {
                _PyErr_ChainExceptions1(exc);
            }
            else {
                Py_DECREF(result);
                PyErr_SetRaisedException(exc);
            }
        }
        return NULL;
    }
    Py_DECREF(result);

    Py_RETURN_FALSE;
}

/*[clinic input]
_sqlite3.Connection.setlimit as setlimit

    category: int
        The limit category to be set.
    limit: int
        The new limit. If the new limit is a negative number, the limit is
        unchanged.
    /

Set connection run-time limits.

Attempts to increase a limit above its hard upper bound are silently truncated
to the hard upper bound. Regardless of whether or not the limit was changed,
the prior value of the limit is returned.
[clinic start generated code]*/

static PyObject *
setlimit_impl(pysqlite_Connection *self, int category, int limit)
/*[clinic end generated code: output=0d208213f8d68ccd input=9bd469537e195635]*/
{
    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    int old_limit = sqlite3_limit(self->db, category, limit);
    if (old_limit < 0) {
        PyErr_SetString(self->ProgrammingError, "'category' is out of bounds");
        return NULL;
    }
    return PyLong_FromLong(old_limit);
}

/*[clinic input]
_sqlite3.Connection.getlimit as getlimit

    category: int
        The limit category to be queried.
    /

Get connection run-time limits.
[clinic start generated code]*/

static PyObject *
getlimit_impl(pysqlite_Connection *self, int category)
/*[clinic end generated code: output=7c3f5d11f24cecb1 input=61e0849fb4fb058f]*/
{
    return setlimit_impl(self, category, -1);
}

static inline bool
is_int_config(const int op)
{
    switch (op) {
        case SQLITE_DBCONFIG_ENABLE_FKEY:
        case SQLITE_DBCONFIG_ENABLE_TRIGGER:
#if SQLITE_VERSION_NUMBER >= 3012002
        case SQLITE_DBCONFIG_ENABLE_FTS3_TOKENIZER:
#endif
#if SQLITE_VERSION_NUMBER >= 3013000
        case SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION:
#endif
#if SQLITE_VERSION_NUMBER >= 3016000
        case SQLITE_DBCONFIG_NO_CKPT_ON_CLOSE:
#endif
#if SQLITE_VERSION_NUMBER >= 3020000
        case SQLITE_DBCONFIG_ENABLE_QPSG:
#endif
#if SQLITE_VERSION_NUMBER >= 3022000
        case SQLITE_DBCONFIG_TRIGGER_EQP:
#endif
#if SQLITE_VERSION_NUMBER >= 3024000
        case SQLITE_DBCONFIG_RESET_DATABASE:
#endif
#if SQLITE_VERSION_NUMBER >= 3026000
        case SQLITE_DBCONFIG_DEFENSIVE:
#endif
#if SQLITE_VERSION_NUMBER >= 3028000
        case SQLITE_DBCONFIG_WRITABLE_SCHEMA:
#endif
#if SQLITE_VERSION_NUMBER >= 3029000
        case SQLITE_DBCONFIG_DQS_DDL:
        case SQLITE_DBCONFIG_DQS_DML:
        case SQLITE_DBCONFIG_LEGACY_ALTER_TABLE:
#endif
#if SQLITE_VERSION_NUMBER >= 3030000
        case SQLITE_DBCONFIG_ENABLE_VIEW:
#endif
#if SQLITE_VERSION_NUMBER >= 3031000
        case SQLITE_DBCONFIG_LEGACY_FILE_FORMAT:
        case SQLITE_DBCONFIG_TRUSTED_SCHEMA:
#endif
            return true;
        default:
            return false;
    }
}

/*[clinic input]
_sqlite3.Connection.setconfig as setconfig

    op: int
        The configuration verb; one of the sqlite3.SQLITE_DBCONFIG codes.
    enable: bool = True
    /

Set a boolean connection configuration option.
[clinic start generated code]*/

static PyObject *
setconfig_impl(pysqlite_Connection *self, int op, int enable)
/*[clinic end generated code: output=c60b13e618aff873 input=a10f1539c2d7da6b]*/
{
    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }
    if (!is_int_config(op)) {
        return PyErr_Format(PyExc_ValueError, "unknown config 'op': %d", op);
    }

    int actual;
    int rc = sqlite3_db_config(self->db, op, enable, &actual);
    if (rc != SQLITE_OK) {
        (void)_pysqlite_seterror(self->state, self->db);
        return NULL;
    }
    if (enable != actual) {
        PyErr_SetString(self->state->OperationalError, "Unable to set config");
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_sqlite3.Connection.getconfig as getconfig -> bool

    op: int
        The configuration verb; one of the sqlite3.SQLITE_DBCONFIG codes.
    /

Query a boolean connection configuration option.
[clinic start generated code]*/

static int
getconfig_impl(pysqlite_Connection *self, int op)
/*[clinic end generated code: output=25ac05044c7b78a3 input=b0526d7e432e3f2f]*/
{
    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return -1;
    }
    if (!is_int_config(op)) {
        PyErr_Format(PyExc_ValueError, "unknown config 'op': %d", op);
        return -1;
    }

    int current;
    int rc = sqlite3_db_config(self->db, op, -1, &current);
    if (rc != SQLITE_OK) {
        (void)_pysqlite_seterror(self->state, self->db);
        return -1;
    }
    return current;
}

static PyObject *
get_autocommit(pysqlite_Connection *self, void *Py_UNUSED(ctx))
{
    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }
    if (self->autocommit == AUTOCOMMIT_ENABLED) {
        Py_RETURN_TRUE;
    }
    if (self->autocommit == AUTOCOMMIT_DISABLED) {
        Py_RETURN_FALSE;
    }
    return PyLong_FromLong(LEGACY_TRANSACTION_CONTROL);
}

static int
set_autocommit(pysqlite_Connection *self, PyObject *val, void *Py_UNUSED(ctx))
{
    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return -1;
    }
    if (!autocommit_converter(val, &self->autocommit)) {
        return -1;
    }
    if (self->autocommit == AUTOCOMMIT_ENABLED) {
        if (!sqlite3_get_autocommit(self->db)) {
            if (connection_exec_stmt(self, "COMMIT") < 0) {
                return -1;
            }
        }
    }
    else if (self->autocommit == AUTOCOMMIT_DISABLED) {
        if (sqlite3_get_autocommit(self->db)) {
            if (connection_exec_stmt(self, "BEGIN") < 0) {
                return -1;
            }
        }
    }
    return 0;
}


static const char connection_doc[] =
PyDoc_STR("SQLite database connection object.");

static PyGetSetDef connection_getset[] = {
    {"isolation_level",  (getter)pysqlite_connection_get_isolation_level, (setter)pysqlite_connection_set_isolation_level},
    {"total_changes",  (getter)pysqlite_connection_get_total_changes, (setter)0},
    {"in_transaction",  (getter)pysqlite_connection_get_in_transaction, (setter)0},
    {"autocommit",  (getter)get_autocommit, (setter)set_autocommit},
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
    SETLIMIT_METHODDEF
    GETLIMIT_METHODDEF
    SERIALIZE_METHODDEF
    DESERIALIZE_METHODDEF
    CREATE_WINDOW_FUNCTION_METHODDEF
    BLOBOPEN_METHODDEF
    SETCONFIG_METHODDEF
    GETCONFIG_METHODDEF
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
    {Py_tp_finalize, connection_finalize},
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
