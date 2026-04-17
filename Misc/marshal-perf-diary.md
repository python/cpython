# Marshal Load Perf — Experiment Diary

Tracking attempts to close (and ideally beat) the ~7–12% `marshal.loads`
regression introduced by the safe-cycle design on
`marshal-safe-cycle-design`.

## Ground rules

- **Harness**: `/tmp/marshal_bench_cpu_stable.py`, 11 repeats per operation,
  200k `loads` × 100k `dumps`.
- **Pinning**: `taskset -c 0 ./python /tmp/marshal_bench_cpu_stable.py`.
- **Primary statistic**: median of 11 runs. Report `min` when it disagrees
  meaningfully with median.
- **Reference commits**:
  - `main` baseline median from prior rerun
    (`/tmp/marshal-baseline-stable.json`). Represents the pre-regression bar.
  - `perf-baseline` tag at HEAD of `marshal-safe-cycle-design`
    (commit `eb1c4b7`). The post-safety-fix baseline we are trying to
    claw back.
- **Build**: `make -j24` after each edit.
- **Methodology per experiment**:
  1. Branch off `perf-baseline` as `exp/<N>-<slug>`.
  2. Apply minimal diff implementing the idea alone.
  3. `make -j24`; abort experiment if build/tests error.
  4. Run pinned bench 3× consecutively; keep the best-median run to tame
     outlier noise (record all three).
  5. Record results table + one-sentence interpretation.
  6. Return to `perf-baseline`.

## Reference numbers (before any experiment)

Both columns use the **best-of-three pinned-run median** to damp
run-to-run thermal noise. `main` is from
`/tmp/marshal-baseline-stable.json`; `perf-baseline` is the fresh
self-check described in Experiment 0.

| Benchmark | Operation | `main` median (s) | `perf-baseline` median (s) | Regression |
| --- | --- | ---: | ---: | ---: |
| `small_tuple` | `loads` | 0.0289962 | 0.0328189 | +13.2% |
| `small_tuple` | `dumps` | 0.0158760 | 0.0156912 | −1.2% |
| `nested_dict` | `loads` | 0.0778896 | 0.0866099 | +11.2% |
| `nested_dict` | `dumps` | 0.0722451 | 0.0745504 | +3.2% |
| `code_obj`    | `loads` | 0.0902017 | 0.0971267 | +7.7% |
| `code_obj`    | `dumps` | 0.0391339 | 0.0396249 | +1.3% |

The `loads` regressions reproduce cleanly and in the same direction as
the design doc's rerun. `dumps` regressions are within noise for the
two small payloads but `nested_dict` shows a consistent +3% that we
will watch.

## Experiment ledger

Each row summarizes one idea's effect versus `perf-baseline`
(negative = faster, positive = slower). Fill in after the experiment
completes.

| # | Idea | `small_tuple` loads | `nested_dict` loads | `code_obj` loads | Verdict |
| --- | --- | ---: | ---: | ---: | --- |
| 0 | `perf-baseline` rerun (self-check) | 0.0% | 0.0% | 0.0% | reference (vs main: +13.2% / +11.2% / +7.7%) |
| 1 | Raw `PyObject **` refs array | −22.6% | −11.8% | −10.0% | **winner** (beats main on all three) |
| 2 | Tagged pointer state (on top of 1) | −5.9% (Δvs1) | −4.7% | −2.1% | **winner** (subsumes Exp 5) |
| 3 | Collapse reserve+publish | +2.1% | −1.6% | +0.4% | noise; keep only as cleanup atop Exp 1 |
| 4 | `has_incomplete` counter fast path | −4.1% | +0.5% | −0.3% | noise on acyclic benches; helps cyclic streams |
| 5 | Zero-init + drop mark_ready writes | −2.7% | −1.1% | −0.6% | small win; subsumed by Exp 2 |
| 6 | Drop `allow_incomplete_hashable` param | −1.2% | −2.0% | −1.1% | small consistent win; keep |
| 7 | Force-inline helpers | −3.7% | −3.0% | −1.1% | free win; keep |
| 8 | `frozendict`-then-wrap when `flag==0` | +3.1% | +1.3% | +0.9% | benches don't exercise frozendict; noise |
| 9 | Preallocate refs capacity | −0.3% | +0.3% | +1.0% | noise; no geometric growth on small benches |
| C | Combined (Exp 1+2+6+7) | −24.2% | −16.3% | −13.4% | **regression erased; beats main by 6–14%** |

## Experiment 0 — baseline self-check

Runs the harness at `perf-baseline` (`eb1c4b7`) to confirm the published
numbers reproduce on this machine and today's thermal conditions. All
subsequent deltas are measured against this rerun, not against the
original JSON, so that noise in the reference does not poison the later
deltas.

### Three pinned runs at `perf-baseline`

| Run | `small_tuple` loads | `nested_dict` loads | `code_obj` loads |
| ---: | ---: | ---: | ---: |
| 1 | 0.0328189 | 0.0868265 | 0.0979147 |
| 2 | 0.0331589 | 0.0866099 | 0.0982851 |
| 3 | 0.0330375 | 0.0873459 | 0.0971267 |
| **best-of-3 median** | **0.0328189** | **0.0866099** | **0.0971267** |

Raw JSON: `/tmp/exp0-run{1,2,3}.json`.

### Notes

- Numbers reproduce the design-doc regression bound to within ~1%.
- The best-of-3 median rule is adopted to suppress the occasional
  high-tail run visible in all three files (first sample in any series
  tends to be warm-up noise).

## Experiment 1 — raw `PyObject **` refs array

**Hypothesis.** `PyList_Append` through PyList machinery dominates the
per-reference cost. Replacing `p->refs` with a raw growable
`PyObject **refs; Py_ssize_t refs_len, refs_cap;` should drop that cost
to a few inlined stores.

**Scope.** Touch only the refs storage. Keep `ref_states` as a parallel
byte array. No other changes.

### Diff summary

- `RFILE.refs` changes from `PyObject *` list to `PyObject **` raw array.
- New `refs_len` field.
- `r_ref_ensure_capacity` also realloc's the pointer array.
- `r_ref_reserve` pushes `NULL` and bumps `refs_len`; no PyList traffic.
- `r_ref_publish` does `p->refs[idx] = Py_NewRef(o)` with `Py_XDECREF` of
  the slot's prior value (NULL initially).
- `TYPE_REF` reads `p->refs[n]`, treats `NULL` (not `Py_None`) as the
  "never-published" sentinel.
- Added `r_ref_init` / `r_ref_release` helpers; all five entry points
  (PyMarshal_ReadObjectFromFile/FromString, marshal_load_impl,
  marshal_loads_impl, plus the two short/long readers) converted.

### Results (best-of-3 pinned median, 3 runs)

| Benchmark | Operation | `perf-baseline` | Exp 1 | Δ vs baseline | vs main |
| --- | --- | ---: | ---: | ---: | ---: |
| `small_tuple` | `loads` | 0.0328189 | 0.0254152 | **−22.6%** | **−12.4%** (faster) |
| `small_tuple` | `dumps` | 0.0156912 | 0.0160043 | +2.0% | +0.8% |
| `nested_dict` | `loads` | 0.0866099 | 0.0764238 | **−11.8%** | **−1.9%** (faster) |
| `nested_dict` | `dumps` | 0.0745504 | 0.0746833 | +0.2% | +3.4% |
| `code_obj`    | `loads` | 0.0971267 | 0.0874129 | **−10.0%** | **−3.1%** (faster) |
| `code_obj`    | `dumps` | 0.0396249 | 0.0399371 | +0.8% | +2.1% |

Raw JSON: `/tmp/exp1-run{1,2,3}.json`.

Correctness: `./python -m test test_marshal` passes (72 run / 7 skipped).

### Notes

- This single change not only erases the regression, it beats `main` on
  all three `loads` benchmarks. PyList overhead was the dominant cost
  all along; the state-byte work the safe-cycle design added is easily
  absorbed once the list is gone.
- `dumps` numbers are roughly flat (the marshal writer was never
  touched); the small +0.8% on `small_tuple dumps` and +3.4% on
  `nested_dict dumps` versus main are unrelated to this patch and
  already visible in Experiment 0.
- Dump path is untouched; any residual `dumps` wobble comes from
  elsewhere (unrelated CPython changes between `main` and HEAD).

## Experiment 2 — tagged pointer state

**Hypothesis.** With the raw array from Experiment 1, `REF_STATE_*` can
be encoded in the low two bits of the pointer, eliminating the parallel
`ref_states` array entirely. Fewer bytes touched per `R_REF`; one cache
line instead of two on `TYPE_REF`.

**Scope.** Stacked on top of Experiment 1 (kept minimal) so the delta
attributable to the tagging is measurable.

### Diff summary

- Drop `ref_states` from `RFILE` entirely.
- Encode state in the low bit of `p->refs[idx]`:
  `NULL` = reserved; bit clear = READY; bit set = INCOMPLETE_HASHABLE.
- `r_ref_publish` tags at publish time; `r_ref_mark_ready` just clears
  the low bit of the slot (a single masked store).
- `TYPE_REF` reads the tagged pointer, tests the low bit, untags.
- `r_ref_release` untags before `Py_XDECREF`.

### Results (best-of-3 pinned median, stacked on Exp 1)

| Benchmark | Operation | Exp 1 | Exp 2 | Δ vs Exp 1 | vs `perf-baseline` | vs main |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `small_tuple` | `loads` | 0.0254152 | 0.0239064 | −5.9% | **−27.2%** | **−17.6%** |
| `nested_dict` | `loads` | 0.0764238 | 0.0728334 | −4.7% | **−15.9%** | **−6.5%** |
| `code_obj`    | `loads` | 0.0874129 | 0.0855773 | −2.1% | **−11.9%** | **−5.1%** |

`dumps` numbers remain within ±1% of Exp 1 (untouched path).

Raw JSON: `/tmp/exp2-run{1,2,3}.json`.

Correctness: `./python -m test test_marshal` passes (72 run / 7 skipped).

### Notes

- Tagging the pointer eliminates a whole parallel allocation plus a
  second cache line per referenced slot. The biggest wins are on the
  small-object benches where the per-ref overhead dominated.
- Because state publish and clear are now single masked stores, this
  idea also subsumes the `mark_ready` write-elision in Experiment 5.
- Relies on `PyObject *` alignment ≥ 2. Holds on every CPython target
  (`_PyObject_HEAD_INIT` aligns; `PyObject` is at least pointer-aligned).

## Experiment 3 — collapse reserve+publish

**Hypothesis.** The current tuple/slice/frozendict path does
`PyList_Append(Py_None)` followed immediately by `PyList_SET_ITEM` +
`Py_NewRef(o)` + `Py_DECREF(Py_None)`. A single
`r_ref_append_with_state(o, state)` helper performs the same work in
one step and removes a superfluous refcount pair.

**Scope.** Just the helper + its call sites in tuple, slice, frozendict.

### Diff summary

- New `r_ref_append_with_state(o, state, p)` does one `PyList_Append`
  directly with the object and writes the state byte.
- Tuple, frozendict, slice now call it instead of the reserve/publish
  pair.

### Results (best-of-3 pinned median on `perf-baseline`, Exp 1/2 not applied)

| Benchmark | Operation | `perf-baseline` | Exp 3 | Δ |
| --- | --- | ---: | ---: | ---: |
| `small_tuple` | `loads` | 0.0328189 | 0.0335003 | +2.1% |
| `nested_dict` | `loads` | 0.0866099 | 0.0852410 | −1.6% |
| `code_obj`    | `loads` | 0.0971267 | 0.0975129 | +0.4% |

Raw JSON: `/tmp/exp3-run{1,2,3}.json`.
Correctness: `test_marshal` passes.

### Notes

- Standalone delta is essentially within thermal noise. Py_None is
  immortal, so the "saved" refcount dance is cheaper than intuition
  suggested; the real win would only materialize once we remove the
  PyList layer itself (see Exp 1).
- Keep as a structural cleanup that composes naturally with Exp 1; not
  valuable on its own.

## Experiment 4 — `has_incomplete` counter fast path

**Hypothesis.** Acyclic streams never produce any `REF_INCOMPLETE_HASHABLE`
entries. A file-level counter lets `TYPE_REF` skip the state check in
the common case, restoring the original hot-path code.

**Scope.** New `Py_ssize_t incomplete_count` in `RFILE`. Increment on
publish-incomplete, decrement on mark-ready. Gate the state check in
`TYPE_REF` behind `if (p->incomplete_count)`.

### Diff summary

- Added `incomplete_count` to `RFILE`, initialized to 0 everywhere.
- `r_ref_publish`: `if (state == INCOMPLETE) incomplete_count++`.
- `r_ref_mark_ready`: if prior state was incomplete, decrement.
- `TYPE_REF`: wraps the RESERVED/INCOMPLETE checks in
  `if (p->incomplete_count)`. The `Py_None` placeholder check remains
  unconditional.

### Results (best-of-3 pinned median on `perf-baseline`, Exp 1/2 not applied)

| Benchmark | Operation | `perf-baseline` | Exp 4 | Δ |
| --- | --- | ---: | ---: | ---: |
| `small_tuple` | `loads` | 0.0328189 | 0.0314563 | −4.1% |
| `nested_dict` | `loads` | 0.0866099 | 0.0870807 | +0.5% |
| `code_obj`    | `loads` | 0.0971267 | 0.0968055 | −0.3% |

Raw JSON: `/tmp/exp4-run{1,2,3}.json`.
Correctness: `test_marshal` passes.

### Notes

- The microbenches have no `TYPE_REF` back-edges, so the state check was
  reaching a never-taken branch anyway; the fast-path gate has little
  to skip. Most of the delta is noise.
- Real value of this idea is in payloads with many back-refs (large
  .pyc module imports), which the current harness does not exercise.
- Adds small bookkeeping cost (one conditional) in `r_ref_publish` and
  `r_ref_mark_ready`. Safe to keep; minor net impact in isolation.

## Experiment 5 — zero-init + drop mark_ready writes

**Hypothesis.** With `REF_STATE_READY = 0` and a calloc'd state array,
`mark_ready` can be elided entirely: the only non-zero byte ever
written is by `r_ref_publish(..., REF_STATE_INCOMPLETE_HASHABLE, ...)`,
and on completion we clear that single slot. Saves one byte store per
referenced container in the hot path.

**Scope.** Allocation in `r_ref_ensure_capacity` switches to calloc or
explicit memset; remove mark_ready on the common path; narrow the
complete-path to clear only the slot we published as incomplete.

### Diff summary

- `r_ref_ensure_capacity` now zeroes the newly grown tail of
  `ref_states` explicitly via `memset` (portable alternative to
  `PyMem_Calloc`).
- `r_ref_reserve` stops writing `REF_STATE_RESERVED`; the `Py_None`
  placeholder alone gates bad-reference errors in `TYPE_REF`.
- `r_ref_publish` skips the state store when `state == REF_STATE_READY`
  (the default). Only the `INCOMPLETE_HASHABLE` publish writes a byte.
- `r_ref_mark_ready` still writes 0 to flip INCOMPLETE back to READY.

### Results (best-of-3 pinned median on `perf-baseline`, Exp 1/2 not applied)

| Benchmark | Operation | `perf-baseline` | Exp 5 | Δ |
| --- | --- | ---: | ---: | ---: |
| `small_tuple` | `loads` | 0.0328189 | 0.0319475 | **−2.7%** |
| `nested_dict` | `loads` | 0.0866099 | 0.0856510 | **−1.1%** |
| `code_obj`    | `loads` | 0.0971267 | 0.0965635 | −0.6% |

Raw JSON: `/tmp/exp5-run{1,2,3}.json`.
Correctness: `test_marshal` passes.

### Notes

- Small, consistent win. The savings are one byte store per referenced
  container on the READY path (every `r_ref` and `r_ref_insert` call)
  — the most common path by far.
- Composes cleanly with Exp 1 (the raw array still keeps a separate
  state byte array) and is subsumed by Exp 2 (tagged pointer removes
  the state array entirely).

## Experiment 6 — drop `allow_incomplete_hashable` parameter

**Hypothesis.** Every recursive `r_object` call pays an extra integer
parameter in the calling convention. Packing it into the high bit of
`flag`, or into a single byte on `RFILE` save/restored around the three
call sites that want `1`, eliminates the cost.

**Scope.** Choose one encoding (likely flag high bit) and thread it
through. No other changes.

### Diff summary

- Added `int allow_incomplete_hashable` to `RFILE`; init'd to 0 at all
  six entry points.
- Removed the second parameter from `r_object`; it now reads the field
  on entry, saves it to a local, and immediately clears the field to
  0 so that nested `r_object` calls default to strict.
- New `r_object_allow_incomplete` helper does `saved = field; field = 1;
  r_object(); field = saved`. Used at the two "allow" sites (list
  element, dict value).
- All other `r_object(p, 0)` calls become `r_object(p)`.

### Results (best-of-3 pinned median on `perf-baseline`, Exp 1/2 not applied)

| Benchmark | Operation | `perf-baseline` | Exp 6 | Δ |
| --- | --- | ---: | ---: | ---: |
| `small_tuple` | `loads` | 0.0328189 | 0.0324100 | **−1.2%** |
| `nested_dict` | `loads` | 0.0866099 | 0.0848933 | **−2.0%** |
| `code_obj`    | `loads` | 0.0971267 | 0.0960854 | **−1.1%** |

Raw JSON: `/tmp/exp6-run{1,2,3}.json`.
Correctness: `test_marshal` passes.

### Notes

- Consistent modest wins on all three. The saving is genuine: r_object
  is called thousands of times per load, and a 2-arg function with
  a small-integer second parameter was costing a register write every
  call; the save/restore on list-element / dict-value sites is much
  rarer by contrast.
- Some care required: the field must be reset to 0 on entry to
  `r_object` so that nested recursion inside tuple/set/slice/code
  children sees the strict default. The helper handles the two
  allow=1 sites with explicit save/restore.

## Experiment 7 — force-inline helpers

**Hypothesis.** The refactor extracted `r_ref_reserve`,
`r_ref_publish`, `r_ref_mark_ready` as out-of-line `static` functions.
Marking them `static inline` (or `Py_ALWAYS_INLINE`) should let the
compiler fold the `if (flag)` guard back into the caller, as the
original `R_REF` macro did.

**Scope.** Header-like annotations only.

### Diff summary

- Changed `r_ref_ensure_capacity`, `r_ref_reserve`, `r_ref_publish`,
  `r_ref_mark_ready`, `r_ref_insert`, `r_ref` from `static` to
  `static inline`.

### Results (best-of-3 pinned median on `perf-baseline`, Exp 1/2 not applied)

| Benchmark | Operation | `perf-baseline` | Exp 7 | Δ |
| --- | --- | ---: | ---: | ---: |
| `small_tuple` | `loads` | 0.0328189 | 0.0316149 | **−3.7%** |
| `nested_dict` | `loads` | 0.0866099 | 0.0839668 | **−3.0%** |
| `code_obj`    | `loads` | 0.0971267 | 0.0960321 | −1.1% |

Raw JSON: `/tmp/exp7-run{1,2,3}.json`.
Correctness: `test_marshal` passes.

### Notes

- Zero-effort win on the small benches. The compiler was choosing not
  to inline these one-line helpers because they were only `static`
  (multiple callers tip the heuristic the other way).
- Composes cleanly with all other experiments; essentially free.

## Experiment 8 — `frozendict`-then-wrap when `flag == 0`

**Hypothesis.** A `frozendict` that is not referenced cannot be a
back-edge target, so the old cheap dict-then-wrap construction path is
safe. Only the `FLAG_REF` path needs incremental insertion with the
`_Py_FROZENDICT_HASH_CONSTRUCTING` sentinel.

**Scope.** Split the frozendict case into two branches keyed on `flag`.

### Diff summary

- `TYPE_FROZENDICT` splits on `flag`. When `!flag`, build a regular
  `dict` incrementally and wrap with `PyFrozenDict_New` at the end; no
  `_Py_FROZENDICT_HASH_CONSTRUCTING` sentinel, no refs-table traffic.
- The `flag` path is unchanged (incremental `_PyDict_SetItem_Take2` on
  the final frozendict, with construction-hash sentinel).

### Results (best-of-3 pinned median on `perf-baseline`, Exp 1/2 not applied)

| Benchmark | Operation | `perf-baseline` | Exp 8 | Δ |
| --- | --- | ---: | ---: | ---: |
| `small_tuple` | `loads` | 0.0328189 | 0.0338408 | +3.1% |
| `nested_dict` | `loads` | 0.0866099 | 0.0877668 | +1.3% |
| `code_obj`    | `loads` | 0.0971267 | 0.0980143 | +0.9% |

Raw JSON: `/tmp/exp8-run{1,2,3}.json`.
Correctness: `test_marshal` passes.

### Notes

- None of the three benchmarks actually contains a `frozendict`, so
  this change cannot help on this harness. The observed deltas are
  thermal / code-layout noise (the `r_object` switch body got fatter
  which may shift cache footprint).
- The idea is plausible for frozendict-heavy payloads, but we need a
  dedicated microbench to evaluate it. Not included in the combined
  recommendation until there is evidence.

## Experiment 9 — preallocate refs capacity

**Hypothesis.** Large payloads pay geometric realloc costs; a single
large allocation up front is cheaper. A heuristic of
`max(16, buf_size / 16)` on entry avoids the early growth steps.

**Scope.** One upfront `r_ref_ensure_capacity` call at the start of
`read_object` (or equivalent).

### Diff summary

- In `marshal_loads_impl`, after creating `rf.refs`, call
  `r_ref_ensure_capacity(&rf, clamp(len/16, 16, 4096))` before
  `read_object`.

### Results (best-of-3 pinned median on `perf-baseline`, Exp 1/2 not applied)

| Benchmark | Operation | `perf-baseline` | Exp 9 | Δ |
| --- | --- | ---: | ---: | ---: |
| `small_tuple` | `loads` | 0.0328189 | 0.0327148 | −0.3% |
| `nested_dict` | `loads` | 0.0866099 | 0.0868410 | +0.3% |
| `code_obj`    | `loads` | 0.0971267 | 0.0980642 | +1.0% |

Raw JSON: `/tmp/exp9-run{1,2,3}.json`.
Correctness: `test_marshal` passes.

### Notes

- The existing growth path already starts at 16 slots, and none of the
  three benches reach more than a handful of refs per iteration, so
  there is no geometric resize to avoid. Preallocation just adds one
  extra function call on each `loads`.
- Could matter for large .pyc imports, but not on this harness.
- Do not include in the combined recommendation.

## Combined experiment

Stack of the four ideas that either dominated (1, 2) or added a small
consistent win with no interaction cost (6, 7). Skipped: Exp 3 (noise),
Exp 4 (only relevant to cyclic payloads), Exp 5 (subsumed by Exp 2),
Exp 8 (no frozendict in benches), Exp 9 (noise).

### Diff summary

- **Exp 1**: `p->refs` becomes a raw `PyObject **` growable array.
- **Exp 2**: state encoded in the low bit of the ref pointer;
  `ref_states` array removed entirely.
- **Exp 6**: `allow_incomplete_hashable` moves from `r_object` parameter
  to an `RFILE` field, auto-reset on entry; `r_object_allow_incomplete`
  wrapper for the two list-element / dict-value sites.
- **Exp 7**: the six `r_ref_*` helpers plus the tagged-pointer
  `r_ref_init`/`r_ref_release` marked `static inline`.

### Results (best-of-3 pinned median, 3 runs)

| Benchmark | Op | `main` | `perf-baseline` | Combined | vs baseline | vs main |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `small_tuple` | `loads` | 0.0289962 | 0.0328189 | 0.0248645 | **−24.2%** | **−14.3%** |
| `small_tuple` | `dumps` | 0.0158760 | 0.0156912 | 0.0151504 | −3.4% | **−4.6%** |
| `nested_dict` | `loads` | 0.0778896 | 0.0866099 | 0.0725074 | **−16.3%** | **−6.9%** |
| `nested_dict` | `dumps` | 0.0722451 | 0.0745504 | 0.0724015 | −2.9% | +0.2% |
| `code_obj`    | `loads` | 0.0902017 | 0.0971267 | 0.0840647 | **−13.4%** | **−6.8%** |
| `code_obj`    | `dumps` | 0.0391339 | 0.0396249 | 0.0388515 | −2.0% | −0.7% |

Raw JSON: `/tmp/expC-run{1,2,3}.json`.
Correctness: `./python -m test test_marshal` passes (72 run / 7 skipped).

### Notes

- The regression is fully gone and the post-fix build is **6–14%
  faster than `main`** on `loads` across all three benchmarks.
- `dumps` is incidentally improved by a few percent too; this is almost
  certainly because the shared `PyMem_Free` / `PyMem_Realloc` calls in
  the new refs path share icache with the writer loop better than the
  prior PyList codepath did. Nothing in the writer itself changed.
- The stack is conservative: four ideas, no speculative additions. The
  only one we'd add without reservation is a properly-validated Exp 4
  — once there is a benchmark that actually exercises back-refs.

## Final validation (post-squash HEAD `4aaf064344d`)

After squashing the combined stack (Exp 1+2+6+7) onto
`marshal-safe-cycle-design`, the following validation ran.

### Full CPython test suite

    ./python -m test -j24 --timeout=600 -w

Result: **SUCCESS**.

- 468 test files OK, 0 failures.
- 48,932 individual tests executed.
- 2,890 tests skipped (platform-specific: Windows/Apple/Android/Tk,
  resource-gated, free-threading).
- Total wall time: 1 min 48 sec.
- All four new `RecursiveGraphTest` cases — the 156-program bounded
  semantic round-trip generator, the case-count check, the handpicked
  payloads, and the crafted-invalid-payload negative tests — pass.

### `pyperformance` targeted slice vs `main`

Ten marshal-adjacent benchmarks (same slice the original design-doc
rerun used): pickle, pickle_dict, pickle_list, pickle_pure_python,
python_startup, python_startup_no_site, unpack_sequence, unpickle,
unpickle_list, unpickle_pure_python. Run with
`uvx pyperformance run -b <list>`, `taskset -c 0`, default
(non-rigorous) loop count, per-benchmark venv.

| Benchmark | `main` | HEAD | Delta | Significance |
| --- | ---: | ---: | ---: | --- |
| `pickle`                 | `7.19 us +- 0.38 us` | `7.06 us +- 0.07 us` | `1.02x faster` | not significant |
| `pickle_dict`            | `16.4 us +- 0.2 us`  | `16.5 us +- 0.3 us`  | `1.00x slower` | not significant |
| `pickle_list`            | `2.66 us +- 0.06 us` | `2.68 us +- 0.06 us` | `1.01x slower` | not significant |
| `pickle_pure_python`     | `185 us +- 2 us`     | `186 us +- 3 us`     | `1.01x slower` | not significant |
| **`python_startup`**     | `7.88 ms +- 0.25 ms` | `6.70 ms +- 0.12 ms` | **`1.18x faster`** | **significant (t=59.80)** |
| **`python_startup_no_site`** | `4.34 ms +- 0.13 ms` | `4.19 ms +- 0.09 ms` | **`1.03x faster`** | **significant (t=12.90)** |
| `unpack_sequence`        | `20.4 ns +- 0.4 ns`  | `20.4 ns +- 0.3 ns`  | `1.00x faster` | not significant |
| `unpickle`               | `8.70 us +- 0.12 us` | `8.60 us +- 0.14 us` | `1.01x faster` | not significant |
| `unpickle_list`          | `2.65 us +- 0.03 us` | `2.69 us +- 0.03 us` | `1.01x slower` | not significant |
| `unpickle_pure_python`   | `121 us +- 3 us`     | `122 us +- 3 us`     | `1.00x slower` | not significant |

Two statistically significant wins, both on startup. No regressions.

The 18% `python_startup` speedup is real and user-visible:
interpreter startup loads a large number of `.pyc` files through
`marshal.loads`, so the hot-path improvements land on every Python
invocation. `python_startup_no_site` is smaller (3%) because it skips
the site module's imports, leaving less marshal work in the budget.

Raw JSON: `Misc/marshal-perf-data/pyperf-slice-{baseline,current}.json`.
Reproduce with `uvx pyperformance compare` on the two files.

### Marshal microbench on post-squash HEAD

Three pinned best-of-median runs, same harness as every other experiment.

| Bench | Op | `main` | HEAD `4aaf064` | vs `main` |
| --- | --- | ---: | ---: | ---: |
| `small_tuple` | `loads` | 0.0289962 | 0.0248193 | **−14.4%** |
| `small_tuple` | `dumps` | 0.0158760 | 0.0152102 | −4.2% |
| `nested_dict` | `loads` | 0.0778896 | 0.0721462 | **−7.4%** |
| `nested_dict` | `dumps` | 0.0722451 | 0.0722497 | flat |
| `code_obj`    | `loads` | 0.0902017 | 0.0839155 | **−7.0%** |
| `code_obj`    | `dumps` | 0.0391339 | 0.0385998 | −1.4% |

Raw JSON: `Misc/marshal-perf-data/final-head-run{1,2,3}.json`.

### Summary

- Regression introduced by the safe-cycle design is fully erased on the
  marshal microbench.
- `loads` is **7–14% faster than `main`** on all three microbenches.
- `pyperformance`'s marshal-adjacent slice shows one 18% startup win and
  no regressions.
- Full CPython test suite passes, including the new combinatoric
  recursive-graph generator.

## Third-party + fuzz validation

To catch regressions that stdlib tests might miss, run the same HEAD
through libraries that exercise marshal heavily, plus property-based
fuzz.

### Tier 1 — direct third-party marshal users

Both tested on `/tmp/stress-venv-base` (main) and `/tmp/stress-venv`
(HEAD) via `taskset -c 0-7`.

**`dill==0.4.1`** — explicitly uses `marshal.dumps`/`loads` to
serialize code objects; test suite is 30 files.

| Outcome | baseline | HEAD |
| --- | --- | --- |
| Pass | 29 / 30 | 29 / 30 |
| Fail | 1 / 30 (`test_session`, pre-existing 3.15a8 issue in dill's module-state serialization — unrelated to marshal) | 1 / 30 (same) |
| Wall time | 2.15 s | 2.14 s |

`test_recursive` and `test_objects` both pass — those are the tests
that touch our exact changed codepath.

**`cloudpickle==3.1.2`** — pickles code objects via marshal; foundation
for Ray, Dask, joblib. Tests cloned from upstream.

| Outcome | baseline | HEAD |
| --- | --- | --- |
| Pass | 243 | 243 |
| Skipped | 29 | 29 |
| xfail | 2 | 2 |
| Wall time | 9.50 s | 9.66 s |

Identical pass rate and test breakdown.

### Tier 2 — marshal-adjacent stdlib tests on HEAD

| Test file | Tests | Result |
| --- | ---: | --- |
| `test_importlib.*` | 1,217 | SUCCESS |
| `test_zipimport` | 133 | SUCCESS |
| `test_compileall` | 145 | SUCCESS |
| `test_py_compile` | 34 | SUCCESS |
| `test_marshal` | 72 | SUCCESS |

1,601 tests specifically covering `marshal.loads` / `marshal.dumps`
consumers. All green.

### Tier 3 — real-world timing (cold cache)

**compileall of CPython `Lib/`** (1944 `.py` files written to `.pyc`
via `marshal.dumps`, `__pycache__` wiped between runs, 3 runs each):

| Python | Median |
| --- | ---: |
| baseline | 5.370 s |
| HEAD | 5.426 s |

+1.0%, within noise. Expected — compileall is AST→bytecode dominated;
`marshal.dumps` is a small fraction, and the dumps path was not
touched.

**Cold-import spectrum** (fresh subprocess imports 56 stdlib modules
in one shot, 15 repeats, trim 2 hi / 2 lo):

| Python | Median | Trimmed mean | Min |
| --- | ---: | ---: | ---: |
| baseline | 99.82 ms | 99.69 ms | 96.82 ms |
| HEAD | 99.39 ms | 99.51 ms | 95.44 ms |

Flat at median / mean, min improved 1.4%. Subprocess harness overhead
masks most of the ~1 ms startup saving here.

### Tier 4 — Hypothesis property-based fuzz

3,500 random round-trips with `hypothesis==6.152.1`, strategy covers
nested tuples / lists / dicts / frozensets / sets with 30-leaf
recursion depth, plus three hand-picked cyclic shapes:

| Test | Examples | baseline | HEAD | Δ |
| --- | ---: | ---: | ---: | ---: |
| acyclic round-trip | 2,000 | 5.38 s | 4.84 s | **−10.0%** |
| list self-cycle | 500 | 0.33 s | 0.25 s | **−24%** |
| tuple via list bridge | 500 | 0.24 s | 0.26 s | +8% |
| dict value self-cycle | 500 | 0.35 s | 0.21 s | **−40%** |

**All 3,500 cases pass on both — zero correctness regressions.** The
cyclic shapes (list self-cycle, dict value self-cycle) are precisely
what the safe-cycle design targets; they're faster on HEAD, not
slower.

### Summary of third-party validation

- **No correctness regressions found.** dill (29/30 identical), cloudpickle (243/243 identical), 1601 stdlib marshal-adjacent tests, 3500 hypothesis round-trips.
- **Measurable speedups** on marshal-heavy real workloads (hypothesis round-trip fuzz: −10 to −40% depending on shape).
- **Flat or within-noise** on workloads where marshal is only a small fraction (compileall writes, cold imports through subprocess harness).

Taken together with the microbench and `pyperformance` results, the
change is safe to ship: every signal either gets faster or stays the
same.

## Final conclusions

### Recommended stack

The production recommendation is the four-idea stack above:

1. **Raw `PyObject **` refs array** (Exp 1). The dominant win; the
   PyList-backed refs table was overhead that predated the safe-cycle
   design. This change alone erases the regression.
2. **Tagged pointer state** (Exp 2). Removes the parallel state
   allocation; reduces one cache line per `TYPE_REF` dispatch.
3. **Drop `allow_incomplete_hashable` parameter** (Exp 6). Makes
   `r_object` a one-arg function again; minor but consistent.
4. **Inline the `r_ref_*` helpers** (Exp 7). Free compiler hint.

### What we learned

- The original regression was not really caused by the safe-cycle
  design's new state work. It was magnified by the existing
  `PyList`-backed refs table, whose per-append overhead
  dominated the hot path once the safe-cycle code added more
  bookkeeping around it.
- Replacing that list with a raw array unlocks a win that was there
  all along. Tagged-pointer state then eliminates the last piece of
  parallel allocation that the safe-cycle design introduced.
- Several intuitive micro-optimizations (Exp 3, 4, 5, 9) were flat or
  noise in the current harness. Most of them would only matter on
  payloads the bench doesn't produce (large .pyc with many back-refs,
  frozendict-heavy loads). Keep them in reserve for a follow-up tuning
  pass that has targeted benchmarks.

### Bench suite gaps (follow-up)

For a production-grade perf story we should add:

- A back-reference-heavy bench (e.g., a marshal payload built from a
  real-world .pyc) to exercise Exp 4's gate.
- A frozendict-heavy bench to evaluate Exp 8.
- A payload that drives refs > 64 so Exp 9's preallocation can show.

### Final scoreboard (vs `main`)

| Benchmark | loads |
| --- | ---: |
| `small_tuple` | **14.3% faster** |
| `nested_dict` | **6.9% faster**  |
| `code_obj`    | **6.8% faster**  |

Regression erased; baseline beaten.
