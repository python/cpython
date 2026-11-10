# GIL statistics: notes for the upstream port

These are working notes for porting `_gilstats` (see `Misc/README.gilstats.md`)
from this 3.13 internal build to CPython `main`. They are not user
documentation.

## Free-threaded builds with the GIL enabled

**Goal:** collect GIL statistics on `--disable-gil` builds whenever the GIL is
on. That covers `PYTHON_GIL=1`, and the GIL being re-enabled automatically when
an extension module without `Py_mod_gil` is imported. Free-threaded with the
GIL is still a GIL, and waiting on it is the same problem.

**Estimate:** about 1–2 days, most of it building and testing.

### What already works

When the GIL is on, a free-threaded build goes through the same `take_gil()`
and the same `gil->mutex`. That means none of the following needs to change:

- the producer path in `take_gil()`: lazy timestamps, waiter counts and the
  high-water mark, the threshold filter, and the single-writer ring push;
- the aggregator thread, the histogram and top-K, and the seqlock double
  buffer;
- the callback attaching through `PyEval_RestoreThread`, and leaving the
  aggregator's own waits out of the stats;
- the fork handling.

If the GIL is switched off while threads wait in `take_gil()`, they leave
through the existing `#ifdef Py_GIL_DISABLED` check after the loop.
`_PyGILStats_EndWait()` already runs before that check, so `waiters` stays
balanced and nothing gets recorded.

### What breaks: "the caller holds the GIL"

`_PyGILStats_Enable`, `_PyGILStats_Disable` and `_PyGILStats_SetCallback` in
`Python/gil_stats.c` rely on the caller holding the GIL for three things:

1. **Ring buffer lifetime.** The pointer is installed and freed on the
   assumption that no thread can be in the record section of `take_gil()`.
2. **Callback object.** `agg->callback` is read and written only under the GIL.
3. **API state.** The lazy `get_agg()` allocation and the
   `AGG_STOPPED`/`AGG_RUNNING`/`AGG_STOPPING` transitions are only safe
   because the GIL serializes the callers.

In a free-threaded build all three fail:

- The GIL can switch on and off at runtime (`_PyEval_EnableGILTransiently`,
  `_PyEval_EnableGILPermanently`, `_PyEval_DisableGIL` in `Python/ceval_gil.c`).
  While it is off, callers hold nothing.
- Stop-the-world does not protect the ring. `_PyThreadState_Attach()`
  (`Python/pystate.c`, around line 2099) calls `_PyEval_AcquireLock()`, which is
  `take_gil()`, *before* `tstate_try_attach()`. A thread can therefore be in
  the record section while the world is stopped.
- Python threads can call the API truly in parallel.

### Changes

1. **Swap the ring pointer under `gil->mutex`.**
   - Producers only touch the ring while holding `gil->mutex` (see
     `_PyGILStats_RecordWait`), so taking that mutex in `enable()`/`disable()`
     to install or clear the pointer rules out use-after-free.
   - Free the ring only after unlocking, and only after the aggregator has been
     joined.
   - Use `PyMUTEX_LOCK(&gil->mutex)` from `condvar.h`. The `MUTEX_LOCK` macros
     are local to `ceval_gil.c`.
   - This also removes the "caller holds the GIL" reliance in the default
     build, so do it for both builds.
2. **Add a `PyMutex api_lock` to `struct _gil_stats_agg`.**
   - It protects the `state` field and `callback`.
   - `get_agg()` lazily allocates the aggregator state, so it needs its own
     guard: a static `PyMutex` or an atomic compare-and-swap on `st->agg`.
   - `_PyGILStats_Disable()` sets `AGG_STOPPING` under the lock, releases it,
     joins the thread with the thread state detached, then takes the lock again
     to finish.
   - The aggregator takes a strong reference to `callback` under `api_lock`
     before calling it, and drops `api_lock` while the callback runs.
   - `PyMutex` parks with the thread state detached, so taking it from an
     attached thread is fine in both builds.
3. **Remove the `NotImplementedError`** from `_PyGILStats_Enable`, and the
   `#ifndef Py_GIL_DISABLED` guard in `_PyGILStats_InitMain`.
   `Py_mod_gil = Py_MOD_GIL_NOT_USED` is already set on the module.
4. **Report whether the GIL was on** so that an empty window is not read as
   "no contention":
   - Add `gil_enabled_ns` (time the GIL was enabled during the window) to the
     snapshot.
   - Record the enabled and disabled transitions in the
     `_PyEval_EnableGIL*` / `_PyEval_DisableGIL` paths. These are rare and
     already happen during stop-the-world, so the cost is negligible.
   - On default builds, report `gil_enabled_ns` as the full window length.
   - Update `Misc/README.gilstats.md` and the field table.

### Tests

- Build `--disable-gil`, and run `test_gilstats` with `PYTHON_GIL=1`. Remove
  the module-level skip in `test_gilstats.py`, which currently skips when
  `sys._is_gil_enabled()` is false.
- With `PYTHON_GIL=0`: `enable()` works, windows still publish, `count == 0`,
  and `gil_enabled_ns == 0`.
- Transient enable: import a single-phase-init or non-`Py_mod_gil` test
  extension (see how `test_import` / `test_capi` exercise GIL re-enabling) and
  check that contention appears while the GIL is on.
- Concurrency: several threads call `enable()`/`disable()`/`set_callback()` in a
  loop at the same time, with the GIL off, without crashing.
- TSan: `./configure --disable-gil --with-thread-sanitizer` on Linux, then run
  `test_gilstats` and `test_threading` with stats enabled.

## Other open items before upstreaming

- **Config:** replace the `PYTHONGILSTATS` getenv in `_PyGILStats_InitMain`
  with a `PyConfig` field, `-X gilstats`, and `PYTHONGILSTATS`. Update
  `test_embed`'s expected config.
- **Docs and NEWS:** turn `Misc/README.gilstats.md` into a `Doc/library` page
  (or a section of `sys`), document the env var in `Doc/using/cmdline.rst`, and
  add a NEWS entry. Decide on a public name; `_gilstats` is private.
- **Discussion:** open a Discourse thread before the PR. It adds an
  always-compiled hook in `take_gil()` and a background thread.
- **Rebase:** `main`'s `ceval_gil.c` has diverged (for example gh-154175's
  eval-breaker change). Re-check the hook positions in `take_gil()`.
- **Flaky test:** `test_adaptive_drain_interval` fails about 1 in 10 runs under
  parallel load (`0.002 != 0.001`). A 16-slot ring does not always fill more
  than half within 1 ms. Assert that the interval shrank well below the max
  instead of reaching the exact min.
- **Ground-truth test:** none of the tests check a measured wait against a known
  true value. Add one:
  - set `sys.setswitchinterval(1.0)`;
  - have thread A spin for a known 50 ms while thread B waits;
  - assert that B's wait falls in the right bucket and that `top_waits` is
    within a few ms of 50 ms.
- **Overhead numbers:** run the microbenchmarks in three states: unpatched
  baseline, stats disabled, and stats enabled. Cover uncontended release and
  reacquire, a contended ping-pong, and CPU-bound forced switching. Target:
  disabled within noise, enabled under 1% on contended switches. Spot-check
  with `pyperformance`.
- **`make check-c-globals`:** this only runs with gcc. Run it on Linux. Add
  `Tools/c-analyzer` ignores if the module's static method and module tables
  get flagged.
- **Windows:** the `PCbuild` and `PC/config.c` entries were added but not
  built. Build and test on Windows.
