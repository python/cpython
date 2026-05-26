"""pyperf microbenchmark for JIT int fast path.

Measures int arithmetic performance in hot JIT loops where
intermediate results overflow the 30-bit compact range.

Usage:
    python Tools/scripts/jit_int_benchmark_pyperf.py
    python Tools/scripts/jit_int_benchmark_pyperf.py -o result.json --rigorous
"""

import time


def bench_intermediate_overflow(loops):
    """a + b exceeds 30-bit range, (a+b)-c fits back in."""
    a = (1 << 30) - 1
    b = (1 << 30) - 1000
    c = 1 << 30
    t0 = time.perf_counter()
    for _ in range(loops):
        # inner loop: 50 ops to amplify the signal
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
        x = a + b - c
    return time.perf_counter() - t0


def bench_double_add(loops):
    """(a+b)+c where a+b is non-compact but a,b,c are compact."""
    a = (1 << 30) - 1
    b = (1 << 30) - 1000
    c = -500
    t0 = time.perf_counter()
    for _ in range(loops):
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
        x = a + b + c
    return time.perf_counter() - t0


def bench_accumulate(loops):
    """Values grow through compact boundary (2^30)."""
    total = 0
    t0 = time.perf_counter()
    for _ in range(loops):
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
        total += 1000000
    return time.perf_counter() - t0


def bench_always_large(loops):
    """Compact inputs, non-compact result."""
    total = 0
    va = 1 << 29
    vb = 1 << 29
    t0 = time.perf_counter()
    for _ in range(loops):
        total += va + vb
        total &= (1 << 62) - 1
        total += va + vb
        total &= (1 << 62) - 1
        total += va + vb
        total &= (1 << 62) - 1
        total += va + vb
        total &= (1 << 62) - 1
        total += va + vb
        total &= (1 << 62) - 1
        total += va + vb
        total &= (1 << 62) - 1
        total += va + vb
        total &= (1 << 62) - 1
        total += va + vb
        total &= (1 << 62) - 1
        total += va + vb
        total &= (1 << 62) - 1
        total += va + vb
        total &= (1 << 62) - 1
    return time.perf_counter() - t0


def bench_mixed(loops):
    """Alternating small/medium ints."""
    total = 0
    t0 = time.perf_counter()
    for _ in range(loops):
        total += (1 << 30) + 1
        total += 100
        total += (1 << 30) + 2
        total += 200
        total += (1 << 30) + 3
        total += 300
        total += (1 << 30) + 4
        total += 400
        total += (1 << 30) + 5
        total += 500
    return time.perf_counter() - t0


def bench_small(loops):
    """All small ints (baseline)."""
    total = 0
    t0 = time.perf_counter()
    for _ in range(loops):
        total += 1
        total += 2
        total += 3
        total += 4
        total += 5
        total += 6
        total += 7
        total += 8
        total += 9
        total += 10
        total += 11
        total += 12
        total += 13
        total += 14
        total += 15
        total += 16
        total += 17
        total += 18
        total += 19
        total += 20
    return time.perf_counter() - t0


BENCHMARKS = [
    ("jit_int_small", bench_small),
    ("jit_int_intermediate_overflow", bench_intermediate_overflow),
    ("jit_int_double_add", bench_double_add),
    ("jit_int_accumulate", bench_accumulate),
    ("jit_int_always_large", bench_always_large),
    ("jit_int_mixed", bench_mixed),
]

if __name__ == "__main__":
    import pyperf
    runner = pyperf.Runner()
    for name, fn in BENCHMARKS:
        runner.bench_time_func(name, fn)
