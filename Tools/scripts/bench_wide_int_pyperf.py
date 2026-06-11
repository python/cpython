"""Microbenchmark compact vs wide int add/sub with pyperf.

Use this with PYTHON_JIT=0 for a stable interpreter-only run:

    PYTHON_JIT=0 ./python.exe Tools/scripts/bench_wide_int_pyperf.py
"""

from __future__ import annotations

import pyperf

_BATCH = 10_000
_sink = 0


def _bench_add(a: int, b: int) -> None:
    global _sink
    x = 0
    for _ in range(_BATCH):
        x = a + b
    _sink = x


def _bench_sub(a: int, b: int) -> None:
    global _sink
    x = 0
    for _ in range(_BATCH):
        x = a - b
    _sink = x


def bench_add_compact() -> None:
    _bench_add(1, 2)


def bench_add_wide() -> None:
    _bench_add(1 << 40, 1)


def bench_sub_compact() -> None:
    _bench_sub(1, 2)


def bench_sub_wide() -> None:
    _bench_sub(1 << 40, 1)


def bench_chain_compact() -> None:
    global _sink
    total = 0
    a = 2000
    b = 3
    c = 4000
    for _ in range(_BATCH):
        total += a * b + c
    _sink = total


def bench_chain_wide() -> None:
    global _sink
    total = 0
    a = 1 << 40
    b = 3
    c = 1 << 20
    for _ in range(_BATCH):
        total += a * b + c
    _sink = total


def main() -> None:
    runner = pyperf.Runner()
    runner.bench_func("add_compact", bench_add_compact)
    runner.bench_func("add_wide", bench_add_wide)
    runner.bench_func("sub_compact", bench_sub_compact)
    runner.bench_func("sub_wide", bench_sub_wide)
    runner.bench_func("chain_compact", bench_chain_compact)
    runner.bench_func("chain_wide", bench_chain_wide)


if __name__ == "__main__":
    main()
