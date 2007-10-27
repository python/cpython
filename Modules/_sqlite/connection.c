/* connection.c - the connection type
 *
 * Copyright (C) 2004-2006 Gerhard Häring <gh@ghaering.de>
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

#include "cache.h"
#include "module.h"
#include "connection.h"
#include "statement.h"
#include "cursor.h"
#include "prepare_protocol.h"
#include "util.h"
#include "sqlitecompat.h"

#include "pythread.h"

static int pysqlite_connection_set_isolation_level(pysqlite_Connection* self, PyObject* isolation_level);


void _sqlite3_result_error(sqlite3_context* ctx, const char* errmsg, int len)
{
    /* in older SQLite versions, calling sqlite3_result_error in callbacks
     * triggers a bug in SQLite that leads either to irritating results or
     * segfaults, depending on the SQLite version */
#if SQLITE_VERSION_NUMBER >= 3003003
    sqlite3_result_error(ctx, errmsg, len);
#else
    PyErr_SetString(pysqlite_OperationalError, errmsg);
#endif
}

int pysqlite_connection_init(pysqlite_Connection* self, PyObject* args, PyObject* kwargs)
{
    static char *kwlist[] = {"database", "timeout", "detect_types", "isolation_level", "check_same_thread", "factory", "cached_statements", NULL, NULL};

    char* database;
    int detect_types = 0;
    PyObject* isolation_level = NULL;
    PyObject* factory = NULL;
    int check_same_thread = 1;
    int cached_statements = 100;
    double timeout = 5.0;
    int rc;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|diOiOi", kwlist,
                                     &database, &timeout, &detect_types, &isolation_level, &check_same_thread, &factory, &cached_statements))
    {
        return -1; 
    }

    self->begin_statement = NULL;

    self->statement_cache = NULL;
    self->statements = NULL;

    Py_INCREF(Py_None);
    self->row_factory = Py_None;

    Py_INCREF(&PyUnicode_Type);
    self->text_factory = (PyObject*)&PyUnicode_Type;

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_open(database, &self->db);
    Py_END_ALLOW_THREADS

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
    self->isolation_level = NULL;
    pysqlite_connection_set_isolation_level(self, isolation_level);
    Py_DECREF(isolation_level);

    self->statement_cache = (pysqlite_Cache*)PyObject_CallFunction((PyObject*)&pysqlite_CacheType, "Oi", self, cached_statements);
    if (PyErr_Occurred()) {
        return -1;
    }

    self->statements = PyList_New(0);
    if (!self->statements) {
        return -1;
    }
    self->created_statements = 0;

    /* By default, the Cache class INCREFs the factory in its initializer, and
     * decrefs it in its deallocator method. Since this would create a circular
     * reference here, we're breaking it by decrementing self, and telling the
     * cache class to not decref the factory (self) in its deallocator.
     */
    self->statement_cache->decref_factory = 0;
    Py_DECREF(self);

    self->inTransaction = 0;
    self->detect_types = detect_types;
    self->timeout = timeout;
    (void)sqlite3_busy_timeout(self->db, (int)(timeout*1000));

    self->thread_ident = PyThread_get_thread_ident();
    self->check_same_thread = check_same_thread;

    self->function_pinboard = PyDict_New();
    if (!self->function_pinboard) {
        return -1;
    }

    self->collations = PyDict_New();
    if (!self->collations) {
        return -1;
    }

    self->Warning               = pysqlite_Warning;
    self->Error                 = pysqlite_Error;
    self->InterfaceError        = pysqlite_InterfaceError;
    self->DatabaseError         = pysqlite_DatabaseError;
    self->DataError             = pysqlite_DataError;
    self->OperationalError      = pysqlite_OperationalError;
    self->IntegrityError        = pysqlite_IntegrityError;
    self->InternalError         = pysqlite_InternalError;
    self->ProgrammingError      = pysqlite_ProgrammingError;
    self->NotSupportedError     = pysqlite_NotSupportedError;

    return 0;
}

/* Empty the entire statement cache of this connection */
void pysqlite_flush_statement_cache(pysqlite_Connection* self)
{
    pysqlite_Node* node;
    pysqlite_Statement* statement;

    node = self->statement_cache->first;

    while (node) {
        statement = (pysqlite_Statement*)(node->data);
        (void)pysqlite_statement_finalize(statement);
        node = node->next;
    }

    Py_DECREF(self->statement_cache);
    self->statement_cache = (pysqlite_Cache*)PyObject_CallFunction((PyObject*)&pysqlite_CacheType, "O", self);
    Py_DECREF(self);
    self->statement_cache->decref_factory = 0;
}

void pysqlite_reset_all_statements(pysqlite_Connection* self)
{
    int i;
    PyObject* weakref;
    PyObject* statement;

    for (i = 0; i < PyList_Size(self->statements); i++) {
        weakref = PyList_GetItem(self->statements, i);
        statement = PyWeakref_GetObject(weakref);
        if (statement != Py_None) {
            (void)pysqlite_statement_reset((pysqlite_Statement*)statement);
        }
    }
}

void pysqlite_connection_dealloc(pysqlite_Connection* self)
{
    Py_XDECREF(self->statement_cache);

    /* Clean up if user has not called .close() explicitly. */
    if (self->db) {
        Py_BEGIN_ALLOW_THREADS
        sqlite3_close(self->db);
        Py_END_ALLOW_THREADS
    }

    if (self->begin_statement) {
        PyMem_Free(self->begin_statement);
    }
    Py_XDECREF(self->isolation_level);
    Py_XDECREF(self->function_pinboard);
    Py_XDECREF(self->row_factory);
    Py_XDECREF(self->text_factory);
    Py_XDECREF(self->collations);
    Py_XDECREF(self->statements);

    Py_Type(self)->tp_free((PyObject*)self);
}

PyObject* pysqlite_connection_cursor(pysqlite_Connection* self, PyObject* args, PyObject* kwargs)
{
    static char *kwlist[] = {"factory", NULL, NULL};
    PyObject* factory = NULL;
    PyObject* cursor;


    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist,
                                     &factory)) {
        return NULL;
    }

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    if (factory == NULL) {
        factory = (PyObject*)&pysqlite_CursorType;
    }

    cursor = PyObject_CallFunction(factory, "O", self);

    if (cursor && self->row_factory != Py_None) {
        Py_XDECREF(((pysqlite_Cursor*)cursor)->row_factory);
        Py_INCREF(self->row_factory);
        ((pysqlite_Cursor*)cursor)->row_factory = self->row_factory;
    }

    return cursor;
}

PyObject* pysqlite_connection_close(pysqlite_Connection* self, PyObject* args)
{
    int rc;

    if (!pysqlite_check_thread(self)) {
        return NULL;
    }

    pysqlite_flush_statement_cache(self);

    if (self->db) {
        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_close(self->db);
        Py_END_ALLOW_THREADS

        if (rc != SQLITE_OK) {
            _pysqlite_seterror(self->db);
            return NULL;
        } else {
            self->db = NULL;
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
}

/*
 * Checks if a connection object is usable (i. e. not closed).
 *
 * 0 => error; 1 => ok
 */
int pysqlite_check_connection(pysqlite_Connection* con)
{
    if (!con->db) {
        PyErr_SetString(pysqlite_ProgrammingError, "Cannot operate on a closed database.");
        return 0;
    } else {
        return 1;
    }
}

PyObject* _pysqlite_connection_begin(pysqlite_Connection* self)
{
    int rc;
    const char* tail;
    sqlite3_stmt* statement;

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_prepare(self->db, self->begin_statement, -1, &statement, &tail);
    Py_END_ALLOW_THREADS

    if (rc != SQLITE_OK) {
        _pysqlite_seterror(self->db);
        goto error;
    }

    rc = _sqlite_step_with_busyhandler(statement, self);
    if (rc == SQLITE_DONE) {
        self->inTransaction = 1;
    } else {
        _pysqlite_seterror(self->db);
    }

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_finalize(statement);
    Py_END_ALLOW_THREADS

    if (rc != SQLITE_OK && !PyErr_Occurred()) {
        _pysqlite_seterror(self->db);
    }

error:
    if (PyErr_Occurred()) {
        return NULL;
    } else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

PyObject* pysqlite_connection_commit(pysqlite_Connection* self, PyObject* args)
{
    int rc;
    const char* tail;
    sqlite3_stmt* statement;

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    if (self->inTransaction) {
        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_prepare(self->db, "COMMIT", -1, &statement, &tail);
        Py_END_ALLOW_THREADS
        if (rc != SQLITE_OK) {
            _pysqlite_seterror(self->db);
            goto error;
        }

        rc = _sqlite_step_with_busyhandler(statement, self);
        if (rc == SQLITE_DONE) {
            self->inTransaction = 0;
        } else {
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
        Py_INCREF(Py_None);
        return Py_None;
    }
}

PyObject* pysqlite_connection_rollback(pysqlite_Connection* self, PyObject* args)
{
    int rc;
    const char* tail;
    sqlite3_stmt* statement;

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        return NULL;
    }

    if (self->inTransaction) {
        pysqlite_reset_all_statements(self);

        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_prepare(self->db, "ROLLBACK", -1, &statement, &tail);
        Py_END_ALLOW_THREADS
        if (rc != SQLITE_OK) {
            _pysqlite_seterror(self->db);
            goto error;
        }

        rc = _sqlite_step_with_busyhandler(statement, self);
        if (rc == SQLITE_DONE) {
            self->inTransaction = 0;
        } else {
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
        Py_INCREF(Py_None);
        return Py_None;
    }
}

void _pysqlite_set_result(sqlite3_context* context, PyObject* py_val)
{
    long longval;
    const char* buffer;
    Py_ssize_t buflen;

    if ((!py_val) || PyErr_Occurred()) {
        sqlite3_result_null(context);
    } else if (py_val == Py_None) {
        sqlite3_result_null(context);
    } else if (PyInt_Check(py_val)) {
        longval = PyInt_AsLong(py_val);
        sqlite3_result_int64(context, (PY_LONG_LONG)longval);
    } else if (PyFloat_Check(py_val)) {
        sqlite3_result_double(context, PyFloat_AsDouble(py_val));
    } else if (PyString_Check(py_val)) {
        sqlite3_result_text(context, PyString_AsString(py_val), -1, SQLITE_TRANSIENT);
    } else if (PyUnicode_Check(py_val)) {
        sqlite3_result_text(context, PyUnicode_AsString(py_val), -1, SQLITE_TRANSIENT);
    } else if (PyObject_CheckBuffer(py_val)) {
        if (PyObject_AsCharBuffer(py_val, &buffer, &buflen) != 0) {
            PyErr_SetString(PyExc_ValueError, "could not convert BLOB to buffer");
        } else {
            sqlite3_result_blob(context, buffer, buflen, SQLITE_TRANSIENT);
        }
    } else {
        /* TODO: raise error */
    }
}

PyObject* _pysqlite_build_py_params(sqlite3_context *context, int argc, sqlite3_value** argv)
{
    PyObject* args;
    int i;
    sqlite3_value* cur_value;
    PyObject* cur_py_value;
    const char* val_str;
    PY_LONG_LONG val_int;
    Py_ssize_t buflen;

    args = PyTuple_New(argc);
    if (!args) {
        return NULL;
    }

    for (i = 0; i < argc; i++) {
        cur_value = argv[i];
        switch (sqlite3_value_type(argv[i])) {
            case SQLITE_INTEGER:
                val_int = sqlite3_value_int64(cur_value);
                cur_py_value = PyInt_FromLong((long)val_int);
                break;
            case SQLITE_FLOAT:
                cur_py_value = PyFloat_FromDouble(sqlite3_value_double(cur_value));
                break;
            case SQLITE_TEXT:
                val_str = (const char*)sqlite3_value_text(cur_value);
                cur_py_value = PyUnicode_DecodeUTF8(val_str, strlen(val_str), NULL);
                /* TODO: have a way to show errors here */
                if (!cur_py_value) {
                    PyErr_Clear();
                    Py_INCREF(Py_None);
                    cur_py_value = Py_None;
                }
                break;
            case SQLITE_BLOB:
                buflen = sqlite3_value_bytes(cur_value);
                cur_py_value = PyBytes_FromStringAndSize(
                    sqlite3_value_blob(cur_value), buflen);
                break;
            case SQLITE_NULL:
            default:
                Py_INCREF(Py_None);
                cur_py_value = Py_None;
        }

        if (!cur_py_value) {
            Py_DECREF(args);
            return NULL;
        }

        PyTuple_SetItem(args, i, cur_py_value);

    }

    return args;
}

void _pysqlite_func_callback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    PyObject* args;
    PyObject* py_func;
    PyObject* py_retval = NULL;

    PyGILState_STATE threadstate;

    threadstate = PyGILState_Ensure();

    py_func = (PyObject*)sqlite3_user_data(context);

    args = _pysqlite_build_py_params(context, argc, argv);
    if (args) {
        py_retval = PyObject_CallObject(py_func, args);
        Py_DECREF(args);
    }

    if (py_retval) {
        _pysqlite_set_result(context, py_retval);
        Py_DECREF(py_retval);
    } else {
        if (_enable_callback_tracebacks) {
            PyErr_Print();
        } else {
            PyErr_Clear();
        }
        _sqlite3_result_error(context, "user-defined function raised exception", -1);
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

    if (*aggregate_instance == 0) {
        *aggregate_instance = PyObject_CallFunction(aggregate_class, "");

        if (PyErr_Occurred()) {
            *aggregate_instance = 0;
            if (_enable_callback_tracebacks) {
                PyErr_Print();
            } else {
                PyErr_Clear();
            }
            _sqlite3_result_error(context, "user-defined aggregate's '__init__' method raised error", -1);
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
        if (_enable_callback_tracebacks) {
            PyErr_Print();
        } else {
            PyErr_Clear();
        }
        _sqlite3_result_error(context, "user-defined aggregate's 'step' method raised error", -1);
    }

error:
    Py_XDECREF(stepmethod);
    Py_XDECREF(function_result);

    PyGILState_Release(threadstate);
}

void _pysqlite_final_callback(sqlite3_context* context)
{
    PyObject* function_result = NULL;
    PyObject** aggregate_instance;
    PyObject* aggregate_class;

    PyGILState_STATE threadstate;

    threadstate = PyGILState_Ensure();

    aggregate_class = (PyObject*)sqlite3_user_data(context);

    aggregate_instance = (PyObject**)sqlite3_aggregate_context(context, sizeof(PyObject*));
    if (!*aggregate_instance) {
        /* this branch is executed if there was an exception in the aggregate's
         * __init__ */

        goto error;
    }

    function_result = PyObject_CallMethod(*aggregate_instance, "finalize", "");
    if (!function_result) {
        if (_enable_callback_tracebacks) {
            PyErr_Print();
        } else {
            PyErr_Clear();
        }
        _sqlite3_result_error(context, "user-defined aggregate's 'finalize' method raised error", -1);
    } else {
        _pysqlite_set_result(context, function_result);
    }

error:
    Py_XDECREF(*aggregate_instance);
    Py_XDECREF(function_result);

    PyGILState_Release(threadstate);
}

void _pysqlite_drop_unused_statement_references(pysqlite_Connection* self)
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

    Py_DECREF(self->statements);
    self->statements = new_list;
}

PyObject* pysqlite_connection_create_function(pysqlite_Connection* self, PyObject* args, PyObject* kwargs)
{
    static char *kwlist[] = {"name", "narg", "func", NULL, NULL};

    PyObject* func;
    char* name;
    int narg;
    int rc;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "siO", kwlist,
                                     &name, &narg, &func))
    {
        return NULL;
    }

    rc = sqlite3_create_function(self->db, name, narg, SQLITE_UTF8, (void*)func, _pysqlite_func_callback, NULL, NULL);

    if (rc != SQLITE_OK) {
        /* Workaround for SQLite bug: no error code or string is available here */
        PyErr_SetString(pysqlite_OperationalError, "Error creating function");
        return NULL;
    } else {
        PyDict_SetItem(self->function_pinboard, func, Py_None);

        Py_INCREF(Py_None);
        return Py_None;
    }
}

PyObject* pysqlite_connection_create_aggregate(pysqlite_Connection* self, PyObject* args, PyObject* kwargs)
{
    PyObject* aggregate_class;

    int n_arg;
    char* name;
    static char *kwlist[] = { "name", "n_arg", "aggregate_class", NULL };
    int rc;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "siO:create_aggregate",
                                      kwlist, &name, &n_arg, &aggregate_class)) {
        return NULL;
    }

    rc = sqlite3_create_function(self->db, name, n_arg, SQLITE_UTF8, (void*)aggregate_class, 0, &_pysqlite_step_callback, &_pysqlite_final_callback);
    if (rc != SQLITE_OK) {
        /* Workaround for SQLite bug: no error code or string is available here */
        PyErr_SetString(pysqlite_OperationalError, "Error creating aggregate");
        return NULL;
    } else {
        PyDict_SetItem(self->function_pinboard, aggregate_class, Py_None);

        Py_INCREF(Py_None);
        return Py_None;
    }
}

static int _authorizer_callback(void* user_arg, int action, const char* arg1, const char* arg2 , const char* dbname, const char* access_attempt_source)
{
    PyObject *ret;
    int rc;
    PyGILState_STATE gilstate;

    gilstate = PyGILState_Ensure();
    ret = PyObject_CallFunction((PyObject*)user_arg, "issss", action, arg1, arg2, dbname, access_attempt_source);

    if (!ret) {
        if (_enable_callback_tracebacks) {
            PyErr_Print();
        } else {
            PyErr_Clear();
        }

        rc = SQLITE_DENY;
    } else {
        if (PyInt_Check(ret)) {
            rc = (int)PyInt_AsLong(ret);
        } else {
            rc = SQLITE_DENY;
        }
        Py_DECREF(ret);
    }

    PyGILState_Release(gilstate);
    return rc;
}

PyObject* pysqlite_connection_set_authorizer(pysqlite_Connection* self, PyObject* args, PyObject* kwargs)
{
    PyObject* authorizer_cb;

    static char *kwlist[] = { "authorizer_callback", NULL };
    int rc;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:set_authorizer",
                                      kwlist, &authorizer_cb)) {
        return NULL;
    }

    rc = sqlite3_set_authorizer(self->db, _authorizer_callback, (void*)authorizer_cb);

    if (rc != SQLITE_OK) {
        PyErr_SetString(pysqlite_OperationalError, "Error setting authorizer callback");
        return NULL;
    } else {
        PyDict_SetItem(self->function_pinboard, authorizer_cb, Py_None);

        Py_INCREF(Py_None);
        return Py_None;
    }
}

int pysqlite_check_thread(pysqlite_Connection* self)
{
    if (self->check_same_thread) {
        if (PyThread_get_thread_ident() != self->thread_ident) {
            PyErr_Format(pysqlite_ProgrammingError,
                        "SQLite objects created in a thread can only be used in that same thread."
                        "The object was created in thread id %ld and this is thread id %ld",
                        self->thread_ident, PyThread_get_thread_ident());
            return 0;
        }

    }

    return 1;
}

static PyObject* pysqlite_connection_get_isolation_level(pysqlite_Connection* self, void* unused)
{
    Py_INCREF(self->isolation_level);
    return self->isolation_level;
}

static PyObject* pysqlite_connection_get_total_changes(pysqlite_Connection* self, void* unused)
{
    if (!pysqlite_check_connection(self)) {
        return NULL;
    } else {
        return Py_BuildValue("i", sqlite3_total_changes(self->db));
    }
}

static int pysqlite_connection_set_isolation_level(pysqlite_Connection* self, PyObject* isolation_level)
{
    PyObject* res;
    PyObject* begin_statement;

    Py_XDECREF(self->isolation_level);

    if (self->begin_statement) {
        PyMem_Free(self->begin_statement);
        self->begin_statement = NULL;
    }

    if (isolation_level == Py_None) {
        Py_INCREF(Py_None);
        self->isolation_level = Py_None;

        res = pysqlite_connection_commit(self, NULL);
        if (!res) {
            return -1;
        }
        Py_DECREF(res);

        self->inTransaction = 0;
    } else {
        const char *statement;
        Py_ssize_t size;

        Py_INCREF(isolation_level);
        self->isolation_level = isolation_level;

        begin_statement = PyUnicode_FromString("BEGIN ");
        if (!begin_statement) {
            return -1;
        }
        PyUnicode_Concat(begin_statement, isolation_level);
        if (!begin_statement) {
            return -1;
        }

        statement = PyUnicode_AsStringAndSize(begin_statement, &size);
        self->begin_statement = PyMem_Malloc(size + 2);
        if (!self->begin_statement) {
            return -1;
        }

        strcpy(self->begin_statement, statement);
        Py_DECREF(begin_statement);
    }

    return 0;
}

PyObject* pysqlite_connection_call(pysqlite_Connection* self, PyObject* args, PyObject* kwargs)
{
    PyObject* sql;
    pysqlite_Statement* statement;
    PyObject* weakref;
    int rc;

    if (!PyArg_ParseTuple(args, "O", &sql)) {
        return NULL;
    }

    _pysqlite_drop_unused_statement_references(self);

    statement = PyObject_New(pysqlite_Statement, &pysqlite_StatementType);
    if (!statement) {
        return NULL;
    }

    rc = pysqlite_statement_create(statement, self, sql);

    if (rc != SQLITE_OK) {
        if (rc == PYSQLITE_TOO_MUCH_SQL) {
            PyErr_SetString(pysqlite_Warning, "You can only execute one statement at a time.");
        } else if (rc == PYSQLITE_SQL_WRONG_TYPE) {
            PyErr_SetString(pysqlite_Warning, "SQL is of wrong type. Must be string or unicode.");
        } else {
            _pysqlite_seterror(self->db);
        }

        Py_DECREF(statement);
        statement = 0;
    } else {
        weakref = PyWeakref_NewRef((PyObject*)statement, NULL);
        if (!weakref) {
            Py_DECREF(statement);
            statement = 0;
            goto error;
        }

        if (PyList_Append(self->statements, weakref) != 0) {
            Py_DECREF(weakref);
            statement = 0;
            goto error;
        }

        Py_DECREF(weakref);
    }

error:
    return (PyObject*)statement;
}

PyObject* pysqlite_connection_execute(pysqlite_Connection* self, PyObject* args, PyObject* kwargs)
{
    PyObject* cursor = 0;
    PyObject* result = 0;
    PyObject* method = 0;

    cursor = PyObject_CallMethod((PyObject*)self, "cursor", "");
    if (!cursor) {
        goto error;
    }

    method = PyObject_GetAttrString(cursor, "execute");
    if (!method) {
        Py_DECREF(cursor);
        cursor = 0;
        goto error;
    }

    result = PyObject_CallObject(method, args);
    if (!result) {
        Py_DECREF(cursor);
        cursor = 0;
    }

error:
    Py_XDECREF(result);
    Py_XDECREF(method);

    return cursor;
}

PyObject* pysqlite_connection_executemany(pysqlite_Connection* self, PyObject* args, PyObject* kwargs)
{
    PyObject* cursor = 0;
    PyObject* result = 0;
    PyObject* method = 0;

    cursor = PyObject_CallMethod((PyObject*)self, "cursor", "");
    if (!cursor) {
        goto error;
    }

    method = PyObject_GetAttrString(cursor, "executemany");
    if (!method) {
        Py_DECREF(cursor);
        cursor = 0;
        goto error;
    }

    result = PyObject_CallObject(method, args);
    if (!result) {
        Py_DECREF(cursor);
        cursor = 0;
    }

error:
    Py_XDECREF(result);
    Py_XDECREF(method);

    return cursor;
}

PyObject* pysqlite_connection_executescript(pysqlite_Connection* self, PyObject* args, PyObject* kwargs)
{
    PyObject* cursor = 0;
    PyObject* result = 0;
    PyObject* method = 0;

    cursor = PyObject_CallMethod((PyObject*)self, "cursor", "");
    if (!cursor) {
        goto error;
    }

    method = PyObject_GetAttrString(cursor, "executescript");
    if (!method) {
        Py_DECREF(cursor);
        cursor = 0;
        goto error;
    }

    result = PyObject_CallObject(method, args);
    if (!result) {
        Py_DECREF(cursor);
        cursor = 0;
    }

error:
    Py_XDECREF(result);
    Py_XDECREF(method);

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
    int result = 0;

    gilstate = PyGILState_Ensure();

    if (PyErr_Occurred()) {
        goto finally;
    }

    string1 = PyString_FromStringAndSize((const char*)text1_data, text1_length);
    string2 = PyString_FromStringAndSize((const char*)text2_data, text2_length);

    if (!string1 || !string2) {
        goto finally; /* failed to allocate strings */
    }

    retval = PyObject_CallFunctionObjArgs(callback, string1, string2, NULL);

    if (!retval) {
        /* execution failed */
        goto finally;
    }

    result = PyInt_AsLong(retval);
    if (PyErr_Occurred()) {
        result = 0;
    }

finally:
    Py_XDECREF(string1);
    Py_XDECREF(string2);
    Py_XDECREF(retval);

    PyGILState_Release(gilstate);

    return result;
}

static PyObject *
pysqlite_connection_interrupt(pysqlite_Connection* self, PyObject* args)
{
    PyObject* retval = NULL;

    if (!pysqlite_check_connection(self)) {
        goto finally;
    }

    sqlite3_interrupt(self->db);

    Py_INCREF(Py_None);
    retval = Py_None;

finally:
    return retval;
}

static PyObject *
pysqlite_connection_create_collation(pysqlite_Connection* self, PyObject* args)
{
    PyObject* callable;
    PyObject* uppercase_name = 0;
    PyObject* name;
    PyObject* retval;
    char* chk;
    int rc;

    if (!pysqlite_check_thread(self) || !pysqlite_check_connection(self)) {
        goto finally;
    }

    if (!PyArg_ParseTuple(args, "O!O:create_collation(name, callback)", &PyUnicode_Type, &name, &callable)) {
        goto finally;
    }

    uppercase_name = PyObject_CallMethod(name, "upper", "");
    if (!uppercase_name) {
        goto finally;
    }

    chk = PyString_AsString(uppercase_name);
    while (*chk) {
        if ((*chk >= '0' && *chk <= '9')
         || (*chk >= 'A' && *chk <= 'Z')
         || (*chk == '_'))
        {
            chk++;
        } else {
            PyErr_SetString(pysqlite_ProgrammingError, "invalid character in collation name");
            goto finally;
        }
    }

    if (callable != Py_None && !PyCallable_Check(callable)) {
        PyErr_SetString(PyExc_TypeError, "parameter must be callable");
        goto finally;
    }

    if (callable != Py_None) {
        PyDict_SetItem(self->collations, uppercase_name, callable);
    } else {
        PyDict_DelItem(self->collations, uppercase_name);
    }

    rc = sqlite3_create_collation(self->db,
                                  PyString_AsString(uppercase_name),
                                  SQLITE_UTF8,
                                  (callable != Py_None) ? callable : NULL,
                                  (callable != Py_None) ? pysqlite_collation_callback : NULL);
    if (rc != SQLITE_OK) {
        PyDict_DelItem(self->collations, uppercase_name);
        _pysqlite_seterror(self->db);
        goto finally;
    }

finally:
    Py_XDECREF(uppercase_name);

    if (PyErr_Occurred()) {
        retval = NULL;
    } else {
        Py_INCREF(Py_None);
        retval = Py_None;
    }

    return retval;
}

static char connection_doc[] =
PyDoc_STR("SQLite database connection object.");

static PyGetSetDef connection_getset[] = {
    {"isolation_level",  (getter)pysqlite_connection_get_isolation_level, (setter)pysqlite_connection_set_isolation_level},
    {"total_changes",  (getter)pysqlite_connection_get_total_changes, (setter)0},
    {NULL}
};

static PyMethodDef connection_methods[] = {
    {"cursor", (PyCFunction)pysqlite_connection_cursor, METH_VARARGS|METH_KEYWORDS,
        PyDoc_STR("Return a cursor for the connection.")},
    {"close", (PyCFunction)pysqlite_connection_close, METH_NOARGS,
        PyDoc_STR("Closes the connection.")},
    {"commit", (PyCFunction)pysqlite_connection_commit, METH_NOARGS,
        PyDoc_STR("Commit the current transaction.")},
    {"rollback", (PyCFunction)pysqlite_connection_rollback, METH_NOARGS,
        PyDoc_STR("Roll back the current transaction.")},
    {"create_function", (PyCFunction)pysqlite_connection_create_function, METH_VARARGS|METH_KEYWORDS,
        PyDoc_STR("Creates a new function. Non-standard.")},
    {"create_aggregate", (PyCFunction)pysqlite_connection_create_aggregate, METH_VARARGS|METH_KEYWORDS,
        PyDoc_STR("Creates a new aggregate. Non-standard.")},
    {"set_authorizer", (PyCFunction)pysqlite_connection_set_authorizer, METH_VARARGS|METH_KEYWORDS,
        PyDoc_STR("Sets authorizer callback. Non-standard.")},
    {"execute", (PyCFunction)pysqlite_connection_execute, METH_VARARGS,
        PyDoc_STR("Executes a SQL statement. Non-standard.")},
    {"executemany", (PyCFunction)pysqlite_connection_executemany, METH_VARARGS,
        PyDoc_STR("Repeatedly executes a SQL statement. Non-standard.")},
    {"executescript", (PyCFunction)pysqlite_connection_executescript, METH_VARARGS,
        PyDoc_STR("Executes a multiple SQL statements at once. Non-standard.")},
    {"create_collation", (PyCFunction)pysqlite_connection_create_collation, METH_VARARGS,
        PyDoc_STR("Creates a collation function. Non-standard.")},
    {"interrupt", (PyCFunction)pysqlite_connection_interrupt, METH_NOARGS,
        PyDoc_STR("Abort any pending database operation. Non-standard.")},
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

PyTypeObject pysqlite_ConnectionType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        MODULE_NAME ".Connection",                      /* tp_name */
        sizeof(pysqlite_Connection),                    /* tp_basicsize */
        0,                                              /* tp_itemsize */
        (destructor)pysqlite_connection_dealloc,        /* tp_dealloc */
        0,                                              /* tp_print */
        0,                                              /* tp_getattr */
        0,                                              /* tp_setattr */
        0,                                              /* tp_compare */
        0,                                              /* tp_repr */
        0,                                              /* tp_as_number */
        0,                                              /* tp_as_sequence */
        0,                                              /* tp_as_mapping */
        0,                                              /* tp_hash */
        (ternaryfunc)pysqlite_connection_call,          /* tp_call */
        0,                                              /* tp_str */
        0,                                              /* tp_getattro */
        0,                                              /* tp_setattro */
        0,                                              /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,         /* tp_flags */
        connection_doc,                                 /* tp_doc */
        0,                                              /* tp_traverse */
        0,                                              /* tp_clear */
        0,                                              /* tp_richcompare */
        0,                                              /* tp_weaklistoffset */
        0,                                              /* tp_iter */
        0,                                              /* tp_iternext */
        connection_methods,                             /* tp_methods */
        connection_members,                             /* tp_members */
        connection_getset,                              /* tp_getset */
        0,                                              /* tp_base */
        0,                                              /* tp_dict */
        0,                                              /* tp_descr_get */
        0,                                              /* tp_descr_set */
        0,                                              /* tp_dictoffset */
        (initproc)pysqlite_connection_init,             /* tp_init */
        0,                                              /* tp_alloc */
        0,                                              /* tp_new */
        0                                               /* tp_free */
};

extern int pysqlite_connection_setup_types(void)
{
    pysqlite_ConnectionType.tp_new = PyType_GenericNew;
    return PyType_Ready(&pysqlite_ConnectionType);
}
