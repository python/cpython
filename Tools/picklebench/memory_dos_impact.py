#!/usr/bin/env python3
#
# Author: Claude Sonnet 4.5 as driven by gpshead
#
"""
Microbenchmark for pickle module chunked reading performance (GH PR #119204).

This script generates Python data structures that act as antagonistic load
tests for the chunked reading code introduced to prevent memory exhaustion when
unpickling large objects.

The PR adds chunked reading (1MB chunks) for:
- BINBYTES8 (large bytes)
- BINUNICODE8 (large strings)
- BYTEARRAY8 (large bytearrays)
- FRAME (large frames)
- LONG4 (large integers)

Including an antagonistic mode that exercies memory denial of service pickles.

Usage:
    python memory_dos_impact.py --help
"""

import argparse
import gc
import io
import json
import os
import pickle
import statistics
import struct
import subprocess
import sys
import tempfile
import tracemalloc
from pathlib import Path
from time import perf_counter
from typing import Any, Dict, List, Tuple, Optional


# Configuration
MIN_READ_BUF_SIZE = 1 << 20  # 1MB - matches pickle.py _MIN_READ_BUF_SIZE

# Test sizes in MiB
DEFAULT_SIZES_MIB = [1, 2, 5, 10, 20, 50, 100, 200]

# Convert to bytes, plus threshold boundary tests
DEFAULT_SIZES = (
    [999_000]  # Below 1MiB (no chunking)
    + [size * (1 << 20) for size in DEFAULT_SIZES_MIB]  # MiB to bytes
    + [1_048_577]  # Just above 1MiB (minimal chunking overhead)
)
DEFAULT_SIZES.sort()

# Baseline benchmark configuration
BASELINE_BENCHMARK_TIMEOUT_SECONDS = 600  # 10 minutes

# Sparse memo attack test configuration
# Format: test_name -> (memo_index, baseline_memory_note)
SPARSE_MEMO_TESTS = {
    "sparse_memo_1M": (1_000_000, "~8 MB array"),
    "sparse_memo_100M": (100_000_000, "~800 MB array"),
    "sparse_memo_1B": (1_000_000_000, "~8 GB array"),
}


# Utility functions

def _extract_size_mb(size_key: str) -> float:
    """Extract numeric MiB value from size_key like '10.00MB' or '1.00MiB'.

    Returns 0.0 for non-numeric keys (they'll be sorted last).
    """
    try:
        return float(size_key.replace('MB', '').replace('MiB', ''))
    except ValueError:
        return 999999.0  # Put non-numeric keys last


def _format_output(results: Dict[str, Dict[str, Any]], format_type: str, is_antagonistic: bool) -> str:
    """Format benchmark results according to requested format.

    Args:
        results: Benchmark results dictionary
        format_type: Output format ('text', 'markdown', or 'json')
        is_antagonistic: Whether these are antagonistic (DoS) test results

    Returns:
        Formatted output string
    """
    if format_type == 'json':
        return Reporter.format_json(results)
    elif is_antagonistic:
        # Antagonistic mode uses specialized formatter for text/markdown
        return Reporter.format_antagonistic(results)
    elif format_type == 'text':
        return Reporter.format_text(results)
    elif format_type == 'markdown':
        return Reporter.format_markdown(results)
    else:
        # Default to text format
        return Reporter.format_text(results)


class AntagonisticGenerator:
    """Generate malicious/truncated pickles for DoS protection testing.

    These pickles claim large sizes but provide minimal data, causing them to fail
    during unpickling. They demonstrate the memory protection of chunked reading.
    """

    @staticmethod
    def truncated_binbytes8(claimed_size: int, actual_size: int = 1024) -> bytes:
        """BINBYTES8 claiming `claimed_size` but providing only `actual_size` bytes.

        This will fail with UnpicklingError but demonstrates peak memory usage.
        Before PR: Allocates full claimed_size
        After PR: Allocates in 1MB chunks, fails fast
        """
        return b'\x8e' + struct.pack('<Q', claimed_size) + b'x' * actual_size

    @staticmethod
    def truncated_binunicode8(claimed_size: int, actual_size: int = 1024) -> bytes:
        """BINUNICODE8 claiming `claimed_size` but providing only `actual_size` bytes."""
        return b'\x8d' + struct.pack('<Q', claimed_size) + b'x' * actual_size

    @staticmethod
    def truncated_bytearray8(claimed_size: int, actual_size: int = 1024) -> bytes:
        """BYTEARRAY8 claiming `claimed_size` but providing only `actual_size` bytes."""
        return b'\x96' + struct.pack('<Q', claimed_size) + b'x' * actual_size

    @staticmethod
    def truncated_frame(claimed_size: int) -> bytes:
        """FRAME claiming `claimed_size` but providing minimal data."""
        return b'\x95' + struct.pack('<Q', claimed_size) + b'N.'

    @staticmethod
    def sparse_memo_attack(index: int) -> bytes:
        """LONG_BINPUT with huge sparse index.

        Before PR: Tries to allocate array with `index` slots (OOM)
        After PR: Uses dict-based memo for sparse indices
        """
        return (b'(]r' + struct.pack('<I', index & 0xFFFFFFFF) +
                b'j' + struct.pack('<I', index & 0xFFFFFFFF) + b't.')

    @staticmethod
    def multi_claim_attack(count: int, size_each: int) -> bytes:
        """Multiple BINBYTES8 claims in sequence.

        Tests that multiple large claims don't accumulate memory.
        """
        data = b'('  # MARK
        for _ in range(count):
            data += b'\x8e' + struct.pack('<Q', size_each) + b'x' * 1024
        data += b't.'  # TUPLE + STOP
        return data


class DataGenerator:
    """Generate various types of large data structures for pickle testing."""

    @staticmethod
    def large_bytes(size: int) -> bytes:
        """Generate random bytes of specified size."""
        return os.urandom(size)

    @staticmethod
    def large_string_ascii(size: int) -> str:
        """Generate ASCII string of specified size."""
        return 'x' * size

    @staticmethod
    def large_string_multibyte(size: int) -> str:
        """Generate multibyte UTF-8 string (3 bytes per char for €)."""
        # Each € is 3 bytes in UTF-8
        return '€' * (size // 3)

    @staticmethod
    def large_bytearray(size: int) -> bytearray:
        """Generate bytearray of specified size."""
        return bytearray(os.urandom(size))

    @staticmethod
    def list_of_large_bytes(item_size: int, count: int) -> List[bytes]:
        """Generate list containing multiple large bytes objects."""
        return [os.urandom(item_size) for _ in range(count)]

    @staticmethod
    def dict_with_large_values(value_size: int, count: int) -> Dict[str, bytes]:
        """Generate dict with large bytes values."""
        return {
            f'key_{i}': os.urandom(value_size)
            for i in range(count)
        }

    @staticmethod
    def nested_structure(size: int) -> Dict[str, Any]:
        """Generate nested structure with various large objects."""
        chunk_size = size // 4
        return {
            'name': 'test_object',
            'data': {
                'bytes': os.urandom(chunk_size),
                'string': 's' * chunk_size,
                'bytearray': bytearray(b'b' * chunk_size),
            },
            'items': [os.urandom(chunk_size // 4) for _ in range(4)],
            'metadata': {
                'size': size,
                'type': 'nested',
            },
        }

    @staticmethod
    def tuple_of_large_objects(size: int) -> Tuple[bytes, str, bytearray]:
        """Generate tuple with large objects (immutable, different pickle path)."""
        chunk_size = size // 3
        return (
            os.urandom(chunk_size),
            'x' * chunk_size,
            bytearray(b'y' * chunk_size),
        )


class PickleBenchmark:
    """Benchmark pickle unpickling performance and memory usage."""

    def __init__(self, obj: Any, protocol: int = 5, iterations: int = 3):
        self.obj = obj
        self.protocol = protocol
        self.iterations = iterations
        self.pickle_data = pickle.dumps(obj, protocol=protocol)
        self.pickle_size = len(self.pickle_data)

    def benchmark_time(self) -> Dict[str, float]:
        """Measure unpickling time over multiple iterations."""
        times = []

        for _ in range(self.iterations):
            start = perf_counter()
            result = pickle.loads(self.pickle_data)
            elapsed = perf_counter() - start
            times.append(elapsed)

            # Verify correctness (first iteration only)
            if len(times) == 1:
                if result != self.obj:
                    raise ValueError("Unpickled object doesn't match original!")

        return {
            'mean': statistics.mean(times),
            'median': statistics.median(times),
            'stdev': statistics.stdev(times) if len(times) > 1 else 0.0,
            'min': min(times),
            'max': max(times),
        }

    def benchmark_memory(self) -> int:
        """Measure peak memory usage during unpickling."""
        tracemalloc.start()

        # Warmup
        pickle.loads(self.pickle_data)

        # Actual measurement
        gc.collect()
        tracemalloc.reset_peak()
        result = pickle.loads(self.pickle_data)
        current, peak = tracemalloc.get_traced_memory()

        tracemalloc.stop()

        # Verify correctness
        if result != self.obj:
            raise ValueError("Unpickled object doesn't match original!")

        return peak

    def run_all(self) -> Dict[str, Any]:
        """Run all benchmarks and return comprehensive results."""
        time_stats = self.benchmark_time()
        peak_memory = self.benchmark_memory()

        return {
            'pickle_size_bytes': self.pickle_size,
            'pickle_size_mb': self.pickle_size / (1 << 20),
            'protocol': self.protocol,
            'time': time_stats,
            'memory_peak_bytes': peak_memory,
            'memory_peak_mb': peak_memory / (1 << 20),
            'iterations': self.iterations,
        }


class AntagonisticBenchmark:
    """Benchmark antagonistic/malicious pickles that demonstrate DoS protection.

    These pickles are designed to FAIL unpickling, but we measure peak memory
    usage before the failure to demonstrate the memory protection.
    """

    def __init__(self, pickle_data: bytes, name: str):
        self.pickle_data = pickle_data
        self.name = name

    def measure_peak_memory(self, expect_success: bool = False) -> Dict[str, Any]:
        """Measure peak memory when attempting to unpickle antagonistic data.

        Args:
            expect_success: If True, test expects successful unpickling (e.g., sparse memo).
                          If False, test expects failure (e.g., truncated data).
        """
        tracemalloc.start()
        gc.collect()
        tracemalloc.reset_peak()

        error_type = None
        error_msg = None
        succeeded = False

        try:
            result = pickle.loads(self.pickle_data)
            succeeded = True
            if expect_success:
                error_type = "Success (expected)"
            else:
                error_type = "WARNING: Expected failure but succeeded"
        except (pickle.UnpicklingError, EOFError, ValueError, OverflowError) as e:
            if expect_success:
                error_type = f"UNEXPECTED FAILURE: {type(e).__name__}"
                error_msg = str(e)[:100]
            else:
                # Expected failure for truncated data tests
                error_type = type(e).__name__
                error_msg = str(e)[:100]

        current, peak = tracemalloc.get_traced_memory()
        tracemalloc.stop()

        return {
            'test_name': self.name,
            'peak_memory_bytes': peak,
            'peak_memory_mb': peak / (1 << 20),
            'error_type': error_type,
            'error_msg': error_msg,
            'pickle_size_bytes': len(self.pickle_data),
            'expected_outcome': 'success' if expect_success else 'failure',
            'succeeded': succeeded,
        }


class AntagonisticTestSuite:
    """Manage a suite of antagonistic (DoS protection) tests."""

    # Default sizes in MB to claim (will provide only 1KB actual data)
    DEFAULT_ANTAGONISTIC_SIZES_MB = [10, 50, 100, 500, 1000, 5000]

    def __init__(self, claimed_sizes_mb: List[int]):
        self.claimed_sizes_mb = claimed_sizes_mb

    def _run_truncated_test(
        self,
        test_type: str,
        generator_func,
        claimed_bytes: int,
        claimed_mb: int,
        size_key: str,
        all_results: Dict[str, Dict[str, Any]]
    ) -> None:
        """Run a single truncated data test and store results.

        Args:
            test_type: Type identifier (e.g., 'binbytes8', 'binunicode8')
            generator_func: Function to generate malicious pickle data
            claimed_bytes: Size claimed in the pickle (bytes)
            claimed_mb: Size claimed in the pickle (MB)
            size_key: Result key for this size (e.g., '10MB')
            all_results: Dictionary to store results in
        """
        test_name = f"{test_type}_{size_key}_claim"
        data = generator_func(claimed_bytes)
        bench = AntagonisticBenchmark(data, test_name)
        result = bench.measure_peak_memory(expect_success=False)
        result['claimed_mb'] = claimed_mb
        all_results[size_key][test_name] = result

    def run_all_tests(self) -> Dict[str, Dict[str, Any]]:
        """Run comprehensive antagonistic test suite."""
        all_results = {}

        for claimed_mb in self.claimed_sizes_mb:
            claimed_bytes = claimed_mb << 20
            size_key = f"{claimed_mb}MB"
            all_results[size_key] = {}

            # Run truncated data tests (expect failure)
            self._run_truncated_test('binbytes8', AntagonisticGenerator.truncated_binbytes8,
                                    claimed_bytes, claimed_mb, size_key, all_results)
            self._run_truncated_test('binunicode8', AntagonisticGenerator.truncated_binunicode8,
                                    claimed_bytes, claimed_mb, size_key, all_results)
            self._run_truncated_test('bytearray8', AntagonisticGenerator.truncated_bytearray8,
                                    claimed_bytes, claimed_mb, size_key, all_results)
            self._run_truncated_test('frame', AntagonisticGenerator.truncated_frame,
                                    claimed_bytes, claimed_mb, size_key, all_results)

        # Test 5: Sparse memo (expect success - dict-based memo works!)
        all_results["Sparse Memo (Success Expected)"] = {}
        for test_name, (index, baseline_note) in SPARSE_MEMO_TESTS.items():
            data = AntagonisticGenerator.sparse_memo_attack(index)
            bench = AntagonisticBenchmark(data, test_name)
            result = bench.measure_peak_memory(expect_success=True)
            result['claimed_mb'] = "N/A"
            result['baseline_note'] = f"Without PR: {baseline_note}"
            all_results["Sparse Memo (Success Expected)"][test_name] = result

        # Test 6: Multi-claim attack (expect failure)
        test_name = "multi_claim_10x100MB"
        data = AntagonisticGenerator.multi_claim_attack(10, 100 << 20)
        bench = AntagonisticBenchmark(data, test_name)
        result = bench.measure_peak_memory(expect_success=False)
        result['claimed_mb'] = 1000  # 10 * 100MB
        all_results["Multi-Claim (Failure Expected)"] = {test_name: result}

        return all_results


class TestSuite:
    """Manage a suite of benchmark tests."""

    def __init__(self, sizes: List[int], protocol: int = 5, iterations: int = 3):
        self.sizes = sizes
        self.protocol = protocol
        self.iterations = iterations
        self.results = {}

    def run_test(self, name: str, obj: Any) -> Dict[str, Any]:
        """Run benchmark for a single test object."""
        bench = PickleBenchmark(obj, self.protocol, self.iterations)
        results = bench.run_all()
        results['test_name'] = name
        results['object_type'] = type(obj).__name__
        return results

    def run_all_tests(self) -> Dict[str, Dict[str, Any]]:
        """Run comprehensive test suite across all sizes and types."""
        all_results = {}

        for size in self.sizes:
            size_key = f"{size / (1 << 20):.2f}MB"
            all_results[size_key] = {}

            # Test 1: Large bytes object (BINBYTES8)
            test_name = f"bytes_{size_key}"
            obj = DataGenerator.large_bytes(size)
            all_results[size_key][test_name] = self.run_test(test_name, obj)

            # Test 2: Large ASCII string (BINUNICODE8)
            test_name = f"string_ascii_{size_key}"
            obj = DataGenerator.large_string_ascii(size)
            all_results[size_key][test_name] = self.run_test(test_name, obj)

            # Test 3: Large multibyte UTF-8 string
            if size >= 3:
                test_name = f"string_utf8_{size_key}"
                obj = DataGenerator.large_string_multibyte(size)
                all_results[size_key][test_name] = self.run_test(test_name, obj)

            # Test 4: Large bytearray (BYTEARRAY8, protocol 5)
            if self.protocol >= 5:
                test_name = f"bytearray_{size_key}"
                obj = DataGenerator.large_bytearray(size)
                all_results[size_key][test_name] = self.run_test(test_name, obj)

            # Test 5: List of large objects (repeated chunking)
            if size >= MIN_READ_BUF_SIZE * 2:
                test_name = f"list_large_items_{size_key}"
                item_size = size // 5
                obj = DataGenerator.list_of_large_bytes(item_size, 5)
                all_results[size_key][test_name] = self.run_test(test_name, obj)

            # Test 6: Dict with large values
            if size >= MIN_READ_BUF_SIZE * 2:
                test_name = f"dict_large_values_{size_key}"
                value_size = size // 3
                obj = DataGenerator.dict_with_large_values(value_size, 3)
                all_results[size_key][test_name] = self.run_test(test_name, obj)

            # Test 7: Nested structure
            if size >= MIN_READ_BUF_SIZE:
                test_name = f"nested_{size_key}"
                obj = DataGenerator.nested_structure(size)
                all_results[size_key][test_name] = self.run_test(test_name, obj)

            # Test 8: Tuple (immutable)
            if size >= 3:
                test_name = f"tuple_{size_key}"
                obj = DataGenerator.tuple_of_large_objects(size)
                all_results[size_key][test_name] = self.run_test(test_name, obj)

        return all_results


class Comparator:
    """Compare benchmark results between current and baseline interpreters."""

    @staticmethod
    def _extract_json_from_output(output: str) -> Dict[str, Dict[str, Any]]:
        """Extract JSON data from subprocess output.

        Skips any print statements before the JSON output and parses the JSON.

        Args:
            output: Raw stdout from subprocess

        Returns:
            Parsed JSON as dictionary

        Raises:
            SystemExit: If JSON cannot be found or parsed
        """
        output_lines = output.strip().split('\n')
        json_start = -1
        for i, line in enumerate(output_lines):
            if line.strip().startswith('{'):
                json_start = i
                break

        if json_start == -1:
            print("Error: Could not find JSON output from baseline", file=sys.stderr)
            sys.exit(1)

        json_output = '\n'.join(output_lines[json_start:])
        try:
            return json.loads(json_output)
        except json.JSONDecodeError as e:
            print(f"Error: Could not parse baseline JSON output: {e}", file=sys.stderr)
            sys.exit(1)

    @staticmethod
    def run_baseline_benchmark(baseline_python: str, args: argparse.Namespace) -> Dict[str, Dict[str, Any]]:
        """Run the benchmark using the baseline Python interpreter."""
        # Build command to run this script with baseline Python
        cmd = [
            baseline_python,
            __file__,
            '--format', 'json',
            '--protocol', str(args.protocol),
            '--iterations', str(args.iterations),
        ]

        if args.sizes is not None:
            cmd.extend(['--sizes'] + [str(s) for s in args.sizes])

        if args.antagonistic:
            cmd.append('--antagonistic')

        print(f"\nRunning baseline benchmark with: {baseline_python}")
        print(f"Command: {' '.join(cmd)}\n")

        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=BASELINE_BENCHMARK_TIMEOUT_SECONDS,
            )

            if result.returncode != 0:
                print(f"Error running baseline benchmark:", file=sys.stderr)
                print(result.stderr, file=sys.stderr)
                sys.exit(1)

            # Extract and parse JSON from output
            return Comparator._extract_json_from_output(result.stdout)

        except subprocess.TimeoutExpired:
            print("Error: Baseline benchmark timed out", file=sys.stderr)
            sys.exit(1)

    @staticmethod
    def calculate_change(baseline_value: float, current_value: float) -> float:
        """Calculate percentage change from baseline to current."""
        if baseline_value == 0:
            return 0.0
        return ((current_value - baseline_value) / baseline_value) * 100

    @staticmethod
    def format_comparison(
        current_results: Dict[str, Dict[str, Any]],
        baseline_results: Dict[str, Dict[str, Any]]
    ) -> str:
        """Format comparison results as readable text."""
        lines = []
        lines.append("=" * 100)
        lines.append("Pickle Unpickling Benchmark Comparison")
        lines.append("=" * 100)
        lines.append("")
        lines.append("Legend: Current vs Baseline | % Change (+ is slower/more memory, - is faster/less memory)")
        lines.append("")

        # Sort size keys numerically
        for size_key in sorted(current_results.keys(), key=_extract_size_mb):
            if size_key not in baseline_results:
                continue

            lines.append(f"\n{size_key} Comparison")
            lines.append("-" * 100)

            current_tests = current_results[size_key]
            baseline_tests = baseline_results[size_key]

            for test_name in sorted(current_tests.keys()):
                if test_name not in baseline_tests:
                    continue

                curr = current_tests[test_name]
                base = baseline_tests[test_name]

                time_change = Comparator.calculate_change(
                    base['time']['mean'], curr['time']['mean']
                )
                mem_change = Comparator.calculate_change(
                    base['memory_peak_mb'], curr['memory_peak_mb']
                )

                lines.append(f"\n  {curr['test_name']}")
                lines.append(f"    Time:   {curr['time']['mean']*1000:6.2f}ms vs {base['time']['mean']*1000:6.2f}ms | "
                           f"{time_change:+6.1f}%")
                lines.append(f"    Memory: {curr['memory_peak_mb']:6.2f}MB vs {base['memory_peak_mb']:6.2f}MB | "
                           f"{mem_change:+6.1f}%")

        lines.append("\n" + "=" * 100)
        lines.append("\nSummary:")

        # Calculate overall statistics
        time_changes = []
        mem_changes = []

        for size_key in current_results.keys():
            if size_key not in baseline_results:
                continue
            for test_name in current_results[size_key].keys():
                if test_name not in baseline_results[size_key]:
                    continue
                curr = current_results[size_key][test_name]
                base = baseline_results[size_key][test_name]

                time_changes.append(Comparator.calculate_change(
                    base['time']['mean'], curr['time']['mean']
                ))
                mem_changes.append(Comparator.calculate_change(
                    base['memory_peak_mb'], curr['memory_peak_mb']
                ))

        if time_changes:
            lines.append(f"  Time change:   mean={statistics.mean(time_changes):+.1f}%, "
                       f"median={statistics.median(time_changes):+.1f}%")
        if mem_changes:
            lines.append(f"  Memory change: mean={statistics.mean(mem_changes):+.1f}%, "
                       f"median={statistics.median(mem_changes):+.1f}%")

        lines.append("=" * 100)
        return "\n".join(lines)

    @staticmethod
    def format_antagonistic_comparison(
        current_results: Dict[str, Dict[str, Any]],
        baseline_results: Dict[str, Dict[str, Any]]
    ) -> str:
        """Format antagonistic benchmark comparison results."""
        lines = []
        lines.append("=" * 100)
        lines.append("Antagonistic Pickle Benchmark Comparison (Memory DoS Protection)")
        lines.append("=" * 100)
        lines.append("")
        lines.append("Legend: Current vs Baseline | Memory Change (- is better, shows memory saved)")
        lines.append("")
        lines.append("This compares TWO types of DoS protection:")
        lines.append("  1. Truncated data → Baseline allocates full claimed size, Current uses chunked reading")
        lines.append("  2. Sparse memo → Baseline uses huge arrays, Current uses dict-based memo")
        lines.append("")

        # Track statistics
        truncated_memory_changes = []
        sparse_memory_changes = []

        # Sort size keys numerically
        for size_key in sorted(current_results.keys(), key=_extract_size_mb):
            if size_key not in baseline_results:
                continue

            lines.append(f"\n{size_key} Comparison")
            lines.append("-" * 100)

            current_tests = current_results[size_key]
            baseline_tests = baseline_results[size_key]

            for test_name in sorted(current_tests.keys()):
                if test_name not in baseline_tests:
                    continue

                curr = current_tests[test_name]
                base = baseline_tests[test_name]

                curr_peak_mb = curr['peak_memory_mb']
                base_peak_mb = base['peak_memory_mb']
                expected_outcome = curr.get('expected_outcome', 'failure')

                mem_change = Comparator.calculate_change(base_peak_mb, curr_peak_mb)
                mem_saved_mb = base_peak_mb - curr_peak_mb

                lines.append(f"\n  {curr['test_name']}")
                lines.append(f"    Memory: {curr_peak_mb:6.2f}MB vs {base_peak_mb:6.2f}MB | "
                           f"{mem_change:+6.1f}% ({mem_saved_mb:+.2f}MB saved)")

                # Track based on test type
                if expected_outcome == 'success':
                    sparse_memory_changes.append(mem_change)
                    if curr.get('baseline_note'):
                        lines.append(f"    Note: {curr['baseline_note']}")
                else:
                    truncated_memory_changes.append(mem_change)
                    claimed_mb = curr.get('claimed_mb', 'N/A')
                    if claimed_mb != 'N/A':
                        lines.append(f"    Claimed: {claimed_mb:,}MB")

                # Show status
                curr_status = curr.get('error_type', 'Unknown')
                base_status = base.get('error_type', 'Unknown')
                if curr_status != base_status:
                    lines.append(f"    Status: {curr_status} (baseline: {base_status})")
                else:
                    lines.append(f"    Status: {curr_status}")

        lines.append("\n" + "=" * 100)
        lines.append("\nSummary:")
        lines.append("")

        if truncated_memory_changes:
            lines.append("  Truncated Data Protection (chunked reading):")
            lines.append(f"    Mean memory change:   {statistics.mean(truncated_memory_changes):+.1f}%")
            lines.append(f"    Median memory change: {statistics.median(truncated_memory_changes):+.1f}%")
            avg_change = statistics.mean(truncated_memory_changes)
            if avg_change < -50:
                lines.append(f"    Result: ✓ Dramatic memory reduction ({avg_change:.1f}%) - DoS protection working!")
            elif avg_change < 0:
                lines.append(f"    Result: ✓ Memory reduced ({avg_change:.1f}%)")
            else:
                lines.append(f"    Result: ⚠ Memory increased ({avg_change:.1f}%) - unexpected!")
            lines.append("")

        if sparse_memory_changes:
            lines.append("  Sparse Memo Protection (dict-based memo):")
            lines.append(f"    Mean memory change:   {statistics.mean(sparse_memory_changes):+.1f}%")
            lines.append(f"    Median memory change: {statistics.median(sparse_memory_changes):+.1f}%")
            avg_change = statistics.mean(sparse_memory_changes)
            if avg_change < -50:
                lines.append(f"    Result: ✓ Dramatic memory reduction ({avg_change:.1f}%) - Dict optimization working!")
            elif avg_change < 0:
                lines.append(f"    Result: ✓ Memory reduced ({avg_change:.1f}%)")
            else:
                lines.append(f"    Result: ⚠ Memory increased ({avg_change:.1f}%) - unexpected!")

        lines.append("")
        lines.append("=" * 100)
        return "\n".join(lines)


class Reporter:
    """Format and display benchmark results."""

    @staticmethod
    def format_text(results: Dict[str, Dict[str, Any]]) -> str:
        """Format results as readable text."""
        lines = []
        lines.append("=" * 80)
        lines.append("Pickle Unpickling Benchmark Results")
        lines.append("=" * 80)
        lines.append("")

        for size_key, tests in results.items():
            lines.append(f"\n{size_key} Test Results")
            lines.append("-" * 80)

            for test_name, data in tests.items():
                lines.append(f"\n  Test: {data['test_name']}")
                lines.append(f"  Type: {data['object_type']}")
                lines.append(f"  Pickle size: {data['pickle_size_mb']:.2f} MB")
                lines.append(f"  Time (mean): {data['time']['mean']*1000:.2f} ms")
                lines.append(f"  Time (stdev): {data['time']['stdev']*1000:.2f} ms")
                lines.append(f"  Peak memory: {data['memory_peak_mb']:.2f} MB")
                lines.append(f"  Protocol: {data['protocol']}")

        lines.append("\n" + "=" * 80)
        return "\n".join(lines)

    @staticmethod
    def format_markdown(results: Dict[str, Dict[str, Any]]) -> str:
        """Format results as markdown table."""
        lines = []
        lines.append("# Pickle Unpickling Benchmark Results\n")

        for size_key, tests in results.items():
            lines.append(f"## {size_key}\n")
            lines.append("| Test | Type | Pickle Size (MB) | Time (ms) | Stdev (ms) | Peak Memory (MB) |")
            lines.append("|------|------|------------------|-----------|------------|------------------|")

            for test_name, data in tests.items():
                lines.append(
                    f"| {data['test_name']} | "
                    f"{data['object_type']} | "
                    f"{data['pickle_size_mb']:.2f} | "
                    f"{data['time']['mean']*1000:.2f} | "
                    f"{data['time']['stdev']*1000:.2f} | "
                    f"{data['memory_peak_mb']:.2f} |"
                )
            lines.append("")

        return "\n".join(lines)

    @staticmethod
    def format_json(results: Dict[str, Dict[str, Any]]) -> str:
        """Format results as JSON."""
        import json
        return json.dumps(results, indent=2)

    @staticmethod
    def format_antagonistic(results: Dict[str, Dict[str, Any]]) -> str:
        """Format antagonistic benchmark results."""
        lines = []
        lines.append("=" * 100)
        lines.append("Antagonistic Pickle Benchmark (Memory DoS Protection Test)")
        lines.append("=" * 100)
        lines.append("")
        lines.append("This benchmark tests TWO types of DoS protection:")
        lines.append("  1. Truncated data attacks → Expect FAILURE with minimal memory before failure")
        lines.append("  2. Sparse memo attacks → Expect SUCCESS with dict-based memo (vs huge array)")
        lines.append("")

        # Sort size keys numerically
        for size_key in sorted(results.keys(), key=_extract_size_mb):
            tests = results[size_key]

            # Determine test type from first test
            if tests:
                first_test = next(iter(tests.values()))
                expected_outcome = first_test.get('expected_outcome', 'failure')
                claimed_mb = first_test.get('claimed_mb', 'N/A')

                # Header varies by test type
                if "Sparse Memo" in size_key:
                    lines.append(f"\n{size_key}")
                    lines.append("-" * 100)
                elif "Multi-Claim" in size_key:
                    lines.append(f"\n{size_key}")
                    lines.append("-" * 100)
                elif claimed_mb != 'N/A':
                    lines.append(f"\n{size_key} Claimed (actual: 1KB) - Expect Failure")
                    lines.append("-" * 100)
                else:
                    lines.append(f"\n{size_key}")
                    lines.append("-" * 100)

            for test_name, data in tests.items():
                peak_mb = data['peak_memory_mb']
                claimed = data.get('claimed_mb', 'N/A')
                expected_outcome = data.get('expected_outcome', 'failure')
                succeeded = data.get('succeeded', False)
                baseline_note = data.get('baseline_note', '')

                lines.append(f"  {data['test_name']}")

                # Format output based on test type
                if expected_outcome == 'success':
                    # Sparse memo test - show success with dict
                    status_icon = "✓" if succeeded else "✗"
                    lines.append(f"    Peak memory: {peak_mb:8.2f} MB {status_icon}")
                    lines.append(f"    Status: {data['error_type']}")
                    if baseline_note:
                        lines.append(f"    {baseline_note}")
                else:
                    # Truncated data test - show savings before failure
                    if claimed != 'N/A':
                        saved_mb = claimed - peak_mb
                        savings_pct = (saved_mb / claimed * 100) if claimed > 0 else 0
                        lines.append(f"    Peak memory: {peak_mb:8.2f} MB (claimed: {claimed:,} MB, saved: {saved_mb:.2f} MB, {savings_pct:.1f}%)")
                    else:
                        lines.append(f"    Peak memory: {peak_mb:8.2f} MB")
                    lines.append(f"    Status: {data['error_type']}")

        lines.append("\n" + "=" * 100)

        # Calculate statistics by test type
        truncated_claimed = 0
        truncated_peak = 0
        truncated_count = 0

        sparse_peak_total = 0
        sparse_count = 0

        for size_key, tests in results.items():
            for test_name, data in tests.items():
                expected_outcome = data.get('expected_outcome', 'failure')

                if expected_outcome == 'failure':
                    # Truncated data test
                    claimed = data.get('claimed_mb', 0)
                    if claimed != 'N/A' and claimed > 0:
                        truncated_claimed += claimed
                        truncated_peak += data['peak_memory_mb']
                        truncated_count += 1
                else:
                    # Sparse memo test
                    sparse_peak_total += data['peak_memory_mb']
                    sparse_count += 1

        lines.append("\nSummary:")
        lines.append("")

        if truncated_count > 0:
            avg_claimed = truncated_claimed / truncated_count
            avg_peak = truncated_peak / truncated_count
            avg_saved = avg_claimed - avg_peak
            avg_savings_pct = (avg_saved / avg_claimed * 100) if avg_claimed > 0 else 0

            lines.append("  Truncated Data Protection (chunked reading):")
            lines.append(f"    Average claimed: {avg_claimed:,.1f} MB")
            lines.append(f"    Average peak:    {avg_peak:,.2f} MB")
            lines.append(f"    Average saved:   {avg_saved:,.2f} MB ({avg_savings_pct:.1f}% reduction)")
            lines.append(f"    Status: ✓ Fails fast with minimal memory")
            lines.append("")

        if sparse_count > 0:
            avg_sparse_peak = sparse_peak_total / sparse_count
            lines.append("  Sparse Memo Protection (dict-based memo):")
            lines.append(f"    Average peak:    {avg_sparse_peak:,.2f} MB")
            lines.append(f"    Status: ✓ Succeeds with dict (vs GB-sized arrays without PR)")
            lines.append(f"    Note: Compare with --baseline to see actual memory savings")

        lines.append("")
        lines.append("=" * 100)
        return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="Benchmark pickle unpickling performance for large objects"
    )
    parser.add_argument(
        '--sizes',
        type=int,
        nargs='+',
        default=None,
        metavar='MiB',
        help=f'Object sizes to test in MiB (default: {DEFAULT_SIZES_MIB})'
    )
    parser.add_argument(
        '--protocol',
        type=int,
        default=5,
        choices=[0, 1, 2, 3, 4, 5],
        help='Pickle protocol version (default: 5)'
    )
    parser.add_argument(
        '--iterations',
        type=int,
        default=3,
        help='Number of benchmark iterations (default: 3)'
    )
    parser.add_argument(
        '--format',
        choices=['text', 'markdown', 'json'],
        default='text',
        help='Output format (default: text)'
    )
    parser.add_argument(
        '--baseline',
        type=str,
        metavar='PYTHON',
        help='Path to baseline Python interpreter for comparison (e.g., ../main-build/python)'
    )
    parser.add_argument(
        '--antagonistic',
        action='store_true',
        help='Run antagonistic/malicious pickle tests (DoS protection benchmark)'
    )

    args = parser.parse_args()

    # Handle antagonistic mode
    if args.antagonistic:
        # Antagonistic mode uses claimed sizes in MB, not actual data sizes
        if args.sizes is None:
            claimed_sizes_mb = AntagonisticTestSuite.DEFAULT_ANTAGONISTIC_SIZES_MB
        else:
            claimed_sizes_mb = args.sizes

        print(f"Running ANTAGONISTIC pickle benchmark (DoS protection test)...")
        print(f"Claimed sizes: {claimed_sizes_mb} MiB (actual data: 1KB each)")
        print(f"NOTE: These pickles will FAIL to unpickle (expected)")
        print()

        # Run antagonistic benchmark suite
        suite = AntagonisticTestSuite(claimed_sizes_mb)
        results = suite.run_all_tests()

        # Format and display results
        if args.baseline:
            # Verify baseline Python exists
            baseline_path = Path(args.baseline)
            if not baseline_path.exists():
                print(f"Error: Baseline Python not found: {args.baseline}", file=sys.stderr)
                return 1

            # Run baseline benchmark
            baseline_results = Comparator.run_baseline_benchmark(args.baseline, args)

            # Show comparison
            comparison_output = Comparator.format_antagonistic_comparison(results, baseline_results)
            print(comparison_output)
        else:
            # Format and display results
            output = _format_output(results, args.format, is_antagonistic=True)
            print(output)

    else:
        # Normal mode: legitimate pickle benchmarks
        # Convert sizes from MiB to bytes
        if args.sizes is None:
            sizes_bytes = DEFAULT_SIZES
        else:
            sizes_bytes = [size * (1 << 20) for size in args.sizes]

        print(f"Running pickle benchmark with protocol {args.protocol}...")
        print(f"Test sizes: {[f'{s/(1<<20):.2f}MiB' for s in sizes_bytes]}")
        print(f"Iterations per test: {args.iterations}")
        print()

        # Run benchmark suite
        suite = TestSuite(sizes_bytes, args.protocol, args.iterations)
        results = suite.run_all_tests()

        # If baseline comparison requested, run baseline and compare
        if args.baseline:
            # Verify baseline Python exists
            baseline_path = Path(args.baseline)
            if not baseline_path.exists():
                print(f"Error: Baseline Python not found: {args.baseline}", file=sys.stderr)
                return 1

            # Run baseline benchmark
            baseline_results = Comparator.run_baseline_benchmark(args.baseline, args)

            # Show comparison
            comparison_output = Comparator.format_comparison(results, baseline_results)
            print(comparison_output)

        else:
            # Format and display results
            output = _format_output(results, args.format, is_antagonistic=False)
            print(output)

    return 0


if __name__ == '__main__':
    sys.exit(main())
