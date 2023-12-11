/* microprotocols.c - minimalist and non-validating protocols implementation
 *
 * Copyright (C) 2003-2004 Federico Di Gregorio <fog@debian.org>
 *
 * This file is part of psycopg and was adapted for pysqlite. Federico Di
 * Gregorio gave the permission to use it within pysqlite under the following
 * license:
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

#include <Python.h>

#include "cursor.h"
#include "microprotocols.h"
#include "prepare_protocol.h"


/* pysqlite_microprotocols_init - initialize the adapters dictionary */

int
pysqlite_microprotocols_init(PyObject *module)
{
    /* create adapters dictionary and put it in module namespace */
    pysqlite_state *state = pysqlite_get_state(module);
    state->psyco_adapters = PyDict_New();
    if (state->psyco_adapters == NULL) {
        return -1;
    }

    return PyModule_AddObjectRef(module, "adapters", state->psyco_adapters);
}


/* pysqlite_microprotocols_add - add a reverse type-caster to the dictionary */

int
pysqlite_microprotocols_add(pysqlite_state *state, PyTypeObject *type,
                            PyObject *proto, PyObject *cast)
{
    PyObject* key;
    int rc;

    assert(type != NULL);
    assert(proto != NULL);
    key = PyTuple_Pack(2, (PyObject *)type, proto);
    if (!key) {
        return -1;
    }

    rc = PyDict_SetItem(state->psyco_adapters, key, cast);
    Py_DECREF(key);

    return rc;
}

/* pysqlite_microprotocols_adapt - adapt an object to the built-in protocol */

PyObject *
pysqlite_microprotocols_adapt(pysqlite_state *state, PyObject *obj,
                              PyObject *proto, PyObject *alt)
{
    PyObject *adapter, *key, *adapted;

    /* we don't check for exact type conformance as specified in PEP 246
       because the PrepareProtocolType type is abstract and there is no
       way to get a quotable object to be its instance */

    /* look for an adapter in the registry */
    key = PyTuple_Pack(2, (PyObject *)Py_TYPE(obj), proto);
    if (!key) {
        return NULL;
    }
    if (PyDict_GetItemRef(state->psyco_adapters, key, &adapter) < 0) {
        Py_DECREF(key);
        return NULL;
    }
    Py_DECREF(key);
    if (adapter) {
        adapted = PyObject_CallOneArg(adapter, obj);
        Py_DECREF(adapter);
        return adapted;
    }

    /* try to have the protocol adapt this object */
    if (PyObject_GetOptionalAttr(proto, state->str___adapt__, &adapter) < 0) {
        return NULL;
    }
    if (adapter) {
        adapted = PyObject_CallOneArg(adapter, obj);
        Py_DECREF(adapter);

        if (adapted == Py_None) {
            Py_DECREF(adapted);
        }
        else if (adapted || !PyErr_ExceptionMatches(PyExc_TypeError)) {
            return adapted;
        }
        else {
            PyErr_Clear();
        }
    }

    /* and finally try to have the object adapt itself */
    if (PyObject_GetOptionalAttr(obj, state->str___conform__, &adapter) < 0) {
        return NULL;
    }
    if (adapter) {
        adapted = PyObject_CallOneArg(adapter, proto);
        Py_DECREF(adapter);

        if (adapted == Py_None) {
            Py_DECREF(adapted);
        }
        else if (adapted || !PyErr_ExceptionMatches(PyExc_TypeError)) {
            return adapted;
        }
        else {
            PyErr_Clear();
        }
    }

    if (alt) {
        return Py_NewRef(alt);
    }
    /* else set the right exception and return NULL */
    PyErr_SetString(state->ProgrammingError, "can't adapt");
    return NULL;
}
