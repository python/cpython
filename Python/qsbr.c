/*
 * Implementation of safe memory reclamation scheme using
 * quiescent states.  See InternalDocs/qsbr.md.
 *
 * This is derived from the "GUS" safe memory reclamation technique
 * in FreeBSD written by Jeffrey Roberson. It is heavily modified. Any bugs
 * in this code are likely due to the modifications.
 *
 * The original copyright is preserved below.
 *
 * Copyright (c) 2019,2020 Jeffrey Roberson <jeff@FreeBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "Python.h"
#include "pycore_interp.h"          // PyInterpreterState
#include "pycore_pystate.h"         // _PyThreadState_GET()
#include "pycore_qsbr.h"
#include "pycore_tstate.h"          // _PyThreadStateImpl


// Starting size of the array of qsbr thread states
#define MIN_ARRAY_SIZE 8

// Allocate a QSBR thread state from the freelist
static struct _qsbr_thread_state *
qsbr_allocate(struct _qsbr_shared *shared)
{
    struct _qsbr_thread_state *qsbr = shared->freelist;
    if (qsbr == NULL) {
        return NULL;
    }
    shared->freelist = qsbr->freelist_next;
    qsbr->freelist_next = NULL;
    qsbr->shared = shared;
    qsbr->allocated = true;
    return qsbr;
}

// Initialize (or reintialize) the freelist of QSBR thread states
static void
initialize_new_array(struct _qsbr_shared *shared)
{
    for (Py_ssize_t i = 0; i != shared->size; i++) {
        struct _qsbr_thread_state *qsbr = &shared->array[i].qsbr;
        if (qsbr->tstate != NULL) {
            // Update the thread state pointer to its QSBR state
            _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)qsbr->tstate;
            tstate->qsbr = qsbr;
        }
        if (!qsbr->allocated) {
            // Push to freelist
            qsbr->freelist_next = shared->freelist;
            shared->freelist = qsbr;
        }
    }
}

// Grow the array of QSBR thread states. Returns 0 on success, -1 on failure.
static int
grow_thread_array(struct _qsbr_shared *shared)
{
    Py_ssize_t new_size = shared->size * 2;
    if (new_size < MIN_ARRAY_SIZE) {
        new_size = MIN_ARRAY_SIZE;
    }

    struct _qsbr_pad *array = PyMem_RawCalloc(new_size, sizeof(*array));
    if (array == NULL) {
        return -1;
    }

    struct _qsbr_pad *old = shared->array;
    if (old != NULL) {
        memcpy(array, shared->array, shared->size * sizeof(*array));
    }

    shared->array = array;
    shared->size = new_size;
    shared->freelist = NULL;
    initialize_new_array(shared);

    PyMem_RawFree(old);
    return 0;
}

uint64_t
_Py_qsbr_advance(struct _qsbr_shared *shared)
{
    // NOTE: with 64-bit sequence numbers, we don't have to worry too much
    // about the wr_seq getting too far ahead of rd_seq, but if we ever use
    // 32-bit sequence numbers, we'll need to be more careful.
    return _Py_atomic_add_uint64(&shared->wr_seq, QSBR_INCR) + QSBR_INCR;
}

uint64_t
_Py_qsbr_shared_next(struct _qsbr_shared *shared)
{
    return _Py_qsbr_shared_current(shared) + QSBR_INCR;
}

static uint64_t
qsbr_poll_scan(struct _qsbr_shared *shared)
{
    // Synchronize with store in _Py_qsbr_attach(). We need to ensure that
    // the reads from each thread's sequence number are not reordered to see
    // earlier "offline" states.
    _Py_atomic_fence_seq_cst();

    // Compute the minimum sequence number of all attached threads
    uint64_t min_seq = _Py_atomic_load_uint64(&shared->wr_seq);
    struct _qsbr_pad *array = shared->array;
    for (Py_ssize_t i = 0, size = shared->size; i != size; i++) {
        struct _qsbr_thread_state *qsbr = &array[i].qsbr;

        uint64_t seq = _Py_atomic_load_uint64(&qsbr->seq);
        if (seq != QSBR_OFFLINE && QSBR_LT(seq, min_seq)) {
            min_seq = seq;
        }
    }

    // Update the shared read sequence
    uint64_t rd_seq = _Py_atomic_load_uint64(&shared->rd_seq);
    if (QSBR_LT(rd_seq, min_seq)) {
        // It's okay if the compare-exchange failed: another thread updated it
        (void)_Py_atomic_compare_exchange_uint64(&shared->rd_seq, &rd_seq, min_seq);
        rd_seq = min_seq;
    }

    return rd_seq;
}

bool
_Py_qsbr_poll(struct _qsbr_thread_state *qsbr, uint64_t goal)
{
    assert(_Py_atomic_load_int_relaxed(&_PyThreadState_GET()->state) == _Py_THREAD_ATTACHED);
    assert(((_PyThreadStateImpl *)_PyThreadState_GET())->qsbr == qsbr);

    if (_Py_qbsr_goal_reached(qsbr, goal)) {
        return true;
    }

    uint64_t rd_seq = qsbr_poll_scan(qsbr->shared);
    return QSBR_LEQ(goal, rd_seq);
}

void
_Py_qsbr_attach(struct _qsbr_thread_state *qsbr)
{
    assert(qsbr->seq == 0 && "already attached");

    uint64_t seq = _Py_qsbr_shared_current(qsbr->shared);
    _Py_atomic_store_uint64(&qsbr->seq, seq);  // needs seq_cst
}

void
_Py_qsbr_detach(struct _qsbr_thread_state *qsbr)
{
    assert(qsbr->seq != 0 && "already detached");

    _Py_atomic_store_uint64_release(&qsbr->seq, QSBR_OFFLINE);
}

Py_ssize_t
_Py_qsbr_reserve(PyInterpreterState *interp)
{
    struct _qsbr_shared *shared = &interp->qsbr;

    PyMutex_Lock(&shared->mutex);
    // Try allocating from our internal freelist
    struct _qsbr_thread_state *qsbr = qsbr_allocate(shared);

    // If there are no free entries, we pause all threads, grow the array,
    // and update the pointers in PyThreadState to entries in the new array.
    if (qsbr == NULL) {
        _PyEval_StopTheWorld(interp);
        if (grow_thread_array(shared) == 0) {
            qsbr = qsbr_allocate(shared);
        }
        _PyEval_StartTheWorld(interp);
    }

    // Return an index rather than the pointer because the array may be
    // resized and the pointer invalidated.
    Py_ssize_t index = -1;
    if (qsbr != NULL) {
        index = (struct _qsbr_pad *)qsbr - shared->array;
    }
    PyMutex_Unlock(&shared->mutex);
    return index;
}

void
_Py_qsbr_register(_PyThreadStateImpl *tstate, PyInterpreterState *interp,
                  Py_ssize_t index)
{
    // Associate the QSBR state with the thread state
    struct _qsbr_shared *shared = &interp->qsbr;

    PyMutex_Lock(&shared->mutex);
    struct _qsbr_thread_state *qsbr = &interp->qsbr.array[index].qsbr;
    assert(qsbr->allocated && qsbr->tstate == NULL);
    qsbr->tstate = (PyThreadState *)tstate;
    tstate->qsbr = qsbr;
    PyMutex_Unlock(&shared->mutex);
}

void
_Py_qsbr_unregister(PyThreadState *tstate)
{
    struct _qsbr_shared *shared = &tstate->interp->qsbr;
    struct _PyThreadStateImpl *tstate_imp = (_PyThreadStateImpl*) tstate;

    // gh-119369: GIL must be released (if held) to prevent deadlocks, because
    // we might not have an active tstate, which means that blocking on PyMutex
    // locks will not implicitly release the GIL.
    assert(!tstate->holds_gil);

    PyMutex_Lock(&shared->mutex);
    // NOTE: we must load (or reload) the thread state's qbsr inside the mutex
    // because the array may have been resized (changing tstate->qsbr) while
    // we waited to acquire the mutex.
    struct _qsbr_thread_state *qsbr = tstate_imp->qsbr;

    assert(qsbr->seq == 0 && "thread state must be detached");
    assert(qsbr->allocated && qsbr->tstate == tstate);

    tstate_imp->qsbr = NULL;
    qsbr->tstate = NULL;
    qsbr->allocated = false;
    qsbr->freelist_next = shared->freelist;
    shared->freelist = qsbr;
    PyMutex_Unlock(&shared->mutex);
}

void
_Py_qsbr_fini(PyInterpreterState *interp)
{
    struct _qsbr_shared *shared = &interp->qsbr;
    PyMem_RawFree(shared->array);
    shared->array = NULL;
    shared->size = 0;
    shared->freelist = NULL;
}

void
_Py_qsbr_after_fork(_PyThreadStateImpl *tstate)
{
    struct _qsbr_thread_state *this_qsbr = tstate->qsbr;
    struct _qsbr_shared *shared = this_qsbr->shared;

    _PyMutex_at_fork_reinit(&shared->mutex);

    for (Py_ssize_t i = 0; i != shared->size; i++) {
        struct _qsbr_thread_state *qsbr = &shared->array[i].qsbr;
        if (qsbr != this_qsbr && qsbr->allocated) {
            qsbr->tstate = NULL;
            qsbr->allocated = false;
            qsbr->freelist_next = shared->freelist;
            shared->freelist = qsbr;
        }
    }
}
