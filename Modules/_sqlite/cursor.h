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
#define PY_SSIZE_T_CLEAN
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
    int reset;
    int locked;
    int initialized;

    /* the next row to be returned, NULL if no next row available */
    PyObject* next_row;

    PyObject* in_weakreflist; /* List of weak references */
} pysqlite_Cursor;

extern PyTypeObject *pysqlite_CursorType;

PyObject* pysqlite_cursor_execute(pysqlite_Cursor* self, PyObject* args);
PyObject* pysqlite_cursor_executemany(pysqlite_Cursor* self, PyObject* args);
PyObject* pysqlite_cursor_getiter(pysqlite_Cursor *self);
PyObject* pysqlite_cursor_iternext(pysqlite_Cursor *self);
PyObject* pysqlite_cursor_fetchone(pysqlite_Cursor* self, PyObject* args);
PyObject* pysqlite_cursor_fetchmany(pysqlite_Cursor* self, PyObject* args, PyObject* kwargs);
PyObject* pysqlite_cursor_fetchall(pysqlite_Cursor* self, PyObject* args);
PyObject* pysqlite_noop(pysqlite_Connection* self, PyObject* args);
PyObject* pysqlite_cursor_close(pysqlite_Cursor* self, PyObject* args);

int pysqlite_cursor_setup_types(PyObject *module);

#define UNKNOWN (-1)
#endif
