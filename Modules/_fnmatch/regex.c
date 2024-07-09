#include "Python.h"

/*
 * Perform a case-sensitive match using regular expressions.
 *
 * Parameters
 *
 *      pattern     A translated regular expression.
 *      name        The filename to match.
 *
 * Returns 1 if the 'name' matches the 'pattern' and 0 otherwise.
 * Returns -1 if something went wrong.
 */
int
_regex_fnmatch_generic(PyObject *matcher, PyObject *name)
{
    // If 'name' is of incorrect type, it will be detected when calling
    // the matcher function (we emulate 're.compile(...).match(name)').
    PyObject *match = PyObject_CallFunction(matcher, "O", name);
    if (match == NULL) {
        return -1;
    }
    int matching = match != Py_None;
    Py_DECREF(match);
    return matching;
}

PyObject *
_regex_fnmatch_filter(PyObject *matcher, PyObject *names)
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
        int rc = _regex_fnmatch_generic(matcher, name);
        if (rc < 0) {
            goto abort;
        }
        if (rc == 1) {
            if (PyList_Append(res, name) < 0) {
                goto abort;
            }
        }
        Py_DECREF(name);
        if (PyErr_Occurred()) {
            Py_DECREF(res);
            Py_DECREF(iter);
            return NULL;
        }
    }
    Py_DECREF(iter);
    return res;
abort:
    Py_XDECREF(name);
    Py_DECREF(iter);
    Py_DECREF(res);
    return NULL;
}
