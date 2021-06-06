/* connection.h - definitions for the connection type
 *
 * Copyright (C) 2004-2010 Gerhard Häring <gh@ghaering.de>
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

#ifndef PYSQLITE_CONNECTION_H
#define PYSQLITE_CONNECTION_H
#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "pythread.h"
#include "structmember.h"

#include "module.h"

#include "sqlite3.h"

typedef struct
{
    PyObject_HEAD
    sqlite3* db;

    /* the type detection mode. Only 0, PARSE_DECLTYPES, PARSE_COLNAMES or a
     * bitwise combination thereof makes sense */
    int detect_types;

    /* None for autocommit, otherwise a PyUnicode with the isolation level */
    PyObject* isolation_level;

    /* NULL for autocommit, otherwise a string with the BEGIN statement */
    const char* begin_statement;

    /* 1 if a check should be performed for each API call if the connection is
     * used from the same thread it was created in */
    int check_same_thread;

    int initialized;

    /* thread identification of the thread the connection was created in */
    unsigned long thread_ident;

    PyObject *statement_cache;

    /* Lists of weak references to statements and cursors used within this connection */
    PyObject* statements;
    PyObject* cursors;

    /* Counters for how many statements/cursors were created in the connection. May be
     * reset to 0 at certain intervals */
    int created_statements;
    int created_cursors;

    PyObject* row_factory;

    /* Determines how bytestrings from SQLite are converted to Python objects:
     * - PyUnicode_Type:        Python Unicode objects are constructed from UTF-8 bytestrings
     * - PyBytes_Type:          The bytestrings are returned as-is.
     * - Any custom callable:   Any object returned from the callable called with the bytestring
     *                          as single parameter.
     */
    PyObject* text_factory;

    /* remember references to object used in trace_callback/progress_handler/authorizer_cb */
    PyObject* function_pinboard_trace_callback;
    PyObject* function_pinboard_progress_handler;
    PyObject* function_pinboard_authorizer_cb;

    /* a dictionary of registered collation name => collation callable mappings */
    PyObject* collations;

    /* Exception objects: borrowed refs. */
    PyObject* Warning;
    PyObject* Error;
    PyObject* InterfaceError;
    PyObject* DatabaseError;
    PyObject* DataError;
    PyObject* OperationalError;
    PyObject* IntegrityError;
    PyObject* InternalError;
    PyObject* ProgrammingError;
    PyObject* NotSupportedError;
} pysqlite_Connection;

extern PyTypeObject *pysqlite_ConnectionType;

int pysqlite_connection_register_cursor(pysqlite_Connection* connection, PyObject* cursor);
int pysqlite_check_thread(pysqlite_Connection* self);
int pysqlite_check_connection(pysqlite_Connection* con);

int pysqlite_connection_setup_types(PyObject *module);

#endif
