#ifndef _FNMATCHMODULE_H
#define _FNMATCHMODULE_H

#include "Python.h"

typedef struct {
    PyObject *re_module; // 're' module
    PyObject *os_module; // 'os' module

    PyObject *lru_cache; // optional cache for regex patterns, if needed
} fnmatchmodule_state;

static inline fnmatchmodule_state *
get_fnmatchmodulestate_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (fnmatchmodule_state *)state;
}

/*
 * The filter() function works differently depending on whether fnmatch(3)
 * is present or not.
 *
 * If fnmatch(3) is present, the match is performed without using regular
 * expressions. The functions being used are
 *
 * If fnmatch(3) is not present, the match is performed using regular
 * expressions.
 */

#ifdef Py_HAVE_FNMATCH
/*
 * Type for a matching function.
 *
 * The function must take as input a pattern and a name,
 * and is used to determine whether the name matches the
 * pattern or not.
 *
 * If the pattern is obtained from str() types, then 'name'
 * must be a string (it is left to the matcher the task for
 * validating this part).
 */
typedef int (*Matcher)(const char *, PyObject *);

extern PyObject *
_posix_fnmatch_filter(const char *pattern, PyObject *names, Matcher match);

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
extern int _posix_fnmatch_encoded(const char *pattern, PyObject *string);
/* Same as _posix_fnmatch_encoded() but for unicode inputs. */
extern int _posix_fnmatch_unicode(const char *pattern, PyObject *string);
#else
extern int _regex_fnmatch_generic(PyObject *matcher, PyObject *name);
extern PyObject *
_regex_fnmatch_filter(PyObject *matcher, PyObject *names);
#endif

extern PyObject *translate(PyObject *module, PyObject *pattern);

#endif // _FNMATCHMODULE_H
