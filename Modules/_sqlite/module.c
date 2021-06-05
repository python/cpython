/* module.c - the module itself
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

#include "connection.h"
#include "statement.h"
#include "cursor.h"
#include "prepare_protocol.h"
#include "microprotocols.h"
#include "row.h"

#if SQLITE_VERSION_NUMBER < 3007015
#error "SQLite 3.7.15 or higher required"
#endif

#include "clinic/module.c.h"
/*[clinic input]
module _sqlite3
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=81e330492d57488e]*/

/* static objects at module-level */

PyObject *pysqlite_Error = NULL;
PyObject *pysqlite_Warning = NULL;
PyObject *pysqlite_InterfaceError = NULL;
PyObject *pysqlite_DatabaseError = NULL;
PyObject *pysqlite_InternalError = NULL;
PyObject *pysqlite_OperationalError = NULL;
PyObject *pysqlite_ProgrammingError = NULL;
PyObject *pysqlite_IntegrityError = NULL;
PyObject *pysqlite_DataError = NULL;
PyObject *pysqlite_NotSupportedError = NULL;

PyObject* _pysqlite_converters = NULL;
int _pysqlite_enable_callback_tracebacks = 0;
int pysqlite_BaseTypeAdapted = 0;

pysqlite_state pysqlite_global_state;

pysqlite_state *
pysqlite_get_state(PyObject *Py_UNUSED(module))
{
    return &pysqlite_global_state;
}

static PyObject* module_connect(PyObject* self, PyObject* args, PyObject*
        kwargs)
{
    /* Python seems to have no way of extracting a single keyword-arg at
     * C-level, so this code is redundant with the one in connection_init in
     * connection.c and must always be copied from there ... */

    static char *kwlist[] = {
        "database", "timeout", "detect_types", "isolation_level",
        "check_same_thread", "factory", "cached_statements", "uri",
        NULL
    };
    PyObject* database;
    int detect_types = 0;
    PyObject* isolation_level;
    PyObject* factory = NULL;
    int check_same_thread = 1;
    int cached_statements;
    int uri = 0;
    double timeout = 5.0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|diOiOip", kwlist,
                                     &database, &timeout, &detect_types,
                                     &isolation_level, &check_same_thread,
                                     &factory, &cached_statements, &uri))
    {
        return NULL;
    }

    if (factory == NULL) {
        factory = (PyObject*)pysqlite_ConnectionType;
    }

    return PyObject_Call(factory, args, kwargs);
}

PyDoc_STRVAR(module_connect_doc,
"connect(database[, timeout, detect_types, isolation_level,\n\
        check_same_thread, factory, cached_statements, uri])\n\
\n\
Opens a connection to the SQLite database file *database*. You can use\n\
\":memory:\" to open a database connection to a database that resides in\n\
RAM instead of on disk.");

/*[clinic input]
_sqlite3.complete_statement as pysqlite_complete_statement

    statement: str

Checks if a string contains a complete SQL statement. Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_complete_statement_impl(PyObject *module, const char *statement)
/*[clinic end generated code: output=e55f1ff1952df558 input=f6b24996b31c5c33]*/
{
    if (sqlite3_complete(statement)) {
        return Py_NewRef(Py_True);
    } else {
        return Py_NewRef(Py_False);
    }
}

/*[clinic input]
_sqlite3.enable_shared_cache as pysqlite_enable_shared_cache

    do_enable: int

Enable or disable shared cache mode for the calling thread.

Experimental/Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_enable_shared_cache_impl(PyObject *module, int do_enable)
/*[clinic end generated code: output=259c74eedee1516b input=8400e41bc58b6b24]*/
{
    int rc;

    rc = sqlite3_enable_shared_cache(do_enable);

    if (rc != SQLITE_OK) {
        PyErr_SetString(pysqlite_OperationalError, "Changing the shared_cache flag failed");
        return NULL;
    } else {
        Py_RETURN_NONE;
    }
}

/*[clinic input]
_sqlite3.register_adapter as pysqlite_register_adapter

    type: object(type='PyTypeObject *')
    caster: object
    /

Registers an adapter with pysqlite's adapter registry. Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_register_adapter_impl(PyObject *module, PyTypeObject *type,
                               PyObject *caster)
/*[clinic end generated code: output=a287e8db18e8af23 input=839dad90e2492725]*/
{
    int rc;

    /* a basic type is adapted; there's a performance optimization if that's not the case
     * (99 % of all usages) */
    if (type == &PyLong_Type || type == &PyFloat_Type
            || type == &PyUnicode_Type || type == &PyByteArray_Type) {
        pysqlite_BaseTypeAdapted = 1;
    }

    rc = pysqlite_microprotocols_add(type, (PyObject*)pysqlite_PrepareProtocolType, caster);
    if (rc == -1)
        return NULL;

    Py_RETURN_NONE;
}

/*[clinic input]
_sqlite3.register_converter as pysqlite_register_converter

    name as orig_name: unicode
    converter as callable: object
    /

Registers a converter with pysqlite. Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_register_converter_impl(PyObject *module, PyObject *orig_name,
                                 PyObject *callable)
/*[clinic end generated code: output=a2f2bfeed7230062 input=e074cf7f4890544f]*/
{
    PyObject* name = NULL;
    PyObject* retval = NULL;
    _Py_IDENTIFIER(upper);

    /* convert the name to upper case */
    name = _PyObject_CallMethodIdNoArgs(orig_name, &PyId_upper);
    if (!name) {
        goto error;
    }

    if (PyDict_SetItem(_pysqlite_converters, name, callable) != 0) {
        goto error;
    }

    retval = Py_NewRef(Py_None);
error:
    Py_XDECREF(name);
    return retval;
}

/*[clinic input]
_sqlite3.enable_callback_tracebacks as pysqlite_enable_callback_trace

    enable: int
    /

Enable or disable callback functions throwing errors to stderr.
[clinic start generated code]*/

static PyObject *
pysqlite_enable_callback_trace_impl(PyObject *module, int enable)
/*[clinic end generated code: output=4ff1d051c698f194 input=cb79d3581eb77c40]*/
{
    _pysqlite_enable_callback_tracebacks = enable;

    Py_RETURN_NONE;
}

/*[clinic input]
_sqlite3.adapt as pysqlite_adapt

    obj: object
    proto: object(c_default='(PyObject*)pysqlite_PrepareProtocolType') = PrepareProtocolType
    alt: object = NULL
    /

Adapt given object to given protocol. Non-standard.
[clinic start generated code]*/

static PyObject *
pysqlite_adapt_impl(PyObject *module, PyObject *obj, PyObject *proto,
                    PyObject *alt)
/*[clinic end generated code: output=0c3927c5fcd23dd9 input=a58ab77fb5ae22dd]*/
{
    return pysqlite_microprotocols_adapt(obj, proto, alt);
}

static int converters_init(PyObject* module)
{
    _pysqlite_converters = PyDict_New();
    if (!_pysqlite_converters) {
        return -1;
    }

    int res = PyModule_AddObjectRef(module, "converters", _pysqlite_converters);
    Py_DECREF(_pysqlite_converters);

    return res;
}

static int
load_functools_lru_cache(PyObject *module)
{
    PyObject *functools = PyImport_ImportModule("functools");
    if (functools == NULL) {
        return -1;
    }

    pysqlite_state *state = pysqlite_get_state(module);
    state->lru_cache = PyObject_GetAttrString(functools, "lru_cache");
    Py_DECREF(functools);
    if (state->lru_cache == NULL) {
        return -1;
    }
    return 0;
}

static PyMethodDef module_methods[] = {
    {"connect",  (PyCFunction)(void(*)(void))module_connect,
     METH_VARARGS | METH_KEYWORDS, module_connect_doc},
    PYSQLITE_ADAPT_METHODDEF
    PYSQLITE_COMPLETE_STATEMENT_METHODDEF
    PYSQLITE_ENABLE_CALLBACK_TRACE_METHODDEF
    PYSQLITE_ENABLE_SHARED_CACHE_METHODDEF
    PYSQLITE_REGISTER_ADAPTER_METHODDEF
    PYSQLITE_REGISTER_CONVERTER_METHODDEF
    {NULL, NULL}
};

static int add_integer_constants(PyObject *module) {
    int ret = 0;

    ret += PyModule_AddIntMacro(module, PARSE_DECLTYPES);
    ret += PyModule_AddIntMacro(module, PARSE_COLNAMES);
    ret += PyModule_AddIntMacro(module, SQLITE_OK);
    ret += PyModule_AddIntMacro(module, SQLITE_DENY);
    ret += PyModule_AddIntMacro(module, SQLITE_IGNORE);
    ret += PyModule_AddIntMacro(module, SQLITE_CREATE_INDEX);
    ret += PyModule_AddIntMacro(module, SQLITE_CREATE_TABLE);
    ret += PyModule_AddIntMacro(module, SQLITE_CREATE_TEMP_INDEX);
    ret += PyModule_AddIntMacro(module, SQLITE_CREATE_TEMP_TABLE);
    ret += PyModule_AddIntMacro(module, SQLITE_CREATE_TEMP_TRIGGER);
    ret += PyModule_AddIntMacro(module, SQLITE_CREATE_TEMP_VIEW);
    ret += PyModule_AddIntMacro(module, SQLITE_CREATE_TRIGGER);
    ret += PyModule_AddIntMacro(module, SQLITE_CREATE_VIEW);
    ret += PyModule_AddIntMacro(module, SQLITE_DELETE);
    ret += PyModule_AddIntMacro(module, SQLITE_DROP_INDEX);
    ret += PyModule_AddIntMacro(module, SQLITE_DROP_TABLE);
    ret += PyModule_AddIntMacro(module, SQLITE_DROP_TEMP_INDEX);
    ret += PyModule_AddIntMacro(module, SQLITE_DROP_TEMP_TABLE);
    ret += PyModule_AddIntMacro(module, SQLITE_DROP_TEMP_TRIGGER);
    ret += PyModule_AddIntMacro(module, SQLITE_DROP_TEMP_VIEW);
    ret += PyModule_AddIntMacro(module, SQLITE_DROP_TRIGGER);
    ret += PyModule_AddIntMacro(module, SQLITE_DROP_VIEW);
    ret += PyModule_AddIntMacro(module, SQLITE_INSERT);
    ret += PyModule_AddIntMacro(module, SQLITE_PRAGMA);
    ret += PyModule_AddIntMacro(module, SQLITE_READ);
    ret += PyModule_AddIntMacro(module, SQLITE_SELECT);
    ret += PyModule_AddIntMacro(module, SQLITE_TRANSACTION);
    ret += PyModule_AddIntMacro(module, SQLITE_UPDATE);
    ret += PyModule_AddIntMacro(module, SQLITE_ATTACH);
    ret += PyModule_AddIntMacro(module, SQLITE_DETACH);
    ret += PyModule_AddIntMacro(module, SQLITE_ALTER_TABLE);
    ret += PyModule_AddIntMacro(module, SQLITE_REINDEX);
    ret += PyModule_AddIntMacro(module, SQLITE_ANALYZE);
    ret += PyModule_AddIntMacro(module, SQLITE_CREATE_VTABLE);
    ret += PyModule_AddIntMacro(module, SQLITE_DROP_VTABLE);
    ret += PyModule_AddIntMacro(module, SQLITE_FUNCTION);
    ret += PyModule_AddIntMacro(module, SQLITE_SAVEPOINT);
#if SQLITE_VERSION_NUMBER >= 3008003
    ret += PyModule_AddIntMacro(module, SQLITE_RECURSIVE);
#endif
    ret += PyModule_AddIntMacro(module, SQLITE_DONE);
    return ret;
}

static struct PyModuleDef _sqlite3module = {
        PyModuleDef_HEAD_INIT,
        "_sqlite3",
        NULL,
        -1,
        module_methods,
        NULL,
        NULL,
        NULL,
        NULL
};

#define ADD_TYPE(module, type)                 \
do {                                           \
    if (PyModule_AddType(module, &type) < 0) { \
        goto error;                            \
    }                                          \
} while (0)

#define ADD_EXCEPTION(module, name, exc, base)                  \
do {                                                            \
    exc = PyErr_NewException(MODULE_NAME "." name, base, NULL); \
    if (!exc) {                                                 \
        goto error;                                             \
    }                                                           \
    int res = PyModule_AddObjectRef(module, name, exc);         \
    Py_DECREF(exc);                                             \
    if (res < 0) {                                              \
        goto error;                                             \
    }                                                           \
} while (0)

PyMODINIT_FUNC PyInit__sqlite3(void)
{
    PyObject *module;

    if (sqlite3_libversion_number() < 3007015) {
        PyErr_SetString(PyExc_ImportError, MODULE_NAME ": SQLite 3.7.15 or higher required");
        return NULL;
    }

    int rc = sqlite3_initialize();
    if (rc != SQLITE_OK) {
        PyErr_SetString(PyExc_ImportError, sqlite3_errstr(rc));
        return NULL;
    }

    module = PyModule_Create(&_sqlite3module);

    if (!module ||
        (pysqlite_row_setup_types(module) < 0) ||
        (pysqlite_cursor_setup_types(module) < 0) ||
        (pysqlite_connection_setup_types(module) < 0) ||
        (pysqlite_statement_setup_types(module) < 0) ||
        (pysqlite_prepare_protocol_setup_types(module) < 0)
       ) {
        goto error;
    }

    ADD_TYPE(module, *pysqlite_ConnectionType);
    ADD_TYPE(module, *pysqlite_CursorType);
    ADD_TYPE(module, *pysqlite_PrepareProtocolType);
    ADD_TYPE(module, *pysqlite_RowType);

    /*** Create DB-API Exception hierarchy */
    ADD_EXCEPTION(module, "Error", pysqlite_Error, PyExc_Exception);
    ADD_EXCEPTION(module, "Warning", pysqlite_Warning, PyExc_Exception);

    /* Error subclasses */
    ADD_EXCEPTION(module, "InterfaceError", pysqlite_InterfaceError, pysqlite_Error);
    ADD_EXCEPTION(module, "DatabaseError", pysqlite_DatabaseError, pysqlite_Error);

    /* pysqlite_DatabaseError subclasses */
    ADD_EXCEPTION(module, "InternalError", pysqlite_InternalError, pysqlite_DatabaseError);
    ADD_EXCEPTION(module, "OperationalError", pysqlite_OperationalError, pysqlite_DatabaseError);
    ADD_EXCEPTION(module, "ProgrammingError", pysqlite_ProgrammingError, pysqlite_DatabaseError);
    ADD_EXCEPTION(module, "IntegrityError", pysqlite_IntegrityError, pysqlite_DatabaseError);
    ADD_EXCEPTION(module, "DataError", pysqlite_DataError, pysqlite_DatabaseError);
    ADD_EXCEPTION(module, "NotSupportedError", pysqlite_NotSupportedError, pysqlite_DatabaseError);

    /* Set integer constants */
    if (add_integer_constants(module) < 0) {
        goto error;
    }

    if (PyModule_AddStringConstant(module, "version", PYSQLITE_VERSION) < 0) {
        goto error;
    }

    if (PyModule_AddStringConstant(module, "sqlite_version", sqlite3_libversion())) {
        goto error;
    }

    /* initialize microprotocols layer */
    if (pysqlite_microprotocols_init(module) < 0) {
        goto error;
    }

    /* initialize the default converters */
    if (converters_init(module) < 0) {
        goto error;
    }

    if (load_functools_lru_cache(module) < 0) {
        goto error;
    }

    return module;

error:
    sqlite3_shutdown();
    PyErr_SetString(PyExc_ImportError, MODULE_NAME ": init failed");
    Py_XDECREF(module);
    return NULL;
}
