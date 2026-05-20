"""
Benchmark for JIT int fast path optimization.

Measures performance of integer arithmetic in hot loops
where intermediate results overflow the 30-bit compact range.

Usage:
    python Tools/scripts/jit_int_benchmark.py
    python -X jit Tools/scripts/jit_int_benchmark.py
    PYTHON_JIT_STRESS=1 python -X jit Tools/scripts/jit_int_benchmark.py

For statistically rigorous comparison:
    python -m pyperf timeit "..."  # microbenchmark
    pyperformance run -b <group>   # application benchmarks
"""

import sys
import time
import statistics


# Benchmark configurations
N = 5_000_000
WARMUP = 3
RUNS = 7


def bench_intermediate_overflow():
    """a + b exceeds 30-bit compact range, but (a+b)-c fits back in.
    Tests the case where JIT would exit on the intermediate result."""
    a = (1 << 30) - 1
    b = (1 << 30) - 1000
    c = 1 << 30
    total = 0
    for i in range(N):
        total += a + b - c
    return total


def bench_double_add():
    """Two adds in a row: (a+b)+c where intermediate is non-compact."""
    a = (1 << 30) - 1
    b = (1 << 30) - 1000
    c = -500
    total = 0
    for i in range(N):
        total += a + b + c
    return total


def bench_accumulate():
    """Values slowly grow through the compact boundary.
    Tests the case where a non-compact value is accumulated."""
    total = 0
    for i in range(N):
        total += 1000000
    return total


def bench_always_large():
    """Both inputs are compact, but the result always exceeds compact range.
    Tests the result widening optimization."""
    total = 0
    a = 1 << 29
    b = 1 << 29
    for i in range(N):
        total += a + b
        total &= (1 << 62) - 1
    return total


def bench_mixed():
    """Mix of small and medium ints alternating.
    Tests ability to stay in JIT trace across varying int sizes."""
    total = 0
    for i in range(N):
        if i & 1:
            total += (1 << 30) + (i & 0xFF)
        else:
            total += i & 0xFFF
        total &= (1 << 61) - 1
    return total


def bench_chain():
    """Chained operations: a + b + c + d.
    Tests consecutive int ops without boxing between them."""
    total = 0
    for i in range(N // 4):
        total += i + i + i + i
    return total


def run_benchmark(name, fn, runs=RUNS, warmup=WARMUP):
    # Warmup
    for _ in range(warmup):
        fn()

    # Timed runs
    times = []
    last_result = None
    for _ in range(runs):
        start = time.perf_counter()
        last_result = fn()
        elapsed = time.perf_counter() - start
        times.append(elapsed)

    mean = statistics.mean(times)
    stdev = statistics.stdev(times) if len(times) > 1 else 0
    ns_per = mean / N * 1e9
    print(
        f"  {name:30s}  {mean * 1000:7.2f} ms  {ns_per:5.1f} ns/iter"
        f"  ±{stdev / mean * 100:4.1f}%  (verify={str(last_result)[:12]})"
    )


if __name__ == "__main__":
    jit = "jit" in sys._xoptions or "PYTHON_JIT" in __import__("os").environ
    print(
        f"Python {sys.version.split()[0]} ({sys.platform})"
        f"  JIT={'on' if jit else 'off'}  PYTHON_JIT_STRESS="
        f"{__import__('os').environ.get('PYTHON_JIT_STRESS', '0')}"
    )
    print()

    benchmarks = [
        ("intermediate_overflow", bench_intermediate_overflow),
        ("double_add", bench_double_add),
        ("always_large", bench_always_large),
        ("accumulate", bench_accumulate),
        ("mixed", bench_mixed),
        ("chain", bench_chain),
    ]

    for name, fn in benchmarks:
        run_benchmark(name, fn)

    print()
    print("Done.")
