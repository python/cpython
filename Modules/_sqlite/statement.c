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

#include "connection.h"
#include "statement.h"
#include "util.h"

/* prototypes */
static const char *lstrip_sql(const char *sql);

typedef enum {
    LINECOMMENT_1,
    IN_LINECOMMENT,
    COMMENTSTART_1,
    IN_COMMENT,
    COMMENTEND_1,
    NORMAL
} parse_remaining_sql_state;

pysqlite_Statement *
pysqlite_statement_create(pysqlite_Connection *connection, PyObject *sql)
{
    pysqlite_state *state = connection->state;
    assert(PyUnicode_Check(sql));
    Py_ssize_t size;
    const char *sql_cstr = PyUnicode_AsUTF8AndSize(sql, &size);
    if (sql_cstr == NULL) {
        return NULL;
    }

    sqlite3 *db = connection->db;
    int max_length = sqlite3_limit(db, SQLITE_LIMIT_SQL_LENGTH, -1);
    if (size > max_length) {
        PyErr_SetString(connection->DataError,
                        "query string is too large");
        return NULL;
    }
    if (strlen(sql_cstr) != (size_t)size) {
        PyErr_SetString(connection->ProgrammingError,
                        "the query contains a null character");
        return NULL;
    }

    sqlite3_stmt *stmt;
    const char *tail;
    int rc;
    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_prepare_v2(db, sql_cstr, (int)size + 1, &stmt, &tail);
    Py_END_ALLOW_THREADS

    if (rc != SQLITE_OK) {
        _pysqlite_seterror(state, db);
        return NULL;
    }

    if (lstrip_sql(tail)) {
        PyErr_SetString(connection->ProgrammingError,
                        "You can only execute one statement at a time.");
        goto error;
    }

    /* Determine if the statement is a DML statement.
       SELECT is the only exception. See #9924. */
    int is_dml = 0;
    for (const char *p = sql_cstr; *p != 0; p++) {
        switch (*p) {
            case ' ':
            case '\r':
            case '\n':
            case '\t':
                continue;
        }

        is_dml = (PyOS_strnicmp(p, "insert", 6) == 0)
                  || (PyOS_strnicmp(p, "update", 6) == 0)
                  || (PyOS_strnicmp(p, "delete", 6) == 0)
                  || (PyOS_strnicmp(p, "replace", 7) == 0);
        break;
    }

    pysqlite_Statement *self = PyObject_GC_New(pysqlite_Statement,
                                               state->StatementType);
    if (self == NULL) {
        goto error;
    }

    self->st = stmt;
    self->in_use = 0;
    self->is_dml = is_dml;

    PyObject_GC_Track(self);
    return self;

error:
    (void)sqlite3_finalize(stmt);
    return NULL;
}

static void
stmt_dealloc(pysqlite_Statement *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    if (self->st) {
        Py_BEGIN_ALLOW_THREADS
        sqlite3_finalize(self->st);
        Py_END_ALLOW_THREADS
        self->st = 0;
    }
    tp->tp_free(self);
    Py_DECREF(tp);
}

static int
stmt_traverse(pysqlite_Statement *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return 0;
}

/*
 * Checks if there is anything left in an SQL string after SQLite compiled it.
 * This is used to check if somebody tried to execute more than one SQL command
 * with one execute()/executemany() command, which the DB-API and we don't
 * allow.
 *
 * Returns 1 if there is more left than should be. 0 if ok.
 */
static const char *
lstrip_sql(const char *sql)
{
    const char *pos = sql;

    parse_remaining_sql_state state = NORMAL;

    for (;;) {
        switch (*pos) {
            case 0:
                return NULL;
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
                    return pos;
                }
                break;
            case '*':
                if (state == NORMAL) {
                    return pos;
                } else if (state == LINECOMMENT_1) {
                    return pos;
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
                    return pos;
                }
        }

        pos++;
    }

    return NULL;
}

static PyType_Slot stmt_slots[] = {
    {Py_tp_dealloc, stmt_dealloc},
    {Py_tp_traverse, stmt_traverse},
    {0, NULL},
};

static PyType_Spec stmt_spec = {
    .name = MODULE_NAME ".Statement",
    .basicsize = sizeof(pysqlite_Statement),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_DISALLOW_INSTANTIATION),
    .slots = stmt_slots,
};

int
pysqlite_statement_setup_types(PyObject *module)
{
    PyObject *type = PyType_FromModuleAndSpec(module, &stmt_spec, NULL);
    if (type == NULL) {
        return -1;
    }
    pysqlite_state *state = pysqlite_get_state(module);
    state->StatementType = (PyTypeObject *)type;
    return 0;
}
