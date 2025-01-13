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

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "module.h"
#include "pycore_long.h"          // _PyLong_AsByteArray()
#include "connection.h"

// Returns non-NULL if a new exception should be raised
static PyObject *
get_exception_class(pysqlite_state *state, int errorcode)
{
    switch (errorcode) {
        case SQLITE_OK:
            PyErr_Clear();
            return NULL;
        case SQLITE_INTERNAL:
        case SQLITE_NOTFOUND:
            return state->InternalError;
        case SQLITE_NOMEM:
            return PyErr_NoMemory();
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
            return state->OperationalError;
        case SQLITE_CORRUPT:
            return state->DatabaseError;
        case SQLITE_TOOBIG:
            return state->DataError;
        case SQLITE_CONSTRAINT:
        case SQLITE_MISMATCH:
            return state->IntegrityError;
        case SQLITE_MISUSE:
        case SQLITE_RANGE:
            return state->InterfaceError;
        default:
            return state->DatabaseError;
    }
}

static void
raise_exception(PyObject *type, int errcode, const char *errmsg)
{
    PyObject *exc = NULL;
    PyObject *args[] = { PyUnicode_FromString(errmsg), };
    if (args[0] == NULL) {
        goto exit;
    }
    exc = PyObject_Vectorcall(type, args, 1, NULL);
    Py_DECREF(args[0]);
    if (exc == NULL) {
        goto exit;
    }

    PyObject *code = PyLong_FromLong(errcode);
    if (code == NULL) {
        goto exit;
    }
    int rc = PyObject_SetAttrString(exc, "sqlite_errorcode", code);
    Py_DECREF(code);
    if (rc < 0) {
        goto exit;
    }

    const char *error_name = pysqlite_error_name(errcode);
    PyObject *name;
    if (error_name) {
        name = PyUnicode_FromString(error_name);
    }
    else {
        name = PyUnicode_InternFromString("unknown");
    }
    if (name == NULL) {
        goto exit;
    }
    rc = PyObject_SetAttrString(exc, "sqlite_errorname", name);
    Py_DECREF(name);
    if (rc < 0) {
        goto exit;
    }

    PyErr_SetObject(type, exc);

exit:
    Py_XDECREF(exc);
}

/**
 * Checks the SQLite error code and sets the appropriate DB-API exception.
 * Returns the error code (0 means no error occurred).
 */
int
_pysqlite_seterror(pysqlite_state *state, sqlite3 *db)
{
    int errorcode = sqlite3_errcode(db);
    PyObject *exc_class = get_exception_class(state, errorcode);
    if (exc_class == NULL) {
        // No new exception need be raised; just pass the error code
        return errorcode;
    }

    /* Create and set the exception. */
    int extended_errcode = sqlite3_extended_errcode(db);
    // sqlite3_errmsg() always returns an UTF-8 encoded message
    const char *errmsg = sqlite3_errmsg(db);
    raise_exception(exc_class, extended_errcode, errmsg);
    return extended_errcode;
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
                                IS_LITTLE_ENDIAN, 1 /* signed */, 0) >= 0) {
            return int64val;
        }
    }
    PyErr_SetString(PyExc_OverflowError,
                    "Python int too large to convert to SQLite INTEGER");
    return -1;
}
