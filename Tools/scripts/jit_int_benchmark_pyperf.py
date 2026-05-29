"""pyperf microbenchmarks for int fast path (interpreter and JIT).

Run twice — once with the JIT disabled, once enabled — to get both sets
of numbers.  Pass --inherit-environ=PYTHON_JIT so pyperf worker processes
pick up the environment variable.

Usage:
    # interpreter only
    PYTHON_JIT=0 ./python Tools/scripts/jit_int_benchmark_pyperf.py \\
        --inherit-environ=PYTHON_JIT -o nojit.json
    # JIT
    PYTHON_JIT=1 ./python Tools/scripts/jit_int_benchmark_pyperf.py \\
        --inherit-environ=PYTHON_JIT -o jit.json
    # compare
    ./python -m pyperf compare_to main_nojit.json branch_nojit.json
    ./python -m pyperf compare_to main_jit.json branch_jit.json
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
    """All small ints, bounded to stay in the small-int cache."""
    total = 0
    t0 = time.perf_counter()
    for _ in range(loops):
        total = (total + 1) & 255
        total = (total + 2) & 255
        total = (total + 3) & 255
        total = (total + 4) & 255
        total = (total + 5) & 255
        total = (total + 6) & 255
        total = (total + 7) & 255
        total = (total + 8) & 255
        total = (total + 9) & 255
        total = (total + 10) & 255
        total = (total + 11) & 255
        total = (total + 12) & 255
        total = (total + 13) & 255
        total = (total + 14) & 255
        total = (total + 15) & 255
        total = (total + 16) & 255
        total = (total + 17) & 255
        total = (total + 18) & 255
        total = (total + 19) & 255
        total = (total + 20) & 255
    return time.perf_counter() - t0


def bench_compact(loops):
    """Compact ints outside the small-int cache, bounded below 2**30."""
    total = 1 << 20
    mask = (1 << 29) - 1
    t0 = time.perf_counter()
    for _ in range(loops):
        total = (total + 10_000_001) & mask
        total = (total + 10_000_003) & mask
        total = (total + 10_000_019) & mask
        total = (total + 10_000_079) & mask
        total = (total + 10_000_103) & mask
        total = (total + 10_000_121) & mask
        total = (total + 10_000_123) & mask
        total = (total + 10_000_133) & mask
        total = (total + 10_000_139) & mask
        total = (total + 10_000_159) & mask
    return time.perf_counter() - t0


BENCHMARKS = [
    ("int_small", bench_small),
    ("int_compact", bench_compact),
    ("int_intermediate_overflow", bench_intermediate_overflow),
    ("int_double_add", bench_double_add),
    ("int_accumulate", bench_accumulate),
    ("int_always_large", bench_always_large),
    ("int_mixed", bench_mixed),
]

if __name__ == "__main__":
    import pyperf
    runner = pyperf.Runner()
    for name, fn in BENCHMARKS:
        runner.bench_time_func(name, fn)
