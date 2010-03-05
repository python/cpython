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

int pysqlite_step(sqlite3_stmt* statement, pysqlite_Connection* connection)
{
    int rc;

    if (statement == NULL) {
        /* this is a workaround for SQLite 3.5 and later. it now apparently
         * returns NULL for "no-operation" statements */
        rc = SQLITE_OK;
    } else {
        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_step(statement);
        Py_END_ALLOW_THREADS
    }

    return rc;
}

/**
 * Checks the SQLite error code and sets the appropriate DB-API exception.
 * Returns the error code (0 means no error occurred).
 */
int _pysqlite_seterror(sqlite3* db, sqlite3_stmt* st)
{
    int errorcode;

    /* SQLite often doesn't report anything useful, unless you reset the statement first */
    if (st != NULL) {
        (void)sqlite3_reset(st);
    }

    errorcode = sqlite3_errcode(db);

    switch (errorcode)
    {
        case SQLITE_OK:
            PyErr_Clear();
            break;
        case SQLITE_INTERNAL:
        case SQLITE_NOTFOUND:
            PyErr_SetString(pysqlite_InternalError, sqlite3_errmsg(db));
            break;
        case SQLITE_NOMEM:
            (void)PyErr_NoMemory();
            break;
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
            PyErr_SetString(pysqlite_OperationalError, sqlite3_errmsg(db));
            break;
        case SQLITE_CORRUPT:
            PyErr_SetString(pysqlite_DatabaseError, sqlite3_errmsg(db));
            break;
        case SQLITE_TOOBIG:
            PyErr_SetString(pysqlite_DataError, sqlite3_errmsg(db));
            break;
        case SQLITE_CONSTRAINT:
        case SQLITE_MISMATCH:
            PyErr_SetString(pysqlite_IntegrityError, sqlite3_errmsg(db));
            break;
        case SQLITE_MISUSE:
            PyErr_SetString(pysqlite_ProgrammingError, sqlite3_errmsg(db));
            break;
        default:
            PyErr_SetString(pysqlite_DatabaseError, sqlite3_errmsg(db));
            break;
    }

    return errorcode;
}

