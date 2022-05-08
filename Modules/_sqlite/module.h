/* module.h - definitions for the module
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

#ifndef PYSQLITE_MODULE_H
#define PYSQLITE_MODULE_H
#define PY_SSIZE_T_CLEAN
#include "Python.h"

#define PYSQLITE_VERSION "2.6.0"
#define MODULE_NAME "sqlite3"

typedef struct {
    PyObject *DataError;
    PyObject *DatabaseError;
    PyObject *Error;
    PyObject *IntegrityError;
    PyObject *InterfaceError;
    PyObject *InternalError;
    PyObject *NotSupportedError;
    PyObject *OperationalError;
    PyObject *ProgrammingError;
    PyObject *Warning;


    /* A dictionary, mapping column types (INTEGER, VARCHAR, etc.) to converter
     * functions, that convert the SQL value to the appropriate Python value.
     * The key is uppercase.
     */
    PyObject *converters;

    PyObject *lru_cache;
    PyObject *psyco_adapters;  // The adapters registry
    int BaseTypeAdapted;
    int enable_callback_tracebacks;

    PyTypeObject *BlobType;
    PyTypeObject *ConnectionType;
    PyTypeObject *CursorType;
    PyTypeObject *PrepareProtocolType;
    PyTypeObject *RowType;
    PyTypeObject *StatementType;

    /* Pointers to interned strings */
    PyObject *str___adapt__;
    PyObject *str___conform__;
    PyObject *str_executescript;
    PyObject *str_finalize;
    PyObject *str_inverse;
    PyObject *str_step;
    PyObject *str_upper;
    PyObject *str_value;
} pysqlite_state;

extern pysqlite_state pysqlite_global_state;

static inline pysqlite_state *
pysqlite_get_state(PyObject *module)
{
    pysqlite_state *state = (pysqlite_state *)PyModule_GetState(module);
    assert(state != NULL);
    return state;
}

extern struct PyModuleDef _sqlite3module;
static inline pysqlite_state *
pysqlite_get_state_by_type(PyTypeObject *tp)
{
    PyObject *module = PyType_GetModuleByDef(tp, &_sqlite3module);
    assert(module != NULL);
    return pysqlite_get_state(module);
}

extern const char *pysqlite_error_name(int rc);

#define PARSE_DECLTYPES 1
#define PARSE_COLNAMES 2
#endif
