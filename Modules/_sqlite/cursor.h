/* cursor.h - definitions for the cursor type
 *
 * Copyright (C) 2004-2006 Gerhard Häring <gh@ghaering.de>
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
    Connection* connection;
    PyObject* description;
    PyObject* row_cast_map;
    int arraysize;
    PyObject* lastrowid;
    PyObject* rowcount;
    PyObject* row_factory;
    Statement* statement;

    /* the next row to be returned, NULL if no next row available */
    PyObject* next_row;
} Cursor;

typedef enum {
    STATEMENT_INVALID, STATEMENT_INSERT, STATEMENT_DELETE,
    STATEMENT_UPDATE, STATEMENT_REPLACE, STATEMENT_SELECT,
    STATEMENT_OTHER
} StatementKind;

extern PyTypeObject CursorType;

int cursor_init(Cursor* self, PyObject* args, PyObject* kwargs);
void cursor_dealloc(Cursor* self);
PyObject* cursor_execute(Cursor* self, PyObject* args);
PyObject* cursor_executemany(Cursor* self, PyObject* args);
PyObject* cursor_getiter(Cursor *self);
PyObject* cursor_iternext(Cursor *self);
PyObject* cursor_fetchone(Cursor* self, PyObject* args);
PyObject* cursor_fetchmany(Cursor* self, PyObject* args);
PyObject* cursor_fetchall(Cursor* self, PyObject* args);
PyObject* pysqlite_noop(Connection* self, PyObject* args);
PyObject* cursor_close(Cursor* self, PyObject* args);

int cursor_setup_types(void);

#define UNKNOWN (-1)
#endif
