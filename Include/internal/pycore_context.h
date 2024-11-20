#ifndef Py_INTERNAL_CONTEXT_H
#define Py_INTERNAL_CONTEXT_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_hamt.h"          // PyHamtObject

#define CONTEXT_MAX_WATCHERS 8

extern PyTypeObject _PyContextTokenMissing_Type;

/* runtime lifecycle */

PyStatus _PyContext_Init(PyInterpreterState *);

// Exits any thread-owned contexts (see context_get) at the top of the given
// thread's context stack.  The given thread state is not required to belong to
// the calling thread; if not, the thread is assumed to have exited (or not yet
// started) and no Py_CONTEXT_SWITCHED event is emitted for any context
// changes.  Logs a warning via PyErr_FormatUnraisable if the thread's context
// stack is non-empty afterwards (because those contexts can never be exited or
// re-entered).
void _PyContext_ExitThreadOwned(PyThreadState *);


/* other API */

typedef struct {
    PyObject_HEAD
} _PyContextTokenMissing;

struct _pycontextobject {
    PyObject_HEAD
    PyContext *ctx_prev;
    PyHamtObject *ctx_vars;
    PyObject *ctx_weakreflist;
    _Bool ctx_entered:1;
    // True for the thread's default context created by context_get.  Used to
    // safely determine whether the base context can be exited when clearing a
    // PyThreadState.
    _Bool ctx_owned_by_thread:1;
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
