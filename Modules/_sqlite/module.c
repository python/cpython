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
#include "cache.h"
#include "prepare_protocol.h"
#include "microprotocols.h"
#include "row.h"

#if SQLITE_VERSION_NUMBER < 3007003
#error "SQLite 3.7.3 or higher required"
#endif

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

    PyObject* result;

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

    if (PySys_Audit("sqlite3.connect", "O", database) < 0) {
        return NULL;
    }

    result = PyObject_Call(factory, args, kwargs);

    return result;
}

PyDoc_STRVAR(module_connect_doc,
"connect(database[, timeout, detect_types, isolation_level,\n\
        check_same_thread, factory, cached_statements, uri])\n\
\n\
Opens a connection to the SQLite database file *database*. You can use\n\
\":memory:\" to open a database connection to a database that resides in\n\
RAM instead of on disk.");

static PyObject* module_complete(PyObject* self, PyObject* args, PyObject*
        kwargs)
{
    static char *kwlist[] = {"statement", NULL};
    char* statement;

    PyObject* result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &statement))
    {
        return NULL;
    }

    if (sqlite3_complete(statement)) {
        result = Py_True;
    } else {
        result = Py_False;
    }

    Py_INCREF(result);

    return result;
}

PyDoc_STRVAR(module_complete_doc,
"complete_statement(sql)\n\
\n\
Checks if a string contains a complete SQL statement. Non-standard.");

static PyObject* module_enable_shared_cache(PyObject* self, PyObject* args, PyObject*
        kwargs)
{
    static char *kwlist[] = {"do_enable", NULL};
    int do_enable;
    int rc;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, &do_enable))
    {
        return NULL;
    }

    rc = sqlite3_enable_shared_cache(do_enable);

    if (rc != SQLITE_OK) {
        PyErr_SetString(pysqlite_OperationalError, "Changing the shared_cache flag failed");
        return NULL;
    } else {
        Py_RETURN_NONE;
    }
}

PyDoc_STRVAR(module_enable_shared_cache_doc,
"enable_shared_cache(do_enable)\n\
\n\
Enable or disable shared cache mode for the calling thread.\n\
Experimental/Non-standard.");

static PyObject* module_register_adapter(PyObject* self, PyObject* args)
{
    PyTypeObject* type;
    PyObject* caster;
    int rc;

    if (!PyArg_ParseTuple(args, "OO", &type, &caster)) {
        return NULL;
    }

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

PyDoc_STRVAR(module_register_adapter_doc,
"register_adapter(type, callable)\n\
\n\
Registers an adapter with pysqlite's adapter registry. Non-standard.");

static PyObject* module_register_converter(PyObject* self, PyObject* args)
{
    PyObject* orig_name;
    PyObject* name = NULL;
    PyObject* callable;
    PyObject* retval = NULL;
    _Py_IDENTIFIER(upper);

    if (!PyArg_ParseTuple(args, "UO", &orig_name, &callable)) {
        return NULL;
    }

    /* convert the name to upper case */
    name = _PyObject_CallMethodIdNoArgs(orig_name, &PyId_upper);
    if (!name) {
        goto error;
    }

    if (PyDict_SetItem(_pysqlite_converters, name, callable) != 0) {
        goto error;
    }

    Py_INCREF(Py_None);
    retval = Py_None;
error:
    Py_XDECREF(name);
    return retval;
}

PyDoc_STRVAR(module_register_converter_doc,
"register_converter(typename, callable)\n\
\n\
Registers a converter with pysqlite. Non-standard.");

static PyObject* enable_callback_tracebacks(PyObject* self, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "i", &_pysqlite_enable_callback_tracebacks)) {
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(enable_callback_tracebacks_doc,
"enable_callback_tracebacks(flag)\n\
\n\
Enable or disable callback functions throwing errors to stderr.");

static void converters_init(PyObject* module)
{
    _pysqlite_converters = PyDict_New();
    if (!_pysqlite_converters) {
        return;
    }

    if (PyModule_AddObject(module, "converters", _pysqlite_converters) < 0) {
        Py_DECREF(_pysqlite_converters);
    }
    return;
}

static PyMethodDef module_methods[] = {
    {"connect",  (PyCFunction)(void(*)(void))module_connect,
     METH_VARARGS | METH_KEYWORDS, module_connect_doc},
    {"complete_statement",  (PyCFunction)(void(*)(void))module_complete,
     METH_VARARGS | METH_KEYWORDS, module_complete_doc},
    {"enable_shared_cache",  (PyCFunction)(void(*)(void))module_enable_shared_cache,
     METH_VARARGS | METH_KEYWORDS, module_enable_shared_cache_doc},
    {"register_adapter", (PyCFunction)module_register_adapter,
     METH_VARARGS, module_register_adapter_doc},
    {"register_converter", (PyCFunction)module_register_converter,
     METH_VARARGS, module_register_converter_doc},
    {"adapt",  (PyCFunction)pysqlite_adapt, METH_VARARGS,
     pysqlite_adapt_doc},
    {"enable_callback_tracebacks",  (PyCFunction)enable_callback_tracebacks,
     METH_VARARGS, enable_callback_tracebacks_doc},
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
        Py_DECREF(module);                     \
        return NULL;                           \
    }                                          \
} while (0)

#define ADD_EXCEPTION(module, name, exc, base)                  \
do {                                                            \
    exc = PyErr_NewException(MODULE_NAME "." name, base, NULL); \
    if (!exc) {                                                 \
        goto error;                                             \
    }                                                           \
    if (PyModule_AddObject(module, name, exc) < 0) {            \
        Py_DECREF(exc);                                         \
        goto error;                                             \
    }                                                           \
} while (0)

PyMODINIT_FUNC PyInit__sqlite3(void)
{
    PyObject *module;

    if (sqlite3_libversion_number() < 3007003) {
        PyErr_SetString(PyExc_ImportError, MODULE_NAME ": SQLite 3.7.3 or higher required");
        return NULL;
    }

    module = PyModule_Create(&_sqlite3module);

    if (!module ||
        (pysqlite_row_setup_types(module) < 0) ||
        (pysqlite_cursor_setup_types(module) < 0) ||
        (pysqlite_connection_setup_types(module) < 0) ||
        (pysqlite_cache_setup_types(module) < 0) ||
        (pysqlite_statement_setup_types(module) < 0) ||
        (pysqlite_prepare_protocol_setup_types(module) < 0)
       ) {
        Py_XDECREF(module);
        return NULL;
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

    /* In Python 2.x, setting Connection.text_factory to
       OptimizedUnicode caused Unicode objects to be returned for
       non-ASCII data and bytestrings to be returned for ASCII data.
       Now OptimizedUnicode is an alias for str, so it has no
       effect. */
    Py_INCREF((PyObject*)&PyUnicode_Type);
    if (PyModule_AddObject(module, "OptimizedUnicode", (PyObject*)&PyUnicode_Type) < 0) {
        Py_DECREF((PyObject*)&PyUnicode_Type);
        goto error;
    }

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
    converters_init(module);

error:
    if (PyErr_Occurred())
    {
        PyErr_SetString(PyExc_ImportError, MODULE_NAME ": init failed");
        Py_DECREF(module);
        module = NULL;
    }
    return module;
}
