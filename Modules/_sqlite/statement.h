/* statement.h - definitions for the statement type
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

#ifndef PYSQLITE_STATEMENT_H
#define PYSQLITE_STATEMENT_H
#include "Python.h"

#include "connection.h"
#include "sqlite3.h"

typedef struct
{
    PyObject_HEAD
    sqlite3_stmt* st;
    int is_dml;
} pysqlite_Statement;

pysqlite_Statement *pysqlite_statement_create(pysqlite_Connection *connection, PyObject *sql);

int pysqlite_statement_setup_types(PyObject *module);

#endif
