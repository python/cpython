/*
 * Provide the implementation of the high-level matcher-based functions.
 */

#include "Python.h"

PyObject *
_Py_fnmatch_filter(PyObject *matcher, PyObject *names, PyObject *normcase)
{
    assert(normcase != NULL);
    PyObject *iter = PyObject_GetIter(names);
    if (iter == NULL) {
        return NULL;
    }
    PyObject *res = PyList_New(0);
    if (res == NULL) {
        Py_DECREF(iter);
        return NULL;
    }
    PyObject *name = NULL;
    while ((name = PyIter_Next(iter))) {
        PyObject *normalized = PyObject_CallOneArg(normcase, name);
        if (normalized == NULL) {
            goto abort;
        }
        PyObject *match = PyObject_CallOneArg(matcher, normalized);
        Py_DECREF(normalized);
        if (match == NULL) {
            goto abort;
        }
        int matching = Py_IsNone(match) == 0;
        Py_DECREF(match);
        if (matching && PyList_Append(res, name) < 0) {
            goto abort;
        }
        Py_DECREF(name);
    }
    Py_DECREF(iter);
    if (PyErr_Occurred()) {
        Py_CLEAR(res);
    }
    return res;
abort:
    Py_DECREF(name);
    Py_DECREF(iter);
    Py_DECREF(res);
    return NULL;
}
