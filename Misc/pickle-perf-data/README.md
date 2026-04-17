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

`pickle_save_profile.py` — `cProfile`-based breakdown used once to
identify which internal calls dominate `save()` (informed the priority
ordering of ideas D, E over B).

Run each with `taskset -c 0 ./python <script>` on a quiet machine.

## JSON files

| File | Commit / state |
| --- | --- |
| `pickle-pure-baseline.json` | Clean `main` (2faceeec), no pickle patches |
| `pickle-pure-exp4d.json` | Exp 4 only (exact-container fast paths, `94b53eb`) |
| `pickle-pure-Donly-verify.json` | Exp 4 + D (inlined `commit_frame`) |
| `pickle-pure-BD.json` | Exp 4 + D + B attempt — **used to confirm B regression** |
| `pickle-pure-DE.json` | Exp 4 + D + E (int-only initial form) |
| `pickle-pure-DE-v2.json` | Exp 4 + D + E (str added — the shipped form, `bb9d721`) |

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
