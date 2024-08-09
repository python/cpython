/*
 * This file contains helper prototypes and structures.
 */

#ifndef _FNMATCH_UTIL_H
#define _FNMATCH_UTIL_H

#include "Python.h"

typedef struct {
    PyObject *os_module;            // import os
    PyObject *posixpath_module;     // import posixpath
    PyObject *re_module;            // import re

    PyObject *translator;           // LRU-cached translation unit

    // strings used by translate.c
    PyObject *hyphen_str;           // hyphen '-'
    PyObject *hyphen_esc_str;       // escaped hyphen '\\-'

    PyObject *backslash_str;        // backslash '\\'
    PyObject *backslash_esc_str;    // escaped backslash '\\\\'

    /* set operation tokens (&&, ~~ and ||) are not supported in regex */
    PyObject *setops_str;           // set operation tokens '([&~|])'
    PyObject *setops_repl_str;      // replacement pattern '\\\\\\1'
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
 * Returns a list of matched names, or NULL if an error occurred.
 *
 * Parameters
 *
 *  matcher     A reference to the 'match()' method of a compiled pattern.
 *  names       An iterable of strings (str or bytes objects) to match.
 *  normalizer  Optional normalization function.
 *
 *  This is equivalent to:
 *
 *      [name for name in names if matcher(normalizer(name))]
 */
extern PyObject *
_Py_fnmatch_filter(PyObject *matcher, PyObject *names, PyObject *normalizer);

/*
 * C accelerator for translating UNIX shell patterns into RE patterns.
 *
 * Parameters
 *
 *  module          A module with a state given by get_fnmatchmodule_state().
 *  pattern         A Unicode object to translate.
 *
 * Returns
 *
 *  A translated unicode RE pattern.
 */
extern PyObject *
_Py_fnmatch_translate(PyObject *module, PyObject *pattern);

#endif // _FNMATCH_UTIL_H
