"""Tests for the sampling profiler (profile.sample)."""

import contextlib
import io
import marshal
import os
import socket
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

from profile.pstats_collector import PstatsCollector
from profile.stack_collector import (
    CollapsedStackCollector,
)

from test.support.os_helper import unlink
from test.support import force_not_colorized_test_class, SHORT_TIMEOUT
from test.support.socket_helper import find_unused_port
from test.support import requires_subprocess

PROCESS_VM_READV_SUPPORTED = False

try:
    from _remote_debugging import PROCESS_VM_READV_SUPPORTED
    import _remote_debugging
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when _remote_debugging is available"
    )
else:
    import profile.sample
    from profile.sample import SampleProfiler



class MockFrameInfo:
    """Mock FrameInfo for testing since the real one isn't accessible."""

    def __init__(self, filename, lineno, funcname):
        self.filename = filename
        self.lineno = lineno
        self.funcname = funcname

    def __repr__(self):
        return f"MockFrameInfo(filename='{self.filename}', lineno={self.lineno}, funcname='{self.funcname}')"


skip_if_not_supported = unittest.skipIf(
    (
        sys.platform != "darwin"
        and sys.platform != "linux"
        and sys.platform != "win32"
    ),
    "Test only runs on Linux, Windows and MacOS",
)


@contextlib.contextmanager
def test_subprocess(script):
    # Find an unused port for socket communication
    port = find_unused_port()

    # Inject socket connection code at the beginning of the script
    socket_code = f'''
import socket
_test_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
_test_sock.connect(('localhost', {port}))
_test_sock.sendall(b"ready")
'''

    # Combine socket code with user script
    full_script = socket_code + script

    # Create server socket to wait for process to be ready
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(("localhost", port))
    server_socket.settimeout(SHORT_TIMEOUT)
    server_socket.listen(1)

    proc = subprocess.Popen(
        [sys.executable, "-c", full_script],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    client_socket = None
    try:
        # Wait for process to connect and send ready signal
        client_socket, _ = server_socket.accept()
        server_socket.close()
        response = client_socket.recv(1024)
        if response != b"ready":
            raise RuntimeError(f"Unexpected response from subprocess: {response}")

        yield proc
    finally:
        if client_socket is not None:
            client_socket.close()
        if proc.poll() is None:
            proc.kill()
        proc.wait()


def close_and_unlink(file):
    file.close()
    unlink(file.name)


class TestSampleProfilerComponents(unittest.TestCase):
    """Unit tests for individual profiler components."""

    def test_mock_frame_info_with_empty_and_unicode_values(self):
        """Test MockFrameInfo handles empty strings, unicode characters, and very long names correctly."""
        # Test with empty strings
        frame = MockFrameInfo("", 0, "")
        self.assertEqual(frame.filename, "")
        self.assertEqual(frame.lineno, 0)
        self.assertEqual(frame.funcname, "")
        self.assertIn("filename=''", repr(frame))

        # Test with unicode characters
        frame = MockFrameInfo("文件.py", 42, "函数名")
        self.assertEqual(frame.filename, "文件.py")
        self.assertEqual(frame.funcname, "函数名")

        # Test with very long names
        long_filename = "x" * 1000 + ".py"
        long_funcname = "func_" + "x" * 1000
        frame = MockFrameInfo(long_filename, 999999, long_funcname)
        self.assertEqual(frame.filename, long_filename)
        self.assertEqual(frame.lineno, 999999)
        self.assertEqual(frame.funcname, long_funcname)

    def test_pstats_collector_with_extreme_intervals_and_empty_data(self):
        """Test PstatsCollector handles zero/large intervals, empty frames, None thread IDs, and duplicate frames."""
        # Test with zero interval
        collector = PstatsCollector(sample_interval_usec=0)
        self.assertEqual(collector.sample_interval_usec, 0)

        # Test with very large interval
        collector = PstatsCollector(sample_interval_usec=1000000000)
        self.assertEqual(collector.sample_interval_usec, 1000000000)

        # Test collecting empty frames list
        collector = PstatsCollector(sample_interval_usec=1000)
        collector.collect([])
        self.assertEqual(len(collector.result), 0)

        # Test collecting frames with None thread id
        test_frames = [(None, [MockFrameInfo("file.py", 10, "func")])]
        collector.collect(test_frames)
        # Should still process the frames
        self.assertEqual(len(collector.result), 1)

        # Test collecting duplicate frames in same sample
        test_frames = [
            (
                1,
                [
                    MockFrameInfo("file.py", 10, "func1"),
                    MockFrameInfo("file.py", 10, "func1"),  # Duplicate
                ],
            )
        ]
        collector = PstatsCollector(sample_interval_usec=1000)
        collector.collect(test_frames)
        # Should count both occurrences
        self.assertEqual(
            collector.result[("file.py", 10, "func1")]["cumulative_calls"], 2
        )

    def test_pstats_collector_single_frame_stacks(self):
        """Test PstatsCollector with single-frame call stacks to trigger len(frames) <= 1 branch."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Test with exactly one frame (should trigger the <= 1 condition)
        single_frame = [(1, [MockFrameInfo("single.py", 10, "single_func")])]
        collector.collect(single_frame)

        # Should record the single frame with inline call
        self.assertEqual(len(collector.result), 1)
        single_key = ("single.py", 10, "single_func")
        self.assertIn(single_key, collector.result)
        self.assertEqual(collector.result[single_key]["direct_calls"], 1)
        self.assertEqual(collector.result[single_key]["cumulative_calls"], 1)

        # Test with empty frames (should also trigger <= 1 condition)
        empty_frames = [(1, [])]
        collector.collect(empty_frames)

        # Should not add any new entries
        self.assertEqual(
            len(collector.result), 1
        )  # Still just the single frame

        # Test mixed single and multi-frame stacks
        mixed_frames = [
            (
                1,
                [MockFrameInfo("single2.py", 20, "single_func2")],
            ),  # Single frame
            (
                2,
                [  # Multi-frame stack
                    MockFrameInfo("multi.py", 30, "multi_func1"),
                    MockFrameInfo("multi.py", 40, "multi_func2"),
                ],
            ),
        ]
        collector.collect(mixed_frames)

        # Should have recorded all functions
        self.assertEqual(
            len(collector.result), 4
        )  # single + single2 + multi1 + multi2

        # Verify single frame handling
        single2_key = ("single2.py", 20, "single_func2")
        self.assertIn(single2_key, collector.result)
        self.assertEqual(collector.result[single2_key]["direct_calls"], 1)
        self.assertEqual(collector.result[single2_key]["cumulative_calls"], 1)

        # Verify multi-frame handling still works
        multi1_key = ("multi.py", 30, "multi_func1")
        multi2_key = ("multi.py", 40, "multi_func2")
        self.assertIn(multi1_key, collector.result)
        self.assertIn(multi2_key, collector.result)
        self.assertEqual(collector.result[multi1_key]["direct_calls"], 1)
        self.assertEqual(
            collector.result[multi2_key]["cumulative_calls"], 1
        )  # Called from multi1

    def test_collapsed_stack_collector_with_empty_and_deep_stacks(self):
        """Test CollapsedStackCollector handles empty frames, single-frame stacks, and very deep call stacks."""
        collector = CollapsedStackCollector()

        # Test with empty frames
        collector.collect([])
        self.assertEqual(len(collector.call_trees), 0)

        # Test with single frame stack
        test_frames = [(1, [("file.py", 10, "func")])]
        collector.collect(test_frames)
        self.assertEqual(len(collector.call_trees), 1)
        self.assertEqual(collector.call_trees[0], [("file.py", 10, "func")])

        # Test with very deep stack
        deep_stack = [(f"file{i}.py", i, f"func{i}") for i in range(100)]
        test_frames = [(1, deep_stack)]
        collector = CollapsedStackCollector()
        collector.collect(test_frames)
        self.assertEqual(len(collector.call_trees[0]), 100)
        # Check it's properly reversed
        self.assertEqual(
            collector.call_trees[0][0], ("file99.py", 99, "func99")
        )
        self.assertEqual(collector.call_trees[0][-1], ("file0.py", 0, "func0"))

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

        # Top-level function should have direct call
        self.assertEqual(
            collector.result[("file.py", 10, "func1")]["direct_calls"], 1
        )
        self.assertEqual(
            collector.result[("file.py", 10, "func1")]["cumulative_calls"], 1
        )

        # Calling function should have cumulative call but no direct calls
        self.assertEqual(
            collector.result[("file.py", 20, "func2")]["cumulative_calls"], 1
        )
        self.assertEqual(
            collector.result[("file.py", 20, "func2")]["direct_calls"], 0
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

        # Check stats format: (direct_calls, cumulative_calls, tt, ct, callers)
        func1_stats = collector.stats[("file.py", 10, "func1")]
        self.assertEqual(func1_stats[0], 2)  # direct_calls (top of stack)
        self.assertEqual(func1_stats[1], 2)  # cumulative_calls
        self.assertEqual(
            func1_stats[2], 2.0
        )  # tt (total time - 2 samples * 1 sec)
        self.assertEqual(func1_stats[3], 2.0)  # ct (cumulative time)

        func2_stats = collector.stats[("file.py", 20, "func2")]
        self.assertEqual(
            func2_stats[0], 0
        )  # direct_calls (never top of stack)
        self.assertEqual(
            func2_stats[1], 2
        )  # cumulative_calls (appears in stack)
        self.assertEqual(func2_stats[2], 0.0)  # tt (no direct calls)
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


class TestSampleProfiler(unittest.TestCase):
    """Test the SampleProfiler class."""

    def test_sample_profiler_initialization(self):
        """Test SampleProfiler initialization with various parameters."""
        from profile.sample import SampleProfiler

        # Mock RemoteUnwinder to avoid permission issues
        with mock.patch(
            "_remote_debugging.RemoteUnwinder"
        ) as mock_unwinder_class:
            mock_unwinder_class.return_value = mock.MagicMock()

            # Test basic initialization
            profiler = SampleProfiler(
                pid=12345, sample_interval_usec=1000, all_threads=False
            )
            self.assertEqual(profiler.pid, 12345)
            self.assertEqual(profiler.sample_interval_usec, 1000)
            self.assertEqual(profiler.all_threads, False)

            # Test with all_threads=True
            profiler = SampleProfiler(
                pid=54321, sample_interval_usec=5000, all_threads=True
            )
            self.assertEqual(profiler.pid, 54321)
            self.assertEqual(profiler.sample_interval_usec, 5000)
            self.assertEqual(profiler.all_threads, True)

    def test_sample_profiler_sample_method_timing(self):
        """Test that the sample method respects duration and handles timing correctly."""
        from profile.sample import SampleProfiler

        # Mock the unwinder to avoid needing a real process
        mock_unwinder = mock.MagicMock()
        mock_unwinder.get_stack_trace.return_value = [
            (
                1,
                [
                    mock.MagicMock(
                        filename="test.py", lineno=10, funcname="test_func"
                    )
                ],
            )
        ]

        with mock.patch(
            "_remote_debugging.RemoteUnwinder"
        ) as mock_unwinder_class:
            mock_unwinder_class.return_value = mock_unwinder

            profiler = SampleProfiler(
                pid=12345, sample_interval_usec=100000, all_threads=False
            )  # 100ms interval

            # Mock collector
            mock_collector = mock.MagicMock()

            # Mock time to control the sampling loop
            start_time = 1000.0
            times = [
                start_time + i * 0.1 for i in range(12)
            ]  # 0, 0.1, 0.2, ..., 1.1 seconds

            with mock.patch("time.perf_counter", side_effect=times):
                with io.StringIO() as output:
                    with mock.patch("sys.stdout", output):
                        profiler.sample(mock_collector, duration_sec=1)

                    result = output.getvalue()

            # Should have captured approximately 10 samples (1 second / 0.1 second interval)
            self.assertIn("Captured", result)
            self.assertIn("samples", result)

            # Verify collector was called multiple times
            self.assertGreaterEqual(mock_collector.collect.call_count, 5)
            self.assertLessEqual(mock_collector.collect.call_count, 11)

    def test_sample_profiler_error_handling(self):
        """Test that the sample method handles errors gracefully."""
        from profile.sample import SampleProfiler

        # Mock unwinder that raises errors
        mock_unwinder = mock.MagicMock()
        error_sequence = [
            RuntimeError("Process died"),
            [
                (
                    1,
                    [
                        mock.MagicMock(
                            filename="test.py", lineno=10, funcname="test_func"
                        )
                    ],
                )
            ],
            UnicodeDecodeError("utf-8", b"", 0, 1, "invalid"),
            [
                (
                    1,
                    [
                        mock.MagicMock(
                            filename="test.py",
                            lineno=20,
                            funcname="test_func2",
                        )
                    ],
                )
            ],
            OSError("Permission denied"),
        ]
        mock_unwinder.get_stack_trace.side_effect = error_sequence

        with mock.patch(
            "_remote_debugging.RemoteUnwinder"
        ) as mock_unwinder_class:
            mock_unwinder_class.return_value = mock_unwinder

            profiler = SampleProfiler(
                pid=12345, sample_interval_usec=10000, all_threads=False
            )

            mock_collector = mock.MagicMock()

            # Control timing to run exactly 5 samples
            times = [0.0, 0.01, 0.02, 0.03, 0.04, 0.05, 0.06]

            with mock.patch("time.perf_counter", side_effect=times):
                with io.StringIO() as output:
                    with mock.patch("sys.stdout", output):
                        profiler.sample(mock_collector, duration_sec=0.05)

                    result = output.getvalue()

            # Should report error rate
            self.assertIn("Error rate:", result)
            self.assertIn("%", result)

            # Collector should have been called only for successful samples (should be > 0)
            self.assertGreater(mock_collector.collect.call_count, 0)
            self.assertLessEqual(mock_collector.collect.call_count, 3)

    def test_sample_profiler_missed_samples_warning(self):
        """Test that the profiler warns about missed samples when sampling is too slow."""
        from profile.sample import SampleProfiler

        mock_unwinder = mock.MagicMock()
        mock_unwinder.get_stack_trace.return_value = [
            (
                1,
                [
                    mock.MagicMock(
                        filename="test.py", lineno=10, funcname="test_func"
                    )
                ],
            )
        ]

        with mock.patch(
            "_remote_debugging.RemoteUnwinder"
        ) as mock_unwinder_class:
            mock_unwinder_class.return_value = mock_unwinder

            # Use very short interval that we'll miss
            profiler = SampleProfiler(
                pid=12345, sample_interval_usec=1000, all_threads=False
            )  # 1ms interval

            mock_collector = mock.MagicMock()

            # Simulate slow sampling where we miss many samples
            times = [
                0.0,
                0.1,
                0.2,
                0.3,
                0.4,
                0.5,
                0.6,
                0.7,
            ]  # Extra time points to avoid StopIteration

            with mock.patch("time.perf_counter", side_effect=times):
                with io.StringIO() as output:
                    with mock.patch("sys.stdout", output):
                        profiler.sample(mock_collector, duration_sec=0.5)

                    result = output.getvalue()

            # Should warn about missed samples
            self.assertIn("Warning: missed", result)
            self.assertIn("samples from the expected total", result)


@force_not_colorized_test_class
class TestPrintSampledStats(unittest.TestCase):
    """Test the print_sampled_stats function."""

    def setUp(self):
        """Set up test data."""
        # Mock stats data
        self.mock_stats = mock.MagicMock()
        self.mock_stats.stats = {
            ("file1.py", 10, "func1"): (
                100,
                100,
                0.5,
                0.5,
                {},
            ),  # cc, nc, tt, ct, callers
            ("file2.py", 20, "func2"): (50, 50, 0.25, 0.3, {}),
            ("file3.py", 30, "func3"): (200, 200, 1.5, 2.0, {}),
            ("file4.py", 40, "func4"): (
                10,
                10,
                0.001,
                0.001,
                {},
            ),  # millisecond range
            ("file5.py", 50, "func5"): (
                5,
                5,
                0.000001,
                0.000002,
                {},
            ),  # microsecond range
        }

    def test_print_sampled_stats_basic(self):
        """Test basic print_sampled_stats functionality."""
        from profile.sample import print_sampled_stats

        # Capture output
        with io.StringIO() as output:
            with mock.patch("sys.stdout", output):
                print_sampled_stats(self.mock_stats, sample_interval_usec=100)

            result = output.getvalue()

        # Check header is present
        self.assertIn("Profile Stats:", result)
        self.assertIn("nsamples", result)
        self.assertIn("tottime", result)
        self.assertIn("cumtime", result)

        # Check functions are present
        self.assertIn("func1", result)
        self.assertIn("func2", result)
        self.assertIn("func3", result)

    def test_print_sampled_stats_sorting(self):
        """Test different sorting options."""
        from profile.sample import print_sampled_stats

        # Test sort by calls
        with io.StringIO() as output:
            with mock.patch("sys.stdout", output):
                print_sampled_stats(
                    self.mock_stats, sort=0, sample_interval_usec=100
                )

            result = output.getvalue()
            lines = result.strip().split("\n")

        # Find the data lines (skip header)
        data_lines = [l for l in lines if "file" in l and ".py" in l]
        # func3 should be first (200 calls)
        self.assertIn("func3", data_lines[0])

        # Test sort by time
        with io.StringIO() as output:
            with mock.patch("sys.stdout", output):
                print_sampled_stats(
                    self.mock_stats, sort=1, sample_interval_usec=100
                )

            result = output.getvalue()
            lines = result.strip().split("\n")

        data_lines = [l for l in lines if "file" in l and ".py" in l]
        # func3 should be first (1.5s time)
        self.assertIn("func3", data_lines[0])

    def test_print_sampled_stats_limit(self):
        """Test limiting output rows."""
        from profile.sample import print_sampled_stats

        with io.StringIO() as output:
            with mock.patch("sys.stdout", output):
                print_sampled_stats(
                    self.mock_stats, limit=2, sample_interval_usec=100
                )

            result = output.getvalue()

        # Count function entries in the main stats section (not in summary)
        lines = result.split("\n")
        # Find where the main stats section ends (before summary)
        main_section_lines = []
        for line in lines:
            if "Summary of Interesting Functions:" in line:
                break
            main_section_lines.append(line)

        # Count function entries only in main section
        func_count = sum(
            1
            for line in main_section_lines
            if "func" in line and ".py" in line
        )
        self.assertEqual(func_count, 2)

    def test_print_sampled_stats_time_units(self):
        """Test proper time unit selection."""
        from profile.sample import print_sampled_stats

        with io.StringIO() as output:
            with mock.patch("sys.stdout", output):
                print_sampled_stats(self.mock_stats, sample_interval_usec=100)

            result = output.getvalue()

        # Should use seconds for the header since max time is > 1s
        self.assertIn("tottime (s)", result)
        self.assertIn("cumtime (s)", result)

        # Test with only microsecond-range times
        micro_stats = mock.MagicMock()
        micro_stats.stats = {
            ("file1.py", 10, "func1"): (100, 100, 0.000005, 0.000010, {}),
        }

        with io.StringIO() as output:
            with mock.patch("sys.stdout", output):
                print_sampled_stats(micro_stats, sample_interval_usec=100)

            result = output.getvalue()

        # Should use microseconds
        self.assertIn("tottime (μs)", result)
        self.assertIn("cumtime (μs)", result)

    def test_print_sampled_stats_summary(self):
        """Test summary section generation."""
        from profile.sample import print_sampled_stats

        with io.StringIO() as output:
            with mock.patch("sys.stdout", output):
                print_sampled_stats(
                    self.mock_stats,
                    show_summary=True,
                    sample_interval_usec=100,
                )

            result = output.getvalue()

        # Check summary sections are present
        self.assertIn("Summary of Interesting Functions:", result)
        self.assertIn(
            "Functions with Highest Direct/Cumulative Ratio (Hot Spots):",
            result,
        )
        self.assertIn(
            "Functions with Highest Call Frequency (Indirect Calls):", result
        )
        self.assertIn(
            "Functions with Highest Call Magnification (Cumulative/Direct):",
            result,
        )

    def test_print_sampled_stats_no_summary(self):
        """Test disabling summary output."""
        from profile.sample import print_sampled_stats

        with io.StringIO() as output:
            with mock.patch("sys.stdout", output):
                print_sampled_stats(
                    self.mock_stats,
                    show_summary=False,
                    sample_interval_usec=100,
                )

            result = output.getvalue()

        # Summary should not be present
        self.assertNotIn("Summary of Interesting Functions:", result)

    def test_print_sampled_stats_empty_stats(self):
        """Test with empty stats."""
        from profile.sample import print_sampled_stats

        empty_stats = mock.MagicMock()
        empty_stats.stats = {}

        with io.StringIO() as output:
            with mock.patch("sys.stdout", output):
                print_sampled_stats(empty_stats, sample_interval_usec=100)

            result = output.getvalue()

        # Should still print header
        self.assertIn("Profile Stats:", result)

    def test_print_sampled_stats_sample_percentage_sorting(self):
        """Test sample percentage sorting options."""
        from profile.sample import print_sampled_stats

        # Add a function with high sample percentage (more direct calls than func3's 200)
        self.mock_stats.stats[("expensive.py", 60, "expensive_func")] = (
            300,  # direct calls (higher than func3's 200)
            300,  # cumulative calls
            1.0,  # total time
            1.0,  # cumulative time
            {},
        )

        # Test sort by sample percentage
        with io.StringIO() as output:
            with mock.patch("sys.stdout", output):
                print_sampled_stats(
                    self.mock_stats, sort=3, sample_interval_usec=100
                )  # sample percentage

            result = output.getvalue()
            lines = result.strip().split("\n")

        data_lines = [l for l in lines if ".py" in l and "func" in l]
        # expensive_func should be first (highest sample percentage)
        self.assertIn("expensive_func", data_lines[0])

    def test_print_sampled_stats_with_recursive_calls(self):
        """Test print_sampled_stats with recursive calls where nc != cc."""
        from profile.sample import print_sampled_stats

        # Create stats with recursive calls (nc != cc)
        recursive_stats = mock.MagicMock()
        recursive_stats.stats = {
            # (direct_calls, cumulative_calls, tt, ct, callers) - recursive function
            ("recursive.py", 10, "factorial"): (
                5,  # direct_calls
                10,  # cumulative_calls (appears more times in stack due to recursion)
                0.5,
                0.6,
                {},
            ),
            ("normal.py", 20, "normal_func"): (
                3,  # direct_calls
                3,  # cumulative_calls (same as direct for non-recursive)
                0.2,
                0.2,
                {},
            ),
        }

        with io.StringIO() as output:
            with mock.patch("sys.stdout", output):
                print_sampled_stats(recursive_stats, sample_interval_usec=100)

            result = output.getvalue()

        # Should display recursive calls as "5/10" format
        self.assertIn("5/10", result)  # nc/cc format for recursive calls
        self.assertIn("3", result)  # just nc for non-recursive calls
        self.assertIn("factorial", result)
        self.assertIn("normal_func", result)

    def test_print_sampled_stats_with_zero_call_counts(self):
        """Test print_sampled_stats with zero call counts to trigger division protection."""
        from profile.sample import print_sampled_stats

        # Create stats with zero call counts
        zero_stats = mock.MagicMock()
        zero_stats.stats = {
            ("file.py", 10, "zero_calls"): (0, 0, 0.0, 0.0, {}),  # Zero calls
            ("file.py", 20, "normal_func"): (
                5,
                5,
                0.1,
                0.1,
                {},
            ),  # Normal function
        }

        with io.StringIO() as output:
            with mock.patch("sys.stdout", output):
                print_sampled_stats(zero_stats, sample_interval_usec=100)

            result = output.getvalue()

        # Should handle zero call counts gracefully
        self.assertIn("zero_calls", result)
        self.assertIn("zero_calls", result)
        self.assertIn("normal_func", result)

    def test_print_sampled_stats_sort_by_name(self):
        """Test sort by function name option."""
        from profile.sample import print_sampled_stats

        with io.StringIO() as output:
            with mock.patch("sys.stdout", output):
                print_sampled_stats(
                    self.mock_stats, sort=-1, sample_interval_usec=100
                )  # sort by name

            result = output.getvalue()
            lines = result.strip().split("\n")

        # Find the data lines (skip header and summary)
        # Data lines start with whitespace and numbers, and contain filename:lineno(function)
        data_lines = []
        for line in lines:
            # Skip header lines and summary sections
            if (
                line.startswith("     ")
                and "(" in line
                and ")" in line
                and not line.startswith(
                    "     1."
                )  # Skip summary lines that start with times
                and not line.startswith(
                    "     0."
                )  # Skip summary lines that start with times
                and not "per call" in line  # Skip summary lines
                and not "calls" in line  # Skip summary lines
                and not "total time" in line  # Skip summary lines
                and not "cumulative time" in line
            ):  # Skip summary lines
                data_lines.append(line)

        # Extract just the function names for comparison
        func_names = []
        import re

        for line in data_lines:
            # Function name is between the last ( and ), accounting for ANSI color codes
            match = re.search(r"\(([^)]+)\)$", line)
            if match:
                func_name = match.group(1)
                # Remove ANSI color codes
                func_name = re.sub(r"\x1b\[[0-9;]*m", "", func_name)
                func_names.append(func_name)

        # Verify we extracted function names and they are sorted
        self.assertGreater(
            len(func_names), 0, "Should have extracted some function names"
        )
        self.assertEqual(
            func_names,
            sorted(func_names),
            f"Function names {func_names} should be sorted alphabetically",
        )

    def test_print_sampled_stats_with_zero_time_functions(self):
        """Test summary sections with functions that have zero time."""
        from profile.sample import print_sampled_stats

        # Create stats with zero-time functions
        zero_time_stats = mock.MagicMock()
        zero_time_stats.stats = {
            ("file1.py", 10, "zero_time_func"): (
                5,
                5,
                0.0,
                0.0,
                {},
            ),  # Zero time
            ("file2.py", 20, "normal_func"): (
                3,
                3,
                0.1,
                0.1,
                {},
            ),  # Normal time
        }

        with io.StringIO() as output:
            with mock.patch("sys.stdout", output):
                print_sampled_stats(
                    zero_time_stats,
                    show_summary=True,
                    sample_interval_usec=100,
                )

            result = output.getvalue()

        # Should handle zero-time functions gracefully in summary
        self.assertIn("Summary of Interesting Functions:", result)
        self.assertIn("zero_time_func", result)
        self.assertIn("normal_func", result)

    def test_print_sampled_stats_with_malformed_qualified_names(self):
        """Test summary generation with function names that don't contain colons."""
        from profile.sample import print_sampled_stats

        # Create stats with function names that would create malformed qualified names
        malformed_stats = mock.MagicMock()
        malformed_stats.stats = {
            # Function name without clear module separation
            ("no_colon_func", 10, "func"): (3, 3, 0.1, 0.1, {}),
            ("", 20, "empty_filename_func"): (2, 2, 0.05, 0.05, {}),
            ("normal.py", 30, "normal_func"): (5, 5, 0.2, 0.2, {}),
        }

        with io.StringIO() as output:
            with mock.patch("sys.stdout", output):
                print_sampled_stats(
                    malformed_stats,
                    show_summary=True,
                    sample_interval_usec=100,
                )

            result = output.getvalue()

        # Should handle malformed names gracefully in summary aggregation
        self.assertIn("Summary of Interesting Functions:", result)
        # All function names should appear somewhere in the output
        self.assertIn("func", result)
        self.assertIn("empty_filename_func", result)
        self.assertIn("normal_func", result)

    def test_print_sampled_stats_with_recursive_call_stats_creation(self):
        """Test create_stats with recursive call data to trigger total_rec_calls branch."""
        collector = PstatsCollector(sample_interval_usec=1000000)  # 1 second

        # Simulate recursive function data where total_rec_calls would be set
        # We need to manually manipulate the collector result to test this branch
        collector.result = {
            ("recursive.py", 10, "factorial"): {
                "total_rec_calls": 3,  # Non-zero recursive calls
                "direct_calls": 5,
                "cumulative_calls": 10,
            },
            ("normal.py", 20, "normal_func"): {
                "total_rec_calls": 0,  # Zero recursive calls
                "direct_calls": 2,
                "cumulative_calls": 5,
            },
        }

        collector.create_stats()

        # Check that recursive calls are handled differently from non-recursive
        factorial_stats = collector.stats[("recursive.py", 10, "factorial")]
        normal_stats = collector.stats[("normal.py", 20, "normal_func")]

        # factorial should use cumulative_calls (10) as nc
        self.assertEqual(
            factorial_stats[1], 10
        )  # nc should be cumulative_calls
        self.assertEqual(factorial_stats[0], 5)  # cc should be direct_calls

        # normal_func should use cumulative_calls as nc
        self.assertEqual(normal_stats[1], 5)  # nc should be cumulative_calls
        self.assertEqual(normal_stats[0], 2)  # cc should be direct_calls


@skip_if_not_supported
@unittest.skipIf(
    sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
    "Test only runs on Linux with process_vm_readv support",
)
class TestRecursiveFunctionProfiling(unittest.TestCase):
    """Test profiling of recursive functions and complex call patterns."""

    def test_recursive_function_call_counting(self):
        """Test that recursive function calls are counted correctly."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Simulate a recursive call pattern: fibonacci(5) calling itself
        recursive_frames = [
            (
                1,
                [  # First sample: deep in recursion
                    MockFrameInfo("fib.py", 10, "fibonacci"),
                    MockFrameInfo("fib.py", 10, "fibonacci"),  # recursive call
                    MockFrameInfo(
                        "fib.py", 10, "fibonacci"
                    ),  # deeper recursion
                    MockFrameInfo("fib.py", 10, "fibonacci"),  # even deeper
                    MockFrameInfo("main.py", 5, "main"),  # main caller
                ],
            ),
            (
                1,
                [  # Second sample: different recursion depth
                    MockFrameInfo("fib.py", 10, "fibonacci"),
                    MockFrameInfo("fib.py", 10, "fibonacci"),  # recursive call
                    MockFrameInfo("main.py", 5, "main"),  # main caller
                ],
            ),
            (
                1,
                [  # Third sample: back to deeper recursion
                    MockFrameInfo("fib.py", 10, "fibonacci"),
                    MockFrameInfo("fib.py", 10, "fibonacci"),
                    MockFrameInfo("fib.py", 10, "fibonacci"),
                    MockFrameInfo("main.py", 5, "main"),
                ],
            ),
        ]

        for frames in recursive_frames:
            collector.collect([frames])

        collector.create_stats()

        # Check that recursive calls are counted properly
        fib_key = ("fib.py", 10, "fibonacci")
        main_key = ("main.py", 5, "main")

        self.assertIn(fib_key, collector.stats)
        self.assertIn(main_key, collector.stats)

        # Fibonacci should have many calls due to recursion
        fib_stats = collector.stats[fib_key]
        direct_calls, cumulative_calls, tt, ct, callers = fib_stats

        # Should have recorded multiple calls (9 total appearances in samples)
        self.assertEqual(cumulative_calls, 9)
        self.assertGreater(tt, 0)  # Should have some total time
        self.assertGreater(ct, 0)  # Should have some cumulative time

        # Main should have fewer calls
        main_stats = collector.stats[main_key]
        main_direct_calls, main_cumulative_calls = main_stats[0], main_stats[1]
        self.assertEqual(main_direct_calls, 0)  # Never directly executing
        self.assertEqual(main_cumulative_calls, 3)  # Appears in all 3 samples

    def test_nested_function_hierarchy(self):
        """Test profiling of deeply nested function calls."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Simulate a deep call hierarchy
        deep_call_frames = [
            (
                1,
                [
                    MockFrameInfo("level1.py", 10, "level1_func"),
                    MockFrameInfo("level2.py", 20, "level2_func"),
                    MockFrameInfo("level3.py", 30, "level3_func"),
                    MockFrameInfo("level4.py", 40, "level4_func"),
                    MockFrameInfo("level5.py", 50, "level5_func"),
                    MockFrameInfo("main.py", 5, "main"),
                ],
            ),
            (
                1,
                [  # Same hierarchy sampled again
                    MockFrameInfo("level1.py", 10, "level1_func"),
                    MockFrameInfo("level2.py", 20, "level2_func"),
                    MockFrameInfo("level3.py", 30, "level3_func"),
                    MockFrameInfo("level4.py", 40, "level4_func"),
                    MockFrameInfo("level5.py", 50, "level5_func"),
                    MockFrameInfo("main.py", 5, "main"),
                ],
            ),
        ]

        for frames in deep_call_frames:
            collector.collect([frames])

        collector.create_stats()

        # All levels should be recorded
        for level in range(1, 6):
            key = (f"level{level}.py", level * 10, f"level{level}_func")
            self.assertIn(key, collector.stats)

            stats = collector.stats[key]
            direct_calls, cumulative_calls, tt, ct, callers = stats

            # Each level should appear in stack twice (2 samples)
            self.assertEqual(cumulative_calls, 2)

            # Only level1 (deepest) should have direct calls
            if level == 1:
                self.assertEqual(direct_calls, 2)
            else:
                self.assertEqual(direct_calls, 0)

            # Deeper levels should have lower cumulative time than higher levels
            # (since they don't include time from functions they call)
            if level == 1:  # Deepest level with most time
                self.assertGreater(ct, 0)

    def test_alternating_call_patterns(self):
        """Test profiling with alternating call patterns."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Simulate alternating execution paths
        pattern_frames = [
            # Pattern A: path through func_a
            (
                1,
                [
                    MockFrameInfo("module.py", 10, "func_a"),
                    MockFrameInfo("module.py", 30, "shared_func"),
                    MockFrameInfo("main.py", 5, "main"),
                ],
            ),
            # Pattern B: path through func_b
            (
                1,
                [
                    MockFrameInfo("module.py", 20, "func_b"),
                    MockFrameInfo("module.py", 30, "shared_func"),
                    MockFrameInfo("main.py", 5, "main"),
                ],
            ),
            # Pattern A again
            (
                1,
                [
                    MockFrameInfo("module.py", 10, "func_a"),
                    MockFrameInfo("module.py", 30, "shared_func"),
                    MockFrameInfo("main.py", 5, "main"),
                ],
            ),
            # Pattern B again
            (
                1,
                [
                    MockFrameInfo("module.py", 20, "func_b"),
                    MockFrameInfo("module.py", 30, "shared_func"),
                    MockFrameInfo("main.py", 5, "main"),
                ],
            ),
        ]

        for frames in pattern_frames:
            collector.collect([frames])

        collector.create_stats()

        # Check that both paths are recorded equally
        func_a_key = ("module.py", 10, "func_a")
        func_b_key = ("module.py", 20, "func_b")
        shared_key = ("module.py", 30, "shared_func")
        main_key = ("main.py", 5, "main")

        # func_a and func_b should each be directly executing twice
        self.assertEqual(collector.stats[func_a_key][0], 2)  # direct_calls
        self.assertEqual(collector.stats[func_a_key][1], 2)  # cumulative_calls
        self.assertEqual(collector.stats[func_b_key][0], 2)  # direct_calls
        self.assertEqual(collector.stats[func_b_key][1], 2)  # cumulative_calls

        # shared_func should appear in all samples (4 times) but never directly executing
        self.assertEqual(collector.stats[shared_key][0], 0)  # direct_calls
        self.assertEqual(collector.stats[shared_key][1], 4)  # cumulative_calls

        # main should appear in all samples but never directly executing
        self.assertEqual(collector.stats[main_key][0], 0)  # direct_calls
        self.assertEqual(collector.stats[main_key][1], 4)  # cumulative_calls

    def test_collapsed_stack_with_recursion(self):
        """Test collapsed stack collector with recursive patterns."""
        collector = CollapsedStackCollector()

        # Recursive call pattern
        recursive_frames = [
            (
                1,
                [
                    ("factorial.py", 10, "factorial"),
                    ("factorial.py", 10, "factorial"),  # recursive
                    ("factorial.py", 10, "factorial"),  # deeper
                    ("main.py", 5, "main"),
                ],
            ),
            (
                1,
                [
                    ("factorial.py", 10, "factorial"),
                    ("factorial.py", 10, "factorial"),  # different depth
                    ("main.py", 5, "main"),
                ],
            ),
        ]

        for frames in recursive_frames:
            collector.collect([frames])

        # Should capture both call trees
        self.assertEqual(len(collector.call_trees), 2)

        # First tree should be longer (deeper recursion)
        tree1 = collector.call_trees[0]
        tree2 = collector.call_trees[1]

        # Trees should be different lengths due to different recursion depths
        self.assertNotEqual(len(tree1), len(tree2))

        # Both should contain factorial calls
        self.assertTrue(any("factorial" in str(frame) for frame in tree1))
        self.assertTrue(any("factorial" in str(frame) for frame in tree2))

        # Function samples should count all occurrences
        factorial_key = ("factorial.py", 10, "factorial")
        main_key = ("main.py", 5, "main")

        # factorial appears 5 times total (3 + 2)
        self.assertEqual(collector.function_samples[factorial_key], 5)
        # main appears 2 times total
        self.assertEqual(collector.function_samples[main_key], 2)


@requires_subprocess()
@skip_if_not_supported
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


@skip_if_not_supported
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

    def test_invalid_output_format_with_mocked_profiler(self):
        """Test invalid output format with proper mocking to avoid permission issues."""
        with mock.patch(
            "profile.sample.SampleProfiler"
        ) as mock_profiler_class:
            mock_profiler = mock.MagicMock()
            mock_profiler_class.return_value = mock_profiler

            with self.assertRaises(ValueError) as cm:
                profile.sample.sample(
                    12345,
                    duration_sec=1,
                    output_format="unknown_format",
                )

            # Should raise ValueError with the invalid format name
            self.assertIn(
                "Invalid output format: unknown_format", str(cm.exception)
            )

    def test_is_process_running(self):
        with test_subprocess("import time; time.sleep(1000)") as proc:
            try:
                profiler = SampleProfiler(pid=proc.pid, sample_interval_usec=1000, all_threads=False)
            except PermissionError:
                self.skipTest(
                    "Insufficient permissions to read the stack trace"
                )
            self.assertTrue(profiler._is_process_running())
            self.assertIsNotNone(profiler.unwinder.get_stack_trace())
            proc.kill()
            proc.wait()
            # ValueError on MacOS (yeah I know), ProcessLookupError on Linux and Windows
            self.assertRaises((ValueError, ProcessLookupError), profiler.unwinder.get_stack_trace)

        # Exit the context manager to ensure the process is terminated
        self.assertFalse(profiler._is_process_running())
        self.assertRaises((ValueError, ProcessLookupError), profiler.unwinder.get_stack_trace)

    @unittest.skipUnless(sys.platform == "linux", "Only valid on Linux")
    def test_esrch_signal_handling(self):
        with test_subprocess("import time; time.sleep(1000)") as proc:
            try:
                unwinder = _remote_debugging.RemoteUnwinder(proc.pid)
            except PermissionError:
                self.skipTest(
                    "Insufficient permissions to read the stack trace"
                )
            initial_trace = unwinder.get_stack_trace()
            self.assertIsNotNone(initial_trace)

            proc.kill()

            # Wait for the process to die and try to get another trace
            proc.wait()

            with self.assertRaises(ProcessLookupError):
                unwinder.get_stack_trace()



class TestSampleProfilerCLI(unittest.TestCase):
    def test_cli_collapsed_format_validation(self):
        """Test that CLI properly validates incompatible options with collapsed format."""
        test_cases = [
            # Test sort options are invalid with collapsed
            (
                ["profile.sample", "--collapsed", "--sort-nsamples", "12345"],
                "sort",
            ),
            (
                ["profile.sample", "--collapsed", "--sort-tottime", "12345"],
                "sort",
            ),
            (
                [
                    "profile.sample",
                    "--collapsed",
                    "--sort-cumtime",
                    "12345",
                ],
                "sort",
            ),
            (
                [
                    "profile.sample",
                    "--collapsed",
                    "--sort-sample-pct",
                    "12345",
                ],
                "sort",
            ),
            (
                [
                    "profile.sample",
                    "--collapsed",
                    "--sort-cumul-pct",
                    "12345",
                ],
                "sort",
            ),
            (
                ["profile.sample", "--collapsed", "--sort-name", "12345"],
                "sort",
            ),
            # Test limit option is invalid with collapsed
            (["profile.sample", "--collapsed", "-l", "20", "12345"], "limit"),
            (
                ["profile.sample", "--collapsed", "--limit", "20", "12345"],
                "limit",
            ),
            # Test no-summary option is invalid with collapsed
            (
                ["profile.sample", "--collapsed", "--no-summary", "12345"],
                "summary",
            ),
        ]

        for test_args, expected_error_keyword in test_cases:
            with (
                mock.patch("sys.argv", test_args),
                mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
                self.assertRaises(SystemExit) as cm,
            ):
                profile.sample.main()

            self.assertEqual(cm.exception.code, 2)  # argparse error code
            error_msg = mock_stderr.getvalue()
            self.assertIn("error:", error_msg)
            self.assertIn("--pstats format", error_msg)

    def test_cli_default_collapsed_filename(self):
        """Test that collapsed format gets a default filename when not specified."""
        test_args = ["profile.sample", "--collapsed", "12345"]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profile.sample.sample") as mock_sample,
        ):
            profile.sample.main()

            # Check that filename was set to default collapsed format
            mock_sample.assert_called_once()
            call_args = mock_sample.call_args[1]
            self.assertEqual(call_args["output_format"], "collapsed")
            self.assertEqual(call_args["filename"], "collapsed.12345.txt")

    def test_cli_custom_output_filenames(self):
        """Test custom output filenames for both formats."""
        test_cases = [
            (
                ["profile.sample", "--pstats", "-o", "custom.pstats", "12345"],
                "custom.pstats",
                "pstats",
            ),
            (
                ["profile.sample", "--collapsed", "-o", "custom.txt", "12345"],
                "custom.txt",
                "collapsed",
            ),
        ]

        for test_args, expected_filename, expected_format in test_cases:
            with (
                mock.patch("sys.argv", test_args),
                mock.patch("profile.sample.sample") as mock_sample,
            ):
                profile.sample.main()

                mock_sample.assert_called_once()
                call_args = mock_sample.call_args[1]
                self.assertEqual(call_args["filename"], expected_filename)
                self.assertEqual(call_args["output_format"], expected_format)

    def test_cli_missing_required_arguments(self):
        """Test that CLI requires PID argument."""
        with (
            mock.patch("sys.argv", ["profile.sample"]),
            mock.patch("sys.stderr", io.StringIO()),
        ):
            with self.assertRaises(SystemExit):
                profile.sample.main()

    def test_cli_mutually_exclusive_format_options(self):
        """Test that pstats and collapsed options are mutually exclusive."""
        with (
            mock.patch(
                "sys.argv",
                ["profile.sample", "--pstats", "--collapsed", "12345"],
            ),
            mock.patch("sys.stderr", io.StringIO()),
        ):
            with self.assertRaises(SystemExit):
                profile.sample.main()

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
                sort=2,
                show_summary=True,
                output_format="pstats",
                realtime_stats=False,
            )

    def test_sort_options(self):
        sort_options = [
            ("--sort-nsamples", 0),
            ("--sort-tottime", 1),
            ("--sort-cumtime", 2),
            ("--sort-sample-pct", 3),
            ("--sort-cumul-pct", 4),
            ("--sort-name", -1),
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
