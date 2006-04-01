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
#include "connection.h"

/* prototypes */
int check_remaining_sql(const char* tail);

typedef enum {
    LINECOMMENT_1,
    IN_LINECOMMENT,
    COMMENTSTART_1,
    IN_COMMENT,
    COMMENTEND_1,
    NORMAL
} parse_remaining_sql_state;

int statement_create(Statement* self, Connection* connection, PyObject* sql)
{
    const char* tail;
    int rc;
    PyObject* sql_str;
    char* sql_cstr;

    self->st = NULL;

    self->st = NULL;
    self->in_use = 0;

    if (PyString_Check(sql)) {
        sql_str = sql;
        Py_INCREF(sql_str);
    } else if (PyUnicode_Check(sql)) {
        sql_str = PyUnicode_AsUTF8String(sql);
        if (!sql_str) {
            rc = PYSQLITE_SQL_WRONG_TYPE;
            return rc;
        }
    } else {
        rc = PYSQLITE_SQL_WRONG_TYPE;
        return rc;
    }

    self->sql = sql_str;

    sql_cstr = PyString_AsString(sql_str);

    rc = sqlite3_prepare(connection->db,
                         sql_cstr,
                         -1,
                         &self->st,
                         &tail);

    self->db = connection->db;

    if (rc == SQLITE_OK && check_remaining_sql(tail)) {
        (void)sqlite3_finalize(self->st);
        rc = PYSQLITE_TOO_MUCH_SQL;
    }

    return rc;
}

int statement_recompile(Statement* self)
{
    const char* tail;
    int rc;
    char* sql_cstr;
    sqlite3_stmt* new_st;

    sql_cstr = PyString_AsString(self->sql);

    rc = sqlite3_prepare(self->db,
                         sql_cstr,
                         -1,
                         &new_st,
                         &tail);

    if (rc == SQLITE_OK) {
        (void)sqlite3_transfer_bindings(self->st, new_st);

        (void)sqlite3_finalize(self->st);
        self->st = new_st;
    }

    return rc;
}

int statement_finalize(Statement* self)
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

int statement_reset(Statement* self)
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

void statement_mark_dirty(Statement* self)
{
    self->in_use = 1;
}

void statement_dealloc(Statement* self)
{
    int rc;

    if (self->st) {
        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_finalize(self->st);
        Py_END_ALLOW_THREADS
    }

    self->st = NULL;

    Py_XDECREF(self->sql);

    self->ob_type->tp_free((PyObject*)self);
}

/*
 * Checks if there is anything left in an SQL string after SQLite compiled it.
 * This is used to check if somebody tried to execute more than one SQL command
 * with one execute()/executemany() command, which the DB-API and we don't
 * allow.
 *
 * Returns 1 if there is more left than should be. 0 if ok.
 */
int check_remaining_sql(const char* tail)
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

PyTypeObject StatementType = {
        PyObject_HEAD_INIT(NULL)
        0,                                              /* ob_size */
        "pysqlite2.dbapi2.Statement",                   /* tp_name */
        sizeof(Statement),                              /* tp_basicsize */
        0,                                              /* tp_itemsize */
        (destructor)statement_dealloc,                  /* tp_dealloc */
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
        Py_TPFLAGS_DEFAULT,                             /* tp_flags */
        0,                                              /* tp_doc */
        0,                                              /* tp_traverse */
        0,                                              /* tp_clear */
        0,                                              /* tp_richcompare */
        0,                                              /* tp_weaklistoffset */
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

extern int statement_setup_types(void)
{
    StatementType.tp_new = PyType_GenericNew;
    return PyType_Ready(&StatementType);
}
