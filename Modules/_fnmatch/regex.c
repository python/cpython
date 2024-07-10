#include "Python.h"

#include "_fnmatchmodule.h" // for pre-declarations

// ==== API implementation ====================================================

inline int
_Py_regex_fnmatch_generic(PyObject *matcher, PyObject *name)
{
    // If 'name' is of incorrect type, it will be detected when calling
    // the matcher function (we emulate 're.compile(...).match(name)').
    assert(PyCallable_Check(matcher));
    PyObject *match = PyObject_CallFunction(matcher, "O", name);
    if (match == NULL) {
        return -1;
    }
    int matching = match == Py_None ? 0 : 1;
    Py_DECREF(match);
    return matching;
}

PyObject *
_Py_regex_fnmatch_filter(PyObject *matcher, PyObject *names)
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
        int rc = _Py_regex_fnmatch_generic(matcher, name);
        if (rc < 0) {
            assert(PyErr_Occurred());
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
