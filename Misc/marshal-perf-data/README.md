# Marshal Perf Raw Data

Raw JSON artifacts backing `Misc/marshal-perf-diary.md`. Everything here
is regeneratable — kept checked in so reviewers can inspect numbers
without rerunning the full methodology.

## Marshal microbench (`marshal_bench_cpu_stable.py`)

Three pinned runs per commit, 11 repeats per operation, 200k `loads` ×
100k `dumps`. Pinning: `taskset -c 0`.

| File | Commit / idea |
| --- | --- |
| `exp0-run{1..3}.json` | `perf-baseline` self-check at `eb1c4b7` |
| `exp1-run{1..3}.json` | Experiment 1 (raw `PyObject **` refs array) |
| `exp2-run{1..3}.json` | Experiment 2 (tagged pointer state, stacked on 1) |
| `exp3-run{1..3}.json` | Experiment 3 (collapse reserve+publish) |
| `exp4-run{1..3}.json` | Experiment 4 (`has_incomplete` counter) |
| `exp5-run{1..3}.json` | Experiment 5 (zero-init + drop mark_ready) |
| `exp6-run{1..3}.json` | Experiment 6 (drop `allow_incomplete_hashable` param) |
| `exp7-run{1..3}.json` | Experiment 7 (force-inline helpers) |
| `exp8-run{1..3}.json` | Experiment 8 (`frozendict`-then-wrap when `flag==0`) |
| `exp9-run{1..3}.json` | Experiment 9 (preallocate refs capacity) |
| `expC-run{1..3}.json` | Combined (Exp 1+2+6+7) on `exp/combined-winners` |
| `final-head-run{1..3}.json` | Post-squash HEAD `4aaf064344d` (same as Combined) |

Schema per bench (small_tuple / nested_dict / code_obj):

    loads_number, dumps_number        # inner loop counts
    loads_runs: [11 times]            # raw timings
    dumps_runs: [11 times]
    loads_min, loads_median           # statistics
    dumps_min, dumps_median

## pyperformance slice

Ten marshal-adjacent benchmarks (pickle × 4, unpickle × 3,
python_startup × 2, unpack_sequence). Run with `uvx pyperformance run
-b <list> --affinity 0` via `taskset -c 0`.

| File | Python |
| --- | --- |
| `pyperf-slice-baseline.json` | `main` tip `2faceeec` built at `/tmp/cpython-baseline/python` |
| `pyperf-slice-current.json`  | HEAD `4aaf064344d` built in-tree |

These are standard `pyperf` format; compare with:

    uvx pyperformance compare pyperf-slice-baseline.json pyperf-slice-current.json

## Regeneration

See `Misc/marshal-perf-diary.md` → "Ground rules" for the exact harness
(`/tmp/marshal_bench_cpu_stable.py` inline script) and methodology.
