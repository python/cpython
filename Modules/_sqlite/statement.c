/* statement.c - the statement type
 *
 * Copyright (C) 2005-2010 Gerhard Häring <gh@ghaering.de>
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
#include "util.h"
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

typedef enum {
    TYPE_LONG,
    TYPE_FLOAT,
    TYPE_UNICODE,
    TYPE_BUFFER,
    TYPE_UNKNOWN
} parameter_type;

int pysqlite_statement_create(pysqlite_Statement* self, pysqlite_Connection* connection, PyObject* sql)
{
    const char* tail;
    int rc;
    const char* sql_cstr;
    Py_ssize_t sql_cstr_len;

    self->st = NULL;
    self->in_use = 0;

    sql_cstr = _PyUnicode_AsStringAndSize(sql, &sql_cstr_len);
    if (sql_cstr == NULL) {
        rc = PYSQLITE_SQL_WRONG_TYPE;
        return rc;
    }

    self->in_weakreflist = NULL;
    Py_INCREF(sql);
    self->sql = sql;

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_prepare(connection->db,
                         sql_cstr,
                         -1,
                         &self->st,
                         &tail);
    Py_END_ALLOW_THREADS

    self->db = connection->db;

    if (rc == SQLITE_OK && pysqlite_check_remaining_sql(tail)) {
        (void)sqlite3_finalize(self->st);
        self->st = NULL;
        rc = PYSQLITE_TOO_MUCH_SQL;
    }

    return rc;
}

int pysqlite_statement_bind_parameter(pysqlite_Statement* self, int pos, PyObject* parameter, int allow_8bit_chars)
{
    int rc = SQLITE_OK;
    const char* buffer;
    char* string;
    Py_ssize_t buflen;
    parameter_type paramtype;

    if (parameter == Py_None) {
        rc = sqlite3_bind_null(self->st, pos);
        goto final;
    }

    if (PyLong_CheckExact(parameter)) {
        paramtype = TYPE_LONG;
    } else if (PyFloat_CheckExact(parameter)) {
        paramtype = TYPE_FLOAT;
    } else if (PyUnicode_CheckExact(parameter)) {
        paramtype = TYPE_UNICODE;
    } else if (PyLong_Check(parameter)) {
        paramtype = TYPE_LONG;
    } else if (PyFloat_Check(parameter)) {
        paramtype = TYPE_FLOAT;
    } else if (PyUnicode_Check(parameter)) {
        paramtype = TYPE_UNICODE;
    } else if (PyObject_CheckBuffer(parameter)) {
        paramtype = TYPE_BUFFER;
    } else {
        paramtype = TYPE_UNKNOWN;
    }

    switch (paramtype) {
        case TYPE_LONG: {
            sqlite_int64 value = _pysqlite_long_as_int64(parameter);
            if (value == -1 && PyErr_Occurred())
                rc = -1;
            else
                rc = sqlite3_bind_int64(self->st, pos, value);
            break;
        }
        case TYPE_FLOAT:
            rc = sqlite3_bind_double(self->st, pos, PyFloat_AsDouble(parameter));
            break;
        case TYPE_UNICODE:
            string = _PyUnicode_AsStringAndSize(parameter, &buflen);
            if (string != NULL)
                rc = sqlite3_bind_text(self->st, pos, string, buflen, SQLITE_TRANSIENT);
            else
                rc = -1;
            break;
        case TYPE_BUFFER:
            if (PyObject_AsCharBuffer(parameter, &buffer, &buflen) == 0) {
                rc = sqlite3_bind_blob(self->st, pos, buffer, buflen, SQLITE_TRANSIENT);
            } else {
                PyErr_SetString(PyExc_ValueError, "could not convert BLOB to buffer");
                rc = -1;
            }
            break;
        case TYPE_UNKNOWN:
            rc = -1;
    }

final:
    return rc;
}

/* returns 0 if the object is one of Python's internal ones that don't need to be adapted */
static int _need_adapt(PyObject* obj)
{
    if (pysqlite_BaseTypeAdapted) {
        return 1;
    }

    if (PyLong_CheckExact(obj) || PyFloat_CheckExact(obj)
          || PyUnicode_CheckExact(obj) || PyByteArray_CheckExact(obj)) {
        return 0;
    } else {
        return 1;
    }
}

void pysqlite_statement_bind_parameters(pysqlite_Statement* self, PyObject* parameters, int allow_8bit_chars)
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

    if (PyTuple_CheckExact(parameters) || PyList_CheckExact(parameters) || (!PyDict_Check(parameters) && PySequence_Check(parameters))) {
        /* parameters passed as sequence */
        if (PyTuple_CheckExact(parameters)) {
            num_params = PyTuple_GET_SIZE(parameters);
        } else if (PyList_CheckExact(parameters)) {
            num_params = PyList_GET_SIZE(parameters);
        } else {
            num_params = PySequence_Size(parameters);
        }
        if (num_params != num_params_needed) {
            PyErr_Format(pysqlite_ProgrammingError, "Incorrect number of bindings supplied. The current statement uses %d, and there are %d supplied.",
                         num_params_needed, num_params);
            return;
        }
        for (i = 0; i < num_params; i++) {
            if (PyTuple_CheckExact(parameters)) {
                current_param = PyTuple_GET_ITEM(parameters, i);
                Py_XINCREF(current_param);
            } else if (PyList_CheckExact(parameters)) {
                current_param = PyList_GET_ITEM(parameters, i);
                Py_XINCREF(current_param);
            } else {
                current_param = PySequence_GetItem(parameters, i);
            }
            if (!current_param) {
                return;
            }

            if (!_need_adapt(current_param)) {
                adapted = current_param;
            } else {
                adapted = pysqlite_microprotocols_adapt(current_param, (PyObject*)&pysqlite_PrepareProtocolType, NULL);
                if (adapted) {
                    Py_DECREF(current_param);
                } else {
                    PyErr_Clear();
                    adapted = current_param;
                }
            }

            rc = pysqlite_statement_bind_parameter(self, i + 1, adapted, allow_8bit_chars);
            Py_DECREF(adapted);

            if (rc != SQLITE_OK) {
                if (!PyErr_Occurred()) {
                    PyErr_Format(pysqlite_InterfaceError, "Error binding parameter %d - probably unsupported type.", i);
                }
                return;
            }
        }
    } else if (PyDict_Check(parameters)) {
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
            if (PyDict_CheckExact(parameters)) {
                current_param = PyDict_GetItemString(parameters, binding_name);
                Py_XINCREF(current_param);
            } else {
                current_param = PyMapping_GetItemString(parameters, (char*)binding_name);
            }
            if (!current_param) {
                PyErr_Format(pysqlite_ProgrammingError, "You did not supply a value for binding %d.", i);
                return;
            }

            if (!_need_adapt(current_param)) {
                adapted = current_param;
            } else {
                adapted = pysqlite_microprotocols_adapt(current_param, (PyObject*)&pysqlite_PrepareProtocolType, NULL);
                if (adapted) {
                    Py_DECREF(current_param);
                } else {
                    PyErr_Clear();
                    adapted = current_param;
                }
            }

            rc = pysqlite_statement_bind_parameter(self, i, adapted, allow_8bit_chars);
            Py_DECREF(adapted);

            if (rc != SQLITE_OK) {
                if (!PyErr_Occurred()) {
                    PyErr_Format(pysqlite_InterfaceError, "Error binding parameter :%s - probably unsupported type.", binding_name);
                }
                return;
           }
        }
    } else {
        PyErr_SetString(PyExc_ValueError, "parameters are of unsupported type");
    }
}

int pysqlite_statement_recompile(pysqlite_Statement* self, PyObject* params)
{
    const char* tail;
    int rc;
    const char* sql_cstr;
    Py_ssize_t sql_len;
    sqlite3_stmt* new_st;

    sql_cstr = _PyUnicode_AsStringAndSize(self->sql, &sql_len);
    if (sql_cstr == NULL) {
        rc = PYSQLITE_SQL_WRONG_TYPE;
        return rc;
    }

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_prepare(self->db,
                         sql_cstr,
                         -1,
                         &new_st,
                         &tail);
    Py_END_ALLOW_THREADS

    if (rc == SQLITE_OK) {
        /* The efficient sqlite3_transfer_bindings is only available in SQLite
         * version 3.2.2 or later. For older SQLite releases, that might not
         * even define SQLITE_VERSION_NUMBER, we do it the manual way.
         */
        #ifdef SQLITE_VERSION_NUMBER
        #if SQLITE_VERSION_NUMBER >= 3002002
        /* The check for the number of parameters is necessary to not trigger a
         * bug in certain SQLite versions (experienced in 3.2.8 and 3.3.4). */
        if (sqlite3_bind_parameter_count(self->st) > 0) {
            (void)sqlite3_transfer_bindings(self->st, new_st);
        }
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

    Py_TYPE(self)->tp_free((PyObject*)self);
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
        0,                                              /* tp_reserved */
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
        Py_TPFLAGS_DEFAULT,                             /* tp_flags */
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
