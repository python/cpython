/* cursor.h - definitions for the cursor type
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

#ifndef PYSQLITE_CURSOR_H
#define PYSQLITE_CURSOR_H
#include "Python.h"

#include "statement.h"
#include "connection.h"
#include "module.h"

typedef struct
{
    PyObject_HEAD
    pysqlite_Connection* connection;
    PyObject* description;
    PyObject* row_cast_map;
    int arraysize;
    PyObject* lastrowid;
    long rowcount;
    PyObject* row_factory;
    pysqlite_Statement* statement;
    int closed;
    int locked;
    int initialized;

    PyObject* in_weakreflist; /* List of weak references */
} pysqlite_Cursor;

int pysqlite_cursor_setup_types(PyObject *module);

#endif
