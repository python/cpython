/*
 * C accelerator for the 'fnmatch' module.
 *
 * Most functions expect string or bytes instances, and thus the Python
 * implementation should first pre-process path-like objects, and possibly
 * applying normalizations depending on the platform if needed.
 */

#include "Python.h"

#include "clinic/_fnmatchmodule.c.h"

/*[clinic input]
module _fnmatch
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=356e324d57d93f08]*/

#include <fnmatch.h>

static inline int
validate_encoded_object(PyObject *name)
{
    if (!PyBytes_Check(name)) {
        PyErr_Format(PyExc_TypeError,
                     "name must be a bytes object, got %.200s",
                     Py_TYPE(name)->tp_name);
        return 0;
    }
    return 1;
}

static inline int
validate_unicode_object(PyObject *name)
{
    if (!PyUnicode_Check(name)) {
        PyErr_Format(PyExc_TypeError,
                     "name must be a string object, got %.200s",
                     Py_TYPE(name)->tp_name);
        return 0;
    }
    return 1;
}

static inline int
posix_fnmatch_encoded(const char *pattern, PyObject *name)
{
    if (!validate_encoded_object(name)) {
        return -1;
    }
    // case-insensitive match
#ifdef FNM_CASEFOLD
    return fnmatch(pattern, PyBytes_AS_STRING(name), FNM_CASEFOLD) == 0;
#else
    // todo: fallback to Python implementation
    return -1;
#endif
}

static inline int
posix_fnmatchcase_encoded(const char *pattern, PyObject *name)
{
    if (!validate_encoded_object(name)) {
        return -1;
    }
    // case-sensitive match
    return fnmatch(pattern, PyBytes_AS_STRING(name), 0) == 0;
}

static inline int
posix_fnmatch_unicode(const char *pattern, PyObject *name)
{
    if (!validate_unicode_object(name)) {
        return -1;
    }
    // case-insensitive match
#ifdef FNM_CASEFOLD
    return fnmatch(pattern, PyUnicode_AsUTF8(name), FNM_CASEFOLD) == 0;
#else
    // todo: fallback to Python implementation
    return -1;
#endif
}

static inline int
posix_fnmatchcase_unicode(const char *pattern, PyObject *name)
{
    if (!validate_unicode_object(name)) {
        return -1;
    }
    // case-sensitive match
    return fnmatch(pattern, PyUnicode_AsUTF8(name), 0) == 0;
}

static PyObject *
_fnmatch_filter_generic_impl(PyObject *module,
                             PyObject *names,
                             const char *pattern,
                             int (*match)(const char *, PyObject *))
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
    Py_DECREF(name);
    Py_DECREF(iter);
    Py_DECREF(res);
    return NULL;
}

/*[clinic input]
_fnmatch.filter -> object

    names: object
    pat: object

[clinic start generated code]*/

static PyObject *
_fnmatch_filter_impl(PyObject *module, PyObject *names, PyObject *pat)
/*[clinic end generated code: output=7f11aa68436d05fc input=1d233174e1c4157a]*/
{
    // todo: handle os.path.normcase(...)
    if (PyBytes_Check(pat)) {
        const char *pattern = PyBytes_AS_STRING(pat);
        return _fnmatch_filter_generic_impl(module, names, pattern,
                                            &posix_fnmatch_encoded);
    }
    if (PyUnicode_Check(pat)) {
        const char *pattern = PyUnicode_AsUTF8(pat);
        return _fnmatch_filter_generic_impl(module, names, pattern,
                                            &posix_fnmatch_unicode);
    }
    PyErr_Format(PyExc_TypeError, "pattern must be a string or a bytes object");
    return NULL;
}

/*[clinic input]
_fnmatch.fnmatch -> bool

    name: object
    pat: object

[clinic start generated code]*/

static int
_fnmatch_fnmatch_impl(PyObject *module, PyObject *name, PyObject *pat)
/*[clinic end generated code: output=b4cd0bd911e8bc93 input=c45e0366489540b8]*/
{
    // todo: handle os.path.normcase(...)
    if (PyBytes_Check(pat)) {
        const char *pattern = PyBytes_AS_STRING(pat);
        return posix_fnmatch_encoded(pattern, name);
    }
    if (PyUnicode_Check(pat)) {
        const char *pattern = PyUnicode_AsUTF8(pat);
        return posix_fnmatch_unicode(pattern, name);
    }
    PyErr_Format(PyExc_TypeError, "pattern must be a string or a bytes object");
    return -1;
}

/*[clinic input]
_fnmatch.fnmatchcase -> bool

    name: object
    pat: object

Test whether `name` matches `pattern`, including case.

This is a version of fnmatch() which doesn't case-normalize
its arguments.

[clinic start generated code]*/

static int
_fnmatch_fnmatchcase_impl(PyObject *module, PyObject *name, PyObject *pat)
/*[clinic end generated code: output=4d1283b1b1fc7cb8 input=b02a6a5c8c5a46e2]*/
{
    if (PyBytes_Check(pat)) {
        const char *pattern = PyBytes_AS_STRING(pat);
        return posix_fnmatchcase_encoded(pattern, name);
    }
    if (PyUnicode_Check(pat)) {
        const char *pattern = PyUnicode_AsUTF8(pat);
        return posix_fnmatchcase_unicode(pattern, name);
    }
    PyErr_Format(PyExc_TypeError, "pattern must be a string or a bytes object");
    return -1;
}

static PyMethodDef _fnmatch_methods[] = {
    _FNMATCH_FILTER_METHODDEF
    _FNMATCH_FNMATCH_METHODDEF
    _FNMATCH_FNMATCHCASE_METHODDEF
    {NULL, NULL}
};

static struct PyModuleDef_Slot _fnmatch_slots[] = {
    {0, NULL}
};

static struct PyModuleDef _fnmatchmodule = {
    PyModuleDef_HEAD_INIT,
    "_fnmatch",
    NULL,
    0,
    _fnmatch_methods,
    _fnmatch_slots,
    NULL,
    NULL,
    NULL,
};

PyMODINIT_FUNC
PyInit__fnmatch(void)
{
    return PyModuleDef_Init(&_fnmatchmodule);
}
