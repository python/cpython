#include "Python.h"

#include "_fnmatchmodule.h" // for pre-declarations

#if defined(Py_HAVE_FNMATCH) && !defined(Py_USE_FNMATCH_FALLBACK)

#include <fnmatch.h>        // for fnmatch(3)

#define INVALID_PATTERN_TYPE "pattern must be a %s object, got %.200s"
#define INVALID_NAME_TYPE    "name must be a %s object, got %.200s"

// ==== Helper declarations ===================================================

/*
 * Return a bytes object as a "const char *", or NULL on error.
 *
 * The 'error' message is either INVALID_PATTERN_TYPE or INVALID_NAME_TYPE,
 * and is used to set a TypeError if 'arg' is of incorrect type.
 */
static inline const char *
from_encoded(PyObject *arg, const char *error);

/*
 * Return a str object as a "const char *", or NULL on error.
 *
 * The 'error' message is either INVALID_PATTERN_TYPE or INVALID_NAME_TYPE
 * and is used to set a TypeError if 'arg' is of incorrect type.
 */
static inline const char *
from_unicode(PyObject *arg, const char *error);

/* The type of from_encoded() or from_unicode() conversion functions. */
typedef const char *(*Converter)(PyObject *string, const char *error);

static inline PyObject *
_posix_fnmatch_filter(PyObject *pattern, PyObject *names, Converter converter);

/* cached 'pattern' version of _posix_fnmatch_filter()  */
static /* not inline */ PyObject *
_posix_fnmatch_filter_cached(const char *pattern, PyObject *names, Converter converter);

// ==== API implementation ====================================================

inline PyObject *
_Py_posix_fnmatch_encoded_filter(PyObject *pattern, PyObject *names)
{
    return _posix_fnmatch_filter(pattern, names, &from_encoded);
}

inline PyObject *
_Py_posix_fnmatch_unicode_filter(PyObject *pattern, PyObject *names)
{
    return _posix_fnmatch_filter(pattern, names, &from_unicode);
}

inline PyObject *
_Py_posix_fnmatch_encoded_filter_cached(const char *pattern, PyObject *names)
{
    assert(pattern != NULL);
    return _posix_fnmatch_filter_cached(pattern, names, &from_encoded);
}

inline PyObject *
_Py_posix_fnmatch_unicode_filter_cached(const char *pattern, PyObject *names)
{
    assert(pattern != NULL);
    return _posix_fnmatch_filter_cached(pattern, names, &from_unicode);
}

inline int
_Py_posix_fnmatch_encoded(PyObject *pattern, PyObject *string)
{
    const char *p = from_encoded(pattern, INVALID_PATTERN_TYPE);
    if (p == NULL) {
        return -1;
    }
    return _Py_posix_fnmatch_encoded_cached(p, string);
}

inline int
_Py_posix_fnmatch_unicode(PyObject *pattern, PyObject *string)
{
    const char *p = from_unicode(pattern, INVALID_PATTERN_TYPE);
    if (p == NULL) {
        return -1;
    }
    return _Py_posix_fnmatch_unicode_cached(p, string);
}

#define PROCESS_MATCH_RESULT(r) \
    do { \
        int res = (r); \
        if (res < 0) { \
            return res; \
        } \
        return res != FNM_NOMATCH; \
    } while (0)

inline int
_Py_posix_fnmatch_encoded_cached(const char *pattern, PyObject *string)
{
    assert(pattern != NULL);
    const char *s = from_encoded(string, INVALID_NAME_TYPE);
    if (s == NULL) {
        return -1;
    }
    PROCESS_MATCH_RESULT(fnmatch(pattern, s, 0));
}

inline int
_Py_posix_fnmatch_unicode_cached(const char *pattern, PyObject *string)
{
    assert(pattern != NULL);
    const char *s = from_unicode(string, INVALID_NAME_TYPE);
    if (s == NULL) {
        return -1;
    }
    PROCESS_MATCH_RESULT(fnmatch(pattern, s, 0));
}

#undef PROCESS_MATCH_RESULT

// ==== Helper implementations ================================================

#define GENERATE_CONVERTER(function, predicate, converter, expecting) \
    static inline const char * \
    function(PyObject *arg, const char *error) \
    { \
        if (!predicate(arg)) { \
            PyErr_Format(PyExc_TypeError, error, expecting, Py_TYPE(arg)->tp_name); \
            return NULL; \
        } \
        return converter(arg); \
    }
GENERATE_CONVERTER(from_encoded, PyBytes_Check, PyBytes_AS_STRING, "bytes")
GENERATE_CONVERTER(from_unicode, PyUnicode_Check, PyUnicode_AsUTF8, "str")
#undef GENERATE_CONVERTER

static inline PyObject *
_posix_fnmatch_filter(PyObject *pattern, PyObject *names, Converter converter)
{
    const char *p = converter(pattern, INVALID_PATTERN_TYPE);
    if (p == NULL) {
        return NULL;
    }
    return _posix_fnmatch_filter_cached(p, names, converter);
}

static PyObject *
_posix_fnmatch_filter_cached(const char *pattern, PyObject *names, Converter converter)
{
    assert(pattern != NULL);
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
        const char *n = converter(name, INVALID_NAME_TYPE);
        if (n == NULL) {
            goto abort;
        }
        if (fnmatch(pattern, n, 0) != FNM_NOMATCH) {
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

#undef INVALID_NAME_TYPE
#undef INVALID_PATTERN_TYPE
#endif
