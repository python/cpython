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
    int max_length = sqlite3_limit(db, SQLITE_LIMIT_LENGTH, -1);
    if (size >= max_length) {
        PyErr_SetString(connection->DataError,
                        "query string is too large");
        return NULL;
    }
    if (strlen(sql_cstr) != (size_t)size) {
        PyErr_SetString(PyExc_ValueError,
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

    if (pysqlite_check_remaining_sql(tail)) {
        PyErr_SetString(connection->Warning,
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
    self->in_weakreflist = NULL;

    PyObject_GC_Track(self);
    return self;

error:
    (void)sqlite3_finalize(stmt);
    return NULL;
}

int pysqlite_statement_bind_parameter(pysqlite_Statement* self, int pos, PyObject* parameter)
{
    int rc = SQLITE_OK;
    const char *string;
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
            string = PyUnicode_AsUTF8AndSize(parameter, &buflen);
            if (string == NULL)
                return -1;
            if (buflen > INT_MAX) {
                PyErr_SetString(PyExc_OverflowError,
                                "string longer than INT_MAX bytes");
                return -1;
            }
            rc = sqlite3_bind_text(self->st, pos, string, (int)buflen, SQLITE_TRANSIENT);
            break;
        case TYPE_BUFFER: {
            Py_buffer view;
            if (PyObject_GetBuffer(parameter, &view, PyBUF_SIMPLE) != 0) {
                PyErr_SetString(PyExc_ValueError, "could not convert BLOB to buffer");
                return -1;
            }
            if (view.len > INT_MAX) {
                PyErr_SetString(PyExc_OverflowError,
                                "BLOB longer than INT_MAX bytes");
                PyBuffer_Release(&view);
                return -1;
            }
            rc = sqlite3_bind_blob(self->st, pos, view.buf, (int)view.len, SQLITE_TRANSIENT);
            PyBuffer_Release(&view);
            break;
        }
        case TYPE_UNKNOWN:
            rc = -1;
    }

final:
    return rc;
}

/* returns 0 if the object is one of Python's internal ones that don't need to be adapted */
static int
_need_adapt(pysqlite_state *state, PyObject *obj)
{
    if (state->BaseTypeAdapted) {
        return 1;
    }

    if (PyLong_CheckExact(obj) || PyFloat_CheckExact(obj)
          || PyUnicode_CheckExact(obj) || PyByteArray_CheckExact(obj)) {
        return 0;
    } else {
        return 1;
    }
}

void
pysqlite_statement_bind_parameters(pysqlite_state *state,
                                   pysqlite_Statement *self,
                                   PyObject *parameters)
{
    PyObject* current_param;
    PyObject* adapted;
    const char* binding_name;
    int i;
    int rc;
    int num_params_needed;
    Py_ssize_t num_params;

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
            if (num_params == -1) {
                return;
            }
        }
        if (num_params != num_params_needed) {
            PyErr_Format(state->ProgrammingError,
                         "Incorrect number of bindings supplied. The current "
                         "statement uses %d, and there are %zd supplied.",
                         num_params_needed, num_params);
            return;
        }
        for (i = 0; i < num_params; i++) {
            if (PyTuple_CheckExact(parameters)) {
                PyObject *item = PyTuple_GET_ITEM(parameters, i);
                current_param = Py_NewRef(item);
            } else if (PyList_CheckExact(parameters)) {
                PyObject *item = PyList_GetItem(parameters, i);
                current_param = Py_XNewRef(item);
            } else {
                current_param = PySequence_GetItem(parameters, i);
            }
            if (!current_param) {
                return;
            }

            if (!_need_adapt(state, current_param)) {
                adapted = current_param;
            } else {
                PyObject *protocol = (PyObject *)state->PrepareProtocolType;
                adapted = pysqlite_microprotocols_adapt(state, current_param,
                                                        protocol,
                                                        current_param);
                Py_DECREF(current_param);
                if (!adapted) {
                    return;
                }
            }

            rc = pysqlite_statement_bind_parameter(self, i + 1, adapted);
            Py_DECREF(adapted);

            if (rc != SQLITE_OK) {
                if (!PyErr_Occurred()) {
                    PyErr_Format(state->InterfaceError,
                                 "Error binding parameter %d - "
                                 "probably unsupported type.", i);
                }
                return;
            }
        }
    } else if (PyDict_Check(parameters)) {
        /* parameters passed as dictionary */
        for (i = 1; i <= num_params_needed; i++) {
            PyObject *binding_name_obj;
            Py_BEGIN_ALLOW_THREADS
            binding_name = sqlite3_bind_parameter_name(self->st, i);
            Py_END_ALLOW_THREADS
            if (!binding_name) {
                PyErr_Format(state->ProgrammingError,
                             "Binding %d has no name, but you supplied a "
                             "dictionary (which has only names).", i);
                return;
            }

            binding_name++; /* skip first char (the colon) */
            binding_name_obj = PyUnicode_FromString(binding_name);
            if (!binding_name_obj) {
                return;
            }
            if (PyDict_CheckExact(parameters)) {
                PyObject *item = PyDict_GetItemWithError(parameters, binding_name_obj);
                current_param = Py_XNewRef(item);
            } else {
                current_param = PyObject_GetItem(parameters, binding_name_obj);
            }
            Py_DECREF(binding_name_obj);
            if (!current_param) {
                if (!PyErr_Occurred() || PyErr_ExceptionMatches(PyExc_LookupError)) {
                    PyErr_Format(state->ProgrammingError,
                                 "You did not supply a value for binding "
                                 "parameter :%s.", binding_name);
                }
                return;
            }

            if (!_need_adapt(state, current_param)) {
                adapted = current_param;
            } else {
                PyObject *protocol = (PyObject *)state->PrepareProtocolType;
                adapted = pysqlite_microprotocols_adapt(state, current_param,
                                                        protocol,
                                                        current_param);
                Py_DECREF(current_param);
                if (!adapted) {
                    return;
                }
            }

            rc = pysqlite_statement_bind_parameter(self, i, adapted);
            Py_DECREF(adapted);

            if (rc != SQLITE_OK) {
                if (!PyErr_Occurred()) {
                    PyErr_Format(state->InterfaceError,
                                 "Error binding parameter :%s - "
                                 "probably unsupported type.", binding_name);
                }
                return;
           }
        }
    } else {
        PyErr_SetString(PyExc_ValueError, "parameters are of unsupported type");
    }
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

static void
stmt_dealloc(pysqlite_Statement *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    if (self->in_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject*)self);
    }
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

static PyMemberDef stmt_members[] = {
    {"__weaklistoffset__", T_PYSSIZET, offsetof(pysqlite_Statement, in_weakreflist), READONLY},
    {NULL},
};
static PyType_Slot stmt_slots[] = {
    {Py_tp_members, stmt_members},
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
