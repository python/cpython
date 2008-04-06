/* cursor.c - the cursor type
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

#include "cursor.h"
#include "module.h"
#include "util.h"
#include "sqlitecompat.h"

/* used to decide wether to call PyInt_FromLong or PyLong_FromLongLong */
#ifndef INT32_MIN
#define INT32_MIN (-2147483647 - 1)
#endif
#ifndef INT32_MAX
#define INT32_MAX 2147483647
#endif

PyObject* cursor_iternext(Cursor *self);

static StatementKind detect_statement_type(char* statement)
{
    char buf[20];
    char* src;
    char* dst;

    src = statement;
    /* skip over whitepace */
    while (*src == '\r' || *src == '\n' || *src == ' ' || *src == '\t') {
        src++;
    }

    if (*src == 0)
        return STATEMENT_INVALID;

    dst = buf;
    *dst = 0;
    while (isalpha(*src) && dst - buf < sizeof(buf) - 2) {
        *dst++ = tolower(*src++);
    }

    *dst = 0;

    if (!strcmp(buf, "select")) {
        return STATEMENT_SELECT;
    } else if (!strcmp(buf, "insert")) {
        return STATEMENT_INSERT;
    } else if (!strcmp(buf, "update")) {
        return STATEMENT_UPDATE;
    } else if (!strcmp(buf, "delete")) {
        return STATEMENT_DELETE;
    } else if (!strcmp(buf, "replace")) {
        return STATEMENT_REPLACE;
    } else {
        return STATEMENT_OTHER;
    }
}

int cursor_init(Cursor* self, PyObject* args, PyObject* kwargs)
{
    Connection* connection;

    if (!PyArg_ParseTuple(args, "O!", &ConnectionType, &connection))
    {
        return -1; 
    }

    Py_INCREF(connection);
    self->connection = connection;
    self->statement = NULL;
    self->next_row = NULL;

    self->row_cast_map = PyList_New(0);
    if (!self->row_cast_map) {
        return -1;
    }

    Py_INCREF(Py_None);
    self->description = Py_None;

    Py_INCREF(Py_None);
    self->lastrowid= Py_None;

    self->arraysize = 1;

    self->rowcount = PyInt_FromLong(-1L);
    if (!self->rowcount) {
        return -1;
    }

    Py_INCREF(Py_None);
    self->row_factory = Py_None;

    if (!check_thread(self->connection)) {
        return -1;
    }

    return 0;
}

void cursor_dealloc(Cursor* self)
{
    int rc;

    /* Reset the statement if the user has not closed the cursor */
    if (self->statement) {
        rc = statement_reset(self->statement);
        Py_DECREF(self->statement);
    }

    Py_XDECREF(self->connection);
    Py_XDECREF(self->row_cast_map);
    Py_XDECREF(self->description);
    Py_XDECREF(self->lastrowid);
    Py_XDECREF(self->rowcount);
    Py_XDECREF(self->row_factory);
    Py_XDECREF(self->next_row);

    self->ob_type->tp_free((PyObject*)self);
}

PyObject* _get_converter(PyObject* key)
{
    PyObject* upcase_key;
    PyObject* retval;

    upcase_key = PyObject_CallMethod(key, "upper", "");
    if (!upcase_key) {
        return NULL;
    }

    retval = PyDict_GetItem(converters, upcase_key);
    Py_DECREF(upcase_key);

    return retval;
}

int build_row_cast_map(Cursor* self)
{
    int i;
    const char* type_start = (const char*)-1;
    const char* pos;

    const char* colname;
    const char* decltype;
    PyObject* py_decltype;
    PyObject* converter;
    PyObject* key;

    if (!self->connection->detect_types) {
        return 0;
    }

    Py_XDECREF(self->row_cast_map);
    self->row_cast_map = PyList_New(0);

    for (i = 0; i < sqlite3_column_count(self->statement->st); i++) {
        converter = NULL;

        if (self->connection->detect_types | PARSE_COLNAMES) {
            colname = sqlite3_column_name(self->statement->st, i);
            if (colname) {
                for (pos = colname; *pos != 0; pos++) {
                    if (*pos == '[') {
                        type_start = pos + 1;
                    } else if (*pos == ']' && type_start != (const char*)-1) {
                        key = PyString_FromStringAndSize(type_start, pos - type_start);
                        if (!key) {
                            /* creating a string failed, but it is too complicated
                             * to propagate the error here, we just assume there is
                             * no converter and proceed */
                            break;
                        }

                        converter = _get_converter(key);
                        Py_DECREF(key);
                        break;
                    }
                }
            }
        }

        if (!converter && self->connection->detect_types | PARSE_DECLTYPES) {
            decltype = sqlite3_column_decltype(self->statement->st, i);
            if (decltype) {
                for (pos = decltype;;pos++) {
                    if (*pos == ' ' || *pos == 0) {
                        py_decltype = PyString_FromStringAndSize(decltype, pos - decltype);
                        if (!py_decltype) {
                            return -1;
                        }
                        break;
                    }
                }

                converter = _get_converter(py_decltype);
                Py_DECREF(py_decltype);
            }
        }

        if (!converter) {
            converter = Py_None;
        }

        if (PyList_Append(self->row_cast_map, converter) != 0) {
            if (converter != Py_None) {
                Py_DECREF(converter);
            }
            Py_XDECREF(self->row_cast_map);
            self->row_cast_map = NULL;

            return -1;
        }
    }

    return 0;
}

PyObject* _build_column_name(const char* colname)
{
    const char* pos;

    if (!colname) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    for (pos = colname;; pos++) {
        if (*pos == 0 || *pos == '[') {
            if ((*pos == '[') && (pos > colname) && (*(pos-1) == ' ')) {
                pos--;
            }
            return PyString_FromStringAndSize(colname, pos - colname);
        }
    }
}

PyObject* unicode_from_string(const char* val_str, int optimize)
{
    const char* check;
    int is_ascii = 0;

    if (optimize) {
        is_ascii = 1;

        check = val_str;
        while (*check) {
            if (*check & 0x80) {
                is_ascii = 0;
                break;
            }

            check++;
        }
    }

    if (is_ascii) {
        return PyString_FromString(val_str);
    } else {
        return PyUnicode_DecodeUTF8(val_str, strlen(val_str), NULL);
    }
}

/*
 * Returns a row from the currently active SQLite statement
 *
 * Precondidition:
 * - sqlite3_step() has been called before and it returned SQLITE_ROW.
 */
PyObject* _fetch_one_row(Cursor* self)
{
    int i, numcols;
    PyObject* row;
    PyObject* item = NULL;
    int coltype;
    PY_LONG_LONG intval;
    PyObject* converter;
    PyObject* converted;
    Py_ssize_t nbytes;
    PyObject* buffer;
    void* raw_buffer;
    const char* val_str;
    char buf[200];
    const char* colname;

    Py_BEGIN_ALLOW_THREADS
    numcols = sqlite3_data_count(self->statement->st);
    Py_END_ALLOW_THREADS

    row = PyTuple_New(numcols);
    if (!row) {
        return NULL;
    }

    for (i = 0; i < numcols; i++) {
        if (self->connection->detect_types) {
            converter = PyList_GetItem(self->row_cast_map, i);
            if (!converter) {
                converter = Py_None;
            }
        } else {
            converter = Py_None;
        }

        if (converter != Py_None) {
            nbytes = sqlite3_column_bytes(self->statement->st, i);
            val_str = (const char*)sqlite3_column_blob(self->statement->st, i);
            if (!val_str) {
                Py_INCREF(Py_None);
                converted = Py_None;
            } else {
                item = PyString_FromStringAndSize(val_str, nbytes);
                if (!item) {
                    return NULL;
                }
                converted = PyObject_CallFunction(converter, "O", item);
                Py_DECREF(item);
                if (!converted) {
                    break;
                }
            }
        } else {
            Py_BEGIN_ALLOW_THREADS
            coltype = sqlite3_column_type(self->statement->st, i);
            Py_END_ALLOW_THREADS
            if (coltype == SQLITE_NULL) {
                Py_INCREF(Py_None);
                converted = Py_None;
            } else if (coltype == SQLITE_INTEGER) {
                intval = sqlite3_column_int64(self->statement->st, i);
                if (intval < INT32_MIN || intval > INT32_MAX) {
                    converted = PyLong_FromLongLong(intval);
                } else {
                    converted = PyInt_FromLong((long)intval);
                }
            } else if (coltype == SQLITE_FLOAT) {
                converted = PyFloat_FromDouble(sqlite3_column_double(self->statement->st, i));
            } else if (coltype == SQLITE_TEXT) {
                val_str = (const char*)sqlite3_column_text(self->statement->st, i);
                if ((self->connection->text_factory == (PyObject*)&PyUnicode_Type)
                    || (self->connection->text_factory == OptimizedUnicode)) {

                    converted = unicode_from_string(val_str,
                        self->connection->text_factory == OptimizedUnicode ? 1 : 0);

                    if (!converted) {
                        colname = sqlite3_column_name(self->statement->st, i);
                        if (!colname) {
                            colname = "<unknown column name>";
                        }
                        PyOS_snprintf(buf, sizeof(buf) - 1, "Could not decode to UTF-8 column '%s' with text '%s'",
                                     colname , val_str);
                        PyErr_SetString(OperationalError, buf);
                    }
                } else if (self->connection->text_factory == (PyObject*)&PyString_Type) {
                    converted = PyString_FromString(val_str);
                } else {
                    converted = PyObject_CallFunction(self->connection->text_factory, "s", val_str);
                }
            } else {
                /* coltype == SQLITE_BLOB */
                nbytes = sqlite3_column_bytes(self->statement->st, i);
                buffer = PyBuffer_New(nbytes);
                if (!buffer) {
                    break;
                }
                if (PyObject_AsWriteBuffer(buffer, &raw_buffer, &nbytes)) {
                    break;
                }
                memcpy(raw_buffer, sqlite3_column_blob(self->statement->st, i), nbytes);
                converted = buffer;
            }
        }

        if (converted) {
            PyTuple_SetItem(row, i, converted);
        } else {
            Py_INCREF(Py_None);
            PyTuple_SetItem(row, i, Py_None);
        }
    }

    if (PyErr_Occurred()) {
        Py_DECREF(row);
        row = NULL;
    }

    return row;
}

PyObject* _query_execute(Cursor* self, int multiple, PyObject* args)
{
    PyObject* operation;
    PyObject* operation_bytestr = NULL;
    char* operation_cstr;
    PyObject* parameters_list = NULL;
    PyObject* parameters_iter = NULL;
    PyObject* parameters = NULL;
    int i;
    int rc;
    PyObject* func_args;
    PyObject* result;
    int numcols;
    PY_LONG_LONG lastrowid;
    int statement_type;
    PyObject* descriptor;
    PyObject* second_argument = NULL;
    long rowcount = 0;

    if (!check_thread(self->connection) || !check_connection(self->connection)) {
        return NULL;
    }

    Py_XDECREF(self->next_row);
    self->next_row = NULL;

    if (multiple) {
        /* executemany() */
        if (!PyArg_ParseTuple(args, "OO", &operation, &second_argument)) {
            return NULL; 
        }

        if (!PyString_Check(operation) && !PyUnicode_Check(operation)) {
            PyErr_SetString(PyExc_ValueError, "operation parameter must be str or unicode");
            return NULL;
        }

        if (PyIter_Check(second_argument)) {
            /* iterator */
            Py_INCREF(second_argument);
            parameters_iter = second_argument;
        } else {
            /* sequence */
            parameters_iter = PyObject_GetIter(second_argument);
            if (!parameters_iter) {
                return NULL;
            }
        }
    } else {
        /* execute() */
        if (!PyArg_ParseTuple(args, "O|O", &operation, &second_argument)) {
            return NULL; 
        }

        if (!PyString_Check(operation) && !PyUnicode_Check(operation)) {
            PyErr_SetString(PyExc_ValueError, "operation parameter must be str or unicode");
            return NULL;
        }

        parameters_list = PyList_New(0);
        if (!parameters_list) {
            return NULL;
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
        rc = statement_reset(self->statement);
    }

    if (PyString_Check(operation)) {
        operation_cstr = PyString_AsString(operation);
    } else {
        operation_bytestr = PyUnicode_AsUTF8String(operation);
        if (!operation_bytestr) {
            goto error;
        }

        operation_cstr = PyString_AsString(operation_bytestr);
    }

    /* reset description and rowcount */
    Py_DECREF(self->description);
    Py_INCREF(Py_None);
    self->description = Py_None;

    Py_DECREF(self->rowcount);
    self->rowcount = PyInt_FromLong(-1L);
    if (!self->rowcount) {
        goto error;
    }

    statement_type = detect_statement_type(operation_cstr);
    if (self->connection->begin_statement) {
        switch (statement_type) {
            case STATEMENT_UPDATE:
            case STATEMENT_DELETE:
            case STATEMENT_INSERT:
            case STATEMENT_REPLACE:
                if (!self->connection->inTransaction) {
                    result = _connection_begin(self->connection);
                    if (!result) {
                        goto error;
                    }
                    Py_DECREF(result);
                }
                break;
            case STATEMENT_OTHER:
                /* it's a DDL statement or something similar
                   - we better COMMIT first so it works for all cases */
                if (self->connection->inTransaction) {
                    result = connection_commit(self->connection, NULL);
                    if (!result) {
                        goto error;
                    }
                    Py_DECREF(result);
                }
                break;
            case STATEMENT_SELECT:
                if (multiple) {
                    PyErr_SetString(ProgrammingError,
                                "You cannot execute SELECT statements in executemany().");
                    goto error;
                }
                break;
        }
    }

    func_args = PyTuple_New(1);
    if (!func_args) {
        goto error;
    }
    Py_INCREF(operation);
    if (PyTuple_SetItem(func_args, 0, operation) != 0) {
        goto error;
    }

    if (self->statement) {
        (void)statement_reset(self->statement);
        Py_DECREF(self->statement);
    }

    self->statement = (Statement*)cache_get(self->connection->statement_cache, func_args);
    Py_DECREF(func_args);

    if (!self->statement) {
        goto error;
    }

    if (self->statement->in_use) {
        Py_DECREF(self->statement);
        self->statement = PyObject_New(Statement, &StatementType);
        if (!self->statement) {
            goto error;
        }
        rc = statement_create(self->statement, self->connection, operation);
        if (rc != SQLITE_OK) {
            self->statement = 0;
            goto error;
        }
    }

    statement_reset(self->statement);
    statement_mark_dirty(self->statement);

    while (1) {
        parameters = PyIter_Next(parameters_iter);
        if (!parameters) {
            break;
        }

        statement_mark_dirty(self->statement);

        statement_bind_parameters(self->statement, parameters);
        if (PyErr_Occurred()) {
            goto error;
        }

        if (build_row_cast_map(self) != 0) {
            PyErr_SetString(OperationalError, "Error while building row_cast_map");
            goto error;
        }

        rc = _sqlite_step_with_busyhandler(self->statement->st, self->connection);
        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            rc = statement_reset(self->statement);
            if (rc == SQLITE_SCHEMA) {
                rc = statement_recompile(self->statement, parameters);
                if (rc == SQLITE_OK) {
                    rc = _sqlite_step_with_busyhandler(self->statement->st, self->connection);
                } else {
                    _seterror(self->connection->db);
                    goto error;
                }
            } else {
                if (PyErr_Occurred()) {
                    /* there was an error that occurred in a user-defined callback */
                    if (_enable_callback_tracebacks) {
                        PyErr_Print();
                    } else {
                        PyErr_Clear();
                    }
                }
                _seterror(self->connection->db);
                goto error;
            }
        }

        if (rc == SQLITE_ROW || (rc == SQLITE_DONE && statement_type == STATEMENT_SELECT)) {
            Py_BEGIN_ALLOW_THREADS
            numcols = sqlite3_column_count(self->statement->st);
            Py_END_ALLOW_THREADS

            if (self->description == Py_None) {
                Py_DECREF(self->description);
                self->description = PyTuple_New(numcols);
                if (!self->description) {
                    goto error;
                }
                for (i = 0; i < numcols; i++) {
                    descriptor = PyTuple_New(7);
                    if (!descriptor) {
                        goto error;
                    }
                    PyTuple_SetItem(descriptor, 0, _build_column_name(sqlite3_column_name(self->statement->st, i)));
                    Py_INCREF(Py_None); PyTuple_SetItem(descriptor, 1, Py_None);
                    Py_INCREF(Py_None); PyTuple_SetItem(descriptor, 2, Py_None);
                    Py_INCREF(Py_None); PyTuple_SetItem(descriptor, 3, Py_None);
                    Py_INCREF(Py_None); PyTuple_SetItem(descriptor, 4, Py_None);
                    Py_INCREF(Py_None); PyTuple_SetItem(descriptor, 5, Py_None);
                    Py_INCREF(Py_None); PyTuple_SetItem(descriptor, 6, Py_None);
                    PyTuple_SetItem(self->description, i, descriptor);
                }
            }
        }

        if (rc == SQLITE_ROW) {
            if (multiple) {
                PyErr_SetString(ProgrammingError, "executemany() can only execute DML statements.");
                goto error;
            }

            self->next_row = _fetch_one_row(self);
        } else if (rc == SQLITE_DONE && !multiple) {
            statement_reset(self->statement);
            Py_DECREF(self->statement);
            self->statement = 0;
        }

        switch (statement_type) {
            case STATEMENT_UPDATE:
            case STATEMENT_DELETE:
            case STATEMENT_INSERT:
            case STATEMENT_REPLACE:
                Py_BEGIN_ALLOW_THREADS
                rowcount += (long)sqlite3_changes(self->connection->db);
                Py_END_ALLOW_THREADS
                Py_DECREF(self->rowcount);
                self->rowcount = PyInt_FromLong(rowcount);
        }

        Py_DECREF(self->lastrowid);
        if (statement_type == STATEMENT_INSERT) {
            Py_BEGIN_ALLOW_THREADS
            lastrowid = sqlite3_last_insert_rowid(self->connection->db);
            Py_END_ALLOW_THREADS
            self->lastrowid = PyInt_FromLong((long)lastrowid);
        } else {
            Py_INCREF(Py_None);
            self->lastrowid = Py_None;
        }

        if (multiple) {
            rc = statement_reset(self->statement);
        }
        Py_XDECREF(parameters);
    }

error:
    Py_XDECREF(operation_bytestr);
    Py_XDECREF(parameters);
    Py_XDECREF(parameters_iter);
    Py_XDECREF(parameters_list);

    if (PyErr_Occurred()) {
        return NULL;
    } else {
        Py_INCREF(self);
        return (PyObject*)self;
    }
}

PyObject* cursor_execute(Cursor* self, PyObject* args)
{
    return _query_execute(self, 0, args);
}

PyObject* cursor_executemany(Cursor* self, PyObject* args)
{
    return _query_execute(self, 1, args);
}

PyObject* cursor_executescript(Cursor* self, PyObject* args)
{
    PyObject* script_obj;
    PyObject* script_str = NULL;
    const char* script_cstr;
    sqlite3_stmt* statement;
    int rc;
    PyObject* result;
    int statement_completed = 0;

    if (!PyArg_ParseTuple(args, "O", &script_obj)) {
        return NULL; 
    }

    if (!check_thread(self->connection) || !check_connection(self->connection)) {
        return NULL;
    }

    if (PyString_Check(script_obj)) {
        script_cstr = PyString_AsString(script_obj);
    } else if (PyUnicode_Check(script_obj)) {
        script_str = PyUnicode_AsUTF8String(script_obj);
        if (!script_str) {
            return NULL;
        }

        script_cstr = PyString_AsString(script_str);
    } else {
        PyErr_SetString(PyExc_ValueError, "script argument must be unicode or string.");
        return NULL;
    }

    /* commit first */
    result = connection_commit(self->connection, NULL);
    if (!result) {
        goto error;
    }
    Py_DECREF(result);

    while (1) {
        if (!sqlite3_complete(script_cstr)) {
            break;
        }
        statement_completed = 1;

        rc = sqlite3_prepare(self->connection->db,
                             script_cstr,
                             -1,
                             &statement,
                             &script_cstr);
        if (rc != SQLITE_OK) {
            _seterror(self->connection->db);
            goto error;
        }

        /* execute statement, and ignore results of SELECT statements */
        rc = SQLITE_ROW;
        while (rc == SQLITE_ROW) {
            rc = _sqlite_step_with_busyhandler(statement, self->connection);
        }

        if (rc != SQLITE_DONE) {
            (void)sqlite3_finalize(statement);
            _seterror(self->connection->db);
            goto error;
        }

        rc = sqlite3_finalize(statement);
        if (rc != SQLITE_OK) {
            _seterror(self->connection->db);
            goto error;
        }
    }

error:
    Py_XDECREF(script_str);

    if (!statement_completed) {
        PyErr_SetString(ProgrammingError, "you did not provide a complete SQL statement");
    }

    if (PyErr_Occurred()) {
        return NULL;
    } else {
        Py_INCREF(self);
        return (PyObject*)self;
    }
}

PyObject* cursor_getiter(Cursor *self)
{
    Py_INCREF(self);
    return (PyObject*)self;
}

PyObject* cursor_iternext(Cursor *self)
{
    PyObject* next_row_tuple;
    PyObject* next_row;
    int rc;

    if (!check_thread(self->connection) || !check_connection(self->connection)) {
        return NULL;
    }

    if (!self->next_row) {
         if (self->statement) {
            (void)statement_reset(self->statement);
            Py_DECREF(self->statement);
            self->statement = NULL;
        }
        return NULL;
    }

    next_row_tuple = self->next_row;
    self->next_row = NULL;

    if (self->row_factory != Py_None) {
        next_row = PyObject_CallFunction(self->row_factory, "OO", self, next_row_tuple);
        Py_DECREF(next_row_tuple);
    } else {
        next_row = next_row_tuple;
    }

    if (self->statement) {
        rc = _sqlite_step_with_busyhandler(self->statement->st, self->connection);
        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            Py_DECREF(next_row);
            _seterror(self->connection->db);
            return NULL;
        }

        if (rc == SQLITE_ROW) {
            self->next_row = _fetch_one_row(self);
        }
    }

    return next_row;
}

PyObject* cursor_fetchone(Cursor* self, PyObject* args)
{
    PyObject* row;

    row = cursor_iternext(self);
    if (!row && !PyErr_Occurred()) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    return row;
}

PyObject* cursor_fetchmany(Cursor* self, PyObject* args)
{
    PyObject* row;
    PyObject* list;
    int maxrows = self->arraysize;
    int counter = 0;

    if (!PyArg_ParseTuple(args, "|i", &maxrows)) {
        return NULL; 
    }

    list = PyList_New(0);
    if (!list) {
        return NULL;
    }

    /* just make sure we enter the loop */
    row = Py_None;

    while (row) {
        row = cursor_iternext(self);
        if (row) {
            PyList_Append(list, row);
            Py_DECREF(row);
        } else {
            break;
        }

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

PyObject* cursor_fetchall(Cursor* self, PyObject* args)
{
    PyObject* row;
    PyObject* list;

    list = PyList_New(0);
    if (!list) {
        return NULL;
    }

    /* just make sure we enter the loop */
    row = (PyObject*)Py_None;

    while (row) {
        row = cursor_iternext(self);
        if (row) {
            PyList_Append(list, row);
            Py_DECREF(row);
        }
    }

    if (PyErr_Occurred()) {
        Py_DECREF(list);
        return NULL;
    } else {
        return list;
    }
}

PyObject* pysqlite_noop(Connection* self, PyObject* args)
{
    /* don't care, return None */
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* cursor_close(Cursor* self, PyObject* args)
{
    if (!check_thread(self->connection) || !check_connection(self->connection)) {
        return NULL;
    }

    if (self->statement) {
        (void)statement_reset(self->statement);
        Py_DECREF(self->statement);
        self->statement = 0;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef cursor_methods[] = {
    {"execute", (PyCFunction)cursor_execute, METH_VARARGS,
        PyDoc_STR("Executes a SQL statement.")},
    {"executemany", (PyCFunction)cursor_executemany, METH_VARARGS,
        PyDoc_STR("Repeatedly executes a SQL statement.")},
    {"executescript", (PyCFunction)cursor_executescript, METH_VARARGS,
        PyDoc_STR("Executes a multiple SQL statements at once. Non-standard.")},
    {"fetchone", (PyCFunction)cursor_fetchone, METH_NOARGS,
        PyDoc_STR("Fetches several rows from the resultset.")},
    {"fetchmany", (PyCFunction)cursor_fetchmany, METH_VARARGS,
        PyDoc_STR("Fetches all rows from the resultset.")},
    {"fetchall", (PyCFunction)cursor_fetchall, METH_NOARGS,
        PyDoc_STR("Fetches one row from the resultset.")},
    {"close", (PyCFunction)cursor_close, METH_NOARGS,
        PyDoc_STR("Closes the cursor.")},
    {"setinputsizes", (PyCFunction)pysqlite_noop, METH_VARARGS,
        PyDoc_STR("Required by DB-API. Does nothing in pysqlite.")},
    {"setoutputsize", (PyCFunction)pysqlite_noop, METH_VARARGS,
        PyDoc_STR("Required by DB-API. Does nothing in pysqlite.")},
    {NULL, NULL}
};

static struct PyMemberDef cursor_members[] =
{
    {"connection", T_OBJECT, offsetof(Cursor, connection), RO},
    {"description", T_OBJECT, offsetof(Cursor, description), RO},
    {"arraysize", T_INT, offsetof(Cursor, arraysize), 0},
    {"lastrowid", T_OBJECT, offsetof(Cursor, lastrowid), RO},
    {"rowcount", T_OBJECT, offsetof(Cursor, rowcount), RO},
    {"row_factory", T_OBJECT, offsetof(Cursor, row_factory), 0},
    {NULL}
};

static char cursor_doc[] =
PyDoc_STR("SQLite database cursor class.");

PyTypeObject CursorType = {
        PyObject_HEAD_INIT(NULL)
        0,                                              /* ob_size */
        MODULE_NAME ".Cursor",                          /* tp_name */
        sizeof(Cursor),                                 /* tp_basicsize */
        0,                                              /* tp_itemsize */
        (destructor)cursor_dealloc,                     /* tp_dealloc */
        0,                                              /* tp_print */
        0,                                              /* tp_getattr */
        0,                                              /* tp_setattr */
        0,                                              /* tp_compare */
        0,                                              /* tp_repr */
        0,                                              /* tp_as_number */
        0,                                              /* tp_as_sequence */
        0,                                              /* tp_as_mapping */
        0,                                              /* tp_hash */
        0,                                              /* tp_call */
        0,                                              /* tp_str */
        0,                                              /* tp_getattro */
        0,                                              /* tp_setattro */
        0,                                              /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_ITER|Py_TPFLAGS_BASETYPE, /* tp_flags */
        cursor_doc,                                     /* tp_doc */
        0,                                              /* tp_traverse */
        0,                                              /* tp_clear */
        0,                                              /* tp_richcompare */
        0,                                              /* tp_weaklistoffset */
        (getiterfunc)cursor_getiter,                    /* tp_iter */
        (iternextfunc)cursor_iternext,                  /* tp_iternext */
        cursor_methods,                                 /* tp_methods */
        cursor_members,                                 /* tp_members */
        0,                                              /* tp_getset */
        0,                                              /* tp_base */
        0,                                              /* tp_dict */
        0,                                              /* tp_descr_get */
        0,                                              /* tp_descr_set */
        0,                                              /* tp_dictoffset */
        (initproc)cursor_init,                          /* tp_init */
        0,                                              /* tp_alloc */
        0,                                              /* tp_new */
        0                                               /* tp_free */
};

extern int cursor_setup_types(void)
{
    CursorType.tp_new = PyType_GenericNew;
    return PyType_Ready(&CursorType);
}
