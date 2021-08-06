/* util.c - various utility functions
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

#include "module.h"
#include "connection.h"

int
pysqlite_step(sqlite3_stmt *statement)
{
    int rc;

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_step(statement);
    Py_END_ALLOW_THREADS

    return rc;
}

/**
 * Checks the SQLite error code and sets the appropriate DB-API exception.
 * Returns the error code (0 means no error occurred).
 */
int
_pysqlite_seterror(pysqlite_state *state, sqlite3 *db)
{
    PyObject *exc_class;
    int errorcode = sqlite3_errcode(db);

    switch (errorcode)
    {
        case SQLITE_OK:
            PyErr_Clear();
            return errorcode;
        case SQLITE_INTERNAL:
        case SQLITE_NOTFOUND:
            exc_class = state->InternalError;
            break;
        case SQLITE_NOMEM:
            (void)PyErr_NoMemory();
            return errorcode;
        case SQLITE_ERROR:
        case SQLITE_PERM:
        case SQLITE_ABORT:
        case SQLITE_BUSY:
        case SQLITE_LOCKED:
        case SQLITE_READONLY:
        case SQLITE_INTERRUPT:
        case SQLITE_IOERR:
        case SQLITE_FULL:
        case SQLITE_CANTOPEN:
        case SQLITE_PROTOCOL:
        case SQLITE_EMPTY:
        case SQLITE_SCHEMA:
            exc_class = state->OperationalError;
            break;
        case SQLITE_CORRUPT:
            exc_class = state->DatabaseError;
            break;
        case SQLITE_TOOBIG:
            exc_class = state->DataError;
            break;
        case SQLITE_CONSTRAINT:
        case SQLITE_MISMATCH:
            exc_class = state->IntegrityError;
            break;
        case SQLITE_MISUSE:
            exc_class = state->ProgrammingError;
            break;
        default:
            exc_class = state->DatabaseError;
            break;
    }

    /* Create and set the exception. */
    const char *error_msg;
    const char *error_name;
    PyObject *exc = NULL;
    PyObject *args = NULL;
    PyObject *py_code = NULL;
    PyObject *py_name = NULL;

    error_name = pysqlite_error_name(errorcode);
    error_msg = sqlite3_errmsg(db);

    args = Py_BuildValue("(s)", error_msg);
    if (args == NULL) {
        goto error;
    }

    exc = PyObject_Call(exc_class, args, NULL);
    if (exc == NULL) {
        goto error;
    }

    py_code = Py_BuildValue("i", errorcode);
    if (py_code == NULL) {
        goto error;
    }

    if (PyObject_SetAttrString(exc, "sqlite_errorcode", py_code) < 0) {
        goto error;
    }

    py_name = Py_BuildValue("s", error_name ? error_name : "SQLITE_UNKNOWN");
    if (py_name == NULL) {
        goto error;
    }

    if (PyObject_SetAttrString(exc, "sqlite_errorname", py_name) < 0) {
        goto error;
    }

    PyErr_SetObject((PyObject *) Py_TYPE(exc), exc);

error:
    Py_XDECREF(py_code);
    Py_XDECREF(py_name);
    Py_XDECREF(args);
    Py_XDECREF(exc);
    return errorcode;
}

#ifdef WORDS_BIGENDIAN
# define IS_LITTLE_ENDIAN 0
#else
# define IS_LITTLE_ENDIAN 1
#endif

sqlite_int64
_pysqlite_long_as_int64(PyObject * py_val)
{
    int overflow;
    long long value = PyLong_AsLongLongAndOverflow(py_val, &overflow);
    if (value == -1 && PyErr_Occurred())
        return -1;
    if (!overflow) {
# if SIZEOF_LONG_LONG > 8
        if (-0x8000000000000000LL <= value && value <= 0x7FFFFFFFFFFFFFFFLL)
# endif
            return value;
    }
    else if (sizeof(value) < sizeof(sqlite_int64)) {
        sqlite_int64 int64val;
        if (_PyLong_AsByteArray((PyLongObject *)py_val,
                                (unsigned char *)&int64val, sizeof(int64val),
                                IS_LITTLE_ENDIAN, 1 /* signed */) >= 0) {
            return int64val;
        }
    }
    PyErr_SetString(PyExc_OverflowError,
                    "Python int too large to convert to SQLite INTEGER");
    return -1;
}
