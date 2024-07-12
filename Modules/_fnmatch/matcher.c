/*
 * Provide the implementation of the high-level matcher-based functions.
 */

#include "_fnmatchmodule.h"

inline int
_Py_fnmatch_match(PyObject *matcher, PyObject *name)
{
    // If 'name' is of incorrect type, it will be detected when calling
    // the matcher function (we emulate 're.compile(...).match(name)').
    PyObject *match = PyObject_CallOneArg(matcher, name);
    if (match == NULL) {
        return -1;
    }
    int matching = Py_IsNone(match) ? 0 : 1;
    Py_DECREF(match);
    return matching;
}

PyObject *
_Py_fnmatch_filter(PyObject *matcher, PyObject *names)
{
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
        int matching = _Py_fnmatch_match(matcher, name);
        if (matching < 0 || (matching == 1 && PyList_Append(res, name) < 0)) {
            goto abort;
        }
        Py_DECREF(name);
    }
    Py_DECREF(iter);
    return res;
abort:
    Py_DECREF(name);
    Py_DECREF(iter);
    Py_DECREF(res);
    return NULL;
}

PyObject *
_Py_fnmatch_filter_normalized(PyObject *matcher, PyObject *names, PyObject *normcase)
{
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
        int matching = _Py_fnmatch_match(matcher, normalized);
        Py_DECREF(normalized);
        // add the non-normalized name if its normalization matches
        if (matching < 0 || (matching == 1 && PyList_Append(res, name) < 0)) {
            goto abort;
        }
        Py_DECREF(name);
    }
    Py_DECREF(iter);
    return res;
abort:
    Py_DECREF(name);
    Py_DECREF(iter);
    Py_DECREF(res);
    return NULL;
}
