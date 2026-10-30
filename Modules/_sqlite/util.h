/* util.h - various utility functions
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

#ifndef PYSQLITE_UTIL_H
#define PYSQLITE_UTIL_H
#include "Python.h"
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_STORE_PTR_RELEASE()
#include "pythread.h"
#include "sqlite3.h"
#include "connection.h"

/**
 * Checks the SQLite error code and sets the appropriate DB-API exception.
 */
int set_error_from_db(pysqlite_state *state, sqlite3 *db);
void set_error_from_code(pysqlite_state *state, int code);

sqlite_int64 _pysqlite_long_as_int64(PyObject * value);

/* Take a strong reference while the owning object's lock is held. */
static inline PyObject *
pysqlite_get_ref(PyObject *owner, PyObject *const *field, PyObject *fallback)
{
    PyObject *value;
    Py_BEGIN_CRITICAL_SECTION(owner);
    value = Py_NewRef(*field != NULL ? *field : fallback);
    Py_END_CRITICAL_SECTION();
    return value;
}

/* Steal value. Use the same store as PyMember_SetOne() since a readonly
 * member descriptor may load the field without taking the object's lock. */
static inline void
pysqlite_set_ref(PyObject *owner, PyObject **field, PyObject *value)
{
    PyObject *old;
    Py_BEGIN_CRITICAL_SECTION(owner);
    old = *field;
    FT_ATOMIC_STORE_PTR_RELEASE(*field, value);
    Py_END_CRITICAL_SECTION();
    Py_XDECREF(old);
}

#endif
