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

    if (lstrip_sql(tail) != NULL) {
        PyErr_SetString(connection->ProgrammingError,
                        "You can only execute one statement at a time.");
        goto error;
    }

    /* Determine if the statement is a DML statement.
       SELECT is the only exception. See #9924. */
    int is_dml = 0;
    const char *p = lstrip_sql(sql_cstr);
    if (p != NULL) {
        is_dml = (PyOS_strnicmp(p, "insert", 6) == 0)
                  || (PyOS_strnicmp(p, "update", 6) == 0)
                  || (PyOS_strnicmp(p, "delete", 6) == 0)
                  || (PyOS_strnicmp(p, "replace", 7) == 0);
    }

    pysqlite_Statement *self = PyObject_GC_New(pysqlite_Statement,
                                               state->StatementType);
    if (self == NULL) {
        goto error;
    }

    self->st = stmt;
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
 * Strip leading whitespace and comments from incoming SQL (null terminated C
 * string) and return a pointer to the first non-whitespace, non-comment
 * character.
 *
 * This is used to check if somebody tries to execute more than one SQL query
 * with one execute()/executemany() command, which the DB-API don't allow.
 *
 * It is also used to harden DML query detection.
 */
static inline const char *
lstrip_sql(const char *sql)
{
    // This loop is borrowed from the SQLite source code.
    for (const char *pos = sql; *pos; pos++) {
        switch (*pos) {
            case ' ':
            case '\t':
            case '\f':
            case '\n':
            case '\r':
                // Skip whitespace.
                break;
            case '-':
                // Skip line comments.
                if (pos[1] == '-') {
                    pos += 2;
                    while (pos[0] && pos[0] != '\n') {
                        pos++;
                    }
                    if (pos[0] == '\0') {
                        return NULL;
                    }
                    continue;
                }
                return pos;
            case '/':
                // Skip C style comments.
                if (pos[1] == '*') {
                    pos += 2;
                    while (pos[0] && (pos[0] != '*' || pos[1] != '/')) {
                        pos++;
                    }
                    if (pos[0] == '\0') {
                        return NULL;
                    }
                    pos++;
                    continue;
                }
                return pos;
            default:
                return pos;
        }
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
