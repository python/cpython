"""Tests for sampling profiler core functionality."""

import io
from unittest import mock
import unittest

try:
    import _remote_debugging  # noqa: F401
    from profiling.sampling.sample import SampleProfiler
    from profiling.sampling.pstats_collector import PstatsCollector
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when _remote_debugging is available"
    )

from test.support import force_not_colorized_test_class


def print_sampled_stats(stats, sort=-1, limit=None, show_summary=True, sample_interval_usec=100):
    """Helper function to maintain compatibility with old test API.

    This wraps the new PstatsCollector.print_stats() API to work with the
    existing test infrastructure.
    """
    # Create a mock collector that populates stats correctly
    collector = PstatsCollector(sample_interval_usec=sample_interval_usec)

    # Override create_stats to populate self.stats with the provided stats
    def mock_create_stats():
        collector.stats = stats.stats
    collector.create_stats = mock_create_stats

    # Call the new print_stats method
    collector.print_stats(sort=sort, limit=limit, show_summary=show_summary)


class TestSampleProfiler(unittest.TestCase):
    """Test the SampleProfiler class."""

    def test_sample_profiler_initialization(self):
        """Test SampleProfiler initialization with various parameters."""

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

        empty_stats = mock.MagicMock()
        empty_stats.stats = {}

        with io.StringIO() as output:
            with mock.patch("sys.stdout", output):
                print_sampled_stats(empty_stats, sample_interval_usec=100)

            result = output.getvalue()

        # Should print message about no samples
        self.assertIn("No samples were collected.", result)

    def test_print_sampled_stats_sample_percentage_sorting(self):
        """Test sample percentage sorting options."""

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
