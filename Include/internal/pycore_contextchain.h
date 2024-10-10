#ifndef Py_INTERNAL_CONTEXTCHAIN_H
#define Py_INTERNAL_CONTEXTCHAIN_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pytypedefs.h"  // PyObject


// Circularly linked chain of multiple independent context stacks, used to give
// coroutines (including generators) their own (optional) independent context
// stacks.
//
// Detailed notes on how this chain is used:
//   * The chain is circular simply to save a pointer's worth of memory in
//     _PyThreadStateImpl.  It is actually used as an ordinary linear linked
//     list.  It is called "chain" instead of "stack" or "list" to evoke "call
//     chain", which it is related to, and to avoid confusion with "context
//     stack".
//   * There is one chain per thread, and _PyThreadStateImpl::_ctx_chain::prev
//     points to the head of the thread's chain.
//   * A thread's chain is never empty.
//   * _PyThreadStateImpl::_ctx_chain is always the tail entry of the thread's
//     chain.
//   * _PyThreadStateImpl::_ctx_chain is usually the only link in the thread's
//     chain, so _PyThreadStateImpl::_ctx_chain::prev usually points to the
//     _PyThreadStateImpl::_ctx_chain itself.
//   * The "active context stack" is always at the head link in a thread's
//     context chain.  Contexts are entered by pushing onto the active context
//     stack and exited by popping off of the active context stack.
//   * The "current context" is the top context in the active context stack.
//     Context variable accesses (reads/writes) use the current context.
//   * A *dependent* coroutine or generator is a coroutine or generator that
//     does not have its own independent context stack.  When a dependent
//     coroutine starts or resumes execution, the current context -- as
//     observed by the coroutine -- is the same context that was current just
//     before the coroutine's `send` method was called.  This means that the
//     current context as observed by a dependent coroutine can change
//     arbitrarily during a yield/await.  Dependent coroutines are so-named
//     because they depend on their senders to enter the appropriate context
//     before each send.  Coroutines and generators are dependent by default
//     for backwards compatibility.
//   * The purpose of the context chain is to enable *independent* coroutines
//     and generators, which have their own context stacks.  Whenever an
//     independent coroutine starts or resumes execution, the current context
//     automatically switches to the context associated with the coroutine.
//     This is accomplished by linking the coroutine's chain link (at
//     PyGenObject::_ctx_chain) to the head of the thread's chain.  Independent
//     coroutines are so-named because they do not depend on their senders to
//     enter the appropriate context before each send.
//   * The head link is unlinked from the thread's chain when its associated
//     independent coroutine or generator stops executing (yields, awaits,
//     returns, or throws).
//   * A running dependent coroutine's chain link is linked into the thread's
//     chain if the coroutine is upgraded from dependent to independent by
//     assigning a context to the coroutine's `_context` property.  The chain
//     link is inserted at the position corresponding to the coroutine's
//     position in the call chain relative to any other currently running
//     independent coroutines.  For example, if dependent coroutine `coro_a`
//     calls function `func_b` which resumes independent coroutine `coro_c`
//     which assigns a context to `coro_a._context`, then `coro_a` becomes an
//     independent coroutine with its chain link inserted after `coro_c`'s
//     chain link (which remains the head link).
//   * A running independent coroutine's chain link is unlinked from the
//     thread's chain if the coroutine is downgraded from independent to
//     dependent by assigning `None` to its `_context` property.
//   * The references to the object at the `prev` link in the chain are
//     implicit (borrowed).
typedef struct _PyContextChain {
    // NULL for dependent coroutines/generators, non-NULL for independent
    // coroutines/generators.
    PyObject *ctx;
    // NULL if unlinked from the thread's context chain, non-NULL otherwise.
    struct _PyContextChain *prev;
} _PyContextChain;


#endif /* !Py_INTERNAL_CONTEXTCHAIN_H */
