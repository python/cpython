#ifndef Py_INTERNAL_CONTEXT_H
#define Py_INTERNAL_CONTEXT_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "cpython/context.h"
#include "cpython/genobject.h"    // PyGenObject
#include "pycore_genobject.h"     // PyGenObject
#include "pycore_hamt.h"          // PyHamtObject
#include "pycore_tstate.h"        // _PyThreadStateImpl

#define CONTEXT_MAX_WATCHERS 8

extern PyTypeObject _PyContextTokenMissing_Type;

/* runtime lifecycle */

PyStatus _PyContext_Init(PyInterpreterState *);


/* other API */

typedef struct {
    PyObject_HEAD
} _PyContextTokenMissing;

struct _pycontextobject {
    PyObject_HEAD
    PyObject *ctx_prev;
    PyHamtObject *ctx_vars;
    PyObject *ctx_weakreflist;
    int ctx_entered;
};

// Resets a coroutine's independent context stack to ctx.  If ctx is NULL or
// Py_None, the coroutine will be a dependent coroutine (its context stack will
// be empty) upon successful return.  Otherwise, the coroutine will be an
// independent coroutine upon successful return, with ctx as the sole item on
// its context stack.
//
// The coroutine's existing stack must be empty (NULL) or contain only a single
// entry (from a previous call to this function).  If the coroutine is
// currently executing, this function must be called from the coroutine's
// thread.
//
// Unless ctx already equals the coroutine's existing context stack, the
// context on the existing stack (if one exists) is immediately exited and ctx
// (if non-NULL) is immediately entered.
int _PyGen_ResetContext(PyThreadState *ts, PyGenObject *self, PyObject *ctx);

void _PyGen_ActivateContextImpl(_PyThreadStateImpl *tsi, PyGenObject *self);
void _PyGen_DeactivateContextImpl(_PyThreadStateImpl *tsi, PyGenObject *self);

// Makes the given coroutine's context stack the active context stack for the
// thread, shadowing (temporarily deactivating) the thread's previously active
// context stack.  The context stack remains active until deactivated with a
// call to _PyGen_DeactivateContext, as long as it is not shadowed by another
// activated context stack.
//
// Each activated context stack must eventually be deactivated by calling
// _PyGen_DeactivateContext.  The same context stack cannot be activated again
// until deactivated.
//
// If the coroutine's context stack is empty this function has no effect.
//
// This is called each time a value is sent to a coroutine, so it is inlined to
// avoid function call overhead in the common case of dependent coroutines.
static inline void
_PyGen_ActivateContext(PyThreadState *ts, PyGenObject *self)
{
    _PyThreadStateImpl *tsi = (_PyThreadStateImpl *)ts;
    assert(self->_ctx_chain.prev == NULL);
    if (self->_ctx_chain.ctx == NULL) {
        return;
    }
    _PyGen_ActivateContextImpl(tsi, self);
}

// Deactivates the given coroutine's context stack, un-shadowing (reactivating)
// the thread's previously active context stack.  Does not affect any contexts
// in the coroutine's context stack (they remain entered).
//
// Must not be called if a different context stack is currently shadowing the
// coroutine's context stack.
//
// If the coroutine's context stack is not the active context stack this
// function has no effect.
//
// This is called each time a value is sent to a coroutine, so it is inlined to
// avoid function call overhead in the common case of dependent coroutines.
static inline void
_PyGen_DeactivateContext(PyThreadState *ts, PyGenObject *self)
{
    _PyThreadStateImpl *tsi = (_PyThreadStateImpl *)ts;
    if (tsi->_ctx_chain.prev != &self->_ctx_chain) {
        assert(self->_ctx_chain.ctx == NULL);
        assert(self->_ctx_chain.prev == NULL);
        return;
    }
    assert(self->_ctx_chain.ctx != NULL);
    _PyGen_DeactivateContextImpl(tsi, self);
}


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
