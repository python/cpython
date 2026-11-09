#ifndef Py_INTERNAL_GIL_STATS_H
#define Py_INTERNAL_GIL_STATS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_gil.h"           // struct _gil_stats_state

/* GIL contention statistics.

   The producer side lives in take_gil() (Python/ceval_gil.c) and is made of
   the static inline helpers below.  They are always called with gil->mutex
   held, which serializes all producers; the "record" helpers are only called
   by the thread that has just become the GIL holder.  This makes the event
   ring single-producer/single-consumer, with the aggregator thread
   (Python/gil_stats.c) as the only consumer. */

/* One contended wait that reached the ring: 8 bytes. */
typedef struct {
    uint32_t wait_us;
    uint32_t waiters_at_entry;
} _PyGILStatsEvent;

#define _PyGILSTATS_CACHE_LINE 64

struct _gil_stats_ring {
    /* Next slot to write.  Written (release) only by the GIL holder. */
    uint64_t head;
    char _pad_head[_PyGILSTATS_CACHE_LINE - sizeof(uint64_t)];
    /* Next slot to read.  Written (release) only by the aggregator. */
    uint64_t tail;
    char _pad_tail[_PyGILSTATS_CACHE_LINE - sizeof(uint64_t)];
    /* capacity - 1; capacity is a power of two.  Immutable. */
    uint64_t mask;
    _PyGILStatsEvent events[1];   /* really `mask + 1` entries */
};

/* Increment a holder-only counter: writers are serialized by gil->mutex,
   so a relaxed load + store is enough (no read-modify-write). */
static inline void
_PyGILStats_HolderInc(uint64_t *counter)
{
    _Py_atomic_store_uint64_relaxed(
        counter, _Py_atomic_load_uint64_relaxed(counter) + 1);
}

/* Called under gil->mutex on the first iteration of the wait loop. */
static inline void
_PyGILStats_BeginWait(struct _gil_stats_state *st,
                      PyTime_t *t0, uint32_t *waiters_at_entry)
{
    (void)PyTime_PerfCounterRaw(t0);
    /* `waiters` is only modified under gil->mutex: no RMW needed. */
    Py_ssize_t n = _Py_atomic_load_ssize_relaxed(&st->waiters) + 1;
    _Py_atomic_store_ssize_relaxed(&st->waiters, n);
    /* High-water mark.  Counted here, before the wait duration is known, so
       that waits later filtered out by the threshold still count.  The only
       concurrent writer is the aggregator's reset (an exchange); losing that
       race just attributes `n` to the next window, where it also occurred. */
    if (n > _Py_atomic_load_ssize_relaxed(&st->max_waiters_hw)) {
        _Py_atomic_store_ssize_relaxed(&st->max_waiters_hw, n);
    }
    *waiters_at_entry = n > UINT32_MAX ? UINT32_MAX : (uint32_t)n;
}

/* Called under gil->mutex when the wait loop exits (on every path). */
static inline PyTime_t
_PyGILStats_EndWait(struct _gil_stats_state *st, PyTime_t t0)
{
    PyTime_t t1;
    (void)PyTime_PerfCounterRaw(&t1);
    _Py_atomic_store_ssize_relaxed(
        &st->waiters, _Py_atomic_load_ssize_relaxed(&st->waiters) - 1);
    return t1 > t0 ? t1 - t0 : 0;
}

/* Called by the new GIL holder, still under gil->mutex. */
static inline void
_PyGILStats_RecordWait(struct _gil_stats_state *st,
                       PyTime_t wait_ns, uint32_t waiters_at_entry)
{
    uint64_t us = (uint64_t)wait_ns / 1000;
    if (us > UINT32_MAX) {
        us = UINT32_MAX;
    }
    if (us < _Py_atomic_load_uint32_relaxed(&st->threshold_us)) {
        _PyGILStats_HolderInc(&st->below_threshold);
        return;
    }
    struct _gil_stats_ring *ring = _Py_atomic_load_ptr_relaxed(&st->ring);
    if (ring == NULL) {
        return;
    }
    /* SPSC push.  `head` is ours (relaxed); `tail` is the consumer's
       (acquire, pairs with its release store after reading the slots). */
    uint64_t head = _Py_atomic_load_uint64_relaxed(&ring->head);
    uint64_t tail = _Py_atomic_load_uint64_acquire(&ring->tail);
    if (head - tail > ring->mask) {
        _PyGILStats_HolderInc(&st->dropped);
        return;
    }
    _PyGILStatsEvent *ev = &ring->events[head & ring->mask];
    ev->wait_us = (uint32_t)us;
    ev->waiters_at_entry = waiters_at_entry;
    /* Publish the slot (pairs with the consumer's acquire load of head). */
    _Py_atomic_store_uint64_release(&ring->head, head + 1);
}


/* Aggregator / API side (Python/gil_stats.c). */

typedef struct {
    uint32_t ring_size;           /* power of two, >= 2 */
    uint32_t threshold_us;
    double drain_interval_min;    /* seconds */
    double drain_interval_max;    /* seconds */
} _PyGILStatsConfig;

#define _PyGILSTATS_DEFAULT_RING_SIZE 65536
#define _PyGILSTATS_DEFAULT_THRESHOLD_US 100
#define _PyGILSTATS_DEFAULT_DRAIN_MIN 0.01
#define _PyGILSTATS_DEFAULT_DRAIN_MAX 2.0
#define _PyGILSTATS_DEFAULT_PERIOD 60.0
#define _PyGILSTATS_DEFAULT_MIN_EVENTS 1000
#define _PyGILSTATS_DEFAULT_MAX_PERIOD 600.0

extern void _PyGILStats_InitMain(PyThreadState *tstate);
extern void _PyGILStats_Fini(PyThreadState *tstate);
extern void _PyGILStats_AfterForkChild(PyThreadState *tstate);
/* OS threads owned by GIL statistics that are safe across fork()
   (0 or 1).  Used to keep them out of the multi-threaded fork warning. */
extern Py_ssize_t _PyGILStats_ForkSafeThreadCount(void);

/* The functions below must be called with the main interpreter's GIL held.
   They return -1 with an exception set on error. */
extern int _PyGILStats_Enable(PyInterpreterState *interp,
                              const _PyGILStatsConfig *config);
extern int _PyGILStats_Disable(PyInterpreterState *interp);
extern int _PyGILStats_IsEnabled(PyInterpreterState *interp);
extern int _PyGILStats_SetCallback(PyInterpreterState *interp,
                                   PyObject *callback, double period,
                                   uint64_t min_events, double max_period);
/* Last completed window as a dict, or None if none was published yet. */
extern PyObject* _PyGILStats_Snapshot(PyInterpreterState *interp);
/* Test helper: summarize synthetic waits with the aggregator's code. */
extern PyObject* _PyGILStats_Summarize(PyObject *waits,
                                       uint64_t below_threshold,
                                       uint32_t threshold_us);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GIL_STATS_H */
