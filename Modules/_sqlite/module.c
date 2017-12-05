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

#if SQLITE_VERSION_NUMBER >= 3003003
#define HAVE_SHARED_CACHE
#endif

/* static objects at module-level */

PyObject* pysqlite_Error, *pysqlite_Warning, *pysqlite_InterfaceError, *pysqlite_DatabaseError,
    *pysqlite_InternalError, *pysqlite_OperationalError, *pysqlite_ProgrammingError,
    *pysqlite_IntegrityError, *pysqlite_DataError, *pysqlite_NotSupportedError;

PyObject* converters;
int _enable_callback_tracebacks;
int pysqlite_BaseTypeAdapted;

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
        factory = (PyObject*)&pysqlite_ConnectionType;
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
    static char *kwlist[] = {"statement", NULL, NULL};
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

#ifdef HAVE_SHARED_CACHE
static PyObject* module_enable_shared_cache(PyObject* self, PyObject* args, PyObject*
        kwargs)
{
    static char *kwlist[] = {"do_enable", NULL, NULL};
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
#endif /* HAVE_SHARED_CACHE */

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

    rc = pysqlite_microprotocols_add(type, (PyObject*)&pysqlite_PrepareProtocolType, caster);
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
    name = _PyObject_CallMethodId(orig_name, &PyId_upper, NULL);
    if (!name) {
        goto error;
    }

    if (PyDict_SetItem(converters, name, callable) != 0) {
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
    if (!PyArg_ParseTuple(args, "i", &_enable_callback_tracebacks)) {
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(enable_callback_tracebacks_doc,
"enable_callback_tracebacks(flag)\n\
\n\
Enable or disable callback functions throwing errors to stderr.");

static void converters_init(PyObject* dict)
{
    converters = PyDict_New();
    if (!converters) {
        return;
    }

    PyDict_SetItemString(dict, "converters", converters);
}

static PyMethodDef module_methods[] = {
    {"connect",  (PyCFunction)module_connect,
     METH_VARARGS | METH_KEYWORDS, module_connect_doc},
    {"complete_statement",  (PyCFunction)module_complete,
     METH_VARARGS | METH_KEYWORDS, module_complete_doc},
#ifdef HAVE_SHARED_CACHE
    {"enable_shared_cache",  (PyCFunction)module_enable_shared_cache,
     METH_VARARGS | METH_KEYWORDS, module_enable_shared_cache_doc},
#endif
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

struct _IntConstantPair {
    const char *constant_name;
    int constant_value;
};

typedef struct _IntConstantPair IntConstantPair;

static const IntConstantPair _int_constants[] = {
    {"PARSE_DECLTYPES", PARSE_DECLTYPES},
    {"PARSE_COLNAMES", PARSE_COLNAMES},

    {"SQLITE_OK", SQLITE_OK},
    {"SQLITE_DENY", SQLITE_DENY},
    {"SQLITE_IGNORE", SQLITE_IGNORE},
    {"SQLITE_CREATE_INDEX", SQLITE_CREATE_INDEX},
    {"SQLITE_CREATE_TABLE", SQLITE_CREATE_TABLE},
    {"SQLITE_CREATE_TEMP_INDEX", SQLITE_CREATE_TEMP_INDEX},
    {"SQLITE_CREATE_TEMP_TABLE", SQLITE_CREATE_TEMP_TABLE},
    {"SQLITE_CREATE_TEMP_TRIGGER", SQLITE_CREATE_TEMP_TRIGGER},
    {"SQLITE_CREATE_TEMP_VIEW", SQLITE_CREATE_TEMP_VIEW},
    {"SQLITE_CREATE_TRIGGER", SQLITE_CREATE_TRIGGER},
    {"SQLITE_CREATE_VIEW", SQLITE_CREATE_VIEW},
    {"SQLITE_DELETE", SQLITE_DELETE},
    {"SQLITE_DROP_INDEX", SQLITE_DROP_INDEX},
    {"SQLITE_DROP_TABLE", SQLITE_DROP_TABLE},
    {"SQLITE_DROP_TEMP_INDEX", SQLITE_DROP_TEMP_INDEX},
    {"SQLITE_DROP_TEMP_TABLE", SQLITE_DROP_TEMP_TABLE},
    {"SQLITE_DROP_TEMP_TRIGGER", SQLITE_DROP_TEMP_TRIGGER},
    {"SQLITE_DROP_TEMP_VIEW", SQLITE_DROP_TEMP_VIEW},
    {"SQLITE_DROP_TRIGGER", SQLITE_DROP_TRIGGER},
    {"SQLITE_DROP_VIEW", SQLITE_DROP_VIEW},
    {"SQLITE_INSERT", SQLITE_INSERT},
    {"SQLITE_PRAGMA", SQLITE_PRAGMA},
    {"SQLITE_READ", SQLITE_READ},
    {"SQLITE_SELECT", SQLITE_SELECT},
    {"SQLITE_TRANSACTION", SQLITE_TRANSACTION},
    {"SQLITE_UPDATE", SQLITE_UPDATE},
    {"SQLITE_ATTACH", SQLITE_ATTACH},
    {"SQLITE_DETACH", SQLITE_DETACH},
#if SQLITE_VERSION_NUMBER >= 3002001
    {"SQLITE_ALTER_TABLE", SQLITE_ALTER_TABLE},
    {"SQLITE_REINDEX", SQLITE_REINDEX},
#endif
#if SQLITE_VERSION_NUMBER >= 3003000
    {"SQLITE_ANALYZE", SQLITE_ANALYZE},
#endif
#if SQLITE_VERSION_NUMBER >= 3003007
    {"SQLITE_CREATE_VTABLE", SQLITE_CREATE_VTABLE},
    {"SQLITE_DROP_VTABLE", SQLITE_DROP_VTABLE},
#endif
#if SQLITE_VERSION_NUMBER >= 3003008
    {"SQLITE_FUNCTION", SQLITE_FUNCTION},
#endif
#if SQLITE_VERSION_NUMBER >= 3006008
    {"SQLITE_SAVEPOINT", SQLITE_SAVEPOINT},
#endif
#if SQLITE_VERSION_NUMBER >= 3008003
    {"SQLITE_RECURSIVE", SQLITE_RECURSIVE},
#endif
    {(char*)NULL, 0}
};


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

PyMODINIT_FUNC PyInit__sqlite3(void)
{
    PyObject *module, *dict;
    PyObject *tmp_obj;
    int i;

    module = PyModule_Create(&_sqlite3module);

    if (!module ||
        (pysqlite_row_setup_types() < 0) ||
        (pysqlite_cursor_setup_types() < 0) ||
        (pysqlite_connection_setup_types() < 0) ||
        (pysqlite_cache_setup_types() < 0) ||
        (pysqlite_statement_setup_types() < 0) ||
        (pysqlite_prepare_protocol_setup_types() < 0)
       ) {
        Py_XDECREF(module);
        return NULL;
    }

    Py_INCREF(&pysqlite_ConnectionType);
    PyModule_AddObject(module, "Connection", (PyObject*) &pysqlite_ConnectionType);
    Py_INCREF(&pysqlite_CursorType);
    PyModule_AddObject(module, "Cursor", (PyObject*) &pysqlite_CursorType);
    Py_INCREF(&pysqlite_CacheType);
    PyModule_AddObject(module, "Statement", (PyObject*)&pysqlite_StatementType);
    Py_INCREF(&pysqlite_StatementType);
    PyModule_AddObject(module, "Cache", (PyObject*) &pysqlite_CacheType);
    Py_INCREF(&pysqlite_PrepareProtocolType);
    PyModule_AddObject(module, "PrepareProtocol", (PyObject*) &pysqlite_PrepareProtocolType);
    Py_INCREF(&pysqlite_RowType);
    PyModule_AddObject(module, "Row", (PyObject*) &pysqlite_RowType);

    if (!(dict = PyModule_GetDict(module))) {
        goto error;
    }

    /*** Create DB-API Exception hierarchy */

    if (!(pysqlite_Error = PyErr_NewException(MODULE_NAME ".Error", PyExc_Exception, NULL))) {
        goto error;
    }
    PyDict_SetItemString(dict, "Error", pysqlite_Error);

    if (!(pysqlite_Warning = PyErr_NewException(MODULE_NAME ".Warning", PyExc_Exception, NULL))) {
        goto error;
    }
    PyDict_SetItemString(dict, "Warning", pysqlite_Warning);

    /* Error subclasses */

    if (!(pysqlite_InterfaceError = PyErr_NewException(MODULE_NAME ".InterfaceError", pysqlite_Error, NULL))) {
        goto error;
    }
    PyDict_SetItemString(dict, "InterfaceError", pysqlite_InterfaceError);

    if (!(pysqlite_DatabaseError = PyErr_NewException(MODULE_NAME ".DatabaseError", pysqlite_Error, NULL))) {
        goto error;
    }
    PyDict_SetItemString(dict, "DatabaseError", pysqlite_DatabaseError);

    /* pysqlite_DatabaseError subclasses */

    if (!(pysqlite_InternalError = PyErr_NewException(MODULE_NAME ".InternalError", pysqlite_DatabaseError, NULL))) {
        goto error;
    }
    PyDict_SetItemString(dict, "InternalError", pysqlite_InternalError);

    if (!(pysqlite_OperationalError = PyErr_NewException(MODULE_NAME ".OperationalError", pysqlite_DatabaseError, NULL))) {
        goto error;
    }
    PyDict_SetItemString(dict, "OperationalError", pysqlite_OperationalError);

    if (!(pysqlite_ProgrammingError = PyErr_NewException(MODULE_NAME ".ProgrammingError", pysqlite_DatabaseError, NULL))) {
        goto error;
    }
    PyDict_SetItemString(dict, "ProgrammingError", pysqlite_ProgrammingError);

    if (!(pysqlite_IntegrityError = PyErr_NewException(MODULE_NAME ".IntegrityError", pysqlite_DatabaseError,NULL))) {
        goto error;
    }
    PyDict_SetItemString(dict, "IntegrityError", pysqlite_IntegrityError);

    if (!(pysqlite_DataError = PyErr_NewException(MODULE_NAME ".DataError", pysqlite_DatabaseError, NULL))) {
        goto error;
    }
    PyDict_SetItemString(dict, "DataError", pysqlite_DataError);

    if (!(pysqlite_NotSupportedError = PyErr_NewException(MODULE_NAME ".NotSupportedError", pysqlite_DatabaseError, NULL))) {
        goto error;
    }
    PyDict_SetItemString(dict, "NotSupportedError", pysqlite_NotSupportedError);

    /* In Python 2.x, setting Connection.text_factory to
       OptimizedUnicode caused Unicode objects to be returned for
       non-ASCII data and bytestrings to be returned for ASCII data.
       Now OptimizedUnicode is an alias for str, so it has no
       effect. */
    Py_INCREF((PyObject*)&PyUnicode_Type);
    PyDict_SetItemString(dict, "OptimizedUnicode", (PyObject*)&PyUnicode_Type);

    /* Set integer constants */
    for (i = 0; _int_constants[i].constant_name != NULL; i++) {
        tmp_obj = PyLong_FromLong(_int_constants[i].constant_value);
        if (!tmp_obj) {
            goto error;
        }
        PyDict_SetItemString(dict, _int_constants[i].constant_name, tmp_obj);
        Py_DECREF(tmp_obj);
    }

    if (!(tmp_obj = PyUnicode_FromString(PYSQLITE_VERSION))) {
        goto error;
    }
    PyDict_SetItemString(dict, "version", tmp_obj);
    Py_DECREF(tmp_obj);

    if (!(tmp_obj = PyUnicode_FromString(sqlite3_libversion()))) {
        goto error;
    }
    PyDict_SetItemString(dict, "sqlite_version", tmp_obj);
    Py_DECREF(tmp_obj);

    /* initialize microprotocols layer */
    pysqlite_microprotocols_init(dict);

    /* initialize the default converters */
    converters_init(dict);

    _enable_callback_tracebacks = 0;

    pysqlite_BaseTypeAdapted = 0;

error:
    if (PyErr_Occurred())
    {
        PyErr_SetString(PyExc_ImportError, MODULE_NAME ": init failed");
        Py_DECREF(module);
        module = NULL;
    }
    return module;
}
