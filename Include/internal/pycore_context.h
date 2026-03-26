#ifndef Py_INTERNAL_CONTEXT_H
#define Py_INTERNAL_CONTEXT_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_structs.h"

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
    // Nesting depth in the context inheritance tree.  Assigned at creation
    // time: an empty/base context has depth 0, and a context produced by
    // copying another (copy_context(), Context.copy(), thread/async context
    // inheritance) has depth one greater than its source.
    uint64_t ctx_depth;
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

PyAPI_FUNC(int) _PyContext_Enter(PyThreadState *ts, PyObject *octx);
PyAPI_FUNC(int) _PyContext_Exit(PyThreadState *ts, PyObject *octx);

/* Return the depth (see struct _pycontextobject.ctx_depth) of the current
   context, or 0 if there is no current context. */
PyAPI_FUNC(uint64_t) _PyContext_CurrentDepth(void);


#endif /* !Py_INTERNAL_CONTEXT_H */
