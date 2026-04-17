# Pickle Pure-Python Perf — Experiment Diary

Working notebook for the pure-Python `pickle._Pickler` fast-path
investigation. Mirrors the structure of `Misc/marshal-perf-diary.md`
from the marshal recovery PR.

**Scope**: only the pure-Python pickler fallback
(`Lib/pickle.py::_Pickler`). The C accelerator (`Modules/_pickle.c`)
is already heavily optimized (custom `PyMemoTable`, `batch_list_exact`
/ `batch_dict_exact` specializations, hybrid array+dict unpickler memo
with recent DoS guard). The pure-Python path is what's exercised
when the C module is unavailable (embedded builds, free-threading
dev, test regression) and by subclasses that override pure-Python
methods.

Branch: `exp-pickle/4-pure-python-exact-containers` (name is historical;
carries the whole pickle stack). Based on `main` at `2faceeec5c0`.

## Ground rules

- **Harness**: `Misc/pickle-perf-data/pickle_pure_bench.py`. Five
  workloads exercising `_Pickler` / `_Unpickler`. Protocol 5. Best-of-9
  median per operation, pinned with `taskset -c 0`.
- **Primary statistic**: `dump_median`. Report `load_median` too, but
  none of the experiments in this diary touch the load path — any
  load delta is measurement noise.
- **Baseline**: fresh `main` tip `2faceeec5c0`, no patches.
- **Build**: `make -j24` after each edit (pure-Python file edits
  don't strictly require rebuild, but we do it anyway to keep other
  state consistent).
- **Correctness gate**: `./python -m test test_pickle` must pass
  before any measurement is taken seriously.

### Failure mode we learned to watch for

One round of benchmarks was taken while a Linux kernel compile was
running on core 0 of the same CPU. Results showed uniform ~2×
slowdowns (+120%) across unrelated ideas — obvious contamination, but
a cheaper form of the same effect (+15-29%) was initially interpreted
as a genuine regression. **Rerun any suspect reading on a confirmed-
quiet machine before drawing conclusions.** The `pickle-pure-D-v2` and
`pickle-pure-BD` files in the data folder are the clean re-runs used
for the actual verdicts below.

## Reference numbers (clean `main`)

| Workload | `dump_median` (s) | `load_median` (s) |
| --- | ---: | ---: |
| `list_of_ints_10k` | 0.7647 | 0.4436 |
| `list_of_strs_1k` | 0.3220 | 0.2101 |
| `dict_str_int_5k` | 1.0872 | 0.7386 |
| `deep_list` | 2.7034 | 1.3373 |
| `nested_list_of_dicts` | 2.0137 | 1.1592 |

Source: `pickle-perf-data/pickle-pure-baseline.json`.

## Experiment ledger

| # | Idea | Lens | Status | Cumulative `dump` Δ vs main |
| --- | --- | --- | --- | ---: |
| 4 | Exact-container fast paths (`_batch_appends_exact`, `_batch_setitems_exact`) | Compiler (user #4) | **Shipped** (`94b53eb`) | −0.2 to −22.1% |
| A | Full per-type dispatch cache in `save()` | Compiler (user #2) | Subsumed by E | n/a |
| B | Hoist `persistent_id` / `reducer_override` hook probes to `__init__` | Compiler (user #2) | **Rejected** | +17 to +36% (worse) |
| C | Skip `memoize()` for atomic-content tuples | Graph (cycle impossible) | **Deferred** | +0 to −3% (test fixture blocker) |
| D | Inline `framer.commit_frame()` hot check | Compiler | **Shipped** (`bb9d721`) | −5 to −10% additional |
| E | Atomic-type `is` short-circuit in `save()` | Empirical / profile | **Shipped** (`bb9d721`) | −0 to −4% additional |

Final shipped: **Exp 4 + D + E**. Final numbers vs `main`:

| Workload | `dump` | `load` |
| --- | ---: | ---: |
| `list_of_ints_10k` | **−11.7%** | −3.9% |
| `list_of_strs_1k` | **−8.9%** | −3.4% |
| `dict_str_int_5k` | **−9.9%** | −4.4% |
| `deep_list` | **−24.6%** | −2.9% |
| `nested_list_of_dicts` | **−28.5%** | −2.8% |

Load-path numbers above are within measurement noise (the load path
was never touched); they move directionally because the same
subprocess runs both benches and thermal state drifts slightly.

## Experiment 4 — exact-container fast paths

**Hypothesis.** C has `batch_list_exact` (`Modules/_pickle.c:3179`)
and `batch_dict_exact` (`:3455`) that skip the generic batching
machinery for `PyList_CheckExact(obj)` / `PyDict_CheckExact(obj)`. The
pure-Python `_batch_appends` (Lib/pickle.py:1037) uses
`itertools.batched(items, _BATCHSIZE)` which, even though `batched`
itself is a C-implemented iterator, forces one tuple allocation and
one `enumerate()` object per batch. For the exact-list / exact-dict
case this is unnecessary overhead.

### Implementation iterations

Three shapes were tried before the final form:

1. **Index-based loop** — `for i in range(total, total+batch): save(obj[i])`.
   Broke `test_evil_pickler_mutating_collection` because `obj[i]`
   raises `IndexError` if `persistent_id` clears the list mid-batch.
   The generic `batched()` path materialised the batch as a tuple
   first and thus tolerated mid-iteration mutation.

2. **Slice snapshot per batch** — `snapshot = obj[idx:idx + batch]`
   before the inner loop. One list-slice allocation amortised over
   `batch_size` items, no `enumerate()` object. Matches the tolerance
   of `batched()`. Chosen for the list side.

3. **Dict: single direct iteration when `n ≤ BATCHSIZE`, materialised
   list when larger.** `obj.items()` itself raises `RuntimeError` on
   mutation, so no snapshot is needed for correctness. The small-dict
   case skips all batching machinery entirely — a single `for k, v in
   items:` loop + `MARK` + `SETITEMS`. The large-dict case does a
   one-time `list(items)` then iterates batches via slicing.

### Final diff summary

- `_batch_appends_exact(obj)` activated when `type(obj) is list and
  self.bin`.
- `_batch_setitems_exact(obj)` activated when `type(obj) is dict and
  self.bin`.
- Other types and proto 0 use the generic paths unchanged.
- About 80 lines added to `Lib/pickle.py`.

### Results (vs `main`)

| Workload | `dump` Δ |
| --- | ---: |
| `list_of_ints_10k` | −3.3% |
| `list_of_strs_1k` | −1.7% |
| `dict_str_int_5k` | −0.2% |
| `deep_list` | −17.2% |
| `nested_list_of_dicts` | −22.1% |

Nested / deep workloads see big wins because every level of nesting
hits the fast path. Flat workloads see small wins because only the
outermost call goes through it and the per-item work dominates.

### Notes

- `test_evil_pickler_mutating_collection` forced the slice-snapshot
  approach. A naive index loop in pure Python is not a drop-in
  replacement for the C path because the C path relies on
  `PyList_GET_SIZE` being re-read each iteration as a guard against
  mid-save mutation, which is free at C level but expensive as a
  per-iteration `len(obj)` in Python.
- The `n <= BATCHSIZE` single-iteration fast path for dicts is the
  real win on `dict_str_int_5k`. For `n > BATCHSIZE` (our 5000-item
  dict) the benefit largely evaporates because `save()` on 5k items
  dominates anyway.

## Experiment A — per-type dispatch cache

**Hypothesis.** Each `save()` call does 6-8 attribute / dict lookups
before it actually dispatches: `persistent_id`, `memo.get`,
`getattr(reducer_override, _NoValue)`, `type(obj)`, `dispatch.get(t)`,
and so on. A per-type closure cache keyed on `type(obj)` would
eliminate the repeated work for repeat types.

**Status.** Subsumed by Experiment E. E is essentially a tight,
hand-rolled version of A for the five dominant types (str, int,
NoneType, bool, float). A general-purpose cache would add a layer of
indirection that the Python-level interpreter's type-attribute cache
already provides via its `tp_version_tag` mechanism for bound-method
lookup. Would likely be a wash or slightly slower. Not pursued
separately.

## Experiment B — hoist `persistent_id` / `reducer_override` hooks

**Hypothesis.** The profile showed `getattr` called 500,000 times
at ~34 ms for `list_of_ints_10k`. Most of that is the
`getattr(self, "reducer_override", _NoValue)` probe which finds
nothing in the 99% case. Precompute a bool at `__init__` indicating
whether either hook is actually overridden, skip the per-call probe.

### First attempt

Store bound methods / unbound class functions in `self._persistent_id_hook`
/ `self._reducer_override_hook` at `__init__`. Failed because storing
a bound method creates a `self → bound-method → self` reference cycle,
breaking `test_pickler_reference_cycle`.

### Second attempt

Store unbound class functions (`cls.persistent_id` etc.) and call them
with explicit `self`. Failed for classmethod-defined hooks because
`cls.persistent_id` on a classmethod is already bound to `cls`;
calling it with `(self, obj)` passes three args to a function that
takes two.

### Third attempt (the one that ran clean)

Precompute only booleans:

    self._has_persistent_id_override = cls.persistent_id is not _Pickler.persistent_id
    self._has_reducer_override = hasattr(cls, "reducer_override")

Guard the call site:

    if save_persistent_id and (
            self._has_persistent_id_override
            or "persistent_id" in self.__dict__):
        pid = self.persistent_id(obj)

Passes all tests (the `__dict__` probe preserves the documented
instance-attribute-override pattern).

### Verdict: REJECTED

Clean-machine measurement confirmed it makes things **worse** by 17%
to 36% across every workload:

| Workload | `vs D-only` |
| --- | ---: |
| `deep_list` | +16.9% |
| `dict_str_int_5k` | +22.0% |
| `list_of_ints_10k` | +31.1% |
| `list_of_strs_1k` | +36.2% |
| `nested_list_of_dicts` | +23.2% |

The initial read during the kernel compile had shown +124% — obvious
contamination; the clean re-run gave these honest numbers. Still a
genuine regression.

Why it fails: Python's type-attribute cache makes `self.persistent_id`
a roughly-constant-time lookup (a type-version tag compare + a slot
index). Beating it requires avoiding the attribute load entirely — but
the `__dict__` probe we added to preserve instance-override semantics
is itself a dict lookup on a non-empty dict, which is about the same
cost as the thing we're trying to avoid. Net: more work per call.

**Lesson.** Don't try to beat `PyType_Lookup`'s type-attribute cache
with hand-written `__dict__` probes. The cache is already a fast path
for exactly this pattern.

## Experiment C — skip memoize for atomic-content tuples

**Hypothesis.** A tuple whose elements are all atomic (int, str,
float, None, bool, bytes) can never be part of a reference cycle.
The memoize call writes a `MEMOIZE` byte + a `dict.__setitem__` entry
per such tuple; dedup savings for one-shot atomic tuples is usually
lost (a list comprehension produces fresh tuples, so `id()` differs
per row even when values repeat).

**Implementation**: in `save_tuple`, after saving the elements, check
whether any element's type is outside `_ATOMIC_TYPES`; only call
`self.memoize(obj)` if so.

### Verdict: DEFERRED

Semantically safe — all tests that check values / round-trip round-trip
pass. But `test_pickle_to_2x` asserts byte-exact output against
fixture constants `DATA_XRANGE` / `DATA_SET2`. Skipping the MEMOIZE /
BINPUT byte shortens the stream, changing the bytes. The test comment
already acknowledges brittleness ("this test is a bit too strong
since we can produce different bytecode that 2.x will still
understand") but the assertion is real.

Potential win is modest (1-3% on tuple-heavy workloads), not worth
the PR-level friction of updating fixture constants across the two
affected tests. Deferred until someone cares enough to do the fixture
update.

## Experiment D — inline `framer.commit_frame()` hot check

**Hypothesis.** Profile showed `commit_frame` called 500,000 times
for `list_of_ints_10k` (cumtime 0.061 s, tottime 0.043 s). Body work
in the common case is trivial — check `self.current_frame is not None`
and `f.tell() < _FRAME_SIZE_TARGET`, both false → return. The method
call overhead is most of the cost.

### Implementation

At the top of `save()`, replace:

    self.framer.commit_frame()

with:

    framer = self.framer
    cf = framer.current_frame
    if cf is not None and cf.tell() >= _Framer._FRAME_SIZE_TARGET:
        framer.commit_frame()

Two attribute loads + a conditional, no function call in the hot
no-op case. Actual `commit_frame()` call falls through when a frame
is actually ready.

### Results (vs Exp 4 baseline)

| Workload | `dump` Δ | `load` Δ |
| --- | ---: | ---: |
| `list_of_ints_10k` | −8.0% | −0.9% |
| `list_of_strs_1k` | −6.8% | −3.0% |
| `dict_str_int_5k` | −6.2% | −2.3% |
| `deep_list` | −7.9% | −2.1% |
| `nested_list_of_dicts` | −9.7% | −2.1% |

Consistent 5-10% win across every workload. Completely independent
of data shape because `save()` is called per-value everywhere.

## Experiment E — atomic-type `is` short-circuit

**Hypothesis.** After the `persistent_id` / `memo` / `reducer_override`
checks, the common-case dispatch is `self.dispatch.get(type(obj))` —
a dict lookup. For the five dominant types in real payloads (`str`,
`int`, `NoneType`, `bool`, `float`) we can skip the dict lookup by
doing a sequence of `type(obj) is T` identity tests and calling the
corresponding `save_*` method directly.

### Implementation iterations

First form checked `int`, `bool`, `None`, `float` only. Helped the
int-heavy workload but measurably hurt `list_of_strs_1k` (+4.5%)
because every string now paid four no-op identity tests before
falling through to the dispatch lookup.

Final form adds `str` first:

    t = type(obj)
    if t is str:
        self.save_str(obj); return
    if t is int:
        self.save_long(obj); return
    if obj is None:
        self.write(NONE); return
    if t is bool:
        self.save_bool(obj); return
    if t is float:
        self.save_float(obj); return
    f = self.dispatch.get(t)  # fallback

Placed after the memo check so repeated strings still dedup via the
memo path (critical for dicts with shared keys).

### Results (vs Exp 4 + D baseline)

| Workload | `dump` Δ | Note |
| --- | ---: | --- |
| `list_of_ints_10k` | −0.7% | Dominant int path, already hits D gain |
| `list_of_strs_1k` | −2.5% | Now a net win after adding str |
| `dict_str_int_5k` | −3.7% | String keys benefit most |
| `deep_list` | −0.4% | Mostly ints, D dominates |
| `nested_list_of_dicts` | −0.1% | Flat; other costs dominate |

Small but universally positive with str included, which matters
because str is the single most common dict-key type in real payloads.

## Combined results and third-party validation

### Cumulative picture (Exp 4 + D + E vs `main`)

See the final-numbers table at the top of the ledger. Summary: **−9%
to −28% dump**, with loads unchanged within noise.

### Third-party regression suite

Same venvs reused from the marshal recovery work; see
`Misc/marshal-perf-diary.md` for setup.

**dill 0.4.1 test suite**: 29/30 pass (unchanged). The one failure
(`test_session`) is a pre-existing 3.15a8 incompatibility in dill's
module-state serialization — reproduces identically on unpatched
`main`, stays identical here. The traceback is entirely in `pickle.py`
+ `dill/_dill.py`, not in my changed code paths.

**cloudpickle 3.1.2**: 236 passed, 22 skipped, 2 xfailed — byte-for-byte
identical pass/skip/xfail breakdown as on unpatched `main`. Both
xfails are upstream-known cross-process determinism issues, not
related to any of our changes.

### Stdlib regression suite

| Test file | Count | Result |
| --- | ---: | --- |
| `test_pickle` | 1,060 | SUCCESS |
| `test_pickletools` | 202 | SUCCESS |
| `test_copy` | 83 | SUCCESS |
| `test_copyreg` | 6 | SUCCESS |
| `test_importlib` | 1,217 | SUCCESS |

2,568 tests covering code paths that exercise `_Pickler`, all pass.

## Final conclusions

### What shipped

Two commits on `exp-pickle/4-pure-python-exact-containers` (local,
unpushed):

1. `94b53eb` — Exp 4 exact-container fast paths.
2. `bb9d721` — Exp D inlined `commit_frame` + Exp E atomic-type
   short-circuit.

Net diff vs `main`: +138 / -4 lines in `Lib/pickle.py`.

### What we learned

- **`save()` overhead dominates on small-object workloads.** 500,000
  save calls × ~2 µs = 1 second. Optimizing the *frame* of each call
  (D) matters more than optimizing the *body* (individual `save_*`
  functions) for this shape of work.
- **`PyType_Lookup`'s type-attribute cache is already good.** Don't
  try to short-circuit `self.method()` with a bool + `__dict__` probe;
  you'll add more work than you save. Exp B learned this the hard way
  on two separate implementation attempts.
- **Byte-exact pickle-output tests block some legitimate wins.** Exp C
  is semantically correct and saves real work but would require
  updating fixture constants in `test_pickle_to_2x`. Noting this for
  anyone willing to push the broader change through review.
- **Contamination is real.** A background kernel compile on the same
  core can produce uniform ~2× slowdowns that look superficially like
  a patch-induced regression. Every experiment in this diary had its
  final verdict set by a clean-machine re-run.

### What we didn't try

- **Specialized `save_list` for homogeneous-content lists** (all ints,
  all strs). Real arrays are often homogeneous. Could probably skip
  per-item type dispatch entirely. Natural next experiment.
- **Memoize-skip for small one-shot dict / list objects.** Same
  cycle-impossibility argument as Exp C, same byte-output sensitivity.
- **A full per-type closure cache in `save()`.** The Python-level
  interpreter already provides this via type-version-tagged attribute
  caches for the method-lookup path; Exp E exploits the cases where
  `is`-check + direct call is even cheaper than a cache hit.
- **Any C-level work on `Modules/_pickle.c`.** The C path was already
  hand-optimized decades ago; this round stayed in pure Python.

### Recommended next move

The Exp 4 + D + E stack is a small (~150-line) self-contained patch
with measurable, reproducible wins, no regressions, and third-party
validation. Would ship comfortably as a standalone PR against
`python/cpython`.

If continuing: Exp C is low-hanging if someone is willing to update
the two fixture constants. Homogeneous-list specialization is the
next experiment worth running — similar methodology, should be a
week-end job.

## Provenance

Generated 2026-04-17 during a follow-up exploration of stdlib perf
opportunities after the marshal safe-cycle recovery PR. Raw data in
`Misc/pickle-perf-data/`; commits `94b53eb` and `bb9d721` on
`exp-pickle/4-pure-python-exact-containers`.
