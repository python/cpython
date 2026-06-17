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
    PyHamtObject *ctx_vars_origin;  /* snapshot of ctx_vars at Enter time */
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

PyAPI_FUNC(int) _PyContext_Enter(PyThreadState *ts, PyObject *octx);
PyAPI_FUNC(int) _PyContext_Exit(PyThreadState *ts, PyObject *octx);

/* Get a value for the variable and check if it was changed.

   Like PyContextVar_Get, but also reports whether the variable was
   changed in the current context scope via a single HAMT lookup.

   Returns -1 if an error occurred during lookup.

   Returns 0 if no error occurred.  In this case:

   - *value will be set the same as for PyContextVar_Get.
   - *changed will be set to 1 if the variable was changed in the
     current context scope, 0 otherwise.  If the variable was not
     found, *changed is always 0.

   '*value' will be a new ref, if not NULL.
*/
PyAPI_FUNC(int) _PyContextVar_GetChanged(
    PyObject *var, PyObject *default_value, PyObject **value, int *changed);


#endif /* !Py_INTERNAL_CONTEXT_H */
