"""Simple profiler example."""

import math
import random

def fibonacci_recursive(n):
    if n <= 1:
        return n
    return fibonacci_recursive(n - 1) + fibonacci_recursive(n - 2)

def process_data(data):
    results = []
    for item in data:
        processed = math.sqrt(abs(item)) * math.sin(item / 100.0)
        results.append(processed)
    return results

def workload():
    numbers = [random.randint(1, 1000) for _ in range(100)]
    processed = process_data(numbers)

    fib_results = []
    for i in range(15, 20):
        fib_results.append(fibonacci_recursive(i))

    return sum(processed) + sum(fib_results)

def main():
    from profiling.sampling import SampleProfilerContextManager

    with SampleProfilerContextManager(
        interval_usec=1000,
        all_threads=False
    ) as profiler:
        workload()

        # Print stats to console
        print("=== Console Output ===")
        profiler.print_stats()

        # Export raw data (can be loaded later with pstats.Stats)
        print("\n=== Export Raw Data ===")
        profiler.export_stats("profile_data.prof")

        # Save formatted report to text file
        print("\n=== Dump Formatted Report ===")
        profiler.dump_to_file("profile_report.txt")

if __name__ == "__main__":
    main()
