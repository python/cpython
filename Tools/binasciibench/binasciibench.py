#!/usr/bin/env python3
"""Benchmark for binascii base64 encoding and decoding performance.

This benchmark measures the throughput of base64 encoding and decoding
operations using the binascii module's C implementation.

Usage:
    python Tools/binasciibench/binasciibench.py [--sizes S1,S2,...]

Each benchmark runs for ~1.5 seconds to ensure accurate measurements.
"""

import argparse
import binascii
import os
import statistics
import sys
import time

# Default test parameters
DEFAULT_SIZES = [64, 1024, 65536, 1048576]

# Timing targets
TARGET_TOTAL_TIME_S = 1.5  # Target ~1.5 seconds total per benchmark
MIN_ITERATIONS = 5         # Minimum iterations for statistical significance
MIN_OPS_PER_ITER = 10      # Minimum operations per iteration


def generate_test_data(size):
    """Generate random binary data of the specified size."""
    return os.urandom(size)


def generate_base64_data(size):
    """Generate valid base64-encoded data of approximately the specified decoded size."""
    binary = os.urandom(size)
    return binascii.b2a_base64(binary, newline=False)


def benchmark_encode(data, num_ops):
    """Benchmark base64 encoding."""
    b2a = binascii.b2a_base64
    start = time.perf_counter_ns()
    for _ in range(num_ops):
        b2a(data, newline=False)
    end = time.perf_counter_ns()
    return end - start


def benchmark_decode(data, num_ops):
    """Benchmark base64 decoding."""
    a2b = binascii.a2b_base64
    start = time.perf_counter_ns()
    for _ in range(num_ops):
        a2b(data)
    end = time.perf_counter_ns()
    return end - start


def calibrate_and_run(bench_func, data, target_total_s):
    """Calibrate and run benchmark to achieve target total time.

    Returns (times_ns, num_ops) where times_ns is a list of per-iteration
    timings and num_ops is the number of operations per iteration.
    """
    # Quick calibration: measure time for a small batch
    num_ops = MIN_OPS_PER_ITER
    elapsed_ns = bench_func(data, num_ops)
    time_per_op_ns = elapsed_ns / num_ops

    # Calculate ops and iterations to hit target total time
    # We want: iterations * num_ops * time_per_op = target_total
    # With constraint: iterations >= MIN_ITERATIONS
    target_ns = target_total_s * 1_000_000_000

    # Start with minimum iterations, calculate required ops
    iterations = MIN_ITERATIONS
    total_ops_needed = int(target_ns / time_per_op_ns)
    num_ops = max(MIN_OPS_PER_ITER, total_ops_needed // iterations)

    # If num_ops would be huge, increase iterations instead
    max_ops_per_iter = 1_000_000
    if num_ops > max_ops_per_iter:
        num_ops = max_ops_per_iter
        iterations = max(MIN_ITERATIONS, total_ops_needed // num_ops)

    # Warmup
    bench_func(data, num_ops)

    # Timed runs
    times_ns = []
    for _ in range(iterations):
        elapsed_ns = bench_func(data, num_ops)
        times_ns.append(elapsed_ns)

    return times_ns, num_ops


def format_throughput(bytes_per_second):
    """Format throughput in human-readable units."""
    if bytes_per_second >= 1_000_000_000:
        return f"{bytes_per_second / 1_000_000_000:.2f} GB/s"
    elif bytes_per_second >= 1_000_000:
        return f"{bytes_per_second / 1_000_000:.2f} MB/s"
    elif bytes_per_second >= 1_000:
        return f"{bytes_per_second / 1_000:.2f} KB/s"
    else:
        return f"{bytes_per_second:.2f} B/s"


def format_size(size):
    """Format size in human-readable units."""
    if size >= 1_048_576:
        return f"{size // 1_048_576}M"
    elif size >= 1024:
        return f"{size // 1024}K"
    else:
        return str(size)


def print_results(name, size, times_ns, num_ops, data_size):
    """Print benchmark results."""
    # Calculate statistics
    times_per_op_ns = [t / num_ops for t in times_ns]
    mean_ns = statistics.mean(times_per_op_ns)
    stdev_ns = statistics.stdev(times_per_op_ns) if len(times_per_op_ns) > 1 else 0

    # Calculate throughput
    bytes_per_ns = data_size / mean_ns
    bytes_per_second = bytes_per_ns * 1_000_000_000
    throughput = format_throughput(bytes_per_second)

    # Calculate coefficient of variation
    cv = (stdev_ns / mean_ns * 100) if mean_ns > 0 else 0

    size_str = format_size(size)
    print(f"{name:<20} {size_str:>8}  {mean_ns:>12.1f} ns  "
          f"(+/- {cv:>5.1f}%)  {throughput:>12}")


def run_all_benchmarks(sizes):
    """Run all benchmark variants for all sizes."""
    print(f"binascii base64 benchmark")
    print(f"Python: {sys.version}")
    print(f"Target time per benchmark: {TARGET_TOTAL_TIME_S}s")
    print()
    print(f"{'Benchmark':<20} {'Size':>8}  {'Time/op':>15}  "
          f"{'Variance':>10}  {'Throughput':>12}")
    print("-" * 75)

    for size in sizes:
        # Generate test data
        binary_data = generate_test_data(size)
        base64_data = generate_base64_data(size)

        # Benchmark encode
        times, num_ops = calibrate_and_run(benchmark_encode, binary_data,
                                           TARGET_TOTAL_TIME_S)
        print_results("b2a_base64", size, times, num_ops, size)

        # Benchmark decode
        times, num_ops = calibrate_and_run(benchmark_decode, base64_data,
                                           TARGET_TOTAL_TIME_S)
        print_results("a2b_base64", size, times, num_ops, size)

        print()


def main():
    parser = argparse.ArgumentParser(
        description="Benchmark binascii base64 encoding and decoding",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "-s", "--sizes",
        type=str,
        default=None,
        help="Comma-separated list of sizes to test (e.g., '64,256,1024')"
    )

    args = parser.parse_args()

    if args.sizes:
        sizes = [int(s.strip()) for s in args.sizes.split(",")]
    else:
        sizes = DEFAULT_SIZES

    run_all_benchmarks(sizes)


if __name__ == "__main__":
    main()
