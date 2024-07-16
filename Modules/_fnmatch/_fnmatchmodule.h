/*
 * C accelerator for the 'fnmatch' module.
 */

#ifndef _FNMATCHMODULE_H
#define _FNMATCHMODULE_H

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"

typedef struct {
    PyObject *os_module;                // import os
    PyObject *posixpath_module;         // import posixpath
    PyObject *re_module;                // import re

    PyObject *translator;               // LRU-cached translation unit

    // strings used by translate.c
    PyObject *hyphen_str;               // hyphen '-'
    PyObject *hyphen_esc_str;           // escaped hyphen '\\-'

    PyObject *backslash_str;            // backslash '\\'
    PyObject *backslash_esc_str;        // escaped backslash '\\\\'

    PyObject *inactive_toks_str;        // inactive tokens '([&~|])'
    PyObject *inactive_toks_repl_str;   // replacement pattern '\\\\\\1'
} fnmatchmodule_state;

static inline fnmatchmodule_state *
get_fnmatchmodule_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (fnmatchmodule_state *)state;
}

// ==== Helper prototypes =====================================================

/*
 * Test whether a name matches a compiled RE pattern.
 *
 * Parameters
 *
 *  matcher     A reference to the 'match()' method of a compiled pattern.
 *  string      The string to match (str or bytes object).
 *
 * Returns
 *
 *  -1  if the call 'matcher(string)' failed (e.g., invalid type),
 *   0  if the 'string' does NOT match the pattern,
 *   1  if the 'string' matches the pattern.
 */
extern int
_Py_fnmatch_match(PyObject *matcher, PyObject *string);

/*
 * Returns a list of matched names, or NULL if an error occurred.
 *
 * Parameters
 *
 *  matcher     A reference to the 'match()' method of a compiled pattern.
 *  names       An iterable of strings (str or bytes objects) to match.
 */
extern PyObject *
_Py_fnmatch_filter(PyObject *matcher, PyObject *names);

/*
 * Similar to _Py_fnmatch_filter() but matches os.path.normcase(name)
 * instead. The returned values are however a sub-sequence of 'names'.
 *
 * The 'normcase' argument is a callable implementing os.path.normcase().
 */
extern PyObject *
_Py_fnmatch_filter_normalized(PyObject *matcher,
                              PyObject *names,
                              PyObject *normcase);

/*
 * C accelerator for translating UNIX shell patterns into RE patterns.
 *
 * The 'pattern' must be a Unicode object (not a bytes) object,
 * and the translated pattern will be a Unicode object as well.
 *
 * Note: this is the C implementation of fnmatch.translate().
 */
extern PyObject *
_Py_fnmatch_translate(PyObject *module, PyObject *pattern);

#endif // _FNMATCHMODULE_H
