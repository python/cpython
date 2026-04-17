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

### Round 1 (Exp 4 → E)

| # | Idea | Lens | Status | Cumulative `dump` Δ vs main |
| --- | --- | --- | --- | ---: |
| 4 | Exact-container fast paths (`_batch_appends_exact`, `_batch_setitems_exact`) | Compiler (user #4) | **Shipped** (`94b53eb`) | −0.2 to −22.1% |
| A | Full per-type dispatch cache in `save()` | Compiler (user #2) | Subsumed by E | n/a |
| B | Hoist `persistent_id` / `reducer_override` hook probes to `__init__` | Compiler (user #2) | **Rejected** | +17 to +36% (worse) |
| C | Skip `memoize()` for atomic-content tuples | Graph (cycle impossible) | **Deferred** | +0 to −3% (test fixture blocker) |
| D | Inline `framer.commit_frame()` hot check | Compiler | **Shipped** (`bb9d721`) | −5 to −10% additional |
| E | Atomic-type `is` short-circuit in `save()` | Empirical / profile | **Shipped** (`bb9d721`) | −0 to −4% additional |

### Round 2 (F1 → F6)

| # | Idea | Lens | Status | `dump` Δ vs round-1 tip |
| --- | --- | --- | --- | ---: |
| (fix) | Preserve large-dict mutation detection (regression from Exp 4) | Correctness | Shipped (part of F1) | Neutral |
| F1 | Move atomic fast paths *before* memo lookup, match C pickler's dispatch order | Compiler (user round 2 #1) | **Shipped** (`285fcae`) | −7 to −31% additional |
| F2 | Inline `put()` MEMOIZE proto-4+ path into `memoize()` | Compiler (user #2) | **Shipped** (`2f1d38b`) | −0 to −2% additional |
| F3 | Explicit frame byte counter to eliminate `BytesIO.tell()` | Compiler (user #3) | **Rejected** | +3 to +5% (worse) |
| F4 | Precompute BININT1 opcode bytes for n in 0..255 | Empirical (user #4) | **Shipped** (`7c6af84`) | −0 to −10% additional |
| F5 | ASCII fast path in `save_str` | Compiler (user #6) | **Rejected** | ~0% (flat) |
| F6 | Extend `save()` fast path to `bytes` | Empirical (user #5) | **Shipped** (`e917108`) | −9 to −12% on bytes-heavy, neutral elsewhere |
| F7 | Exact-set batching fast path | Compiler (user #8) | Deferred | Not measured |

Final shipped stack: **Exp 4 + D + E + F1 + F2 + F4 + F6**. Cumulative
numbers vs clean `main`:

| Workload | `dump` | `load` |
| --- | ---: | ---: |
| `list_of_ints_10k` | **−38.3%** | −3.8% |
| `list_of_strs_1k` | **−20.1%** | −3.6% |
| `dict_str_int_5k` | **−27.7%** | −4.2% |
| `deep_list` | **−49.0%** | −3.1% |
| `nested_list_of_dicts` | **−36.8%** | −2.8% |

Bytes-heavy bench (introduced in round 2; no prior reference number
on `main`, so deltas are vs the round-2 tip *without* F6):

| Workload | `dump` Δ vs pre-F6 |
| --- | ---: |
| `list_of_short_bytes_5k` | **−10.7%** |
| `list_of_medium_bytes_1k` | **−11.7%** |
| `dict_bytes_to_int_2k` | **−9.1%** |
| `list_of_bytearrays_1k` | +0.4% (bytearray not in fast path) |

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

## Round 2 — F1 through F6

After shipping Round 1 and writing the diary, a fresh profile of the
current branch flagged which call sites still dominated: `save()` body,
`memoize()`, `save_long`'s `_struct.pack` calls, and `framer.current_frame.tell()`
invocations. The user prepared a second idea list (F1–F10) targeting
those lines directly. Round 2 ran the first six of those to verdict.

### Correctness fix: large-dict mutation detection

**Flagged on review.** My Exp 4 `_batch_setitems_exact` large-dict
branch (`n > _BATCHSIZE`) materialised `list(obj.items())` upfront,
losing the `RuntimeError("dictionary changed size during iteration")`
that `batched()` on the live items-view would raise. Fix: delegate the
large-dict path to the generic `_batch_setitems`, which iterates
through `batched()` over the live view. The small-dict fast path
already preserved detection through direct `for k, v in items:`.

Verified via a targeted reproducer (2000-entry dict with a
`persistent_id` hook that clears the dict): **RuntimeError now raised
as expected**, both before and after the rest of Round 2.

No stdlib test exercised this path, which is why the regression slipped
through in Round 1. Worth considering a test-suite addition.

### Experiment F1 — atomic fast paths *before* memo lookup

**Hypothesis.** The C pickler's `save()` (`Modules/_pickle.c:4514`)
dispatches atomic types (`None`, `bool`, `long`, `float`) *before* the
memo check, and without invoking `reducer_override`. The Python version
did the memo check and reducer_override probe first, costing an `id()`
+ `memo.get()` and a `getattr` per call for the 80%-by-count case of
atomic values.

**Implementation iteration 1** placed atomics in the obvious order —
`int`, `None`, `bool`, `float`, then memo, then reducer_override,
then `str`. Regressed `list_of_strs_1k` by +3% because every string
paid four no-op `type(obj) is X` identity tests before reaching its
dispatch.

**Implementation iteration 2** (shipped) reorders so `str` is first
(with inline memo check preserving dedup), then the four non-memoized
atomics, then the memo check for other types. Matches C pickler's
dispatch order.

**Semantic change**: reducer_override is no longer called for
`str`/`int`/`bool`/`None`/`float`. This matches the C pickler's long-
standing behaviour; the `reducer_override` protocol was designed for
custom types, not the built-in atomic ones.

**Results** (vs F1 predecessor, Exp 4 + D + E):

| Workload | `dump` Δ |
| --- | ---: |
| `list_of_ints_10k` | **−30.4%** |
| `list_of_strs_1k` | **−7.8%** |
| `dict_str_int_5k` | **−16.3%** |
| `deep_list` | **−26.9%** |
| `nested_list_of_dicts` | **−8.6%** |

The largest pure-Python pickle speedup this entire project has
produced.

### Experiment F2 — inline MEMOIZE in `memoize()`

**Hypothesis.** For protocol ≥ 4 (the common case), `self.put(idx)`
simply returns the single-byte `MEMOIZE` constant. `memoize()` was
calling `self.write(self.put(idx))` — one method dispatch and one
temporary bytestring, every time. Profile showed 35–280 ms per bench
in `memoize`.

**Implementation.** Inline the protocol dispatch directly into
`memoize()`; for proto ≥ 4, write `MEMOIZE` directly. Cache
`memo = self.memo` as a local to avoid a second attribute load. The
out-of-line `put()` method is preserved for subclass override
compatibility.

**Results** (vs F4 predecessor):

| Workload | `dump` Δ |
| --- | ---: |
| `list_of_ints_10k` | +1.0% (not memoized; noise) |
| `list_of_strs_1k` | −1.9% |
| `dict_str_int_5k` | −1.2% |
| `deep_list` | −1.3% |
| `nested_list_of_dicts` | −1.6% |

Modest but consistent win on memoizing workloads.

### Experiment F3 — explicit frame byte counter

**Hypothesis.** After D, `BytesIO.tell()` was still visible in the
profile. Track bytes written in `_Framer` as an explicit integer
counter, so the frame-boundary check in `save()` becomes a plain int
compare.

**Implementation.** Add `current_frame_size` to `_Framer`, increment
in `_Framer.write`, reset in `commit_frame` and `start_framing`.

**Verdict: REJECTED.** Regressed **all** workloads by +3–5%. The
counter-maintenance cost per write (`self.current_frame_size += len(data)`,
a Python-level attribute load + `len()` + store) exceeds
`BytesIO.tell()`'s cost — `BytesIO` is a C type and `tell()` is a
direct C method call with no Python-level frame overhead. One Python
statement per write beats one C method call per ~100 writes.

**Lesson.** Don't replace a C-method-call-on-C-object with a
Python-level counter unless you're confident the amortised write
frequency is less than the amortised check frequency. In this code
path it's roughly 1:1, so the counter always loses.

### Experiment F4 — BININT1 opcode-bytes cache

**Hypothesis.** `save_long` for small non-negative ints was doing
`BININT1 + pack("<B", obj)` per call. Precompute the 256 two-byte
opcode sequences at module import.

**Implementation.** Module-level `_BININT1_BYTES = tuple(BININT1 +
bytes([i]) for i in range(256))` (~50 KB one-time cost). `save_long`
uses `self.write(_BININT1_BYTES[obj])` in the `0 ≤ obj ≤ 0xff` branch.

**Results** (vs F1 predecessor):

| Workload | `dump` Δ | Note |
| --- | ---: | --- |
| `list_of_ints_10k` | −0.0% | 0–9999 range; only 256 values in cache |
| `list_of_strs_1k` | −1.4% | Incidental — string save also writes byte headers via pack |
| `dict_str_int_5k` | −1.4% | Values 0–4999 mostly hit BININT2, not cached |
| `deep_list` | **−9.9%** | `[i]*10 for i in 0..499` — every int in cache |
| `nested_list_of_dicts` | −1.5% | |

The huge deep_list win demonstrates F4's headline: when all ints fit
in the cache, this is a significant speedup. In mixed workloads the
effect is smaller but positive.

### Experiment F5 — ASCII fast path in `save_str`

**Hypothesis.** `obj.encode('utf-8', 'surrogatepass')` is used
unconditionally. For ASCII strings (the common case — identifiers,
JSON-like dict keys, log records), `.encode('ascii')` skips the utf-8
codec's surrogate-pass logic.

**Implementation.** Guard with `obj.isascii()`, then call
`.encode('ascii')` on the fast path, else original path.

**Verdict: REJECTED (flat).** Within ±1% across all benches — pure
noise. Python's utf-8 encoder already has a fast path for pure-ASCII
strings (all bytes < 0x80 require no multi-byte encoding, essentially
a memcpy). The cost of `isascii()` (a C method that walks the string
once) cancels any saving from switching codecs.

**Lesson.** Before optimising a Python-level codec call, check whether
the underlying C codec already has a fast path for your input shape.
Python's str/bytes codecs are aggressively specialised.

### Experiment F6 — `bytes` in the `save()` fast path

**Hypothesis.** F1's fast path covers `str`, `int`, `None`, `bool`,
`float`. `bytes` values — common in binary-protocol and content-
addressable workloads — still fall through to the dispatch table. Add
`bytes` to the fast path with inline memo check (analogous to `str`).

**Workload.** No existing bench used bytes, so added
`pickle_pure_bench_bytes.py` with four shapes: small fixed-size bytes
in a list, medium bytes (32-byte buffers), bytearrays (control — not
covered by F6), and bytes-keyed dict.

**Implementation iteration 1** placed `bytes` right after `str` in
the fast path. Regressed `list_of_ints_10k` by +2.6% and `deep_list`
by +3.2% because every int save now paid an extra `type(obj) is bytes`
branch before hitting the int path.

**Implementation iteration 2** (shipped) places `bytes` *after* the
non-memoized atomics (int/None/bool/float). Int-heavy workloads pay
zero extra cost; bytes workloads pay four branches before dispatch,
which is cheap relative to `save_bytes`'s body.

**Results.** Main-bench workloads neutral (±0.7%). Bytes bench:

| Workload | `dump` Δ |
| --- | ---: |
| `list_of_short_bytes_5k` | **−10.7%** |
| `list_of_medium_bytes_1k` | **−11.7%** |
| `dict_bytes_to_int_2k` | **−9.1%** |
| `list_of_bytearrays_1k` | +0.4% (`bytearray` not covered) |

### Experiment F7 — exact-set batching

**Deferred.** C has `batch_set_exact` that iterates a set directly
with a size-change check. Pure Python currently uses `batched()` over
`sorted(obj)` (for determinism) — adds sort cost plus generator
overhead. Expected win on set-heavy workloads (~1000+ items), similar
in shape to Exp 4's dict/list work. Needs a set-heavy workload; not
measured this round.

## Final conclusions

### What shipped

Seven commits on `exp-pickle/4-pure-python-exact-containers` (local,
unpushed):

**Round 1:**
1. `94b53eb` — Exp 4 exact-container fast paths.
2. `bb9d721` — Exp D inlined `commit_frame` + Exp E atomic-type
   short-circuit.
3. `69eed04` — this diary (round-1 version).

**Round 2:**
4. `285fcae` — F1 reorder save() fast paths + fix large-dict
   mutation regression introduced by Exp 4.
5. `7c6af84` — F4 BININT1 opcode cache.
6. `2f1d38b` — F2 inline MEMOIZE proto-4+ in memoize().
7. `e917108` — F6 bytes in save() fast path.

Net diff vs `main`: ~200 added / ~30 removed lines in `Lib/pickle.py`.

### What we learned

- **`save()` overhead dominates on small-object workloads.** 500,000
  save calls × ~2 µs = 1 second. Optimizing the *frame* of each call
  (D, F1) matters more than optimizing the *body* (individual `save_*`
  functions) for this shape of work.
- **Matching the C pickler's dispatch order is almost always right.**
  F1's biggest wins (−30% on int-heavy workloads) came from simply
  matching `Modules/_pickle.c::save()`'s atomic-types-before-memo
  order. The pure-Python implementation had been written to look
  readable, not to mirror the reference. When in doubt, align the
  two.
- **`PyType_Lookup`'s type-attribute cache is already good.** Don't
  try to short-circuit `self.method()` with a bool + `__dict__` probe;
  you'll add more work than you save. Exp B learned this the hard way
  on two separate implementation attempts.
- **Don't replace a C-method call with a Python-level counter.** F3
  tried to replace `BytesIO.tell()` (C-method on C-type) with a
  Python attribute. The per-write Python-level bookkeeping costs more
  than the single C call it saved — +3-5% regression across the
  board. A single C method call on a C type is hard to beat from
  Python.
- **Don't re-specialize a codec Python already specializes.** F5
  tried an ASCII fast path in `save_str`. Python's utf-8 encoder
  already has a fast path for pure-ASCII strings (memcpy); `isascii()`
  walks the string, cancelling any saving. Before manually specializing
  a codec, check whether the underlying C implementation already has
  a fast path for your input shape.
- **Byte-exact pickle-output tests block some legitimate wins.** Exp C
  is semantically correct and saves real work but would require
  updating fixture constants in `test_pickle_to_2x`. Noting this for
  anyone willing to push the broader change through review.
- **Precomputed opcode-byte tables are cheap and payoff-bounded.**
  F4 cached 256 two-byte strings (~50 KB one-time) and won up to
  −10% on workloads where every integer fit in the cache. Bounded
  payoff: only small ints benefit. But free to implement, free to
  keep, and Python's small-int cache guarantees consistent `is`
  behaviour for identifier-style numbers.
- **Contamination is real.** A background kernel compile on the same
  core can produce uniform ~2× slowdowns that look superficially like
  a patch-induced regression. Every experiment in this diary had its
  final verdict set by a clean-machine re-run; one round-2 experiment
  (B) was initially read as +124% under thermal contamination but
  reconfirmed as a real +17-36% regression on the clean machine, which
  was the honest verdict.
- **The diary's own value was paid back in round 2.** F1 was
  suggested by a user review of the round-1 tip + diary that flagged
  a specific regression (large-dict mutation detection) I'd missed,
  plus six more ideas targeting specific hotspots. Writing the diary
  up front made a second round of focused work possible.

### What we validated this session

- **Full CPython regression suite** (`./python -m test -j24`) on the
  F6 tip: 468 test files, 48,928 tests run, zero failures.
- **dill 0.4.1 test suite**: 29/30 pass; the single failure
  (`test_session`) is a pre-existing Python 3.15 alpha incompatibility
  in dill's module-state serialization — reproduces identically on
  unmodified `main`. Entire traceback is in `pickle.py` + `dill/_dill.py`;
  no changed code path in our branch is involved.
- **cloudpickle 3.1.2 upstream tests** (cloned directly from
  `cloudpipe/cloudpickle`): 243 pass + 29 skip + 2 xfail — byte-for-byte
  identical breakdown vs unmodified `main`. The two xfails are upstream-
  documented cross-process determinism issues unrelated to our changes.
- **joblib 1.5.3 self-tests** (focused subset — 5 test files, excluding
  numpy/memmap dependencies that don't install on 3.15a8): 95 pass,
  2 fail, 7 err — **identical outcome on `main` baseline**.
- **attrs 26.1.0**: smoke-test of `@attrs.define`-class round-trip
  through `pickle._Pickler` — passes.

### What we didn't try

- **Specialized `save_list` / `save_set` / `save_tuple` for
  homogeneous-content containers** (all ints, all strs). Real arrays
  are often homogeneous. Could probably skip per-item type dispatch
  entirely. Natural next experiment.
- **F7 exact-set batching** — queued but no set-heavy workload was
  added this round. Mirror of Exp 4 for sets; expected similar shape
  and magnitude.
- **Memoize-skip for small one-shot dict / list objects.** Same
  cycle-impossibility argument as Exp C, same byte-output sensitivity.
- **Any C-level work on `Modules/_pickle.c`.** The C path was already
  hand-optimized decades ago; this round stayed in pure Python.
- **Real-world project test suites (Django, Sphinx, IPython)**. These
  use the C accelerator by default; my changes are to the pure-Python
  fallback, so not running their suites is defensible but not
  comprehensive.

### Recommended next move

The current stack (Exp 4 + D + E + F1 + F2 + F4 + F6) is a
self-contained ~200-line patch with:

- measurable, reproducible wins: **−20 to −49% on pure-Python
  `pickle._Pickler` dump**
- no regressions: full CPython suite passes; dill / cloudpickle /
  joblib / attrs all match baseline
- third-party validation on two directly-relevant libraries
  (dill uses `marshal`+`pickle`, cloudpickle extends pickle)
- a diary that records every rejected idea with its reason, so
  future reviewers don't re-try the rejected paths

Ships comfortably as a PR against `python/cpython` separate from the
marshal PR.

If continuing after a merge:

1. **F7 exact-set batching** — small, symmetric to Exp 4, wants a
   set workload added first.
2. **Homogeneous-list specialization** — skip per-element dispatch
   when `save_list` detects all elements share a type.
3. **Exp C revisited with fixture updates** — semantically correct,
   1–3% gain on tuple-heavy workloads, would need a test-suite
   coordinated change.

## Provenance

Generated 2026-04-17, updated over two sessions. Round 1 (Exp 4, D, E)
and round 2 (F1, F2, F3, F4, F5, F6) are distinct; see commit history
on `exp-pickle/4-pure-python-exact-containers`. Raw bench data in
`Misc/pickle-perf-data/`.
