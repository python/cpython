"""Microbenchmark compact vs wide int add/sub with pyperf.

Use this with PYTHON_JIT=0 and -S if you want a stable interpreter-only run:

    PYTHON_JIT=0 ./python.exe -S Tools/scripts/bench_wide_int_pyperf.py
"""

from __future__ import annotations

import pyperf


def bench_add_compact() -> int:
    a = 1
    b = 2
    return a + b


def bench_add_wide() -> int:
    a = 10_000_000_000
    b = 1
    return a + b


def bench_sub_compact() -> int:
    a = 1
    b = 2
    return a - b


def bench_sub_wide() -> int:
    a = 10_000_000_000
    b = 1
    return a - b


def main() -> None:
    runner = pyperf.Runner()
    runner.bench_func("add_compact", bench_add_compact)
    runner.bench_func("add_wide", bench_add_wide)
    runner.bench_func("sub_compact", bench_sub_compact)
    runner.bench_func("sub_wide", bench_sub_wide)


if __name__ == "__main__":
    main()
