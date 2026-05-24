#ifndef PY_INTERNAL_TYPECACHE_H
#define PY_INTERNAL_TYPECACHE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_stackref.h"


#define _Py_TYPECACHE_MINSIZE 8

struct type_cache_entry {
    PyObject *name;
    PyObject *value;
};

struct type_cache {
    uint32_t mask;
    uint32_t version_tag;
    uint32_t available;
    uint32_t used;
    struct type_cache_entry hashtable[1];
};

struct _PyTypeCacheLookupResult {
    _PyStackRef value;
    int cache_hit;
    uint32_t version_tag;
};


extern void _PyTypeCache_InitType(PyTypeObject *type);
extern void _PyTypeCache_Insert(PyTypeObject *type, PyObject *name, PyObject *value);
PyAPI_FUNC(struct _PyTypeCacheLookupResult) _PyTypeCache_Lookup(PyTypeObject *type, PyObject *name);
PyAPI_FUNC(void) _PyTypeCache_Invalidate(PyTypeObject *type);

#ifdef __cplusplus
}
#endif
#endif /* PY_INTERNAL_TYPECACHE_H */
