"""Tests for the sampling profiler (profile.sample)."""

import contextlib
import io
import marshal
import os
import subprocess
import sys
import tempfile
import time
import unittest
from unittest import mock

from profile.pstats_collector import PstatsCollector
from profile.stack_collectors import (
    CollapsedStackCollector,
)

from test.support.os_helper import unlink

PROCESS_VM_READV_SUPPORTED = False

try:
    from _remote_debugging import PROCESS_VM_READV_SUPPORTED
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when _remote_debugging is available"
    )
else:
    import profile.sample


class MockFrameInfo:
    """Mock FrameInfo for testing since the real one isn't accessible."""

    def __init__(self, filename, lineno, funcname):
        self.filename = filename
        self.lineno = lineno
        self.funcname = funcname

    def __repr__(self):
        return f"MockFrameInfo(filename='{self.filename}', lineno={self.lineno}, funcname='{self.funcname}')"


@contextlib.contextmanager
def test_subprocess(script, startup_delay=0.1):
    proc = subprocess.Popen(
        [sys.executable, "-c", script],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    try:
        if startup_delay > 0:
            time.sleep(startup_delay)
        yield proc
    finally:
        if proc.poll() is None:
            proc.kill()
        proc.wait()


def close_and_unlink(file):
    file.close()
    unlink(file.name)


class TestSampleProfilerComponents(unittest.TestCase):
    """Unit tests for individual profiler components."""

    def test_pstats_collector_basic(self):
        """Test basic PstatsCollector functionality."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Test empty state
        self.assertEqual(len(collector.result), 0)
        self.assertEqual(len(collector.stats), 0)

        # Test collecting sample data
        test_frames = [
            (
                1,
                [
                    MockFrameInfo("file.py", 10, "func1"),
                    MockFrameInfo("file.py", 20, "func2"),
                ],
            )
        ]
        collector.collect(test_frames)

        # Should have recorded calls for both functions
        self.assertEqual(len(collector.result), 2)
        self.assertIn(("file.py", 10, "func1"), collector.result)
        self.assertIn(("file.py", 20, "func2"), collector.result)

        # Top-level function should have inline call
        self.assertEqual(
            collector.result[("file.py", 10, "func1")]["inline_calls"], 1
        )
        self.assertEqual(
            collector.result[("file.py", 10, "func1")]["total_calls"], 1
        )

        # Calling function should have total call
        self.assertEqual(
            collector.result[("file.py", 20, "func2")]["total_calls"], 1
        )

    def test_pstats_collector_create_stats(self):
        """Test PstatsCollector stats creation."""
        collector = PstatsCollector(
            sample_interval_usec=1000000
        )  # 1 second intervals

        test_frames = [
            (
                1,
                [
                    MockFrameInfo("file.py", 10, "func1"),
                    MockFrameInfo("file.py", 20, "func2"),
                ],
            )
        ]
        collector.collect(test_frames)
        collector.collect(test_frames)  # Collect twice

        collector.create_stats()

        # Check stats format: (cc, nc, tt, ct, callers)
        func1_stats = collector.stats[("file.py", 10, "func1")]
        self.assertEqual(func1_stats[0], 2)  # total_calls
        self.assertEqual(func1_stats[1], 2)  # nc (non-recursive calls)
        self.assertEqual(
            func1_stats[2], 2.0
        )  # tt (total time - 2 samples * 1 sec)
        self.assertEqual(func1_stats[3], 2.0)  # ct (cumulative time)

        func2_stats = collector.stats[("file.py", 20, "func2")]
        self.assertEqual(func2_stats[0], 2)  # total_calls
        self.assertEqual(func2_stats[2], 0.0)  # tt (no inline calls)
        self.assertEqual(func2_stats[3], 2.0)  # ct (cumulative time)

    def test_collapsed_stack_collector_basic(self):
        collector = CollapsedStackCollector()

        # Test empty state
        self.assertEqual(len(collector.call_trees), 0)
        self.assertEqual(len(collector.function_samples), 0)

        # Test collecting sample data
        test_frames = [
            (1, [("file.py", 10, "func1"), ("file.py", 20, "func2")])
        ]
        collector.collect(test_frames)

        # Should store call tree (reversed)
        self.assertEqual(len(collector.call_trees), 1)
        expected_tree = [("file.py", 20, "func2"), ("file.py", 10, "func1")]
        self.assertEqual(collector.call_trees[0], expected_tree)

        # Should count function samples
        self.assertEqual(
            collector.function_samples[("file.py", 10, "func1")], 1
        )
        self.assertEqual(
            collector.function_samples[("file.py", 20, "func2")], 1
        )

    def test_collapsed_stack_collector_export(self):
        collapsed_out = tempfile.NamedTemporaryFile(delete=False)
        self.addCleanup(close_and_unlink, collapsed_out)

        collector = CollapsedStackCollector()

        test_frames1 = [
            (1, [("file.py", 10, "func1"), ("file.py", 20, "func2")])
        ]
        test_frames2 = [
            (1, [("file.py", 10, "func1"), ("file.py", 20, "func2")])
        ]  # Same stack
        test_frames3 = [(1, [("other.py", 5, "other_func")])]

        collector.collect(test_frames1)
        collector.collect(test_frames2)
        collector.collect(test_frames3)

        collector.export(collapsed_out.name)
        # Check file contents
        with open(collapsed_out.name, "r") as f:
            content = f.read()

        lines = content.strip().split("\n")
        self.assertEqual(len(lines), 2)  # Two unique stacks

        # Check collapsed format: file:func:line;file:func:line count
        stack1_expected = "file.py:func2:20;file.py:func1:10 2"
        stack2_expected = "other.py:other_func:5 1"

        self.assertIn(stack1_expected, lines)
        self.assertIn(stack2_expected, lines)

    def test_pstats_collector_export(self):
        collector = PstatsCollector(
            sample_interval_usec=1000000
        )  # 1 second intervals

        test_frames1 = [
            (
                1,
                [
                    MockFrameInfo("file.py", 10, "func1"),
                    MockFrameInfo("file.py", 20, "func2"),
                ],
            )
        ]
        test_frames2 = [
            (
                1,
                [
                    MockFrameInfo("file.py", 10, "func1"),
                    MockFrameInfo("file.py", 20, "func2"),
                ],
            )
        ]  # Same stack
        test_frames3 = [(1, [MockFrameInfo("other.py", 5, "other_func")])]

        collector.collect(test_frames1)
        collector.collect(test_frames2)
        collector.collect(test_frames3)

        pstats_out = tempfile.NamedTemporaryFile(
            suffix=".pstats", delete=False
        )
        self.addCleanup(close_and_unlink, pstats_out)
        collector.export(pstats_out.name)

        # Check file can be loaded with marshal
        with open(pstats_out.name, "rb") as f:
            stats_data = marshal.load(f)

        # Should be a dictionary with the sampled marker
        self.assertIsInstance(stats_data, dict)
        self.assertIn(("__sampled__",), stats_data)
        self.assertTrue(stats_data[("__sampled__",)])

        # Should have function data
        function_entries = [
            k for k in stats_data.keys() if k != ("__sampled__",)
        ]
        self.assertGreater(len(function_entries), 0)

        # Check specific function stats format: (cc, nc, tt, ct, callers)
        func1_key = ("file.py", 10, "func1")
        func2_key = ("file.py", 20, "func2")
        other_key = ("other.py", 5, "other_func")

        self.assertIn(func1_key, stats_data)
        self.assertIn(func2_key, stats_data)
        self.assertIn(other_key, stats_data)

        # Check func1 stats (should have 2 samples)
        func1_stats = stats_data[func1_key]
        self.assertEqual(func1_stats[0], 2)  # total_calls
        self.assertEqual(func1_stats[1], 2)  # nc (non-recursive calls)
        self.assertEqual(func1_stats[2], 2.0)  # tt (total time)
        self.assertEqual(func1_stats[3], 2.0)  # ct (cumulative time)


@unittest.skipIf(
    sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
    "Test only runs on Linux with process_vm_readv support",
)
class TestSampleProfilerIntegration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.test_script = '''
import time
import os

def slow_fibonacci(n):
    """Recursive fibonacci - should show up prominently in profiler."""
    if n <= 1:
        return n
    return slow_fibonacci(n-1) + slow_fibonacci(n-2)

def cpu_intensive_work():
    """CPU intensive work that should show in profiler."""
    result = 0
    for i in range(10000):
        result += i * i
        if i % 100 == 0:
            result = result % 1000000
    return result

def medium_computation():
    """Medium complexity function."""
    result = 0
    for i in range(100):
        result += i * i
    return result

def fast_loop():
    """Fast simple loop."""
    total = 0
    for i in range(50):
        total += i
    return total

def nested_calls():
    """Test nested function calls."""
    def level1():
        def level2():
            return medium_computation()
        return level2()
    return level1()

def main_loop():
    """Main test loop with different execution paths."""
    iteration = 0

    while True:
        iteration += 1

        # Different execution paths - focus on CPU intensive work
        if iteration % 3 == 0:
            # Very CPU intensive
            result = cpu_intensive_work()
        elif iteration % 5 == 0:
            # Expensive recursive operation
            result = slow_fibonacci(12)
        else:
            # Medium operation
            result = nested_calls()

        # No sleep - keep CPU busy

if __name__ == "__main__":
    main_loop()
'''

    def test_sampling_basic_functionality(self):
        with (
            test_subprocess(self.test_script) as proc,
            io.StringIO() as captured_output,
            mock.patch("sys.stdout", captured_output),
        ):
            try:
                profile.sample.sample(
                    proc.pid,
                    duration_sec=2,
                    sample_interval_usec=1000,  # 1ms
                    show_summary=False,
                )
            except PermissionError:
                self.skipTest("Insufficient permissions for remote profiling")

            output = captured_output.getvalue()

        # Basic checks on output
        self.assertIn("Captured", output)
        self.assertIn("samples", output)
        self.assertIn("Profile Stats", output)

        # Should see some of our test functions
        self.assertIn("slow_fibonacci", output)

    def test_sampling_with_pstats_export(self):
        pstats_out = tempfile.NamedTemporaryFile(
            suffix=".pstats", delete=False
        )
        self.addCleanup(close_and_unlink, pstats_out)

        with test_subprocess(self.test_script) as proc:
            # Suppress profiler output when testing file export
            with (
                io.StringIO() as captured_output,
                mock.patch("sys.stdout", captured_output),
            ):
                try:
                    profile.sample.sample(
                        proc.pid,
                        duration_sec=1,
                        filename=pstats_out.name,
                        sample_interval_usec=10000,
                    )
                except PermissionError:
                    self.skipTest(
                        "Insufficient permissions for remote profiling"
                    )

            # Verify file was created and contains valid data
            self.assertTrue(os.path.exists(pstats_out.name))
            self.assertGreater(os.path.getsize(pstats_out.name), 0)

            # Try to load the stats file
            with open(pstats_out.name, "rb") as f:
                stats_data = marshal.load(f)

            # Should be a dictionary with the sampled marker
            self.assertIsInstance(stats_data, dict)
            self.assertIn(("__sampled__",), stats_data)
            self.assertTrue(stats_data[("__sampled__",)])

            # Should have some function data
            function_entries = [
                k for k in stats_data.keys() if k != ("__sampled__",)
            ]
            self.assertGreater(len(function_entries), 0)

    def test_sampling_with_collapsed_export(self):
        collapsed_file = tempfile.NamedTemporaryFile(
            suffix=".txt", delete=False
        )
        self.addCleanup(close_and_unlink, collapsed_file)

        with (
            test_subprocess(self.test_script) as proc,
        ):
            # Suppress profiler output when testing file export
            with (
                io.StringIO() as captured_output,
                mock.patch("sys.stdout", captured_output),
            ):
                try:
                    profile.sample.sample(
                        proc.pid,
                        duration_sec=1,
                        filename=collapsed_file.name,
                        output_format="collapsed",
                        sample_interval_usec=10000,
                    )
                except PermissionError:
                    self.skipTest(
                        "Insufficient permissions for remote profiling"
                    )

            # Verify file was created and contains valid data
            self.assertTrue(os.path.exists(collapsed_file.name))
            self.assertGreater(os.path.getsize(collapsed_file.name), 0)

            # Check file format
            with open(collapsed_file.name, "r") as f:
                content = f.read()

            lines = content.strip().split("\n")
            self.assertGreater(len(lines), 0)

            # Each line should have format: stack_trace count
            for line in lines:
                parts = line.rsplit(" ", 1)
                self.assertEqual(len(parts), 2)

                stack_trace, count_str = parts
                self.assertGreater(len(stack_trace), 0)
                self.assertTrue(count_str.isdigit())
                self.assertGreater(int(count_str), 0)

                # Stack trace should contain semicolon-separated entries
                if ";" in stack_trace:
                    stack_parts = stack_trace.split(";")
                    for part in stack_parts:
                        # Each part should be file:function:line
                        self.assertIn(":", part)

    def test_sampling_all_threads(self):
        with (
            test_subprocess(self.test_script) as proc,
            # Suppress profiler output
            io.StringIO() as captured_output,
            mock.patch("sys.stdout", captured_output),
        ):
            try:
                profile.sample.sample(
                    proc.pid,
                    duration_sec=1,
                    all_threads=True,
                    sample_interval_usec=10000,
                    show_summary=False,
                )
            except PermissionError:
                self.skipTest("Insufficient permissions for remote profiling")

        # Just verify that sampling completed without error
        # We're not testing output format here


@unittest.skipIf(
    sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
    "Test only runs on Linux with process_vm_readv support",
)
class TestSampleProfilerErrorHandling(unittest.TestCase):
    def test_invalid_pid(self):
        with self.assertRaises((OSError, RuntimeError)):
            profile.sample.sample(-1, duration_sec=1)

    def test_process_dies_during_sampling(self):
        with test_subprocess("import time; time.sleep(0.5); exit()") as proc:
            with (
                io.StringIO() as captured_output,
                mock.patch("sys.stdout", captured_output),
            ):
                try:
                    profile.sample.sample(
                        proc.pid,
                        duration_sec=2,  # Longer than process lifetime
                        sample_interval_usec=50000,
                    )
                except PermissionError:
                    self.skipTest(
                        "Insufficient permissions for remote profiling"
                    )

                output = captured_output.getvalue()

            self.assertIn("Error rate", output)

    def test_invalid_output_format(self):
        with self.assertRaises(ValueError):
            profile.sample.sample(
                os.getpid(),
                duration_sec=1,
                output_format="invalid_format",
            )


class TestSampleProfilerCLI(unittest.TestCase):
    def test_argument_parsing_basic(self):
        test_args = ["profile.sample", "12345"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profile.sample.sample") as mock_sample,
        ):
            profile.sample.main()

            mock_sample.assert_called_once_with(
                12345,
                sample_interval_usec=100,
                duration_sec=10,
                filename=None,
                all_threads=False,
                limit=15,
                sort=None,
                show_summary=True,
                output_format="pstats",
            )

    def test_sort_options(self):
        sort_options = [
            ("--sort-calls", 0),
            ("--sort-time", 1),
            ("--sort-cumulative", 2),
            ("--sort-percall", 3),
            ("--sort-cumpercall", 4),
            ("--sort-name", 5),
        ]

        for option, expected_sort_value in sort_options:
            test_args = ["profile.sample", option, "12345"]

            with (
                mock.patch("sys.argv", test_args),
                mock.patch("profile.sample.sample") as mock_sample,
            ):
                profile.sample.main()

                mock_sample.assert_called_once()
                call_args = mock_sample.call_args[1]
                self.assertEqual(
                    call_args["sort"],
                    expected_sort_value,
                )
                mock_sample.reset_mock()


if __name__ == "__main__":
    unittest.main()
