"""Core functionality tests for LiveStatsCollector.

Tests for path simplification, frame processing, collect method,
statistics building, sorting, and formatting.
"""

import os
import unittest
from test.support import requires
from test.support.import_helper import import_module

# Only run these tests if curses is available
requires("curses")
curses = import_module("curses")

from profiling.sampling.live_collector import LiveStatsCollector, MockDisplay
from profiling.sampling.constants import (
    THREAD_STATUS_HAS_GIL,
    THREAD_STATUS_ON_CPU,
)
from ._live_collector_helpers import (
    MockFrameInfo,
    MockThreadInfo,
    MockInterpreterInfo,
)


class TestLiveStatsCollectorPathSimplification(unittest.TestCase):
    """Tests for path simplification functionality."""

    def test_simplify_stdlib_path(self):
        """Test simplification of standard library paths."""
        collector = LiveStatsCollector(1000)
        # Get actual os module path
        os_file = os.__file__
        if os_file:
            stdlib_dir = os.path.dirname(os.path.abspath(os_file))
            test_path = os.path.join(stdlib_dir, "json", "decoder.py")
            simplified = collector.simplify_path(test_path)
            # Should remove the stdlib prefix
            self.assertNotIn(stdlib_dir, simplified)
            self.assertIn("json", simplified)

    def test_simplify_unknown_path(self):
        """Test that unknown paths are returned unchanged."""
        collector = LiveStatsCollector(1000)
        test_path = "/some/unknown/path/file.py"
        simplified = collector.simplify_path(test_path)
        self.assertEqual(simplified, test_path)


class TestLiveStatsCollectorFrameProcessing(unittest.TestCase):
    """Tests for frame processing functionality."""

    def test_process_single_frame(self):
        """Test processing a single frame."""
        collector = LiveStatsCollector(1000)
        frames = [MockFrameInfo("test.py", 10, "test_func")]
        collector.process_frames(frames)

        location = ("test.py", 10, "test_func")
        self.assertEqual(collector.result[location]["direct_calls"], 1)
        self.assertEqual(collector.result[location]["cumulative_calls"], 1)

    def test_process_multiple_frames(self):
        """Test processing a stack of multiple frames."""
        collector = LiveStatsCollector(1000)
        frames = [
            MockFrameInfo("test.py", 10, "inner_func"),
            MockFrameInfo("test.py", 20, "middle_func"),
            MockFrameInfo("test.py", 30, "outer_func"),
        ]
        collector.process_frames(frames)

        # Top frame (inner_func) should have both direct and cumulative
        inner_loc = ("test.py", 10, "inner_func")
        self.assertEqual(collector.result[inner_loc]["direct_calls"], 1)
        self.assertEqual(collector.result[inner_loc]["cumulative_calls"], 1)

        # Other frames should only have cumulative
        middle_loc = ("test.py", 20, "middle_func")
        self.assertEqual(collector.result[middle_loc]["direct_calls"], 0)
        self.assertEqual(collector.result[middle_loc]["cumulative_calls"], 1)

        outer_loc = ("test.py", 30, "outer_func")
        self.assertEqual(collector.result[outer_loc]["direct_calls"], 0)
        self.assertEqual(collector.result[outer_loc]["cumulative_calls"], 1)

    def test_process_empty_frames(self):
        """Test processing empty frames list."""
        collector = LiveStatsCollector(1000)
        collector.process_frames([])
        # Should not raise an error and result should remain empty
        self.assertEqual(len(collector.result), 0)

    def test_process_frames_accumulation(self):
        """Test that multiple calls accumulate correctly."""
        collector = LiveStatsCollector(1000)
        frames = [MockFrameInfo("test.py", 10, "test_func")]

        collector.process_frames(frames)
        collector.process_frames(frames)
        collector.process_frames(frames)

        location = ("test.py", 10, "test_func")
        self.assertEqual(collector.result[location]["direct_calls"], 3)
        self.assertEqual(collector.result[location]["cumulative_calls"], 3)

    def test_process_frames_with_thread_id(self):
        """Test processing frames with per-thread tracking."""
        collector = LiveStatsCollector(1000)
        frames = [MockFrameInfo("test.py", 10, "test_func")]

        # Process frames with thread_id
        collector.process_frames(frames, thread_id=123)

        # Check aggregated result
        location = ("test.py", 10, "test_func")
        self.assertEqual(collector.result[location]["direct_calls"], 1)
        self.assertEqual(collector.result[location]["cumulative_calls"], 1)

        # Check per-thread result
        self.assertIn(123, collector.per_thread_data)
        self.assertEqual(
            collector.per_thread_data[123].result[location]["direct_calls"], 1
        )
        self.assertEqual(
            collector.per_thread_data[123].result[location]["cumulative_calls"], 1
        )

    def test_process_frames_multiple_threads(self):
        """Test processing frames from multiple threads."""
        collector = LiveStatsCollector(1000)
        frames1 = [MockFrameInfo("test.py", 10, "test_func")]
        frames2 = [MockFrameInfo("test.py", 20, "other_func")]

        # Process frames from different threads
        collector.process_frames(frames1, thread_id=123)
        collector.process_frames(frames2, thread_id=456)

        # Check that both threads have their own data
        self.assertIn(123, collector.per_thread_data)
        self.assertIn(456, collector.per_thread_data)

        loc1 = ("test.py", 10, "test_func")
        loc2 = ("test.py", 20, "other_func")

        # Thread 123 should only have func1
        self.assertEqual(
            collector.per_thread_data[123].result[loc1]["direct_calls"], 1
        )
        self.assertNotIn(loc2, collector.per_thread_data[123].result)

        # Thread 456 should only have func2
        self.assertEqual(
            collector.per_thread_data[456].result[loc2]["direct_calls"], 1
        )
        self.assertNotIn(loc1, collector.per_thread_data[456].result)


class TestLiveStatsCollectorCollect(unittest.TestCase):
    """Tests for the collect method."""

    def test_collect_initializes_start_time(self):
        """Test that collect initializes start_time on first call."""
        collector = LiveStatsCollector(1000)
        self.assertIsNone(collector.start_time)

        # Create mock stack frames
        thread_info = MockThreadInfo(123, [])
        interpreter_info = MockInterpreterInfo(0, [thread_info])
        stack_frames = [interpreter_info]

        collector.collect(stack_frames)
        self.assertIsNotNone(collector.start_time)

    def test_collect_increments_sample_count(self):
        """Test that collect increments total_samples."""
        collector = LiveStatsCollector(1000)
        thread_info = MockThreadInfo(123, [])
        interpreter_info = MockInterpreterInfo(0, [thread_info])
        stack_frames = [interpreter_info]

        self.assertEqual(collector.total_samples, 0)
        collector.collect(stack_frames)
        self.assertEqual(collector.total_samples, 1)
        collector.collect(stack_frames)
        self.assertEqual(collector.total_samples, 2)

    def test_collect_with_frames(self):
        """Test collect with actual frame data."""
        collector = LiveStatsCollector(1000)
        frames = [MockFrameInfo("test.py", 10, "test_func")]
        thread_info = MockThreadInfo(123, frames)
        interpreter_info = MockInterpreterInfo(0, [thread_info])
        stack_frames = [interpreter_info]

        collector.collect(stack_frames)

        location = ("test.py", 10, "test_func")
        self.assertEqual(collector.result[location]["direct_calls"], 1)
        self.assertEqual(collector.successful_samples, 1)
        self.assertEqual(collector.failed_samples, 0)

    def test_collect_with_empty_frames(self):
        """Test collect with empty frames."""
        collector = LiveStatsCollector(1000)
        thread_info = MockThreadInfo(123, [])
        interpreter_info = MockInterpreterInfo(0, [thread_info])
        stack_frames = [interpreter_info]

        collector.collect(stack_frames)

        # Empty frames still count as successful since collect() was called successfully
        self.assertEqual(collector.successful_samples, 1)
        self.assertEqual(collector.failed_samples, 0)

    def test_collect_skip_idle_threads(self):
        """Test that idle threads are skipped when skip_idle=True."""
        collector = LiveStatsCollector(1000, skip_idle=True)

        frames = [MockFrameInfo("test.py", 10, "test_func")]
        running_thread = MockThreadInfo(
            123, frames, status=THREAD_STATUS_HAS_GIL | THREAD_STATUS_ON_CPU
        )
        idle_thread = MockThreadInfo(124, frames, status=0)  # No flags = idle
        interpreter_info = MockInterpreterInfo(
            0, [running_thread, idle_thread]
        )
        stack_frames = [interpreter_info]

        collector.collect(stack_frames)

        # Only one thread should be processed
        location = ("test.py", 10, "test_func")
        self.assertEqual(collector.result[location]["direct_calls"], 1)

    def test_collect_multiple_threads(self):
        """Test collect with multiple threads."""
        collector = LiveStatsCollector(1000)

        frames1 = [MockFrameInfo("test1.py", 10, "func1")]
        frames2 = [MockFrameInfo("test2.py", 20, "func2")]
        thread1 = MockThreadInfo(123, frames1)
        thread2 = MockThreadInfo(124, frames2)
        interpreter_info = MockInterpreterInfo(0, [thread1, thread2])
        stack_frames = [interpreter_info]

        collector.collect(stack_frames)

        loc1 = ("test1.py", 10, "func1")
        loc2 = ("test2.py", 20, "func2")
        self.assertEqual(collector.result[loc1]["direct_calls"], 1)
        self.assertEqual(collector.result[loc2]["direct_calls"], 1)

        # Check thread IDs are tracked
        self.assertIn(123, collector.thread_ids)
        self.assertIn(124, collector.thread_ids)


class TestLiveStatsCollectorStatisticsBuilding(unittest.TestCase):
    """Tests for statistics building and sorting."""

    def setUp(self):
        """Set up test fixtures."""
        self.collector = LiveStatsCollector(1000)
        # Add some test data
        self.collector.result[("file1.py", 10, "func1")] = {
            "direct_calls": 100,
            "cumulative_calls": 150,
            "total_rec_calls": 0,
        }
        self.collector.result[("file2.py", 20, "func2")] = {
            "direct_calls": 50,
            "cumulative_calls": 200,
            "total_rec_calls": 0,
        }
        self.collector.result[("file3.py", 30, "func3")] = {
            "direct_calls": 75,
            "cumulative_calls": 75,
            "total_rec_calls": 0,
        }
        self.collector.total_samples = 300

    def test_build_stats_list(self):
        """Test that stats list is built correctly."""
        stats_list = self.collector.build_stats_list()
        self.assertEqual(len(stats_list), 3)

        # Check that all expected keys are present
        for stat in stats_list:
            self.assertIn("func", stat)
            self.assertIn("direct_calls", stat)
            self.assertIn("cumulative_calls", stat)
            self.assertIn("total_time", stat)
            self.assertIn("cumulative_time", stat)

    def test_sort_by_nsamples(self):
        """Test sorting by number of samples."""
        self.collector.sort_by = "nsamples"
        stats_list = self.collector.build_stats_list()

        # Should be sorted by direct_calls descending
        self.assertEqual(stats_list[0]["func"][2], "func1")  # 100 samples
        self.assertEqual(stats_list[1]["func"][2], "func3")  # 75 samples
        self.assertEqual(stats_list[2]["func"][2], "func2")  # 50 samples

    def test_sort_by_tottime(self):
        """Test sorting by total time."""
        self.collector.sort_by = "tottime"
        stats_list = self.collector.build_stats_list()

        # Should be sorted by total_time descending
        # total_time = direct_calls * sample_interval_sec
        self.assertEqual(stats_list[0]["func"][2], "func1")
        self.assertEqual(stats_list[1]["func"][2], "func3")
        self.assertEqual(stats_list[2]["func"][2], "func2")

    def test_sort_by_cumtime(self):
        """Test sorting by cumulative time."""
        self.collector.sort_by = "cumtime"
        stats_list = self.collector.build_stats_list()

        # Should be sorted by cumulative_time descending
        self.assertEqual(stats_list[0]["func"][2], "func2")  # 200 cumulative
        self.assertEqual(stats_list[1]["func"][2], "func1")  # 150 cumulative
        self.assertEqual(stats_list[2]["func"][2], "func3")  # 75 cumulative

    def test_sort_by_sample_pct(self):
        """Test sorting by sample percentage."""
        self.collector.sort_by = "sample_pct"
        stats_list = self.collector.build_stats_list()

        # Should be sorted by percentage of direct_calls
        self.assertEqual(stats_list[0]["func"][2], "func1")  # 33.3%
        self.assertEqual(stats_list[1]["func"][2], "func3")  # 25%
        self.assertEqual(stats_list[2]["func"][2], "func2")  # 16.7%

    def test_sort_by_cumul_pct(self):
        """Test sorting by cumulative percentage."""
        self.collector.sort_by = "cumul_pct"
        stats_list = self.collector.build_stats_list()

        # Should be sorted by percentage of cumulative_calls
        self.assertEqual(stats_list[0]["func"][2], "func2")  # 66.7%
        self.assertEqual(stats_list[1]["func"][2], "func1")  # 50%
        self.assertEqual(stats_list[2]["func"][2], "func3")  # 25%


class TestLiveStatsCollectorSortCycle(unittest.TestCase):
    """Tests for sort mode cycling."""

    def test_cycle_sort_from_nsamples(self):
        """Test cycling from nsamples."""
        collector = LiveStatsCollector(1000, sort_by="nsamples")
        collector._cycle_sort()
        self.assertEqual(collector.sort_by, "sample_pct")

    def test_cycle_sort_from_sample_pct(self):
        """Test cycling from sample_pct."""
        collector = LiveStatsCollector(1000, sort_by="sample_pct")
        collector._cycle_sort()
        self.assertEqual(collector.sort_by, "tottime")

    def test_cycle_sort_from_tottime(self):
        """Test cycling from tottime."""
        collector = LiveStatsCollector(1000, sort_by="tottime")
        collector._cycle_sort()
        self.assertEqual(collector.sort_by, "cumul_pct")

    def test_cycle_sort_from_cumul_pct(self):
        """Test cycling from cumul_pct."""
        collector = LiveStatsCollector(1000, sort_by="cumul_pct")
        collector._cycle_sort()
        self.assertEqual(collector.sort_by, "cumtime")

    def test_cycle_sort_from_cumtime(self):
        """Test cycling from cumtime back to nsamples."""
        collector = LiveStatsCollector(1000, sort_by="cumtime")
        collector._cycle_sort()
        self.assertEqual(collector.sort_by, "nsamples")

    def test_cycle_sort_invalid_mode(self):
        """Test cycling from invalid mode resets to nsamples."""
        collector = LiveStatsCollector(1000)
        collector.sort_by = "invalid_mode"
        collector._cycle_sort()
        self.assertEqual(collector.sort_by, "nsamples")

    def test_cycle_sort_backward_from_nsamples(self):
        """Test cycling backward from nsamples goes to cumtime."""
        collector = LiveStatsCollector(1000, sort_by="nsamples")
        collector._cycle_sort(reverse=True)
        self.assertEqual(collector.sort_by, "cumtime")

    def test_cycle_sort_backward_from_cumtime(self):
        """Test cycling backward from cumtime goes to cumul_pct."""
        collector = LiveStatsCollector(1000, sort_by="cumtime")
        collector._cycle_sort(reverse=True)
        self.assertEqual(collector.sort_by, "cumul_pct")

    def test_cycle_sort_backward_from_sample_pct(self):
        """Test cycling backward from sample_pct goes to nsamples."""
        collector = LiveStatsCollector(1000, sort_by="sample_pct")
        collector._cycle_sort(reverse=True)
        self.assertEqual(collector.sort_by, "nsamples")

    def test_input_lowercase_s_cycles_forward(self):
        """Test that lowercase 's' cycles forward."""
        display = MockDisplay()
        collector = LiveStatsCollector(
            1000, sort_by="nsamples", display=display
        )

        display.simulate_input(ord("s"))
        collector._handle_input()

        self.assertEqual(collector.sort_by, "sample_pct")

    def test_input_uppercase_s_cycles_backward(self):
        """Test that uppercase 'S' cycles backward."""
        display = MockDisplay()
        collector = LiveStatsCollector(
            1000, sort_by="nsamples", display=display
        )

        display.simulate_input(ord("S"))
        collector._handle_input()

        self.assertEqual(collector.sort_by, "cumtime")


class TestLiveStatsCollectorFormatting(unittest.TestCase):
    """Tests for formatting methods."""

    def test_format_uptime_seconds(self):
        """Test uptime formatting for seconds only."""
        collector = LiveStatsCollector(1000, display=MockDisplay())
        colors = collector._setup_colors()
        collector._initialize_widgets(colors)
        self.assertEqual(collector.header_widget.format_uptime(45), "0m45s")

    def test_format_uptime_minutes(self):
        """Test uptime formatting for minutes."""
        collector = LiveStatsCollector(1000, display=MockDisplay())
        colors = collector._setup_colors()
        collector._initialize_widgets(colors)
        self.assertEqual(collector.header_widget.format_uptime(125), "2m05s")

    def test_format_uptime_hours(self):
        """Test uptime formatting for hours."""
        collector = LiveStatsCollector(1000, display=MockDisplay())
        colors = collector._setup_colors()
        collector._initialize_widgets(colors)
        self.assertEqual(
            collector.header_widget.format_uptime(3661), "1h01m01s"
        )

    def test_format_uptime_large_values(self):
        """Test uptime formatting for large time values."""
        collector = LiveStatsCollector(1000, display=MockDisplay())
        colors = collector._setup_colors()
        collector._initialize_widgets(colors)
        self.assertEqual(
            collector.header_widget.format_uptime(86400), "24h00m00s"
        )

    def test_format_uptime_zero(self):
        """Test uptime formatting for zero."""
        collector = LiveStatsCollector(1000, display=MockDisplay())
        colors = collector._setup_colors()
        collector._initialize_widgets(colors)
        self.assertEqual(collector.header_widget.format_uptime(0), "0m00s")


if __name__ == "__main__":
    unittest.main()
