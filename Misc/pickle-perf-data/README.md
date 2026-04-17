# Pickle Perf Raw Data

Raw artifacts backing `Misc/pickle-perf-diary.md`. Regeneratable;
checked in so reviewers can re-verify numbers without rerunning the
methodology.

## Harness

`pickle_pure_bench.py` — the pure-Python `pickle._Pickler` /
`_Unpickler` benchmark used throughout. Five workloads (list-of-ints,
list-of-strs, flat str-keyed dict, deep list-of-lists, nested
list-of-dicts). Each reports a best-of-9 median for dump and load at
protocol 5.

`pickle_pure_bench_bytes.py` — bytes-heavy workload (short bytes,
medium bytes, bytearrays, bytes-keyed dict). Introduced in round 2 to
evaluate F6 (bytes in the save() fast path).

`pickle_save_profile.py` — `cProfile`-based breakdown used to identify
which internal calls dominate `save()` (informed the priority ordering
of ideas D, E over B; in round 2, drove the F1 / F2 / F4 ordering).

Run each with `taskset -c 0 ./python <script>` on a quiet machine.

## JSON files

### Round 1 (Exp 4 → E)

| File | Commit / state |
| --- | --- |
| `pickle-pure-baseline.json` | Clean `main` (2faceeec), no pickle patches |
| `pickle-pure-exp4d.json` | Exp 4 only (exact-container fast paths, `94b53eb`) |
| `pickle-pure-Donly-verify.json` | Exp 4 + D (inlined `commit_frame`) |
| `pickle-pure-BD.json` | Exp 4 + D + B attempt — **used to confirm B regression** |
| `pickle-pure-DE.json` | Exp 4 + D + E (int-only initial form) |
| `pickle-pure-DE-v2.json` | Exp 4 + D + E (str added — `bb9d721`) |

### Round 2 (F1 → F6)

| File | Commit / state |
| --- | --- |
| `pickle-post-fix.json` | Large-dict mutation fix, before F1 reorder |
| `pickle-F1v2.json` | F1 (save() reordered, atomic short-circuit before memo, `285fcae`) |
| `pickle-F3.json` | F3 (frame byte counter) — **rejected**, reverted |
| `pickle-F4.json` | F4 (BININT1 opcode cache, `7c6af84`) |
| `pickle-F2.json` | F2 (inlined MEMOIZE in memoize(), `2f1d38b`) |
| `pickle-F5.json` | F5 (ASCII save_str) — **rejected**, reverted |
| `pickle-F6v2.json` | F6 (bytes in fast path, `e917108`) — current tip |

### Bytes-specific bench (introduced in round 2)

| File | Content |
| --- | --- |
| `pickle-bytes-pre.json` | Before F6; baseline for bytes workloads |
| `pickle-bytes-F6v2.json` | After F6 |

## Interpretation guide

Each JSON has per-workload records with:

    loads_number / dumps_number     # inner loop counts
    loads_runs / dumps_runs         # 9 raw timings
    loads_median / dumps_median     # primary statistic
    loads_min / dumps_min           # outlier-robust secondary

Compare two files with:

    ./python -c "
    import json
    def load(p):
        s = open(p).read()
        return json.loads(s[s.find('{'):])
    a = load('a.json'); b = load('b.json')
    for k in sorted(a):
        print(k, (b[k]['dump_median'] - a[k]['dump_median']) /
                  a[k]['dump_median'] * 100, '% dump')"

(The benchmark prints a summary line before the JSON, hence the
`s.find('{')` trick.)

## What's missing

Earlier-iteration `pickle-pure-exp4.json` / `exp4b.json` / `exp4c.json`
/ `exp4e.json` variants were not copied — they represent abandoned
shapes of the Exp 4 implementation (index-based iteration with its
mutation bug, dict-path using `iter()+next()`) and are superseded by
`exp4d.json`. Still available in `/tmp/` on the authoring machine.

`pickle-pure-D.json` / `pickle-pure-exp4.json` from the kernel-compile
thermal contamination event are excluded — numbers unusable.
