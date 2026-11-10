# GIL contention statistics (`_gilstats`)

This build of CPython 3.13 can measure GIL contention, which is how long
threads wait for the GIL and how many wait at the same time. It is designed
to stay on in production. When it is off, it costs one atomic load per GIL
acquisition. When it is on, only *contended* acquisitions are timed, and a
background thread does all of the aggregation.

`_gilstats` is a private module of this build, and its name and API may
change if the feature goes upstream.

## Enabling

The simplest way is to set the environment variable, which uses the default
settings:

```sh
PYTHONGILSTATS=1 python app.py
```

The other way is to enable it from code, for example during service startup:

```python
import _gilstats

_gilstats.enable()            # defaults shown under "Tuning"
```

Calling `enable()` again restarts collection with the new settings.
`_gilstats.disable()` stops collection, and `_gilstats.is_enabled()` reports
whether it is running. Running `python -E` ignores `PYTHONGILSTATS`.

## Getting the data

Data is reported in **windows**. A window is published once `period` seconds
have passed **and** it contains at least `min_events` contended waits. A
window is also published once `max_period` seconds have passed, even if it
has fewer events, so a quiet process still reports.

There are two ways to consume windows:

- **Push:** register a callback. The background thread calls it with each
  window as a `dict`.
- **Pull:** call `_gilstats.snapshot()`. It returns the last published window,
  or `None` if no window has been published yet.

### Sample callback

```python
import queue
import _gilstats

metrics_queue = queue.SimpleQueue()

def on_window(s):
    # Runs on the stats thread with the GIL held: keep it quick and never
    # block (no network I/O here). Hand the data to your metrics reporter.
    secs = (s["window_end_ns"] - s["window_start_ns"]) / 1e9
    metrics_queue.put({
        "gil.wait.p50_us":   s["p50_us"],
        "gil.wait.p99_us":   s["p99_us"],
        "gil.wait.p999_us":  s["p999_us"],
        "gil.wait.max_us":   s["max_wait_us"],
        "gil.waiters.max":   s["max_waiters"],
        "gil.contended_per_sec": (s["count"] + s["below_threshold"]) / secs,
        "gil.stats.dropped": s["dropped"],
    })

_gilstats.enable()
_gilstats.set_callback(on_window, period=60.0, min_events=1000,
                       max_period=600.0)
```

Exceptions raised by the callback go to `sys.unraisablehook` and do not stop
collection. `set_callback(None)` removes the callback.

## Fields

Times are in microseconds unless the name says otherwise. "Contended" means
the GIL was held by another thread when the thread asked for it.

| Field | Meaning |
|---|---|
| `p50_us`, `p90_us`, `p99_us`, `p999_us` | Quantiles of contended wait time. Each one is an upper bound that is accurate to within 2×, and is capped at the exact max. If a quantile falls among the short, unrecorded waits, the value is `record_threshold_us`. The value is `None` if the window had no contended waits. |
| `max_wait_us` | The exact longest wait in the window. |
| `top_waits` | The 16 longest waits as `(wait_us, waiters_at_entry)` pairs, longest first. Values are exact. |
| `max_waiters` | The most threads waiting at once. It includes short waits that were filtered out. |
| `current_waiters` | The number of threads waiting when the window closed. |
| `count` | Contended waits ≥ `record_threshold_us`. These are the waits in `buckets` and `top_waits`. |
| `below_threshold` | Contended waits shorter than the threshold. They are counted but not recorded individually. |
| `uncontended` | Acquisitions that did not have to wait. They are not part of the quantiles. |
| `sum_us` | The total wait time of the `count` recorded waits. `sum_us / count` is the mean of those waits. |
| `buckets` | The histogram as `(upper_us, n)` pairs. A bucket holds waits in `[upper_us/2, upper_us)`. Empty buckets are left out. |
| `dropped` | Waits lost because the ring buffer was full. This should be 0; see "Tuning". |
| `record_threshold_us`, `drain_interval` | The settings that were in effect for the window. `drain_interval` is in seconds. |
| `window_start_ns`, `window_end_ns` | The window bounds on the `time.monotonic_ns()` clock. |

## Tuning

| Setting | Default | When to change it |
|---|---|---|
| `enable(record_threshold_us=)` | 100 | Raise it to focus on outliers and reduce traffic. Lower it, or set it to 0 to record everything, if you need detail on short waits. `max_waiters` and the quantiles still count the filtered waits either way. |
| `enable(ring_size=)` | 65536 (512 KiB) | Raise it if `dropped` is ever above 0. It must be a power of two. Each slot is 8 bytes. |
| `set_callback(period=, min_events=, max_period=)` | 60 s, 1000, 600 s | Raise `min_events` for steadier quantiles. Lower `max_period` if quiet processes need to report more often. |
| `enable(drain_interval_min=, drain_interval_max=)` | 0.01 s, 2 s | These bound how often the stats thread wakes up. The interval adapts to load on its own, so you rarely need to change these. |

## Caveats

- **The callback briefly takes the GIL.** It happens once per window, and
  other threads wait for it like for any GIL holder. The stats thread does
  not count its own waits.
- **Shutdown waits for a running callback.** A callback that blocks forever
  hangs interpreter exit. Do slow work, such as sending metrics, elsewhere.
- **Unreported data is dropped at exit.** The window in progress at shutdown
  is never reported. If a short-lived process exits before its first window
  is published, nothing is reported for it.
- **Main interpreter only.** The functions raise `RuntimeError` in
  subinterpreters.
- **Not available in free-threaded builds.** There `enable()` raises
  `NotImplementedError`.
- **Forking is supported.** The child process continues collecting with fresh
  counters, and the stats thread does not trigger the "multi-threaded fork()"
  `DeprecationWarning`.
