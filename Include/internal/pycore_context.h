#ifndef Py_INTERNAL_CONTEXT_H
#define Py_INTERNAL_CONTEXT_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_freelist.h"      // _PyFreeListState
#include "pycore_hamt.h"          // PyHamtObject


extern PyTypeObject _PyContextTokenMissing_Type;

/* runtime lifecycle */

PyStatus _PyContext_Init(PyInterpreterState *);


/* other API */

typedef struct {
    PyObject_HEAD
} _PyContextTokenMissing;

struct _pycontextobject {
    PyObject_HEAD
    PyContext *ctx_prev;
    PyHamtObject *ctx_vars;
    PyObject *ctx_weakreflist;
    int ctx_entered;
};


struct _pycontextvarobject {
    PyObject_HEAD
    PyObject *var_name;
    PyObject *var_default;
#ifndef Py_GIL_DISABLED
    PyObject *var_cached;
    uint64_t var_cached_tsid;
    uint64_t var_cached_tsver;
#endif
    Py_hash_t var_hash;
};


struct _pycontexttokenobject {
    PyObject_HEAD
    PyContext *tok_ctx;
    PyContextVar *tok_var;
    PyObject *tok_oldval;
    int tok_used;
};


// _testinternalcapi.hamt() used by tests.
// Export for '_testcapi' shared extension
PyAPI_FUNC(PyObject*) _PyContext_NewHamtForTests(void);


#endif /* !Py_INTERNAL_CONTEXT_H */
