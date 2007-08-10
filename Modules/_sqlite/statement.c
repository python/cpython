/* statement.c - the statement type
 *
 * Copyright (C) 2005-2006 Gerhard Häring <gh@ghaering.de>
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

#include "statement.h"
#include "cursor.h"
#include "connection.h"
#include "microprotocols.h"
#include "prepare_protocol.h"
#include "sqlitecompat.h"

/* prototypes */
static int pysqlite_check_remaining_sql(const char* tail);

typedef enum {
    LINECOMMENT_1,
    IN_LINECOMMENT,
    COMMENTSTART_1,
    IN_COMMENT,
    COMMENTEND_1,
    NORMAL
} parse_remaining_sql_state;

int pysqlite_statement_create(pysqlite_Statement* self, pysqlite_Connection* connection, PyObject* sql)
{
    const char* tail;
    int rc;
    const char* sql_cstr;
    Py_ssize_t sql_cstr_len;

    self->st = NULL;
    self->in_use = 0;

    if (PyObject_AsCharBuffer(sql, &sql_cstr, &sql_cstr_len) < 0) {
        rc = PYSQLITE_SQL_WRONG_TYPE;
        return rc;
    }

    self->in_weakreflist = NULL;
    Py_INCREF(sql);
    self->sql = sql;

    rc = sqlite3_prepare(connection->db,
                         sql_cstr,
                         -1,
                         &self->st,
                         &tail);

    self->db = connection->db;

    if (rc == SQLITE_OK && pysqlite_check_remaining_sql(tail)) {
        (void)sqlite3_finalize(self->st);
        self->st = NULL;
        rc = PYSQLITE_TOO_MUCH_SQL;
    }

    return rc;
}

int pysqlite_statement_bind_parameter(pysqlite_Statement* self, int pos, PyObject* parameter)
{
    int rc = SQLITE_OK;
    long longval;
#ifdef HAVE_LONG_LONG
    PY_LONG_LONG longlongval;
#endif
    const char* buffer;
    char* string;
    Py_ssize_t buflen;
    PyObject* stringval;

    if (parameter == Py_None) {
        rc = sqlite3_bind_null(self->st, pos);
    } else if (PyInt_CheckExact(parameter)) {
        longval = PyInt_AsLong(parameter);
        rc = sqlite3_bind_int64(self->st, pos, (sqlite_int64)longval);
#ifdef HAVE_LONG_LONG
    } else if (PyLong_Check(parameter)) {
        longlongval = PyLong_AsLongLong(parameter);
        /* in the overflow error case, longlongval is -1, and an exception is set */
        rc = sqlite3_bind_int64(self->st, pos, (sqlite_int64)longlongval);
#endif
    } else if (PyFloat_Check(parameter)) {
        rc = sqlite3_bind_double(self->st, pos, PyFloat_AsDouble(parameter));
    } else if (PyBuffer_Check(parameter)) {
        if (PyObject_AsCharBuffer(parameter, &buffer, &buflen) == 0) {
            rc = sqlite3_bind_blob(self->st, pos, buffer, buflen, SQLITE_TRANSIENT);
        } else {
            PyErr_SetString(PyExc_ValueError, "could not convert BLOB to buffer");
            rc = -1;
        }
    } else if PyString_Check(parameter) {
        string = PyString_AsString(parameter);
        rc = sqlite3_bind_text(self->st, pos, string, -1, SQLITE_TRANSIENT);
    } else if PyUnicode_Check(parameter) {
        stringval = PyUnicode_AsUTF8String(parameter);
        string = PyBytes_AsString(stringval);

        rc = sqlite3_bind_text(self->st, pos, string, -1, SQLITE_TRANSIENT);
        Py_DECREF(stringval);
    } else {
        rc = -1;
    }

    return rc;
}

void pysqlite_statement_bind_parameters(pysqlite_Statement* self, PyObject* parameters)
{
    PyObject* current_param;
    PyObject* adapted;
    const char* binding_name;
    int i;
    int rc;
    int num_params_needed;
    int num_params;

    Py_BEGIN_ALLOW_THREADS
    num_params_needed = sqlite3_bind_parameter_count(self->st);
    Py_END_ALLOW_THREADS

    if (PyDict_Check(parameters)) {
        /* parameters passed as dictionary */
        for (i = 1; i <= num_params_needed; i++) {
            Py_BEGIN_ALLOW_THREADS
            binding_name = sqlite3_bind_parameter_name(self->st, i);
            Py_END_ALLOW_THREADS
            if (!binding_name) {
                PyErr_Format(pysqlite_ProgrammingError, "Binding %d has no name, but you supplied a dictionary (which has only names).", i);
                return;
            }

            binding_name++; /* skip first char (the colon) */
            current_param = PyDict_GetItemString(parameters, binding_name);
            if (!current_param) {
                PyErr_Format(pysqlite_ProgrammingError, "You did not supply a value for binding %d.", i);
                return;
            }

            Py_INCREF(current_param);
            adapted = microprotocols_adapt(current_param, (PyObject*)&pysqlite_PrepareProtocolType, NULL);
            if (adapted) {
                Py_DECREF(current_param);
            } else {
                PyErr_Clear();
                adapted = current_param;
            }

            rc = pysqlite_statement_bind_parameter(self, i, adapted);
            Py_DECREF(adapted);

            if (rc != SQLITE_OK) {
                PyErr_Format(pysqlite_InterfaceError, "Error binding parameter :%s - probably unsupported type.", binding_name);
                return;
           }
        }
    } else {
        /* parameters passed as sequence */
        num_params = PySequence_Length(parameters);
        if (num_params != num_params_needed) {
            PyErr_Format(pysqlite_ProgrammingError, "Incorrect number of bindings supplied. The current statement uses %d, and there are %d supplied.",
                         num_params_needed, num_params);
            return;
        }
        for (i = 0; i < num_params; i++) {
            current_param = PySequence_GetItem(parameters, i);
            if (!current_param) {
                return;
            }
            adapted = microprotocols_adapt(current_param, (PyObject*)&pysqlite_PrepareProtocolType, NULL);

            if (adapted) {
                Py_DECREF(current_param);
            } else {
                PyErr_Clear();
                adapted = current_param;
            }

            rc = pysqlite_statement_bind_parameter(self, i + 1, adapted);
            Py_DECREF(adapted);

            if (rc != SQLITE_OK) {
                PyErr_Format(pysqlite_InterfaceError, "Error binding parameter %d - probably unsupported type.", i);
                return;
            }
        }
    }
}

int pysqlite_statement_recompile(pysqlite_Statement* self, PyObject* params)
{
    const char* tail;
    int rc;
    const char* sql_cstr;
    Py_ssize_t sql_len;
    sqlite3_stmt* new_st;

    if (PyObject_AsCharBuffer(self->sql, &sql_cstr, &sql_len) < 0) {
        rc = PYSQLITE_SQL_WRONG_TYPE;
        return rc;
    }

    rc = sqlite3_prepare(self->db,
                         sql_cstr,
                         -1,
                         &new_st,
                         &tail);

    if (rc == SQLITE_OK) {
        /* The efficient sqlite3_transfer_bindings is only available in SQLite
         * version 3.2.2 or later. For older SQLite releases, that might not
         * even define SQLITE_VERSION_NUMBER, we do it the manual way.
         */
        #ifdef SQLITE_VERSION_NUMBER
        #if SQLITE_VERSION_NUMBER >= 3002002
        (void)sqlite3_transfer_bindings(self->st, new_st);
        #endif
        #else
        statement_bind_parameters(self, params);
        #endif

        (void)sqlite3_finalize(self->st);
        self->st = new_st;
    }

    return rc;
}

int pysqlite_statement_finalize(pysqlite_Statement* self)
{
    int rc;

    rc = SQLITE_OK;
    if (self->st) {
        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_finalize(self->st);
        Py_END_ALLOW_THREADS
        self->st = NULL;
    }

    self->in_use = 0;

    return rc;
}

int pysqlite_statement_reset(pysqlite_Statement* self)
{
    int rc;

    rc = SQLITE_OK;

    if (self->in_use && self->st) {
        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_reset(self->st);
        Py_END_ALLOW_THREADS

        if (rc == SQLITE_OK) {
            self->in_use = 0;
        }
    }

    return rc;
}

void pysqlite_statement_mark_dirty(pysqlite_Statement* self)
{
    self->in_use = 1;
}

void pysqlite_statement_dealloc(pysqlite_Statement* self)
{
    int rc;

    if (self->st) {
        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_finalize(self->st);
        Py_END_ALLOW_THREADS
    }

    self->st = NULL;

    Py_XDECREF(self->sql);

    if (self->in_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject*)self);
    }

    Py_Type(self)->tp_free((PyObject*)self);
}

/*
 * Checks if there is anything left in an SQL string after SQLite compiled it.
 * This is used to check if somebody tried to execute more than one SQL command
 * with one execute()/executemany() command, which the DB-API and we don't
 * allow.
 *
 * Returns 1 if there is more left than should be. 0 if ok.
 */
static int pysqlite_check_remaining_sql(const char* tail)
{
    const char* pos = tail;

    parse_remaining_sql_state state = NORMAL;

    for (;;) {
        switch (*pos) {
            case 0:
                return 0;
            case '-':
                if (state == NORMAL) {
                    state  = LINECOMMENT_1;
                } else if (state == LINECOMMENT_1) {
                    state = IN_LINECOMMENT;
                }
                break;
            case ' ':
            case '\t':
                break;
            case '\n':
            case 13:
                if (state == IN_LINECOMMENT) {
                    state = NORMAL;
                }
                break;
            case '/':
                if (state == NORMAL) {
                    state = COMMENTSTART_1;
                } else if (state == COMMENTEND_1) {
                    state = NORMAL;
                } else if (state == COMMENTSTART_1) {
                    return 1;
                }
                break;
            case '*':
                if (state == NORMAL) {
                    return 1;
                } else if (state == LINECOMMENT_1) {
                    return 1;
                } else if (state == COMMENTSTART_1) {
                    state = IN_COMMENT;
                } else if (state == IN_COMMENT) {
                    state = COMMENTEND_1;
                }
                break;
            default:
                if (state == COMMENTEND_1) {
                    state = IN_COMMENT;
                } else if (state == IN_LINECOMMENT) {
                } else if (state == IN_COMMENT) {
                } else {
                    return 1;
                }
        }

        pos++;
    }

    return 0;
}

PyTypeObject pysqlite_StatementType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        MODULE_NAME ".Statement",                       /* tp_name */
        sizeof(pysqlite_Statement),                     /* tp_basicsize */
        0,                                              /* tp_itemsize */
        (destructor)pysqlite_statement_dealloc,         /* tp_dealloc */
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
        Py_TPFLAGS_DEFAULT,				/* tp_flags */
        0,                                              /* tp_doc */
        0,                                              /* tp_traverse */
        0,                                              /* tp_clear */
        0,                                              /* tp_richcompare */
        offsetof(pysqlite_Statement, in_weakreflist),   /* tp_weaklistoffset */
        0,                                              /* tp_iter */
        0,                                              /* tp_iternext */
        0,                                              /* tp_methods */
        0,                                              /* tp_members */
        0,                                              /* tp_getset */
        0,                                              /* tp_base */
        0,                                              /* tp_dict */
        0,                                              /* tp_descr_get */
        0,                                              /* tp_descr_set */
        0,                                              /* tp_dictoffset */
        (initproc)0,                                    /* tp_init */
        0,                                              /* tp_alloc */
        0,                                              /* tp_new */
        0                                               /* tp_free */
};

extern int pysqlite_statement_setup_types(void)
{
    pysqlite_StatementType.tp_new = PyType_GenericNew;
    return PyType_Ready(&pysqlite_StatementType);
}
