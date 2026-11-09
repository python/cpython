/* GIL contention statistics: aggregator thread and API.

   Overview
   --------
   take_gil() (Python/ceval_gil.c) is the producer.  When stats are enabled
   it timestamps only contended acquisitions, tracks the number of waiters
   and its high-water mark, and pushes one 8-byte event per contended wait
   that is at least `threshold_us` long into a lock-free SPSC ring
   (see Include/internal/pycore_gil_stats.h).

   A native aggregator thread, which does not hold the GIL, drains the ring
   into the current window:
     - a log2 histogram over microseconds (bucket i holds [2**(i-1), 2**i),
       bucket 0 holds 0), giving quantiles accurate to within 2x;
     - exact max and the top-K longest waits.
   The drain interval adapts to how full the ring was (halved above 1/2,
   doubled below 1/8, within [drain_min, drain_max]).  Draining is decoupled
   from the reporting window: a window is published once `period` has
   elapsed and it holds at least `min_events` events, or once `max_period`
   has elapsed.

   Publishing uses double buffering guarded by a sequence counter, so
   snapshot() can copy the last complete window without locks while the
   aggregator keeps working.  If a Python callback is registered, the
   aggregator attaches its own thread state, calls it with a dict, and
   detaches.  Its own GIL acquisitions are excluded from the statistics.
*/

#include "Python.h"
#include "pycore_bitutils.h"      // _Py_bit_length()
#include "pycore_gil_stats.h"
#include "pycore_initconfig.h"    // _Py_GetEnv()
#include "pycore_interp.h"        // PyInterpreterState.ceval
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_pythread.h"      // PyThread_start_joinable_thread()

#include "condvar.h"

#define NBUCKETS 33
#define TOPK 16

/* All fields are uint64_t so a window can be copied word by word with
   atomic loads/stores (see publish_window() / read_published()). */
typedef struct {
    uint64_t count;              /* waits that reached the ring */
    uint64_t sum_us;
    uint64_t max_wait_us;
    uint64_t max_waiters;
    uint64_t current_waiters;
    uint64_t uncontended;
    uint64_t below_threshold;
    uint64_t dropped;
    uint64_t threshold_us;
    uint64_t window_start_ns;
    uint64_t window_end_ns;
    uint64_t drain_interval_us;
    uint64_t ntop;
    uint64_t buckets[NBUCKETS];
    /* min-heap on top_wait_us */
    uint64_t top_wait_us[TOPK];
    uint64_t top_waiters[TOPK];
} gilstats_window;

#define WINDOW_WORDS (sizeof(gilstats_window) / sizeof(uint64_t))

enum {
    AGG_STOPPED = 0,
    AGG_RUNNING = 1,
    AGG_STOPPING = 2,
};

struct _gil_stats_agg {
    /* --- Written with the GIL held --- */
    int state;                   /* AGG_* */
    PyThread_handle_t handle;
    PyObject *callback;          /* only touched with the GIL held */

    /* --- Sleep/wake of the aggregator thread --- */
    PyMUTEX_T mutex;
    PyCOND_T cond;
    int stop;                    /* protected by mutex */

    /* --- Config: written with the GIL held, read by the aggregator --- */
    int has_callback;            /* atomic */
    uint64_t period_ns;          /* atomic */
    uint64_t min_events;         /* atomic */
    uint64_t max_period_ns;      /* atomic */
    /* Immutable while the thread runs. */
    int64_t drain_min_us;
    int64_t drain_max_us;

    /* --- Aggregator-private --- */
    int64_t drain_us;
    gilstats_window cur;
    uint64_t base_uncontended;
    uint64_t base_below;
    uint64_t base_dropped;

    /* --- Published windows: double buffer + sequence counter ---
       Publication k (k = 1, 2, ...) writes pub[k & 1] while seq is odd
       (2k - 1) and then sets seq = 2k.  Readers copy pub[(seq / 2) & 1],
       the last complete window; it is only rewritten by publication
       seq/2 + 2, which starts when seq reaches seq + 3. */
    uint64_t seq;                /* atomic */
    gilstats_window pub[2];      /* accessed with relaxed atomics */
};


/* ---------------------------------------------------------------------- */
/* Window accumulation (aggregator thread, or the test helper)            */

static inline int
bucket_index(uint64_t us)
{
    /* 0 -> 0; [2**(i-1), 2**i) -> i; i <= 32 since us <= UINT32_MAX. */
    return us == 0 ? 0 : _Py_bit_length((unsigned long)us);
}

static void
topk_sift_down(gilstats_window *w, uint64_t i)
{
    uint64_t n = w->ntop;
    for (;;) {
        uint64_t l = 2 * i + 1, r = l + 1, m = i;
        if (l < n && w->top_wait_us[l] < w->top_wait_us[m]) {
            m = l;
        }
        if (r < n && w->top_wait_us[r] < w->top_wait_us[m]) {
            m = r;
        }
        if (m == i) {
            return;
        }
        uint64_t tw = w->top_wait_us[i], tn = w->top_waiters[i];
        w->top_wait_us[i] = w->top_wait_us[m];
        w->top_waiters[i] = w->top_waiters[m];
        w->top_wait_us[m] = tw;
        w->top_waiters[m] = tn;
        i = m;
    }
}

static void
topk_insert(gilstats_window *w, uint64_t us, uint64_t waiters)
{
    if (w->ntop < TOPK) {
        /* sift up */
        uint64_t i = w->ntop++;
        while (i > 0) {
            uint64_t p = (i - 1) / 2;
            if (w->top_wait_us[p] <= us) {
                break;
            }
            w->top_wait_us[i] = w->top_wait_us[p];
            w->top_waiters[i] = w->top_waiters[p];
            i = p;
        }
        w->top_wait_us[i] = us;
        w->top_waiters[i] = waiters;
    }
    else if (us > w->top_wait_us[0]) {
        w->top_wait_us[0] = us;
        w->top_waiters[0] = waiters;
        topk_sift_down(w, 0);
    }
}

static void
window_add(gilstats_window *w, uint64_t us, uint64_t waiters)
{
    w->count++;
    w->sum_us += us;
    if (us > w->max_wait_us) {
        w->max_wait_us = us;
    }
    if (waiters > w->max_waiters) {
        w->max_waiters = waiters;
    }
    w->buckets[bucket_index(us)]++;
    topk_insert(w, us, waiters);
}

/* Returns the quantile as an inclusive upper bound in microseconds, or -1
   if the window is empty.  Waits below the threshold form the bottom of
   the distribution and are reported as threshold_us. */
static int64_t
window_quantile(const gilstats_window *w, double q)
{
    uint64_t total = w->below_threshold + w->count;
    if (total == 0) {
        return -1;
    }
    uint64_t rank = (uint64_t)(q * (double)total);
    if ((double)rank < q * (double)total) {
        rank++;
    }
    if (rank == 0) {
        rank = 1;
    }
    uint64_t cum = w->below_threshold;
    if (rank <= cum) {
        return (int64_t)w->threshold_us;
    }
    for (int i = 0; i < NBUCKETS; i++) {
        cum += w->buckets[i];
        if (rank <= cum) {
            uint64_t ub = i == 0 ? 0 : (((uint64_t)1 << i) - 1);
            return (int64_t)(ub < w->max_wait_us ? ub : w->max_wait_us);
        }
    }
    return (int64_t)w->max_wait_us;
}

static int
dict_set_u64(PyObject *d, const char *key, uint64_t v)
{
    PyObject *o = PyLong_FromUnsignedLongLong(v);
    if (o == NULL) {
        return -1;
    }
    int r = PyDict_SetItemString(d, key, o);
    Py_DECREF(o);
    return r;
}

static int
dict_set_quantile(PyObject *d, const char *key, const gilstats_window *w,
                  double q)
{
    int64_t v = window_quantile(w, q);
    PyObject *o = v < 0 ? Py_NewRef(Py_None) : PyLong_FromLongLong(v);
    if (o == NULL) {
        return -1;
    }
    int r = PyDict_SetItemString(d, key, o);
    Py_DECREF(o);
    return r;
}

static PyObject *
window_to_dict(const gilstats_window *w)
{
    PyObject *d = PyDict_New();
    if (d == NULL) {
        return NULL;
    }
    PyObject *buckets = NULL, *top = NULL, *tmp = NULL;
#define SET(name, v) if (dict_set_u64(d, name, (v)) < 0) goto error
    SET("count", w->count);
    SET("uncontended", w->uncontended);
    SET("below_threshold", w->below_threshold);
    SET("sum_us", w->sum_us);
    SET("max_wait_us", w->max_wait_us);
    SET("max_waiters", w->max_waiters);
    SET("current_waiters", w->current_waiters);
    SET("dropped", w->dropped);
    SET("record_threshold_us", w->threshold_us);
    SET("window_start_ns", w->window_start_ns);
    SET("window_end_ns", w->window_end_ns);
#undef SET
    if (dict_set_quantile(d, "p50_us", w, 0.5) < 0 ||
        dict_set_quantile(d, "p90_us", w, 0.9) < 0 ||
        dict_set_quantile(d, "p99_us", w, 0.99) < 0 ||
        dict_set_quantile(d, "p999_us", w, 0.999) < 0)
    {
        goto error;
    }

    tmp = PyFloat_FromDouble((double)w->drain_interval_us / 1e6);
    if (tmp == NULL || PyDict_SetItemString(d, "drain_interval", tmp) < 0) {
        goto error;
    }
    Py_CLEAR(tmp);

    /* buckets: [(upper_us, count), ...]; bucket holds [upper/2, upper),
       and the first one holds 0 (upper_us == 1). */
    buckets = PyList_New(0);
    if (buckets == NULL) {
        goto error;
    }
    for (int i = 0; i < NBUCKETS; i++) {
        if (w->buckets[i] == 0) {
            continue;
        }
        tmp = Py_BuildValue("(KK)", (unsigned long long)((uint64_t)1 << i),
                            (unsigned long long)w->buckets[i]);
        if (tmp == NULL || PyList_Append(buckets, tmp) < 0) {
            goto error;
        }
        Py_CLEAR(tmp);
    }
    if (PyDict_SetItemString(d, "buckets", buckets) < 0) {
        goto error;
    }
    Py_CLEAR(buckets);

    /* top_waits: [(wait_us, waiters_at_entry), ...], longest first. */
    uint64_t n = w->ntop < TOPK ? w->ntop : TOPK;
    top = PyList_New((Py_ssize_t)n);
    if (top == NULL) {
        goto error;
    }
    for (uint64_t i = 0; i < n; i++) {
        tmp = Py_BuildValue("(KK)", (unsigned long long)w->top_wait_us[i],
                            (unsigned long long)w->top_waiters[i]);
        if (tmp == NULL) {
            goto error;
        }
        PyList_SET_ITEM(top, (Py_ssize_t)i, tmp);
        tmp = NULL;
    }
    if (PyList_Sort(top) < 0 || PyList_Reverse(top) < 0 ||
        PyDict_SetItemString(d, "top_waits", top) < 0)
    {
        goto error;
    }
    Py_CLEAR(top);
    return d;

error:
    Py_XDECREF(tmp);
    Py_XDECREF(buckets);
    Py_XDECREF(top);
    Py_DECREF(d);
    return NULL;
}


/* ---------------------------------------------------------------------- */
/* Publication (double buffer + sequence counter)                         */

static void
publish_window(struct _gil_stats_agg *agg, const gilstats_window *w)
{
    /* Only the aggregator thread (or a caller holding the GIL while the
       aggregator is stopped) publishes. */
    uint64_t s = _Py_atomic_load_uint64_relaxed(&agg->seq);
    assert(s % 2 == 0);
    uint64_t k = s / 2 + 1;
    _Py_atomic_store_uint64_relaxed(&agg->seq, s + 1);
    _Py_atomic_fence_release();
    uint64_t *dst = (uint64_t *)&agg->pub[k & 1];
    const uint64_t *src = (const uint64_t *)w;
    for (size_t i = 0; i < WINDOW_WORDS; i++) {
        _Py_atomic_store_uint64_relaxed(&dst[i], src[i]);
    }
    _Py_atomic_store_uint64_release(&agg->seq, s + 2);
}

/* Returns 0 if no window was published yet. */
static int
read_published(struct _gil_stats_agg *agg, gilstats_window *out)
{
    for (;;) {
        uint64_t s1 = _Py_atomic_load_uint64_acquire(&agg->seq);
        uint64_t k = s1 / 2;      /* last complete publication */
        if (k == 0) {
            return 0;
        }
        const uint64_t *src = (const uint64_t *)&agg->pub[k & 1];
        uint64_t *dst = (uint64_t *)out;
        for (size_t i = 0; i < WINDOW_WORDS; i++) {
            dst[i] = _Py_atomic_load_uint64_relaxed(&src[i]);
        }
        _Py_atomic_fence_acquire();
        uint64_t s2 = _Py_atomic_load_uint64_relaxed(&agg->seq);
        /* pub[k & 1] is rewritten by publication k + 2, which starts when
           seq becomes 2(k + 1) + 1. */
        if (s2 < 2 * k + 3) {
            return 1;
        }
    }
}


/* ---------------------------------------------------------------------- */
/* Aggregator thread                                                      */

static uint64_t
now_ns(void)
{
    PyTime_t t;
    (void)PyTime_MonotonicRaw(&t);
    return (uint64_t)t;
}

static void
window_reset(struct _gil_stats_state *st, struct _gil_stats_agg *agg,
             uint64_t start_ns)
{
    memset(&agg->cur, 0, sizeof(agg->cur));
    agg->cur.window_start_ns = start_ns;
    agg->base_uncontended = _Py_atomic_load_uint64_relaxed(&st->uncontended);
    agg->base_below = _Py_atomic_load_uint64_relaxed(&st->below_threshold);
    agg->base_dropped = _Py_atomic_load_uint64_relaxed(&st->dropped);
}

/* Moves all pending events into the current window; returns how many. */
static uint64_t
drain_ring(struct _gil_stats_state *st, struct _gil_stats_agg *agg)
{
    struct _gil_stats_ring *ring = _Py_atomic_load_ptr_relaxed(&st->ring);
    if (ring == NULL) {
        return 0;
    }
    /* Pairs with the producer's release store of head. */
    uint64_t head = _Py_atomic_load_uint64_acquire(&ring->head);
    uint64_t tail = _Py_atomic_load_uint64_relaxed(&ring->tail);
    for (uint64_t i = tail; i != head; i++) {
        const _PyGILStatsEvent *ev = &ring->events[i & ring->mask];
        window_add(&agg->cur, ev->wait_us, ev->waiters_at_entry);
    }
    /* Hand the slots back (pairs with the producer's acquire of tail). */
    _Py_atomic_store_uint64_release(&ring->tail, head);
    return head - tail;
}

static void
adapt_drain_interval(struct _gil_stats_agg *agg, uint64_t drained,
                     uint64_t capacity)
{
    if (drained > capacity / 2) {
        agg->drain_us /= 2;
    }
    else if (drained < capacity / 8) {
        agg->drain_us *= 2;
    }
    if (agg->drain_us < agg->drain_min_us) {
        agg->drain_us = agg->drain_min_us;
    }
    if (agg->drain_us > agg->drain_max_us) {
        agg->drain_us = agg->drain_max_us;
    }
}

/* A window is due once `period` elapsed and it holds at least `min_events`
   contended waits (recorded + below threshold, i.e. the population the
   quantiles are computed over), or once `max_period` elapsed. */
static int
publish_due(struct _gil_stats_state *st, struct _gil_stats_agg *agg,
            uint64_t now)
{
    uint64_t elapsed = now - agg->cur.window_start_ns;
    if (elapsed >= _Py_atomic_load_uint64_relaxed(&agg->max_period_ns)) {
        return 1;
    }
    if (elapsed < _Py_atomic_load_uint64_relaxed(&agg->period_ns)) {
        return 0;
    }
    uint64_t below = (_Py_atomic_load_uint64_relaxed(&st->below_threshold)
                      - agg->base_below);
    return (agg->cur.count + below
            >= _Py_atomic_load_uint64_relaxed(&agg->min_events));
}

/* How long to sleep: the drain interval, shortened so that we wake up when
   the window becomes due. */
static int64_t
sleep_us(struct _gil_stats_agg *agg, uint64_t now)
{
    int64_t us = agg->drain_us;
    uint64_t elapsed = now - agg->cur.window_start_ns;
    uint64_t period = _Py_atomic_load_uint64_relaxed(&agg->period_ns);
    uint64_t max_period = _Py_atomic_load_uint64_relaxed(&agg->max_period_ns);
    uint64_t deadline = elapsed < period ? period : max_period;
    if (elapsed < deadline) {
        uint64_t left_us = (deadline - elapsed + 999) / 1000;
        if ((uint64_t)us > left_us) {
            us = (int64_t)left_us;
        }
    }
    return us > 0 ? us : 1;
}

static void
close_window(struct _gil_stats_state *st, struct _gil_stats_agg *agg,
             uint64_t now, gilstats_window *done)
{
    gilstats_window *w = &agg->cur;
    Py_ssize_t cur_waiters = _Py_atomic_load_ssize_relaxed(&st->waiters);
    if (cur_waiters < 0) {
        cur_waiters = 0;
    }
    /* Reset the high-water mark to the current count, not 0, so threads
       still waiting are counted in the next window too. */
    Py_ssize_t hw = _Py_atomic_exchange_ssize(&st->max_waiters_hw,
                                              cur_waiters);
    if ((uint64_t)hw > w->max_waiters) {
        w->max_waiters = (uint64_t)hw;
    }
    w->current_waiters = (uint64_t)cur_waiters;
    uint64_t unc = _Py_atomic_load_uint64_relaxed(&st->uncontended);
    uint64_t below = _Py_atomic_load_uint64_relaxed(&st->below_threshold);
    uint64_t dropped = _Py_atomic_load_uint64_relaxed(&st->dropped);
    w->uncontended = unc - agg->base_uncontended;
    w->below_threshold = below - agg->base_below;
    w->dropped = dropped - agg->base_dropped;
    w->threshold_us = _Py_atomic_load_uint32_relaxed(&st->threshold_us);
    w->window_end_ns = now;
    w->drain_interval_us = (uint64_t)agg->drain_us;
    *done = *w;

    memset(w, 0, sizeof(*w));
    w->window_start_ns = now;
    agg->base_uncontended = unc;
    agg->base_below = below;
    agg->base_dropped = dropped;
}

static void
run_callback(PyInterpreterState *interp, struct _gil_stats_state *st,
             struct _gil_stats_agg *agg, const gilstats_window *w)
{
    PyThreadState *ts = _Py_atomic_load_ptr_relaxed(&st->agg_tstate);
    if (ts == NULL) {
        /* Binds the new thread state to this OS thread.  Does not need the
           GIL. */
        ts = PyThreadState_New(interp);
        if (ts == NULL) {
            return;
        }
        /* Set before our first take_gil() so it is excluded. */
        _Py_atomic_store_ptr_relaxed(&st->agg_tstate, ts);
    }
    PyEval_RestoreThread(ts);
    PyObject *cb = Py_XNewRef(agg->callback);
    if (cb != NULL) {
        PyObject *d = window_to_dict(w);
        if (d != NULL) {
            PyObject *res = PyObject_CallOneArg(cb, d);
            Py_XDECREF(res);
            Py_DECREF(d);
        }
        if (PyErr_Occurred()) {
            PyErr_FormatUnraisable(
                "Exception ignored in GIL statistics callback %R", cb);
        }
        Py_DECREF(cb);
    }
    (void)PyEval_SaveThread();
}

static void
aggregator_main(void *arg)
{
    PyInterpreterState *interp = (PyInterpreterState *)arg;
    struct _gil_stats_state *st = &interp->ceval.gil->stats;
    struct _gil_stats_agg *agg = st->agg;
    struct _gil_stats_ring *ring = _Py_atomic_load_ptr_relaxed(&st->ring);
    uint64_t capacity = ring->mask + 1;

    PyMUTEX_LOCK(&agg->mutex);
    while (!agg->stop) {
        (void)PyCOND_TIMEDWAIT(&agg->cond, &agg->mutex,
                               sleep_us(agg, now_ns()));
        if (agg->stop) {
            break;
        }
        PyMUTEX_UNLOCK(&agg->mutex);

        uint64_t drained = drain_ring(st, agg);
        adapt_drain_interval(agg, drained, capacity);
        uint64_t now = now_ns();
        if (publish_due(st, agg, now)) {
            gilstats_window done;
            close_window(st, agg, now, &done);
            publish_window(agg, &done);
            if (_Py_atomic_load_int_relaxed(&agg->has_callback)) {
                run_callback(interp, st, agg, &done);
            }
        }

        PyMUTEX_LOCK(&agg->mutex);
    }
    PyMUTEX_UNLOCK(&agg->mutex);

    /* The thread disabling us released the GIL while joining. */
    PyThreadState *ts = _Py_atomic_load_ptr_relaxed(&st->agg_tstate);
    if (ts != NULL) {
        PyEval_RestoreThread(ts);
        PyThreadState_Clear(ts);
        _Py_atomic_store_ptr_relaxed(&st->agg_tstate, NULL);
        PyThreadState_DeleteCurrent();
    }
}


/* ---------------------------------------------------------------------- */
/* API (GIL held)                                                         */

static struct _gil_stats_state *
get_stats(PyInterpreterState *interp)
{
    assert(interp == _PyInterpreterState_Main());
    return &interp->ceval.gil->stats;
}

static int64_t
seconds_to_us(double s)
{
    return (int64_t)(s * 1e6);
}

static struct _gil_stats_agg *
get_agg(struct _gil_stats_state *st)
{
    if (st->agg != NULL) {
        return st->agg;
    }
    struct _gil_stats_agg *agg = PyMem_RawCalloc(1, sizeof(*agg));
    if (agg == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    if (PyMUTEX_INIT(&agg->mutex) != 0) {
        PyMem_RawFree(agg);
        PyErr_SetString(PyExc_RuntimeError, "can't initialize mutex");
        return NULL;
    }
    if (PyCOND_INIT(&agg->cond) != 0) {
        PyMUTEX_FINI(&agg->mutex);
        PyMem_RawFree(agg);
        PyErr_SetString(PyExc_RuntimeError, "can't initialize condvar");
        return NULL;
    }
    agg->period_ns = (uint64_t)(_PyGILSTATS_DEFAULT_PERIOD * 1e9);
    agg->min_events = _PyGILSTATS_DEFAULT_MIN_EVENTS;
    agg->max_period_ns = (uint64_t)(_PyGILSTATS_DEFAULT_MAX_PERIOD * 1e9);
    st->agg = agg;
    return agg;
}

static struct _gil_stats_ring *
ring_new(uint32_t capacity)
{
    size_t size = offsetof(struct _gil_stats_ring, events)
                  + (size_t)capacity * sizeof(_PyGILStatsEvent);
    struct _gil_stats_ring *ring = PyMem_RawCalloc(1, size);
    if (ring == NULL) {
        return NULL;
    }
    ring->mask = capacity - 1;
    return ring;
}

/* Starts the aggregator thread; the ring must already be installed. */
static int
start_aggregator(PyInterpreterState *interp, struct _gil_stats_state *st,
                 struct _gil_stats_agg *agg)
{
    agg->stop = 0;
    agg->drain_us = seconds_to_us(0.25);
    if (agg->drain_us < agg->drain_min_us) {
        agg->drain_us = agg->drain_min_us;
    }
    if (agg->drain_us > agg->drain_max_us) {
        agg->drain_us = agg->drain_max_us;
    }
    window_reset(st, agg, now_ns());
    _Py_atomic_store_ssize_relaxed(
        &st->max_waiters_hw, _Py_atomic_load_ssize_relaxed(&st->waiters));

    PyThread_ident_t ident;
    if (PyThread_start_joinable_thread(aggregator_main, interp,
                                       &ident, &agg->handle) != 0) {
        return -1;
    }
    agg->state = AGG_RUNNING;
    _Py_atomic_store_int_relaxed(&st->enabled, 1);
    return 0;
}

int
_PyGILStats_Enable(PyInterpreterState *interp, const _PyGILStatsConfig *cfg)
{
#ifdef Py_GIL_DISABLED
    PyErr_SetString(PyExc_NotImplementedError,
                    "GIL statistics are not supported on free-threaded builds");
    return -1;
#else
    struct _gil_stats_state *st = get_stats(interp);
    uint32_t size = cfg->ring_size;
    if (size < 2 || (size & (size - 1)) != 0) {
        PyErr_SetString(PyExc_ValueError,
                        "ring_size must be a power of two >= 2");
        return -1;
    }
    if (!(cfg->drain_interval_min > 0) ||
        !(cfg->drain_interval_max >= cfg->drain_interval_min) ||
        cfg->drain_interval_max > 3600)
    {
        PyErr_SetString(PyExc_ValueError,
                        "need 0 < drain_interval_min <= drain_interval_max "
                        "<= 3600");
        return -1;
    }
    struct _gil_stats_agg *agg = get_agg(st);
    if (agg == NULL) {
        return -1;
    }
    if (agg->state == AGG_STOPPING) {
        PyErr_SetString(PyExc_RuntimeError,
                        "GIL statistics are being disabled");
        return -1;
    }
    if (agg->state == AGG_RUNNING) {
        /* Reconfigure: restart with the new settings. */
        if (_PyGILStats_Disable(interp) < 0) {
            return -1;
        }
        if (agg->state != AGG_STOPPED) {
            PyErr_SetString(PyExc_RuntimeError,
                            "GIL statistics are being disabled");
            return -1;
        }
    }

    struct _gil_stats_ring *ring = ring_new(size);
    if (ring == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    agg->drain_min_us = seconds_to_us(cfg->drain_interval_min);
    agg->drain_max_us = seconds_to_us(cfg->drain_interval_max);
    if (agg->drain_min_us < 1) {
        agg->drain_min_us = 1;
    }
    if (agg->drain_max_us < agg->drain_min_us) {
        agg->drain_max_us = agg->drain_min_us;
    }
    _Py_atomic_store_uint64_relaxed(&agg->seq, 0);
    _Py_atomic_store_uint32_relaxed(&st->threshold_us, cfg->threshold_us);
    /* We hold the GIL, so no producer is in its record section. */
    _Py_atomic_store_ptr_release(&st->ring, ring);

    if (start_aggregator(interp, st, agg) < 0) {
        _Py_atomic_store_ptr_relaxed(&st->ring, NULL);
        PyMem_RawFree(ring);
        PyErr_SetString(PyExc_RuntimeError,
                        "can't start GIL statistics thread");
        return -1;
    }
    return 0;
#endif
}

int
_PyGILStats_Disable(PyInterpreterState *interp)
{
    struct _gil_stats_state *st = get_stats(interp);
    struct _gil_stats_agg *agg = st->agg;
    if (agg == NULL || agg->state != AGG_RUNNING) {
        return 0;
    }
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate == _Py_atomic_load_ptr_relaxed(&st->agg_tstate)) {
        PyErr_SetString(PyExc_RuntimeError,
                        "can't disable GIL statistics from the callback");
        return -1;
    }
    agg->state = AGG_STOPPING;
    _Py_atomic_store_int_relaxed(&st->enabled, 0);

    PyMUTEX_LOCK(&agg->mutex);
    agg->stop = 1;
    PyCOND_SIGNAL(&agg->cond);
    PyMUTEX_UNLOCK(&agg->mutex);

    /* The aggregator may need the GIL to finish a callback and to delete
       its thread state. */
    Py_BEGIN_ALLOW_THREADS
    (void)PyThread_join_thread(agg->handle);
    Py_END_ALLOW_THREADS

    /* We hold the GIL again: no producer is in its record section. */
    struct _gil_stats_ring *ring = _Py_atomic_load_ptr_relaxed(&st->ring);
    _Py_atomic_store_ptr_relaxed(&st->ring, NULL);
    PyMem_RawFree(ring);
    assert(_Py_atomic_load_ptr_relaxed(&st->agg_tstate) == NULL);
    agg->state = AGG_STOPPED;
    return 0;
}

int
_PyGILStats_IsEnabled(PyInterpreterState *interp)
{
    struct _gil_stats_state *st = get_stats(interp);
    return st->agg != NULL && st->agg->state == AGG_RUNNING;
}

int
_PyGILStats_SetCallback(PyInterpreterState *interp, PyObject *callback,
                        double period, uint64_t min_events, double max_period)
{
    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "callback must be callable or None");
        return -1;
    }
    if (!(period > 0) || !(max_period >= period) || max_period > 1e9) {
        PyErr_SetString(PyExc_ValueError,
                        "need 0 < period <= max_period");
        return -1;
    }
    struct _gil_stats_agg *agg = get_agg(get_stats(interp));
    if (agg == NULL) {
        return -1;
    }
    _Py_atomic_store_uint64_relaxed(&agg->period_ns,
                                    (uint64_t)(period * 1e9));
    _Py_atomic_store_uint64_relaxed(&agg->max_period_ns,
                                    (uint64_t)(max_period * 1e9));
    _Py_atomic_store_uint64_relaxed(&agg->min_events, min_events);
    PyObject *old = agg->callback;
    agg->callback = callback == Py_None ? NULL : Py_NewRef(callback);
    _Py_atomic_store_int_relaxed(&agg->has_callback, agg->callback != NULL);
    /* Wake the aggregator so the new period takes effect now. */
    if (agg->state == AGG_RUNNING) {
        PyMUTEX_LOCK(&agg->mutex);
        PyCOND_SIGNAL(&agg->cond);
        PyMUTEX_UNLOCK(&agg->mutex);
    }
    Py_XDECREF(old);
    return 0;
}

PyObject *
_PyGILStats_Snapshot(PyInterpreterState *interp)
{
    struct _gil_stats_agg *agg = get_stats(interp)->agg;
    gilstats_window w;
    if (agg == NULL || !read_published(agg, &w)) {
        Py_RETURN_NONE;
    }
    return window_to_dict(&w);
}

PyObject *
_PyGILStats_Summarize(PyObject *waits, uint64_t below_threshold,
                      uint32_t threshold_us)
{
    gilstats_window w;
    memset(&w, 0, sizeof(w));
    w.below_threshold = below_threshold;
    w.threshold_us = threshold_us;
    PyObject *it = PyObject_GetIter(waits);
    if (it == NULL) {
        return NULL;
    }
    PyObject *item;
    while ((item = PyIter_Next(it)) != NULL) {
        unsigned long long us, waiters = 0;
        int ok;
        if (PyTuple_Check(item)) {
            ok = PyArg_ParseTuple(item, "KK", &us, &waiters);
        }
        else {
            us = PyLong_AsUnsignedLongLong(item);
            ok = !(us == (unsigned long long)-1 && PyErr_Occurred());
        }
        Py_DECREF(item);
        if (!ok) {
            Py_DECREF(it);
            return NULL;
        }
        if (us > UINT32_MAX) {
            us = UINT32_MAX;
        }
        window_add(&w, us, waiters);
    }
    Py_DECREF(it);
    if (PyErr_Occurred()) {
        return NULL;
    }
    return window_to_dict(&w);
}


/* ---------------------------------------------------------------------- */
/* Lifecycle                                                              */

void
_PyGILStats_InitMain(PyThreadState *tstate)
{
#ifndef Py_GIL_DISABLED
    PyInterpreterState *interp = tstate->interp;
    assert(_Py_IsMainInterpreter(interp));
    const char *env = _Py_GetEnv(interp->config.use_environment,
                                 "PYTHONGILSTATS");
    if (env == NULL || env[0] == '\0' || strcmp(env, "0") == 0) {
        return;
    }
    _PyGILStatsConfig cfg = {
        .ring_size = _PyGILSTATS_DEFAULT_RING_SIZE,
        .threshold_us = _PyGILSTATS_DEFAULT_THRESHOLD_US,
        .drain_interval_min = _PyGILSTATS_DEFAULT_DRAIN_MIN,
        .drain_interval_max = _PyGILSTATS_DEFAULT_DRAIN_MAX,
    };
    if (_PyGILStats_Enable(interp, &cfg) < 0) {
        /* Metrics must never prevent startup. */
        PyErr_FormatUnraisable("Exception ignored while enabling "
                               "GIL statistics (PYTHONGILSTATS)");
    }
#else
    (void)tstate;
#endif
}

void
_PyGILStats_Fini(PyThreadState *tstate)
{
    PyInterpreterState *interp = tstate->interp;
    if (!_Py_IsMainInterpreter(interp)) {
        return;
    }
    struct _gil_stats_state *st = get_stats(interp);
    struct _gil_stats_agg *agg = st->agg;
    if (agg == NULL) {
        return;
    }
    if (_PyGILStats_Disable(interp) < 0) {
        PyErr_FormatUnraisable("Exception ignored while disabling "
                               "GIL statistics");
    }
    if (agg->state != AGG_STOPPED) {
        /* Another thread is still disabling: leak rather than race. */
        return;
    }
    _Py_atomic_store_int_relaxed(&agg->has_callback, 0);
    Py_CLEAR(agg->callback);
    PyCOND_FINI(&agg->cond);
    PyMUTEX_FINI(&agg->mutex);
    st->agg = NULL;
    PyMem_RawFree(agg);
}

Py_ssize_t
_PyGILStats_ForkSafeThreadCount(void)
{
    PyInterpreterState *interp = _PyInterpreterState_Main();
    if (interp == NULL || interp->ceval.gil == NULL) {
        return 0;
    }
    struct _gil_stats_agg *agg = interp->ceval.gil->stats.agg;
    return agg != NULL && agg->state != AGG_STOPPED;
}

void
_PyGILStats_AfterForkChild(PyThreadState *tstate)
{
    PyInterpreterState *interp = tstate->interp;
    struct _gil_stats_state *st = &interp->ceval.gil->stats;
    struct _gil_stats_agg *agg = st->agg;
    /* Threads that were waiting in the parent do not exist here. */
    _Py_atomic_store_ssize_relaxed(&st->waiters, 0);
    _Py_atomic_store_ssize_relaxed(&st->max_waiters_hw, 0);
    /* The aggregator's thread state was deleted with the other dead
       thread states. */
    _Py_atomic_store_ptr_relaxed(&st->agg_tstate, NULL);
    if (agg == NULL) {
        return;
    }
    /* The aggregator thread may have held these at fork time. */
    (void)PyMUTEX_INIT(&agg->mutex);
    (void)PyCOND_INIT(&agg->cond);
    _Py_atomic_store_uint64_relaxed(&agg->seq, 0);

    struct _gil_stats_ring *ring = _Py_atomic_load_ptr_relaxed(&st->ring);
    if (agg->state == AGG_STOPPING) {
        /* The disabling thread is gone: finish its job. */
        _Py_atomic_store_ptr_relaxed(&st->ring, NULL);
        PyMem_RawFree(ring);
        agg->state = AGG_STOPPED;
        return;
    }
    if (agg->state != AGG_RUNNING) {
        return;
    }
    /* The handle refers to a thread of the parent: don't join it. */
    assert(ring != NULL);
    _Py_atomic_store_uint64_relaxed(&ring->head, 0);
    _Py_atomic_store_uint64_relaxed(&ring->tail, 0);
    agg->state = AGG_STOPPED;
    if (start_aggregator(interp, st, agg) < 0) {
        _Py_atomic_store_int_relaxed(&st->enabled, 0);
        _Py_atomic_store_ptr_relaxed(&st->ring, NULL);
        PyMem_RawFree(ring);
    }
}
