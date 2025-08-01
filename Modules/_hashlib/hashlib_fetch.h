/*
 * Utilities used when fetching a message digest from a digest-like identifier.
 */

#ifndef _HASHLIB_HASHLIB_FETCH_H
#define _HASHLIB_HASHLIB_FETCH_H

#include "Python.h"

/*
 * Internal error messages used for reporting an unsupported hash algorithm.
 * The algorithm can be given by its name, a callable or a PEP-247 module.
 * The same message is raised by Lib/hashlib.py::__get_builtin_constructor()
 * and _hmacmodule.c::find_hash_info().
 */
#define _Py_HASHLIB_UNSUPPORTED_ALGORITHM       "unsupported hash algorithm %S"
#define _Py_HASHLIB_UNSUPPORTED_STR_ALGORITHM   "unsupported hash algorithm %s"

#endif // !_HASHLIB_HASHLIB_FETCH_H
