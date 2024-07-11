/*
 * C accelerator for the 'fnmatch' module.
 */

#ifndef _FNMATCHMODULE_H
#define _FNMATCHMODULE_H

#include "Python.h"

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
_Py_fnmatch_fnmatch(PyObject *matcher, PyObject *string);

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
_Py_fnmatch_filter(PyObject *matcher, PyObject *names);

/*
 * C accelerator for translating UNIX shell patterns into RE patterns.
 *
 * Note: this is the C implementation of fnmatch.translate().
 */
extern PyObject *
_Py_fnmatch_translate(PyObject *module, PyObject *pattern);

#endif // _FNMATCHMODULE_H
