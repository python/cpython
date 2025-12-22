"""Microbenchmarks for collections.Counter.update() iterable fast path.

This is intended for quick before/after comparisons of small C-level changes.
It avoids third-party deps (e.g. pyperf) and prints simple, stable-enough stats.

Run (from repo root):
  PCbuild\\amd64\\python.exe Tools\\scripts\\bench_counter_update.py

You can also override sizes:
  ... bench_counter_update.py --n-keys 1000 --n-elems 200000
"""

from __future__ import annotations

import argparse
import statistics
import sys
import time
from collections import Counter


def _run_timer(func, *, inner_loops: int, repeats: int) -> dict[str, float]:
    # Warmup
    for _ in range(5):
        func()

    samples = []
    for _ in range(repeats):
        t0 = time.perf_counter()
        for _ in range(inner_loops):
            func()
        t1 = time.perf_counter()
        samples.append(t1 - t0)

    return {
        "min_s": min(samples),
        "mean_s": statistics.mean(samples),
        "stdev_s": statistics.pstdev(samples) if len(samples) > 1 else 0.0,
        "repeats": float(repeats),
        "inner_loops": float(inner_loops),
    }


def _format_line(name: str, stats: dict[str, float]) -> str:
    # Report per-call based on min (least noisy for microbench comparisons).
    per_call_ns = (stats["min_s"] / stats["inner_loops"]) * 1e9
    return (
        f"{name:32s}  {per_call_ns:10.1f} ns/call"
        f"  (min={stats['min_s']:.6f}s, mean={stats['mean_s']:.6f}s, "
        f"stdev={stats['stdev_s']:.6f}s, loops={int(stats['inner_loops'])}, reps={int(stats['repeats'])})"
    )


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--n-keys", type=int, default=1000)
    parser.add_argument("--n-elems", type=int, default=100_000)
    parser.add_argument("--repeats", type=int, default=25)
    parser.add_argument("--inner-loops", type=int, default=50)
    args = parser.parse_args(argv)

    n_keys = args.n_keys
    n_elems = args.n_elems

    # Data sets
    keys_unique = list(range(n_keys))

    # Many duplicates; all keys are within [0, n_keys)
    keys_dupes = [i % n_keys for i in range(n_elems)]

    # All elements hit the "oldval != NULL" branch by pre-seeding.
    seeded = Counter({k: 1 for k in range(n_keys)})

    def bench_unique_from_empty() -> None:
        c = Counter()
        c.update(keys_unique)

    def bench_dupes_from_empty() -> None:
        c = Counter()
        c.update(keys_dupes)

    def bench_dupes_all_preseeded() -> None:
        c = seeded.copy()
        c.update(keys_dupes)

    # A string-like workload (common Counter use): update over a repeated alphabet.
    alpha = ("abcdefghijklmnopqrstuvwxyz" * (n_elems // 26 + 1))[:n_elems]

    def bench_string_dupes_from_empty() -> None:
        c = Counter()
        c.update(alpha)

    print(sys.version.replace("\n", " "))
    print(f"n_keys={n_keys}, n_elems={n_elems}, repeats={args.repeats}, inner_loops={args.inner_loops}")
    print()

    for name, fn in (
        ("update(unique) from empty", bench_unique_from_empty),
        ("update(dupes) from empty", bench_dupes_from_empty),
        ("update(dupes) preseeded", bench_dupes_all_preseeded),
        ("update(string dupes) empty", bench_string_dupes_from_empty),
    ):
        stats = _run_timer(fn, inner_loops=args.inner_loops, repeats=args.repeats)
        print(_format_line(name, stats))

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
