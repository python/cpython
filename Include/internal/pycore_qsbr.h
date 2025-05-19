// The QSBR APIs (quiescent state-based reclamation) provide a mechanism for
// the free-threaded build to safely reclaim memory when there may be
// concurrent accesses.
//
// Many operations in the free-threaded build are protected by locks. However,
// in some cases, we want to allow reads to happen concurrently with updates.
// In this case, we need to delay freeing ("reclaiming") any memory that may be
// concurrently accessed by a reader. The QSBR APIs provide a way to do this.
#ifndef Py_INTERNAL_QSBR_H
#define Py_INTERNAL_QSBR_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// The shared write sequence is always odd and incremented by two. Detached
// threads are indicated by a read sequence of zero. This avoids collisions
// between the offline state and any valid sequence number even if the
// sequences numbers wrap around.
#define QSBR_OFFLINE 0
#define QSBR_INITIAL 1
#define QSBR_INCR    2

// Wrap-around safe comparison. This is a holdover from the FreeBSD
// implementation, which uses 32-bit sequence numbers. We currently use 64-bit
// sequence numbers, so wrap-around is unlikely.
#define QSBR_LT(a, b) ((int64_t)((a)-(b)) < 0)
#define QSBR_LEQ(a, b) ((int64_t)((a)-(b)) <= 0)

struct _qsbr_shared;
struct _PyThreadStateImpl;  // forward declare to avoid circular dependency

// Per-thread state
struct _qsbr_thread_state {
    // Last observed write sequence (or 0 if detached)
    uint64_t seq;

    // Shared (per-interpreter) QSBR state
    struct _qsbr_shared *shared;

    // Thread state (or NULL)
    PyThreadState *tstate;

    // Used to defer advancing write sequence a fixed number of times
    int deferrals;

    // Is this thread state allocated?
    bool allocated;
    struct _qsbr_thread_state *freelist_next;
};

// Padding to avoid false sharing
struct _qsbr_pad {
    struct _qsbr_thread_state qsbr;
    char __padding[64 - sizeof(struct _qsbr_thread_state)];
};

// Per-interpreter state
struct _qsbr_shared {
    // Write sequence: always odd, incremented by two
    uint64_t wr_seq;

    // Minimum observed read sequence of all QSBR thread states
    uint64_t rd_seq;

    // Array of QSBR thread states.
    struct _qsbr_pad *array;
    Py_ssize_t size;

    // Freelist of unused _qsbr_thread_states (protected by mutex)
    PyMutex mutex;
    struct _qsbr_thread_state *freelist;
};

static inline uint64_t
_Py_qsbr_shared_current(struct _qsbr_shared *shared)
{
    return _Py_atomic_load_uint64_acquire(&shared->wr_seq);
}

// Reports a quiescent state: the caller no longer holds any pointer to shared
// data not protected by locks or reference counts.
static inline void
_Py_qsbr_quiescent_state(struct _qsbr_thread_state *qsbr)
{
    uint64_t seq = _Py_qsbr_shared_current(qsbr->shared);
    _Py_atomic_store_uint64_release(&qsbr->seq, seq);
}

// Have the read sequences advanced to the given goal? Like `_Py_qsbr_poll()`,
// but does not perform a scan of threads.
static inline bool
_Py_qbsr_goal_reached(struct _qsbr_thread_state *qsbr, uint64_t goal)
{
    uint64_t rd_seq = _Py_atomic_load_uint64(&qsbr->shared->rd_seq);
    return QSBR_LEQ(goal, rd_seq);
}

// Advance the write sequence and return the new goal. This should be called
// after data is removed. The returned goal is used with `_Py_qsbr_poll()` to
// determine when it is safe to reclaim (free) the memory.
extern uint64_t
_Py_qsbr_advance(struct _qsbr_shared *shared);

// Batches requests to advance the write sequence. This advances the write
// sequence every N calls, which reduces overhead but increases time to
// reclamation. Returns the new goal.
extern uint64_t
_Py_qsbr_deferred_advance(struct _qsbr_thread_state *qsbr);

// Have the read sequences advanced to the given goal? If this returns true,
// it safe to reclaim any memory tagged with the goal (or earlier goal).
extern bool
_Py_qsbr_poll(struct _qsbr_thread_state *qsbr, uint64_t goal);

// Called when thread attaches to interpreter
extern void
_Py_qsbr_attach(struct _qsbr_thread_state *qsbr);

// Called when thread detaches from interpreter
extern void
_Py_qsbr_detach(struct _qsbr_thread_state *qsbr);

// Reserves (allocates) a QSBR state and returns its index.
extern Py_ssize_t
_Py_qsbr_reserve(PyInterpreterState *interp);

// Associates a PyThreadState with the QSBR state at the given index
extern void
_Py_qsbr_register(struct _PyThreadStateImpl *tstate,
                  PyInterpreterState *interp, Py_ssize_t index);

// Disassociates a PyThreadState from the QSBR state and frees the QSBR state.
extern void
_Py_qsbr_unregister(PyThreadState *tstate);

extern void
_Py_qsbr_fini(PyInterpreterState *interp);

extern void
_Py_qsbr_after_fork(struct _PyThreadStateImpl *tstate);

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_QSBR_H */
