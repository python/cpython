/* cursor.c - the cursor type
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

#include "cursor.h"
#include "module.h"
#include "util.h"
#include "clinic/cursor.c.h"

/*[clinic input]
module _sqlite3
class _sqlite3.Cursor "pysqlite_Cursor *" "pysqlite_CursorType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=b2072d8db95411d5]*/

static const char errmsg_fetch_across_rollback[] = "Cursor needed to be reset because of commit/rollback and can no longer be fetched from.";

/*[clinic input]
_sqlite3.Cursor.__init__ as pysqlite_cursor_init

    connection: object(type='pysqlite_Connection *', subclass_of='pysqlite_ConnectionType')
    /

[clinic start generated code]*/

static int
pysqlite_cursor_init_impl(pysqlite_Cursor *self,
                          pysqlite_Connection *connection)
/*[clinic end generated code: output=ac59dce49a809ca8 input=a8a4f75ac90999b2]*/
{
    Py_INCREF(connection);
    Py_XSETREF(self->connection, connection);
    Py_CLEAR(self->statement);
    Py_CLEAR(self->next_row);
    Py_CLEAR(self->row_cast_map);

    Py_INCREF(Py_None);
    Py_XSETREF(self->description, Py_None);

    Py_INCREF(Py_None);
    Py_XSETREF(self->lastrowid, Py_None);

    self->arraysize = 1;
    self->closed = 0;
    self->reset = 0;

    self->rowcount = -1L;

    Py_INCREF(Py_None);
    Py_XSETREF(self->row_factory, Py_None);

    if (!pysqlite_check_thread(self->connection)) {
        return -1;
    }

    if (!pysqlite_connection_register_cursor(connection, (PyObject*)self)) {
        return -1;
    }

    self->initialized = 1;

    return 0;
}

static void pysqlite_cursor_dealloc(pysqlite_Cursor* self)
{
    PyTypeObject *tp = Py_TYPE(self);

    /* Reset the statement if the user has not closed the cursor */
    if (self->statement) {
        pysqlite_statement_reset(self->statement);
        Py_DECREF(self->statement);
    }

    Py_XDECREF(self->connection);
    Py_XDECREF(self->row_cast_map);
    Py_XDECREF(self->description);
    Py_XDECREF(self->lastrowid);
    Py_XDECREF(self->row_factory);
    Py_XDECREF(self->next_row);

    if (self->in_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject*)self);
    }

    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyObject *
_pysqlite_get_converter(const char *keystr, Py_ssize_t keylen)
{
    PyObject *key;
    PyObject *upcase_key;
    PyObject *retval;
    _Py_IDENTIFIER(upper);

    key = PyUnicode_FromStringAndSize(keystr, keylen);
    if (!key) {
        return NULL;
    }
    upcase_key = _PyObject_CallMethodIdNoArgs(key, &PyId_upper);
    Py_DECREF(key);
    if (!upcase_key) {
        return NULL;
    }

    retval = PyDict_GetItemWithError(_pysqlite_converters, upcase_key);
    Py_DECREF(upcase_key);

    return retval;
}

static int
pysqlite_build_row_cast_map(pysqlite_Cursor* self)
{
    int i;
    const char* pos;
    const char* decltype;
    PyObject* converter;

    if (!self->connection->detect_types) {
        return 0;
    }

    Py_XSETREF(self->row_cast_map, PyList_New(0));
    if (!self->row_cast_map) {
        return -1;
    }

    for (i = 0; i < sqlite3_column_count(self->statement->st); i++) {
        converter = NULL;

        if (self->connection->detect_types & PARSE_COLNAMES) {
            const char *colname = sqlite3_column_name(self->statement->st, i);
            if (colname == NULL) {
                PyErr_NoMemory();
                Py_CLEAR(self->row_cast_map);
                return -1;
            }
            const char *type_start = NULL;
            for (pos = colname; *pos != 0; pos++) {
                if (*pos == '[') {
                    type_start = pos + 1;
                }
                else if (*pos == ']' && type_start != NULL) {
                    converter = _pysqlite_get_converter(type_start, pos - type_start);
                    if (!converter && PyErr_Occurred()) {
                        Py_CLEAR(self->row_cast_map);
                        return -1;
                    }
                    break;
                }
            }
        }

        if (!converter && self->connection->detect_types & PARSE_DECLTYPES) {
            decltype = sqlite3_column_decltype(self->statement->st, i);
            if (decltype) {
                for (pos = decltype;;pos++) {
                    /* Converter names are split at '(' and blanks.
                     * This allows 'INTEGER NOT NULL' to be treated as 'INTEGER' and
                     * 'NUMBER(10)' to be treated as 'NUMBER', for example.
                     * In other words, it will work as people expect it to work.*/
                    if (*pos == ' ' || *pos == '(' || *pos == 0) {
                        converter = _pysqlite_get_converter(decltype, pos - decltype);
                        if (!converter && PyErr_Occurred()) {
                            Py_CLEAR(self->row_cast_map);
                            return -1;
                        }
                        break;
                    }
                }
            }
        }

        if (!converter) {
            converter = Py_None;
        }

        if (PyList_Append(self->row_cast_map, converter) != 0) {
            Py_CLEAR(self->row_cast_map);
            return -1;
        }
    }

    return 0;
}

static PyObject *
_pysqlite_build_column_name(pysqlite_Cursor *self, const char *colname)
{
    const char* pos;
    Py_ssize_t len;

    if (self->connection->detect_types & PARSE_COLNAMES) {
        for (pos = colname; *pos; pos++) {
            if (*pos == '[') {
                if ((pos != colname) && (*(pos-1) == ' ')) {
                    pos--;
                }
                break;
            }
        }
        len = pos - colname;
    }
    else {
        len = strlen(colname);
    }
    return PyUnicode_FromStringAndSize(colname, len);
}

/*
 * Returns a row from the currently active SQLite statement
 *
 * Precondidition:
 * - sqlite3_step() has been called before and it returned SQLITE_ROW.
 */
static PyObject *
_pysqlite_fetch_one_row(pysqlite_Cursor* self)
{
    int i, numcols;
    PyObject* row;
    PyObject* item = NULL;
    int coltype;
    PyObject* converter;
    PyObject* converted;
    Py_ssize_t nbytes;
    char buf[200];
    const char* colname;
    PyObject* error_msg;

    if (self->reset) {
        PyErr_SetString(pysqlite_InterfaceError, errmsg_fetch_across_rollback);
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    numcols = sqlite3_data_count(self->statement->st);
    Py_END_ALLOW_THREADS

    row = PyTuple_New(numcols);
    if (!row)
        return NULL;

    for (i = 0; i < numcols; i++) {
        if (self->connection->detect_types
                && self->row_cast_map != NULL
                && i < PyList_GET_SIZE(self->row_cast_map))
        {
            converter = PyList_GET_ITEM(self->row_cast_map, i);
        }
        else {
            converter = Py_None;
        }

        /*
         * Note, sqlite3_column_bytes() must come after sqlite3_column_blob()
         * or sqlite3_column_text().
         *
         * See https://sqlite.org/c3ref/column_blob.html for details.
         */
        if (converter != Py_None) {
            const char *blob = (const char*)sqlite3_column_blob(self->statement->st, i);
            nbytes = sqlite3_column_bytes(self->statement->st, i);
            if (!blob) {
                converted = Py_NewRef(Py_None);
            } else {
                item = PyBytes_FromStringAndSize(blob, nbytes);
                if (!item)
                    goto error;
                converted = PyObject_CallOneArg(converter, item);
                Py_DECREF(item);
            }
        } else {
            Py_BEGIN_ALLOW_THREADS
            coltype = sqlite3_column_type(self->statement->st, i);
            Py_END_ALLOW_THREADS
            if (coltype == SQLITE_NULL) {
                converted = Py_NewRef(Py_None);
            } else if (coltype == SQLITE_INTEGER) {
                converted = PyLong_FromLongLong(sqlite3_column_int64(self->statement->st, i));
            } else if (coltype == SQLITE_FLOAT) {
                converted = PyFloat_FromDouble(sqlite3_column_double(self->statement->st, i));
            } else if (coltype == SQLITE_TEXT) {
                const char *text = (const char*)sqlite3_column_text(self->statement->st, i);
                nbytes = sqlite3_column_bytes(self->statement->st, i);
                if (self->connection->text_factory == (PyObject*)&PyUnicode_Type) {
                    converted = PyUnicode_FromStringAndSize(text, nbytes);
                    if (!converted && PyErr_ExceptionMatches(PyExc_UnicodeDecodeError)) {
                        PyErr_Clear();
                        colname = sqlite3_column_name(self->statement->st, i);
                        if (colname == NULL) {
                            PyErr_NoMemory();
                            goto error;
                        }
                        PyOS_snprintf(buf, sizeof(buf) - 1, "Could not decode to UTF-8 column '%s' with text '%s'",
                                     colname , text);
                        error_msg = PyUnicode_Decode(buf, strlen(buf), "ascii", "replace");
                        if (!error_msg) {
                            PyErr_SetString(pysqlite_OperationalError, "Could not decode to UTF-8");
                        } else {
                            PyErr_SetObject(pysqlite_OperationalError, error_msg);
                            Py_DECREF(error_msg);
                        }
                    }
                } else if (self->connection->text_factory == (PyObject*)&PyBytes_Type) {
                    converted = PyBytes_FromStringAndSize(text, nbytes);
                } else if (self->connection->text_factory == (PyObject*)&PyByteArray_Type) {
                    converted = PyByteArray_FromStringAndSize(text, nbytes);
                } else {
                    converted = PyObject_CallFunction(self->connection->text_factory, "y#", text, nbytes);
                }
            } else {
                /* coltype == SQLITE_BLOB */
                const char *blob = sqlite3_column_blob(self->statement->st, i);
                nbytes = sqlite3_column_bytes(self->statement->st, i);
                converted = PyBytes_FromStringAndSize(blob, nbytes);
            }
        }

        if (!converted) {
            goto error;
        }
        PyTuple_SetItem(row, i, converted);
    }

    if (PyErr_Occurred())
        goto error;

    return row;

error:
    Py_DECREF(row);
    return NULL;
}

/*
 * Checks if a cursor object is usable.
 *
 * 0 => error; 1 => ok
 */
static int check_cursor(pysqlite_Cursor* cur)
{
    if (!cur->initialized) {
        PyErr_SetString(pysqlite_ProgrammingError, "Base Cursor.__init__ not called.");
        return 0;
    }

    if (cur->closed) {
        PyErr_SetString(pysqlite_ProgrammingError, "Cannot operate on a closed cursor.");
        return 0;
    }

    if (cur->locked) {
        PyErr_SetString(pysqlite_ProgrammingError, "Recursive use of cursors not allowed.");
        return 0;
    }

    return pysqlite_check_thread(cur->connection) && pysqlite_check_connection(cur->connection);
}

static PyObject *
_pysqlite_query_execute(pysqlite_Cursor* self, int multiple, PyObject* operation, PyObject* second_argument)
{
    PyObject* parameters_list = NULL;
    PyObject* parameters_iter = NULL;
    PyObject* parameters = NULL;
    int i;
    int rc;
    PyObject* func_args;
    PyObject* result;
    int numcols;
    PyObject* descriptor;
    PyObject* column_name;
    sqlite_int64 lastrowid;

    if (!check_cursor(self)) {
        goto error;
    }

    self->locked = 1;
    self->reset = 0;

    Py_CLEAR(self->next_row);

    if (multiple) {
        if (PyIter_Check(second_argument)) {
            /* iterator */
            parameters_iter = Py_NewRef(second_argument);
        } else {
            /* sequence */
            parameters_iter = PyObject_GetIter(second_argument);
            if (!parameters_iter) {
                goto error;
            }
        }
    } else {
        parameters_list = PyList_New(0);
        if (!parameters_list) {
            goto error;
        }

        if (second_argument == NULL) {
            second_argument = PyTuple_New(0);
            if (!second_argument) {
                goto error;
            }
        } else {
            Py_INCREF(second_argument);
        }
        if (PyList_Append(parameters_list, second_argument) != 0) {
            Py_DECREF(second_argument);
            goto error;
        }
        Py_DECREF(second_argument);

        parameters_iter = PyObject_GetIter(parameters_list);
        if (!parameters_iter) {
            goto error;
        }
    }

    if (self->statement != NULL) {
        /* There is an active statement */
        pysqlite_statement_reset(self->statement);
    }

    /* reset description and rowcount */
    Py_INCREF(Py_None);
    Py_SETREF(self->description, Py_None);
    self->rowcount = 0L;

    func_args = PyTuple_New(1);
    if (!func_args) {
        goto error;
    }
    if (PyTuple_SetItem(func_args, 0, Py_NewRef(operation)) != 0) {
        goto error;
    }

    if (self->statement) {
        (void)pysqlite_statement_reset(self->statement);
    }

    Py_XSETREF(self->statement,
              (pysqlite_Statement *)pysqlite_cache_get(self->connection->statement_cache, func_args));
    Py_DECREF(func_args);

    if (!self->statement) {
        goto error;
    }

    if (self->statement->in_use) {
        Py_SETREF(self->statement,
                  PyObject_New(pysqlite_Statement, pysqlite_StatementType));
        if (!self->statement) {
            goto error;
        }
        rc = pysqlite_statement_create(self->statement, self->connection, operation);
        if (rc != SQLITE_OK) {
            Py_CLEAR(self->statement);
            goto error;
        }
    }

    pysqlite_statement_reset(self->statement);
    pysqlite_statement_mark_dirty(self->statement);

    /* We start a transaction implicitly before a DML statement.
       SELECT is the only exception. See #9924. */
    if (self->connection->begin_statement && self->statement->is_dml) {
        if (sqlite3_get_autocommit(self->connection->db)) {
            result = _pysqlite_connection_begin(self->connection);
            if (!result) {
                goto error;
            }
            Py_DECREF(result);
        }
    }

    while (1) {
        parameters = PyIter_Next(parameters_iter);
        if (!parameters) {
            break;
        }

        pysqlite_statement_mark_dirty(self->statement);

        pysqlite_statement_bind_parameters(self->statement, parameters);
        if (PyErr_Occurred()) {
            goto error;
        }

        rc = pysqlite_step(self->statement->st, self->connection);
        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            if (PyErr_Occurred()) {
                /* there was an error that occurred in a user-defined callback */
                if (_pysqlite_enable_callback_tracebacks) {
                    PyErr_Print();
                } else {
                    PyErr_Clear();
                }
            }
            (void)pysqlite_statement_reset(self->statement);
            _pysqlite_seterror(self->connection->db, NULL);
            goto error;
        }

        if (pysqlite_build_row_cast_map(self) != 0) {
            _PyErr_FormatFromCause(pysqlite_OperationalError, "Error while building row_cast_map");
            goto error;
        }

        assert(rc == SQLITE_ROW || rc == SQLITE_DONE);
        Py_BEGIN_ALLOW_THREADS
        numcols = sqlite3_column_count(self->statement->st);
        Py_END_ALLOW_THREADS
        if (self->description == Py_None && numcols > 0) {
            Py_SETREF(self->description, PyTuple_New(numcols));
            if (!self->description) {
                goto error;
            }
            for (i = 0; i < numcols; i++) {
                descriptor = PyTuple_New(7);
                if (!descriptor) {
                    goto error;
                }
                const char *colname;
                colname = sqlite3_column_name(self->statement->st, i);
                if (colname == NULL) {
                    PyErr_NoMemory();
                    Py_DECREF(descriptor);
                    goto error;
                }
                column_name = _pysqlite_build_column_name(self, colname);
                if (column_name == NULL) {
                    Py_DECREF(descriptor);
                    goto error;
                }
                PyTuple_SetItem(descriptor, 0, column_name);
                PyTuple_SetItem(descriptor, 1, Py_NewRef(Py_None));
                PyTuple_SetItem(descriptor, 2, Py_NewRef(Py_None));
                PyTuple_SetItem(descriptor, 3, Py_NewRef(Py_None));
                PyTuple_SetItem(descriptor, 4, Py_NewRef(Py_None));
                PyTuple_SetItem(descriptor, 5, Py_NewRef(Py_None));
                PyTuple_SetItem(descriptor, 6, Py_NewRef(Py_None));
                PyTuple_SetItem(self->description, i, descriptor);
            }
        }

        if (self->statement->is_dml) {
            self->rowcount += (long)sqlite3_changes(self->connection->db);
        } else {
            self->rowcount= -1L;
        }

        if (!multiple) {
            Py_BEGIN_ALLOW_THREADS
            lastrowid = sqlite3_last_insert_rowid(self->connection->db);
            Py_END_ALLOW_THREADS
            Py_SETREF(self->lastrowid, PyLong_FromLongLong(lastrowid));
            if (self->lastrowid == NULL) {
                goto error;
            }
        }

        if (rc == SQLITE_ROW) {
            if (multiple) {
                PyErr_SetString(pysqlite_ProgrammingError, "executemany() can only execute DML statements.");
                goto error;
            }

            self->next_row = _pysqlite_fetch_one_row(self);
            if (self->next_row == NULL)
                goto error;
        } else if (rc == SQLITE_DONE && !multiple) {
            pysqlite_statement_reset(self->statement);
            Py_CLEAR(self->statement);
        }

        if (multiple) {
            pysqlite_statement_reset(self->statement);
        }
        Py_XDECREF(parameters);
    }

error:
    Py_XDECREF(parameters);
    Py_XDECREF(parameters_iter);
    Py_XDECREF(parameters_list);

    self->locked = 0;

    if (PyErr_Occurred()) {
        self->rowcount = -1L;
        return NULL;
    } else {
        return Py_NewRef((PyObject *)self);
    }
}

/*[clinic input]
_sqlite3.Cursor.execute as pysqlite_cursor_execute

    sql: unicode
    parameters: object(c_default = 'NULL') = ()
    /

Executes a SQL statement.
[clinic start generated code]*/

static PyObject *
pysqlite_cursor_execute_impl(pysqlite_Cursor *self, PyObject *sql,
                             PyObject *parameters)
/*[clinic end generated code: output=d81b4655c7c0bbad input=91d7bb36f127f597]*/
{
    return _pysqlite_query_execute(self, 0, sql, parameters);
}

/*[clinic input]
_sqlite3.Cursor.executemany as pysqlite_cursor_executemany

    sql: unicode
    seq_of_parameters: object
    /

Repeatedly executes a SQL statement.
[clinic start generated code]*/

static PyObject *
pysqlite_cursor_executemany_impl(pysqlite_Cursor *self, PyObject *sql,
                                 PyObject *seq_of_parameters)
/*[clinic end generated code: output=2c65a3c4733fb5d8 input=440707b7af87fba8]*/
{
    return _pysqlite_query_execute(self, 1, sql, seq_of_parameters);
}

/*[clinic input]
_sqlite3.Cursor.executescript as pysqlite_cursor_executescript

    sql_script as script_obj: object
    /

Executes a multiple SQL statements at once. Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_cursor_executescript(pysqlite_Cursor *self, PyObject *script_obj)
/*[clinic end generated code: output=115a8132b0f200fe input=38c6fa6de570bb9b]*/
{
    _Py_IDENTIFIER(commit);
    const char* script_cstr;
    sqlite3_stmt* statement;
    int rc;
    PyObject* result;

    if (!check_cursor(self)) {
        return NULL;
    }

    self->reset = 0;

    if (PyUnicode_Check(script_obj)) {
        script_cstr = PyUnicode_AsUTF8(script_obj);
        if (!script_cstr) {
            return NULL;
        }
    } else {
        PyErr_SetString(PyExc_ValueError, "script argument must be unicode.");
        return NULL;
    }

    /* commit first */
    result = _PyObject_CallMethodIdNoArgs((PyObject *)self->connection, &PyId_commit);
    if (!result) {
        goto error;
    }
    Py_DECREF(result);

    while (1) {
        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_prepare_v2(self->connection->db,
                                script_cstr,
                                -1,
                                &statement,
                                &script_cstr);
        Py_END_ALLOW_THREADS
        if (rc != SQLITE_OK) {
            _pysqlite_seterror(self->connection->db, NULL);
            goto error;
        }

        /* execute statement, and ignore results of SELECT statements */
        do {
            rc = pysqlite_step(statement, self->connection);
            if (PyErr_Occurred()) {
                (void)sqlite3_finalize(statement);
                goto error;
            }
        } while (rc == SQLITE_ROW);

        if (rc != SQLITE_DONE) {
            (void)sqlite3_finalize(statement);
            _pysqlite_seterror(self->connection->db, NULL);
            goto error;
        }

        rc = sqlite3_finalize(statement);
        if (rc != SQLITE_OK) {
            _pysqlite_seterror(self->connection->db, NULL);
            goto error;
        }

        if (*script_cstr == (char)0) {
            break;
        }
    }

error:
    if (PyErr_Occurred()) {
        return NULL;
    } else {
        return Py_NewRef((PyObject *)self);
    }
}

static PyObject *
pysqlite_cursor_iternext(pysqlite_Cursor *self)
{
    PyObject* next_row_tuple;
    PyObject* next_row;
    int rc;

    if (!check_cursor(self)) {
        return NULL;
    }

    if (self->reset) {
        PyErr_SetString(pysqlite_InterfaceError, errmsg_fetch_across_rollback);
        return NULL;
    }

    if (!self->next_row) {
         if (self->statement) {
            (void)pysqlite_statement_reset(self->statement);
            Py_CLEAR(self->statement);
        }
        return NULL;
    }

    next_row_tuple = self->next_row;
    assert(next_row_tuple != NULL);
    self->next_row = NULL;

    if (self->row_factory != Py_None) {
        next_row = PyObject_CallFunction(self->row_factory, "OO", self, next_row_tuple);
        if (next_row == NULL) {
            self->next_row = next_row_tuple;
            return NULL;
        }
        Py_DECREF(next_row_tuple);
    } else {
        next_row = next_row_tuple;
    }

    if (self->statement) {
        rc = pysqlite_step(self->statement->st, self->connection);
        if (PyErr_Occurred()) {
            (void)pysqlite_statement_reset(self->statement);
            Py_DECREF(next_row);
            return NULL;
        }
        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            (void)pysqlite_statement_reset(self->statement);
            Py_DECREF(next_row);
            _pysqlite_seterror(self->connection->db, NULL);
            return NULL;
        }

        if (rc == SQLITE_ROW) {
            self->next_row = _pysqlite_fetch_one_row(self);
            if (self->next_row == NULL) {
                (void)pysqlite_statement_reset(self->statement);
                return NULL;
            }
        }
    }

    return next_row;
}

/*[clinic input]
_sqlite3.Cursor.fetchone as pysqlite_cursor_fetchone

Fetches one row from the resultset.
[clinic start generated code]*/

static PyObject *
pysqlite_cursor_fetchone_impl(pysqlite_Cursor *self)
/*[clinic end generated code: output=4bd2eabf5baaddb0 input=e78294ec5980fdba]*/
{
    PyObject* row;

    row = pysqlite_cursor_iternext(self);
    if (!row && !PyErr_Occurred()) {
        Py_RETURN_NONE;
    }

    return row;
}

/*[clinic input]
_sqlite3.Cursor.fetchmany as pysqlite_cursor_fetchmany

    size as maxrows: int(c_default='self->arraysize') = 1
        The default value is set by the Cursor.arraysize attribute.

Fetches several rows from the resultset.
[clinic start generated code]*/

static PyObject *
pysqlite_cursor_fetchmany_impl(pysqlite_Cursor *self, int maxrows)
/*[clinic end generated code: output=a8ef31fea64d0906 input=c26e6ca3f34debd0]*/
{
    PyObject* row;
    PyObject* list;
    int counter = 0;

    list = PyList_New(0);
    if (!list) {
        return NULL;
    }

    while ((row = pysqlite_cursor_iternext(self))) {
        if (PyList_Append(list, row) < 0) {
            Py_DECREF(row);
            break;
        }
        Py_DECREF(row);

        if (++counter == maxrows) {
            break;
        }
    }

    if (PyErr_Occurred()) {
        Py_DECREF(list);
        return NULL;
    } else {
        return list;
    }
}

/*[clinic input]
_sqlite3.Cursor.fetchall as pysqlite_cursor_fetchall

Fetches all rows from the resultset.
[clinic start generated code]*/

static PyObject *
pysqlite_cursor_fetchall_impl(pysqlite_Cursor *self)
/*[clinic end generated code: output=d5da12aca2da4b27 input=f5d401086a8df25a]*/
{
    PyObject* row;
    PyObject* list;

    list = PyList_New(0);
    if (!list) {
        return NULL;
    }

    while ((row = pysqlite_cursor_iternext(self))) {
        if (PyList_Append(list, row) < 0) {
            Py_DECREF(row);
            break;
        }
        Py_DECREF(row);
    }

    if (PyErr_Occurred()) {
        Py_DECREF(list);
        return NULL;
    } else {
        return list;
    }
}

/*[clinic input]
_sqlite3.Cursor.setinputsizes as pysqlite_cursor_setinputsizes

    sizes: object
    /

Required by DB-API. Does nothing in pysqlite.
[clinic start generated code]*/

static PyObject *
pysqlite_cursor_setinputsizes(pysqlite_Cursor *self, PyObject *sizes)
/*[clinic end generated code: output=893c817afe9d08ad input=7cffbb168663bc69]*/
{
    Py_RETURN_NONE;
}

/*[clinic input]
_sqlite3.Cursor.setoutputsize as pysqlite_cursor_setoutputsize

    size: object
    column: object = None
    /

Required by DB-API. Does nothing in pysqlite.
[clinic start generated code]*/

static PyObject *
pysqlite_cursor_setoutputsize_impl(pysqlite_Cursor *self, PyObject *size,
                                   PyObject *column)
/*[clinic end generated code: output=018d7e9129d45efe input=077b017da58b9389]*/
{
    Py_RETURN_NONE;
}

/*[clinic input]
_sqlite3.Cursor.close as pysqlite_cursor_close

Closes the cursor.
[clinic start generated code]*/

static PyObject *
pysqlite_cursor_close_impl(pysqlite_Cursor *self)
/*[clinic end generated code: output=b6055e4ec6fe63b6 input=08b36552dbb9a986]*/
{
    if (!self->connection) {
        PyErr_SetString(pysqlite_ProgrammingError,
                        "Base Cursor.__init__ not called.");
        return NULL;
    }
    if (!pysqlite_check_thread(self->connection) || !pysqlite_check_connection(self->connection)) {
        return NULL;
    }

    if (self->statement) {
        (void)pysqlite_statement_reset(self->statement);
        Py_CLEAR(self->statement);
    }

    self->closed = 1;

    Py_RETURN_NONE;
}

static PyMethodDef cursor_methods[] = {
    PYSQLITE_CURSOR_CLOSE_METHODDEF
    PYSQLITE_CURSOR_EXECUTEMANY_METHODDEF
    PYSQLITE_CURSOR_EXECUTESCRIPT_METHODDEF
    PYSQLITE_CURSOR_EXECUTE_METHODDEF
    PYSQLITE_CURSOR_FETCHALL_METHODDEF
    PYSQLITE_CURSOR_FETCHMANY_METHODDEF
    PYSQLITE_CURSOR_FETCHONE_METHODDEF
    PYSQLITE_CURSOR_SETINPUTSIZES_METHODDEF
    PYSQLITE_CURSOR_SETOUTPUTSIZE_METHODDEF
    {NULL, NULL}
};

static struct PyMemberDef cursor_members[] =
{
    {"connection", T_OBJECT, offsetof(pysqlite_Cursor, connection), READONLY},
    {"description", T_OBJECT, offsetof(pysqlite_Cursor, description), READONLY},
    {"arraysize", T_INT, offsetof(pysqlite_Cursor, arraysize), 0},
    {"lastrowid", T_OBJECT, offsetof(pysqlite_Cursor, lastrowid), READONLY},
    {"rowcount", T_LONG, offsetof(pysqlite_Cursor, rowcount), READONLY},
    {"row_factory", T_OBJECT, offsetof(pysqlite_Cursor, row_factory), 0},
    {"__weaklistoffset__", T_PYSSIZET, offsetof(pysqlite_Cursor, in_weakreflist), READONLY},
    {NULL}
};

static const char cursor_doc[] =
PyDoc_STR("SQLite database cursor class.");

static PyType_Slot cursor_slots[] = {
    {Py_tp_dealloc, pysqlite_cursor_dealloc},
    {Py_tp_doc, (void *)cursor_doc},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, pysqlite_cursor_iternext},
    {Py_tp_methods, cursor_methods},
    {Py_tp_members, cursor_members},
    {Py_tp_new, PyType_GenericNew},
    {Py_tp_init, pysqlite_cursor_init},
    {0, NULL},
};

static PyType_Spec cursor_spec = {
    .name = MODULE_NAME ".Cursor",
    .basicsize = sizeof(pysqlite_Cursor),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = cursor_slots,
};

PyTypeObject *pysqlite_CursorType = NULL;

int
pysqlite_cursor_setup_types(PyObject *module)
{
    pysqlite_CursorType = (PyTypeObject *)PyType_FromModuleAndSpec(module, &cursor_spec, NULL);
    if (pysqlite_CursorType == NULL) {
        return -1;
    }
    return 0;
}
