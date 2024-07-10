/*
* C accelerator for the 'fnmatch' module (POSIX only).
 *
 * Most functions expect string or bytes instances, and thus the Python
 * implementation should first pre-process path-like objects, possibly
 * applying normalizations depending on the platform if needed.
 */

#ifndef _FNMATCHMODULE_H
#define _FNMATCHMODULE_H

#include "Python.h"

#undef Py_USE_FNMATCH_FALLBACK
/*
 * For now, only test the C acceleration of the Python implementation.
 *
 * TODO(picnixz): Currently, I don't know how to handle backslashes
 * TODO(picnixz): in fnmatch(3) so that they are treated correctly
 * TODO(picnixz): depending on whether the string was a raw string
 * TODO(picnixz): or not. To see the bug, uncomment the following
 * TODO(picnixz): macro and run the tests.
 */
#define Py_USE_FNMATCH_FALLBACK 1

typedef struct {
    PyObject *py_module;    // 'fnmatch' module
    PyObject *re_module;    // 're' module
    PyObject *os_module;    // 'os' module

    PyObject *lru_cache;    // the LRU cache decorator
    PyObject *translator;   // the translation unit whose calls are cached
} fnmatchmodule_state;

static inline fnmatchmodule_state *
get_fnmatchmodulestate_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (fnmatchmodule_state *)state;
}

#if defined(Py_HAVE_FNMATCH) && !defined(Py_USE_FNMATCH_FALLBACK)
/*
 * Construct a list of filtered names using fnmatch(3).
 */
extern PyObject *
_Py_posix_fnmatch_encoded_filter(PyObject *pattern, PyObject *names);
/* Same as _Py_posix_fnmatch_encoded_filter() but for unicode inputs. */
extern PyObject *
_Py_posix_fnmatch_unicode_filter(PyObject *pattern, PyObject *names);

/* cached 'pattern' version of _Py_posix_fnmatch_encoded_filter() */
extern PyObject *
_Py_posix_fnmatch_encoded_filter_cached(const char *pattern, PyObject *names);
/* cached 'pattern' version of _Py_posix_fnmatch_unicode_filter() */
extern PyObject *
_Py_posix_fnmatch_unicode_filter_cached(const char *pattern, PyObject *names);

/*
 * Perform a case-sensitive match using fnmatch(3).
 *
 * Parameters
 *
 *      pattern  A UNIX shell pattern.
 *      string   The string to match (bytes object).
 *
 * Returns 1 if the 'string' matches the 'pattern' and 0 otherwise.
 *
 * Returns -1 if (1) 'string' is not a `bytes` object, and
 * sets a TypeError exception, or (2) something went wrong.
 */
extern int
_Py_posix_fnmatch_encoded(PyObject *pattern, PyObject *string);
/* Same as _Py_posix_fnmatch_encoded() but for unicode inputs. */
extern int
_Py_posix_fnmatch_unicode(PyObject *pattern, PyObject *string);

/* cached 'pattern' version of _Py_posix_fnmatch_encoded() */
extern int
_Py_posix_fnmatch_encoded_cached(const char *pattern, PyObject *names);
/* cached 'pattern' version of _Py_posix_fnmatch_encoded() */
extern int
_Py_posix_fnmatch_unicode_cached(const char *pattern, PyObject *names);
#endif

/*
 * Test whether a name matches a compiled RE pattern.
 *
 * Parameters
 *
 *      matcher  A reference to the 'match()' method of a compiled pattern.
 *      string   The string to match (str or bytes object).
 *
 * Returns 1 if the 'string' matches the pattern and 0 otherwise.
 *
 * Returns -1 if (1) 'string' is not a `str` or a `bytes` object,
 * and sets a TypeError exception, or (2) something went wrong.
 */
extern int
_Py_regex_fnmatch_generic(PyObject *matcher, PyObject *string);

/*
 * Perform a case-sensitive match using compiled RE patterns.
 *
 * Parameters
 *
 *      matcher  A reference to the 'match()' method of a compiled pattern.
 *      names    An iterable of strings (str or bytes objects) to match.
 *
 * Returns a list of matched names, or NULL if an error occurred.
 */
extern PyObject *
_Py_regex_fnmatch_filter(PyObject *matcher, PyObject *names);

/*
 * C accelerator for translating UNIX shell patterns into RE patterns.
 */
extern PyObject *
_Py_regex_translate(PyObject *module, PyObject *pattern);

#endif // _FNMATCHMODULE_H
