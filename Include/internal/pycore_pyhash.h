#ifndef Py_INTERNAL_HASH_H
#define Py_INTERNAL_HASH_H

#if !defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_BUILTIN)
#  error "this header requires Py_BUILD_CORE or Py_BUILD_CORE_BUILTIN define"
#endif

uint64_t _Py_KeyedHash(uint64_t, const char *, Py_ssize_t);

#endif
