#ifdef Py_HAVE_FNMATCH

#include <fnmatch.h>        // for fnmatch(3)

#include "Python.h"
#include "_fnmatchmodule.h" // for PosixMatcher

#define INVALID_TYPE_FOR_NAME "name must be a %s object, got %.200s"

#define VERIFY_NAME_ARG_TYPE(name, check, expecting) \
    do { \
        if (!check) { \
            PyErr_Format(PyExc_TypeError, INVALID_TYPE_FOR_NAME, \
                         expecting, Py_TYPE(name)->tp_name); \
            return -1; \
        } \
    } while (0)

#define PROCESS_MATCH_RESULT(r) \
    do { \
        int res = (r); /* avoid variable capture */ \
        if (res < 0) { \
            return res; \
        } \
        return res != FNM_NOMATCH; \
    } while (0)

inline int
_posix_fnmatch_encoded(const char *pattern, PyObject *string)
{
    VERIFY_NAME_ARG_TYPE(string, PyBytes_Check(string), "bytes");
    PROCESS_MATCH_RESULT(fnmatch(pattern, PyBytes_AS_STRING(string), 0));
}

inline int
_posix_fnmatch_unicode(const char *pattern, PyObject *string)
{
    VERIFY_NAME_ARG_TYPE(string, PyUnicode_Check(string), "string");
    PROCESS_MATCH_RESULT(fnmatch(pattern, PyUnicode_AsUTF8(string), 0));
}

PyObject *
_posix_fnmatch_filter(const char *pattern, PyObject *names, Matcher match)
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
        int rc = match(pattern, name);
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
#endif
