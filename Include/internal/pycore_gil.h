#ifndef Py_INTERNAL_GIL_H
#define Py_INTERNAL_GIL_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_condvar.h"       // PyCOND_T

#ifndef Py_HAVE_CONDVAR
#  error You need either a POSIX-compatible or a Windows system!
#endif

/* Enable if you want to force the switching of threads at least
   every `interval`. */
#undef FORCE_SWITCHING
#define FORCE_SWITCHING

/* GIL contention statistics (see Python/gil_stats.c).

   Fields marked "holder-only" are written only by the thread that has just
   acquired the GIL, while it still holds gil->mutex, so writers are
   serialized by the mutex and need no read-modify-write.  They are atomics
   only so that the aggregator thread can read them without the mutex. */
struct _gil_stats_ring;
struct _gil_stats_agg;

struct _gil_stats_state {
    /* Non-zero when collection is enabled.  Read relaxed in take_gil(). */
    int enabled;
    /* Contended waits shorter than this are only counted (below_threshold),
       not pushed to the ring. */
    uint32_t threshold_us;
    /* Threads currently waiting in take_gil()'s wait loop.  Modified under
       gil->mutex. */
    Py_ssize_t waiters;
    /* High-water mark of `waiters` since the aggregator last reset it.
       Updated under gil->mutex; reset by the aggregator via exchange. */
    Py_ssize_t max_waiters_hw;
    /* Cumulative counters (holder-only). */
    uint64_t uncontended;
    uint64_t below_threshold;
    uint64_t dropped;
    /* No lock event ring: produced by the GIL holder, consumed by the
       aggregator. Only replaced while the caller holds the GIL. */
    struct _gil_stats_ring *ring;
    /* Thread state used by the aggregator to run the Python callback;
       its own GIL acquisitions are excluded from the statistics. */
    PyThreadState *agg_tstate;
    /* Aggregator-side state (thread handle, histogram, snapshots). */
    struct _gil_stats_agg *agg;
};

struct _gil_runtime_state {
#ifdef Py_GIL_DISABLED
    /* If this GIL is disabled, enabled == 0.

       If this GIL is enabled transiently (most likely to initialize a module
       of unknown safety), enabled indicates the number of active transient
       requests.

       If this GIL is enabled permanently, enabled == INT_MAX.

       It must not be modified directly; use _PyEval_EnableGILTransiently(),
       _PyEval_EnableGILPermanently(), and _PyEval_DisableGIL()

       It is always read and written atomically, but a thread can assume its
       value will be stable as long as that thread is attached or knows that no
       other threads are attached (e.g., during a stop-the-world.). */
    int enabled;
#endif
    /* microseconds (the Python API uses seconds, though) */
    unsigned long interval;
    /* Last PyThreadState holding / having held the GIL. This helps us
       know whether anyone else was scheduled after we dropped the GIL. */
    PyThreadState* last_holder;
    /* Whether the GIL is already taken (-1 if uninitialized). This is
       atomic because it can be read without any lock taken in ceval.c. */
    int locked;
    /* Number of GIL switches since the beginning. */
    unsigned long switch_number;
    /* This condition variable allows one or several threads to wait
       until the GIL is released. In addition, the mutex also protects
       the above variables. */
    PyCOND_T cond;
    PyMUTEX_T mutex;
#ifdef FORCE_SWITCHING
    /* This condition variable helps the GIL-releasing thread wait for
       a GIL-awaiting thread to be scheduled and take the GIL. */
    PyCOND_T switch_cond;
    PyMUTEX_T switch_mutex;
#endif
    struct _gil_stats_state stats;
};

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GIL_H */
