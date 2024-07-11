#include "Python.h"

#include "_fnmatchmodule.h" // for pre-declarations

// ==== API implementation ====================================================

inline int
_Py_fnmatch_fnmatch(PyObject *matcher, PyObject *name)
{
    // If 'name' is of incorrect type, it will be detected when calling
    // the matcher function (we emulate 're.compile(...).match(name)').
    assert(PyCallable_Check(matcher));
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
    assert(PyCallable_Check(matcher));
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
        int matching = _Py_fnmatch_fnmatch(matcher, name);
        if (matching < 0) {
            assert(PyErr_Occurred());
            goto abort;
        }
        if (matching == 1) {
            if (PyList_Append(res, name) < 0) {
                goto abort;
            }
        }
        Py_DECREF(name);
        if (PyErr_Occurred()) {
            goto error;
        }
    }
    Py_DECREF(iter);
    return res;
abort:
    Py_XDECREF(name);
error:
    Py_DECREF(iter);
    Py_DECREF(res);
    return NULL;
}

PyObject *
_Py_fnmatch_filter_normalized(PyObject *matcher, PyObject *names, PyObject *normcase)
{
    assert(PyCallable_Check(matcher));
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
        int matching = _Py_fnmatch_fnmatch(matcher, normalized);
        Py_DECREF(normalized);
        if (matching < 0) {
            assert(PyErr_Occurred());
            goto abort;
        }
        if (matching == 1) {
            // add the non-normalized name if its normalization matches
            if (PyList_Append(res, name) < 0) {
                goto abort;
            }
        }
        Py_DECREF(name);
        if (PyErr_Occurred()) {
            goto error;
        }
    }
    Py_DECREF(iter);
    return res;
abort:
    Py_XDECREF(name);
error:
    Py_DECREF(iter);
    Py_DECREF(res);
    return NULL;
}
