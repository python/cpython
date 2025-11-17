import os
import sys
import time
import unittest
from unittest import mock
from test.support import requires
from test.support.import_helper import import_module

# Only run these tests if curses is available
requires('curses')
curses = import_module('curses')

from profiling.sampling.live_collector import LiveStatsCollector, MockDisplay
from profiling.sampling.collector import (
    THREAD_STATE_RUNNING,
    THREAD_STATE_IDLE,
)


class MockFrameInfo:
    """Mock FrameInfo for testing."""

    def __init__(self, filename, lineno, funcname):
        self.filename = filename
        self.lineno = lineno
        self.funcname = funcname

    def __repr__(self):
        return f"MockFrameInfo(filename='{self.filename}', lineno={self.lineno}, funcname='{self.funcname}')"


class MockThreadInfo:
    """Mock ThreadInfo for testing."""

    def __init__(self, thread_id, frame_info, status=THREAD_STATE_RUNNING):
        self.thread_id = thread_id
        self.frame_info = frame_info
        self.status = status

    def __repr__(self):
        return f"MockThreadInfo(thread_id={self.thread_id}, frame_info={self.frame_info}, status={self.status})"


class MockInterpreterInfo:
    """Mock InterpreterInfo for testing."""

    def __init__(self, interpreter_id, threads):
        self.interpreter_id = interpreter_id
        self.threads = threads

    def __repr__(self):
        return f"MockInterpreterInfo(interpreter_id={self.interpreter_id}, threads={self.threads})"


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
            simplified = collector._simplify_path(test_path)
            # Should remove the stdlib prefix
            self.assertNotIn(stdlib_dir, simplified)
            self.assertIn("json", simplified)

    def test_simplify_unknown_path(self):
        """Test that unknown paths are returned unchanged."""
        collector = LiveStatsCollector(1000)
        test_path = "/some/unknown/path/file.py"
        simplified = collector._simplify_path(test_path)
        self.assertEqual(simplified, test_path)



class TestLiveStatsCollectorFrameProcessing(unittest.TestCase):
    """Tests for frame processing functionality."""

    def test_process_single_frame(self):
        """Test processing a single frame."""
        collector = LiveStatsCollector(1000)
        frames = [MockFrameInfo("test.py", 10, "test_func")]
        collector._process_frames(frames)

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
        collector._process_frames(frames)

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
        collector._process_frames([])
        # Should not raise an error and result should remain empty
        self.assertEqual(len(collector.result), 0)

    def test_process_frames_accumulation(self):
        """Test that multiple calls accumulate correctly."""
        collector = LiveStatsCollector(1000)
        frames = [MockFrameInfo("test.py", 10, "test_func")]

        collector._process_frames(frames)
        collector._process_frames(frames)
        collector._process_frames(frames)

        location = ("test.py", 10, "test_func")
        self.assertEqual(collector.result[location]["direct_calls"], 3)
        self.assertEqual(collector.result[location]["cumulative_calls"], 3)


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
        self.assertEqual(collector._successful_samples, 1)
        self.assertEqual(collector._failed_samples, 0)

    def test_collect_with_empty_frames(self):
        """Test collect with empty frames."""
        collector = LiveStatsCollector(1000)
        thread_info = MockThreadInfo(123, [])
        interpreter_info = MockInterpreterInfo(0, [thread_info])
        stack_frames = [interpreter_info]

        collector.collect(stack_frames)

        self.assertEqual(collector._successful_samples, 0)
        self.assertEqual(collector._failed_samples, 1)

    def test_collect_skip_idle_threads(self):
        """Test that idle threads are skipped when skip_idle=True."""
        collector = LiveStatsCollector(1000, skip_idle=True)

        frames = [MockFrameInfo("test.py", 10, "test_func")]
        running_thread = MockThreadInfo(
            123, frames, status=THREAD_STATE_RUNNING
        )
        idle_thread = MockThreadInfo(124, frames, status=THREAD_STATE_IDLE)
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
        stats_list = self.collector._build_stats_list()
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
        stats_list = self.collector._build_stats_list()

        # Should be sorted by direct_calls descending
        self.assertEqual(stats_list[0]["func"][2], "func1")  # 100 samples
        self.assertEqual(stats_list[1]["func"][2], "func3")  # 75 samples
        self.assertEqual(stats_list[2]["func"][2], "func2")  # 50 samples

    def test_sort_by_tottime(self):
        """Test sorting by total time."""
        self.collector.sort_by = "tottime"
        stats_list = self.collector._build_stats_list()

        # Should be sorted by total_time descending
        # total_time = direct_calls * sample_interval_sec
        self.assertEqual(stats_list[0]["func"][2], "func1")
        self.assertEqual(stats_list[1]["func"][2], "func3")
        self.assertEqual(stats_list[2]["func"][2], "func2")

    def test_sort_by_cumtime(self):
        """Test sorting by cumulative time."""
        self.collector.sort_by = "cumtime"
        stats_list = self.collector._build_stats_list()

        # Should be sorted by cumulative_time descending
        self.assertEqual(stats_list[0]["func"][2], "func2")  # 200 cumulative
        self.assertEqual(stats_list[1]["func"][2], "func1")  # 150 cumulative
        self.assertEqual(stats_list[2]["func"][2], "func3")  # 75 cumulative

    def test_sort_by_sample_pct(self):
        """Test sorting by sample percentage."""
        self.collector.sort_by = "sample_pct"
        stats_list = self.collector._build_stats_list()

        # Should be sorted by percentage of direct_calls
        self.assertEqual(stats_list[0]["func"][2], "func1")  # 33.3%
        self.assertEqual(stats_list[1]["func"][2], "func3")  # 25%
        self.assertEqual(stats_list[2]["func"][2], "func2")  # 16.7%

    def test_sort_by_cumul_pct(self):
        """Test sorting by cumulative percentage."""
        self.collector.sort_by = "cumul_pct"
        stats_list = self.collector._build_stats_list()

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


class TestLiveStatsCollectorFormatting(unittest.TestCase):
    """Tests for formatting methods."""

    def test_format_uptime_seconds(self):
        """Test uptime formatting for seconds only."""
        collector = LiveStatsCollector(1000, display=MockDisplay())
        colors = collector._setup_colors()
        collector._initialize_widgets(colors)
        self.assertEqual(collector._header_widget.format_uptime(45), "0m45s")

    def test_format_uptime_minutes(self):
        """Test uptime formatting for minutes."""
        collector = LiveStatsCollector(1000, display=MockDisplay())
        colors = collector._setup_colors()
        collector._initialize_widgets(colors)
        self.assertEqual(collector._header_widget.format_uptime(125), "2m05s")

    def test_format_uptime_hours(self):
        """Test uptime formatting for hours."""
        collector = LiveStatsCollector(1000, display=MockDisplay())
        colors = collector._setup_colors()
        collector._initialize_widgets(colors)
        self.assertEqual(
            collector._header_widget.format_uptime(3661), "1h01m01s"
        )

    def test_format_uptime_large_values(self):
        """Test uptime formatting for large time values."""
        collector = LiveStatsCollector(1000, display=MockDisplay())
        colors = collector._setup_colors()
        collector._initialize_widgets(colors)
        self.assertEqual(
            collector._header_widget.format_uptime(86400), "24h00m00s"
        )

    def test_format_uptime_zero(self):
        """Test uptime formatting for zero."""
        collector = LiveStatsCollector(1000, display=MockDisplay())
        colors = collector._setup_colors()
        collector._initialize_widgets(colors)
        self.assertEqual(collector._header_widget.format_uptime(0), "0m00s")


class TestLiveStatsCollectorWithMockDisplay(unittest.TestCase):
    """Tests for display functionality using MockDisplay."""

    def setUp(self):
        """Set up collector with mock display."""
        self.mock_display = MockDisplay(height=40, width=160)
        self.collector = LiveStatsCollector(
            1000, pid=12345, display=self.mock_display
        )
        self.collector.start_time = time.perf_counter()

    def test_update_display_with_mock(self):
        """Test that update_display works with MockDisplay."""
        self.collector.total_samples = 100
        self.collector.result[("test.py", 10, "test_func")] = {
            "direct_calls": 50,
            "cumulative_calls": 75,
            "total_rec_calls": 0,
        }

        self.collector._update_display()

        # Verify display operations were called
        self.assertTrue(self.mock_display.cleared)
        self.assertTrue(self.mock_display.refreshed)
        self.assertTrue(self.mock_display.redrawn)

        # Verify some content was written
        self.assertGreater(len(self.mock_display.buffer), 0)

    def test_handle_input_quit(self):
        """Test that 'q' input stops the collector."""
        self.mock_display.simulate_input(ord("q"))
        self.collector._handle_input()
        self.assertFalse(self.collector.running)

    def test_handle_input_sort_cycle(self):
        """Test that 's' input cycles sort mode."""
        self.collector.sort_by = "tottime"
        self.mock_display.simulate_input(ord("s"))
        self.collector._handle_input()
        self.assertEqual(self.collector.sort_by, "cumul_pct")

    def test_draw_methods_with_mock_display(self):
        """Test that draw methods write to mock display."""
        self.collector.total_samples = 500
        self.collector._successful_samples = 450
        self.collector._failed_samples = 50

        colors = self.collector._setup_colors()
        self.collector._initialize_widgets(colors)

        # Test individual widget methods
        line = self.collector._header_widget.draw_header_info(0, 160, 100.5)
        self.assertEqual(line, 2)  # Title + header info line
        self.assertGreater(len(self.mock_display.buffer), 0)

        # Clear buffer and test next method
        self.mock_display.buffer.clear()
        line = self.collector._header_widget.draw_sample_stats(0, 160, 10.0)
        self.assertEqual(line, 1)
        self.assertGreater(len(self.mock_display.buffer), 0)

    def test_terminal_too_small_message(self):
        """Test terminal too small warning."""
        small_display = MockDisplay(height=10, width=50)
        self.collector.display = small_display

        self.collector._show_terminal_too_small(10, 50)

        # Should have written warning message
        text = small_display.get_text_at(3, 15)  # Approximate center
        self.assertIsNotNone(text)

    def test_full_display_rendering_with_data(self):
        """Test complete display rendering with realistic data."""
        # Add multiple functions with different call counts
        self.collector.total_samples = 1000
        self.collector._successful_samples = 950
        self.collector._failed_samples = 50

        self.collector.result[("app.py", 10, "main")] = {
            "direct_calls": 100,
            "cumulative_calls": 500,
            "total_rec_calls": 0,
        }
        self.collector.result[("utils.py", 20, "helper")] = {
            "direct_calls": 300,
            "cumulative_calls": 400,
            "total_rec_calls": 0,
        }
        self.collector.result[("db.py", 30, "query")] = {
            "direct_calls": 50,
            "cumulative_calls": 100,
            "total_rec_calls": 0,
        }

        self.collector._update_display()

        # Verify the display has content
        self.assertGreater(len(self.mock_display.buffer), 10)

        # Verify PID is shown
        found_pid = False
        for (line, col), (text, attr) in self.mock_display.buffer.items():
            if "12345" in text:
                found_pid = True
                break
        self.assertTrue(found_pid, "PID should be displayed")

    def test_efficiency_bar_visualization(self):
        """Test that efficiency bar shows correct proportions."""
        self.collector.total_samples = 100
        self.collector._successful_samples = 75
        self.collector._failed_samples = 25

        colors = self.collector._setup_colors()
        self.collector._initialize_widgets(colors)
        self.collector._header_widget.draw_efficiency_bar(0, 160)

        # Check that something was drawn to the display
        self.assertGreater(len(self.mock_display.buffer), 0)

    def test_stats_display_with_different_sort_modes(self):
        """Test that stats are displayed correctly with different sort modes."""
        self.collector.total_samples = 100
        self.collector.result[("a.py", 1, "func_a")] = {
            "direct_calls": 10,
            "cumulative_calls": 20,
            "total_rec_calls": 0,
        }
        self.collector.result[("b.py", 2, "func_b")] = {
            "direct_calls": 30,
            "cumulative_calls": 40,
            "total_rec_calls": 0,
        }

        # Test each sort mode
        for sort_mode in [
            "nsamples",
            "tottime",
            "cumtime",
            "sample_pct",
            "cumul_pct",
        ]:
            self.mock_display.buffer.clear()
            self.collector.sort_by = sort_mode

            stats_list = self.collector._build_stats_list()
            self.assertEqual(len(stats_list), 2)

            # Verify sorting worked (func_b should be first for most modes)
            if sort_mode in ["nsamples", "tottime", "sample_pct"]:
                self.assertEqual(stats_list[0]["func"][2], "func_b")

    def test_narrow_terminal_column_hiding(self):
        """Test that columns are hidden on narrow terminals."""
        narrow_display = MockDisplay(height=40, width=70)
        collector = LiveStatsCollector(1000, pid=12345, display=narrow_display)
        collector.start_time = time.perf_counter()

        colors = collector._setup_colors()
        collector._initialize_widgets(colors)
        line, show_sample_pct, show_tottime, show_cumul_pct, show_cumtime = (
            collector._table_widget.draw_column_headers(0, 70)
        )

        # On narrow terminal, some columns should be hidden
        self.assertFalse(
            show_cumul_pct or show_cumtime,
            "Some columns should be hidden on narrow terminal",
        )

    def test_very_narrow_terminal_minimal_columns(self):
        """Test minimal display on very narrow terminal."""
        very_narrow = MockDisplay(height=40, width=60)
        collector = LiveStatsCollector(1000, pid=12345, display=very_narrow)
        collector.start_time = time.perf_counter()

        colors = collector._setup_colors()
        collector._initialize_widgets(colors)
        line, show_sample_pct, show_tottime, show_cumul_pct, show_cumtime = (
            collector._table_widget.draw_column_headers(0, 60)
        )

        # Very narrow should hide even more columns
        self.assertFalse(
            show_sample_pct,
            "Sample % should be hidden on very narrow terminal",
        )

    def test_display_updates_only_at_interval(self):
        """Test that display updates respect the update interval."""
        # Create collector with display
        collector = LiveStatsCollector(1000, display=self.mock_display)

        # Simulate multiple rapid collections
        thread_info = MockThreadInfo(123, [])
        interpreter_info = MockInterpreterInfo(0, [thread_info])
        stack_frames = [interpreter_info]

        # First collect should update display
        collector.collect(stack_frames)
        first_cleared = self.mock_display.cleared

        # Reset flags
        self.mock_display.cleared = False
        self.mock_display.refreshed = False

        # Immediate second collect should NOT update display (too soon)
        collector.collect(stack_frames)
        self.assertFalse(
            self.mock_display.cleared,
            "Display should not update too frequently",
        )

    def test_top_functions_display(self):
        """Test that top functions are highlighted correctly."""
        self.collector.total_samples = 1000

        # Create functions with different sample counts
        for i in range(10):
            self.collector.result[(f"file{i}.py", i * 10, f"func{i}")] = {
                "direct_calls": (10 - i) * 10,  # Decreasing counts
                "cumulative_calls": (10 - i) * 20,
                "total_rec_calls": 0,
            }

        colors = self.collector._setup_colors()
        self.collector._initialize_widgets(colors)
        stats_list = self.collector._build_stats_list()

        self.collector._header_widget.draw_top_functions(0, 160, stats_list)

        # Top functions section should have written something
        self.assertGreater(len(self.mock_display.buffer), 0)


class TestLiveStatsCollectorCursesIntegration(unittest.TestCase):
    """Tests for curses-related functionality using mocks."""

    def setUp(self):
        """Set up mock curses screen."""
        self.mock_stdscr = mock.MagicMock()
        self.mock_stdscr.getmaxyx.return_value = (40, 160)  # height, width
        self.mock_stdscr.getch.return_value = -1  # No input
        # Save original stdout/stderr
        self._orig_stdout = sys.stdout
        self._orig_stderr = sys.stderr

    def tearDown(self):
        """Restore stdout/stderr if changed."""
        sys.stdout = self._orig_stdout
        sys.stderr = self._orig_stderr

    def test_init_curses(self):
        """Test curses initialization."""
        collector = LiveStatsCollector(1000)

        with (
            mock.patch("curses.curs_set"),
            mock.patch("curses.has_colors", return_value=True),
            mock.patch("curses.start_color"),
            mock.patch("curses.use_default_colors"),
            mock.patch("builtins.open", mock.mock_open()) as mock_open_func,
        ):
            collector.init_curses(self.mock_stdscr)

            self.assertIsNotNone(collector.stdscr)
            self.mock_stdscr.nodelay.assert_called_with(True)
            self.mock_stdscr.scrollok.assert_called_with(False)

            # Clean up properly
            if collector._devnull:
                collector._devnull.close()
            collector._saved_stdout = None
            collector._saved_stderr = None

    def test_cleanup_curses(self):
        """Test curses cleanup."""
        mock_display = MockDisplay()
        collector = LiveStatsCollector(1000, display=mock_display)
        collector.stdscr = self.mock_stdscr

        # Mock devnull file to avoid resource warnings
        mock_devnull = mock.MagicMock()
        mock_saved_stdout = mock.MagicMock()
        mock_saved_stderr = mock.MagicMock()

        collector._devnull = mock_devnull
        collector._saved_stdout = mock_saved_stdout
        collector._saved_stderr = mock_saved_stderr

        with mock.patch("curses.curs_set"):
            collector.cleanup_curses()

        mock_devnull.close.assert_called_once()
        # Verify stdout/stderr were set back to the saved values
        self.assertEqual(sys.stdout, mock_saved_stdout)
        self.assertEqual(sys.stderr, mock_saved_stderr)
        # Verify the saved values were cleared
        self.assertIsNone(collector._saved_stdout)
        self.assertIsNone(collector._saved_stderr)
        self.assertIsNone(collector._devnull)

    def test_add_str_with_mock_display(self):
        """Test safe_addstr with MockDisplay."""
        mock_display = MockDisplay(height=40, width=160)
        collector = LiveStatsCollector(1000, display=mock_display)
        colors = collector._setup_colors()
        collector._initialize_widgets(colors)

        collector._header_widget.add_str(5, 10, "Test", 0)
        # Verify it was added to the buffer
        self.assertIn((5, 10), mock_display.buffer)

    def test_setup_colors_with_color_support(self):
        """Test color setup when colors are supported."""
        mock_display = MockDisplay(height=40, width=160)
        mock_display.colors_supported = True
        collector = LiveStatsCollector(1000, display=mock_display)

        colors = collector._setup_colors()

        self.assertIn("header", colors)
        self.assertIn("cyan", colors)
        self.assertIn("yellow", colors)
        self.assertIn("green", colors)
        self.assertIn("magenta", colors)
        self.assertIn("red", colors)

    def test_setup_colors_without_color_support(self):
        """Test color setup when colors are not supported."""
        mock_display = MockDisplay(height=40, width=160)
        mock_display.colors_supported = False
        collector = LiveStatsCollector(1000, display=mock_display)

        colors = collector._setup_colors()

        # Should still have all keys but with fallback values
        self.assertIn("header", colors)
        self.assertIn("cyan", colors)

    def test_handle_input_quit(self):
        """Test handling 'q' key to quit."""
        mock_display = MockDisplay()
        mock_display.simulate_input(ord("q"))
        collector = LiveStatsCollector(1000, display=mock_display)

        self.assertTrue(collector.running)
        collector._handle_input()
        self.assertFalse(collector.running)

    def test_handle_input_quit_uppercase(self):
        """Test handling 'Q' key to quit."""
        mock_display = MockDisplay()
        mock_display.simulate_input(ord("Q"))
        collector = LiveStatsCollector(1000, display=mock_display)

        self.assertTrue(collector.running)
        collector._handle_input()
        self.assertFalse(collector.running)

    def test_handle_input_cycle_sort(self):
        """Test handling 's' key to cycle sort."""
        mock_display = MockDisplay()
        mock_display.simulate_input(ord("s"))
        collector = LiveStatsCollector(
            1000, sort_by="nsamples", display=mock_display
        )

        collector._handle_input()
        self.assertEqual(collector.sort_by, "sample_pct")

    def test_handle_input_cycle_sort_uppercase(self):
        """Test handling 'S' key to cycle sort."""
        mock_display = MockDisplay()
        mock_display.simulate_input(ord("S"))
        collector = LiveStatsCollector(
            1000, sort_by="nsamples", display=mock_display
        )

        collector._handle_input()
        self.assertEqual(collector.sort_by, "sample_pct")

    def test_handle_input_no_key(self):
        """Test handling when no key is pressed."""
        mock_display = MockDisplay()
        collector = LiveStatsCollector(1000, display=mock_display)

        collector._handle_input()
        # Should not change state
        self.assertTrue(collector.running)


class TestLiveStatsCollectorDisplayMethods(unittest.TestCase):
    """Tests for display-related methods."""

    def setUp(self):
        """Set up collector with mock display."""
        self.mock_display = MockDisplay(height=40, width=160)
        self.collector = LiveStatsCollector(
            1000, pid=12345, display=self.mock_display
        )
        self.collector.start_time = time.perf_counter()

    def test_show_terminal_too_small(self):
        """Test terminal too small message display."""
        self.collector._show_terminal_too_small(10, 50)
        # Should have written some content to the display buffer
        self.assertGreater(len(self.mock_display.buffer), 0)

    def test_draw_header_info(self):
        """Test drawing header information."""
        colors = {
            "cyan": curses.A_BOLD,
            "green": curses.A_BOLD,
            "yellow": curses.A_BOLD,
            "magenta": curses.A_BOLD,
        }
        self.collector._initialize_widgets(colors)

        line = self.collector._header_widget.draw_header_info(0, 160, 100.5)
        self.assertEqual(line, 2)  # Title + header info line

    def test_draw_sample_stats(self):
        """Test drawing sample statistics."""
        self.collector.total_samples = 1000
        colors = {"cyan": curses.A_BOLD, "green": curses.A_BOLD}
        self.collector._initialize_widgets(colors)

        line = self.collector._header_widget.draw_sample_stats(0, 160, 10.0)
        self.assertEqual(line, 1)
        self.assertGreater(self.collector._max_sample_rate, 0)

    def test_progress_bar_uses_target_rate(self):
        """Test that progress bar uses target rate instead of max rate."""
        # Set up collector with specific sampling interval
        collector = LiveStatsCollector(10000, pid=12345, display=self.mock_display)  # 10ms = 100Hz target
        collector.start_time = time.perf_counter()
        collector.total_samples = 500
        collector._max_sample_rate = 150  # Higher than target to test we don't use this

        colors = {"cyan": curses.A_BOLD, "green": curses.A_BOLD}
        collector._initialize_widgets(colors)

        # Clear the display buffer to capture only our progress bar content
        self.mock_display.buffer.clear()

        # Draw sample stats with a known elapsed time that gives us a specific sample rate
        elapsed = 10.0  # 500 samples in 10 seconds = 50 samples/second
        line = collector._header_widget.draw_sample_stats(0, 160, elapsed)

        # Verify display was updated
        self.assertEqual(line, 1)
        self.assertGreater(len(self.mock_display.buffer), 0)

        # Verify the label shows current/target format with units instead of "max"
        found_current_target_label = False
        found_max_label = False
        for (line_num, col), (text, attr) in self.mock_display.buffer.items():
            # Should show "50.0Hz/100.0Hz (50.0%)" since we're at 50% of target (50/100)
            if "50.0Hz/100.0Hz" in text and "50.0%" in text:
                found_current_target_label = True
            if "max:" in text:
                found_max_label = True

        self.assertTrue(found_current_target_label, "Should display current/target rate with percentage")
        self.assertFalse(found_max_label, "Should not display max rate label")

    def test_progress_bar_different_intervals(self):
        """Test that progress bar adapts to different sampling intervals."""
        test_cases = [
            (1000, "1.0KHz", "100.0Hz"),    # 1ms interval -> 1000Hz target (1.0KHz), 100Hz current
            (5000, "200.0Hz", "100.0Hz"),   # 5ms interval -> 200Hz target, 100Hz current
            (20000, "50.0Hz", "100.0Hz"),   # 20ms interval -> 50Hz target, 100Hz current
            (100000, "10.0Hz", "100.0Hz"),  # 100ms interval -> 10Hz target, 100Hz current
        ]

        for interval_usec, expected_target_formatted, expected_current_formatted in test_cases:
            with self.subTest(interval=interval_usec):
                collector = LiveStatsCollector(interval_usec, display=MockDisplay())
                collector.start_time = time.perf_counter()
                collector.total_samples = 100

                colors = {"cyan": curses.A_BOLD, "green": curses.A_BOLD}
                collector._initialize_widgets(colors)

                # Clear buffer
                collector.display.buffer.clear()

                # Draw with 1 second elapsed time (gives us current rate of 100Hz)
                collector._header_widget.draw_sample_stats(0, 160, 1.0)

                # Check that the current/target format appears in the display with proper units
                found_current_target_format = False
                for (line_num, col), (text, attr) in collector.display.buffer.items():
                    # Looking for format like "100.0Hz/1.0KHz" or "100.0Hz/200.0Hz"
                    expected_format = f"{expected_current_formatted}/{expected_target_formatted}"
                    if expected_format in text and "%" in text:
                        found_current_target_format = True
                        break

                self.assertTrue(found_current_target_format,
                    f"Should display current/target rate format with units for {interval_usec}µs interval")

    def test_draw_efficiency_bar(self):
        """Test drawing efficiency bar."""
        self.collector._successful_samples = 900
        self.collector._failed_samples = 100
        self.collector.total_samples = 1000
        colors = {"green": curses.A_BOLD, "red": curses.A_BOLD}
        self.collector._initialize_widgets(colors)

        line = self.collector._header_widget.draw_efficiency_bar(0, 160)
        self.assertEqual(line, 1)

    def test_draw_function_stats(self):
        """Test drawing function statistics."""
        self.collector.result[("test.py", 10, "func1")] = {
            "direct_calls": 100,
            "cumulative_calls": 150,
            "total_rec_calls": 0,
        }
        self.collector.result[("test.py", 20, "func2")] = {
            "direct_calls": 0,
            "cumulative_calls": 50,
            "total_rec_calls": 0,
        }

        stats_list = self.collector._build_stats_list()
        colors = {
            "cyan": curses.A_BOLD,
            "green": curses.A_BOLD,
            "yellow": curses.A_BOLD,
            "magenta": curses.A_BOLD,
        }
        self.collector._initialize_widgets(colors)

        line = self.collector._header_widget.draw_function_stats(
            0, 160, stats_list
        )
        self.assertEqual(line, 1)

    def test_draw_top_functions(self):
        """Test drawing top functions."""
        self.collector.total_samples = 300
        self.collector.result[("test.py", 10, "hot_func")] = {
            "direct_calls": 100,
            "cumulative_calls": 150,
            "total_rec_calls": 0,
        }

        stats_list = self.collector._build_stats_list()
        colors = {
            "red": curses.A_BOLD,
            "yellow": curses.A_BOLD,
            "green": curses.A_BOLD,
        }
        self.collector._initialize_widgets(colors)

        line = self.collector._header_widget.draw_top_functions(
            0, 160, stats_list
        )
        self.assertEqual(line, 1)

    def test_draw_column_headers(self):
        """Test drawing column headers."""
        colors = {
            "sorted_header": curses.A_BOLD,
            "normal_header": curses.A_NORMAL,
        }
        self.collector._initialize_widgets(colors)

        (
            line,
            show_sample_pct,
            show_tottime,
            show_cumul_pct,
            show_cumtime,
        ) = self.collector._table_widget.draw_column_headers(0, 160)
        self.assertEqual(line, 1)
        self.assertTrue(show_sample_pct)
        self.assertTrue(show_tottime)
        self.assertTrue(show_cumul_pct)
        self.assertTrue(show_cumtime)

    def test_draw_column_headers_narrow_terminal(self):
        """Test column headers adapt to narrow terminal."""
        colors = {
            "sorted_header": curses.A_BOLD,
            "normal_header": curses.A_NORMAL,
        }
        self.collector._initialize_widgets(colors)

        (
            line,
            show_sample_pct,
            show_tottime,
            show_cumul_pct,
            show_cumtime,
        ) = self.collector._table_widget.draw_column_headers(0, 70)
        self.assertEqual(line, 1)
        # Some columns should be hidden on narrow terminal
        self.assertFalse(show_cumul_pct)

    def test_draw_footer(self):
        """Test drawing footer."""
        colors = self.collector._setup_colors()
        self.collector._initialize_widgets(colors)
        self.collector._footer_widget.render(38, 160)
        # Should have written some content to the display buffer
        self.assertGreater(len(self.mock_display.buffer), 0)

    def test_draw_progress_bar(self):
        """Test progress bar drawing."""
        colors = self.collector._setup_colors()
        self.collector._initialize_widgets(colors)
        bar, length = self.collector._header_widget.progress_bar.render_bar(
            50, 100, 30
        )

        self.assertIn("[", bar)
        self.assertIn("]", bar)
        self.assertGreater(length, 0)
        # Should be roughly 50% filled
        self.assertIn("█", bar)
        self.assertIn("░", bar)


class TestLiveStatsCollectorEdgeCases(unittest.TestCase):
    """Tests for edge cases and error handling."""

    def test_very_long_function_name(self):
        """Test handling of very long function names."""
        collector = LiveStatsCollector(1000)
        long_name = "x" * 200
        collector.result[("test.py", 10, long_name)] = {
            "direct_calls": 10,
            "cumulative_calls": 20,
            "total_rec_calls": 0,
        }

        stats_list = collector._build_stats_list()
        self.assertEqual(len(stats_list), 1)
        self.assertEqual(stats_list[0]["func"][2], long_name)



class TestLiveStatsCollectorUpdateDisplay(unittest.TestCase):
    """Tests for the _update_display method."""

    def setUp(self):
        """Set up collector with mock display."""
        self.mock_display = MockDisplay(height=40, width=160)
        self.collector = LiveStatsCollector(
            1000, pid=12345, display=self.mock_display
        )
        self.collector.start_time = time.perf_counter()

    def test_update_display_terminal_too_small(self):
        """Test update_display when terminal is too small."""
        small_display = MockDisplay(height=10, width=50)
        self.collector.display = small_display

        with mock.patch.object(
            self.collector, "_show_terminal_too_small"
        ) as mock_show:
            self.collector._update_display()
            mock_show.assert_called_once()

    def test_update_display_normal(self):
        """Test normal update_display operation."""
        self.collector.total_samples = 100
        self.collector._successful_samples = 90
        self.collector._failed_samples = 10
        self.collector.result[("test.py", 10, "func")] = {
            "direct_calls": 50,
            "cumulative_calls": 75,
            "total_rec_calls": 0,
        }

        self.collector._update_display()

        self.assertTrue(self.mock_display.cleared)
        self.assertTrue(self.mock_display.refreshed)

    def test_update_display_handles_exception(self):
        """Test that update_display handles exceptions gracefully."""
        # Make one of the methods raise an exception
        with mock.patch.object(
            self.collector,
            "_prepare_display_data",
            side_effect=Exception("Test error"),
        ):
            # Should not raise an exception (it catches and logs via trace_exception)
            try:
                self.collector._update_display()
            except Exception:
                self.fail(
                    "_update_display should handle exceptions gracefully"
                )


class TestLiveCollectorWithMockDisplayHelpers(unittest.TestCase):
    """Tests using the new MockDisplay helper methods."""

    def test_verify_pid_display_with_contains(self):
        """Test verifying PID is displayed using contains_text helper."""
        display = MockDisplay(height=40, width=160)
        collector = LiveStatsCollector(1000, pid=99999, display=display)
        collector.start_time = time.perf_counter()
        collector.total_samples = 10

        collector._update_display()

        # Use the helper method
        self.assertTrue(
            display.contains_text("99999"), "PID should be visible in display"
        )

    def test_verify_function_names_displayed(self):
        """Test verifying function names appear in display."""
        display = MockDisplay(height=40, width=160)
        collector = LiveStatsCollector(1000, pid=12345, display=display)
        collector.start_time = time.perf_counter()

        collector.total_samples = 100
        collector.result[("mymodule.py", 42, "my_special_function")] = {
            "direct_calls": 50,
            "cumulative_calls": 75,
            "total_rec_calls": 0,
        }

        collector._update_display()

        # Verify function name appears
        self.assertTrue(
            display.contains_text("my_special_function"),
            "Function name should be visible",
        )

    def test_get_all_lines_full_display(self):
        """Test getting all lines from a full display render."""
        display = MockDisplay(height=40, width=160)
        collector = LiveStatsCollector(1000, pid=12345, display=display)
        collector.start_time = time.perf_counter()
        collector.total_samples = 100

        collector._update_display()

        lines = display.get_all_lines()

        # Should have multiple lines of content
        self.assertGreater(len(lines), 5)

        # Should have header content
        self.assertTrue(any("PID" in line for line in lines))


class TestLiveCollectorInteractiveControls(unittest.TestCase):
    """Tests for interactive control features."""

    def setUp(self):
        """Set up collector with mock display."""
        self.display = MockDisplay(height=40, width=160)
        self.collector = LiveStatsCollector(
            1000, pid=12345, display=self.display
        )
        self.collector.start_time = time.perf_counter()

    def test_pause_functionality(self):
        """Test pause/resume functionality."""
        self.assertFalse(self.collector.paused)

        # Simulate 'p' key press
        self.display.simulate_input(ord("p"))
        self.collector._handle_input()

        self.assertTrue(self.collector.paused)

        # Press 'p' again to resume
        self.display.simulate_input(ord("p"))
        self.collector._handle_input()

        self.assertFalse(self.collector.paused)

    def test_pause_stops_ui_updates(self):
        """Test that pausing stops UI updates but profiling continues."""
        # Add some data
        self.collector.total_samples = 10
        self.collector.result[("test.py", 1, "func")] = {
            "direct_calls": 5,
            "cumulative_calls": 10,
            "total_rec_calls": 0,
        }

        # Pause
        self.collector.paused = True

        # Simulate a collect call (profiling continues)
        thread_info = MockThreadInfo(123, [])
        interpreter_info = MockInterpreterInfo(0, [thread_info])
        stack_frames = [interpreter_info]

        initial_samples = self.collector.total_samples
        self.collector.collect(stack_frames)

        # Samples should still increment
        self.assertEqual(self.collector.total_samples, initial_samples + 1)

        # But display should not have been updated (buffer stays clear)
        self.display.cleared = False
        self.collector.collect(stack_frames)
        self.assertFalse(
            self.display.cleared, "Display should not update when paused"
        )

    def test_reset_stats(self):
        """Test reset statistics functionality."""
        # Add some stats
        self.collector.total_samples = 100
        self.collector._successful_samples = 90
        self.collector._failed_samples = 10
        self.collector.result[("test.py", 1, "func")] = {
            "direct_calls": 50,
            "cumulative_calls": 75,
            "total_rec_calls": 0,
        }

        # Reset
        self.collector.reset_stats()

        self.assertEqual(self.collector.total_samples, 0)
        self.assertEqual(self.collector._successful_samples, 0)
        self.assertEqual(self.collector._failed_samples, 0)
        self.assertEqual(len(self.collector.result), 0)

    def test_increase_refresh_rate(self):
        """Test increasing refresh rate (faster updates)."""
        from profiling.sampling import live_collector

        initial_interval = live_collector.DISPLAY_UPDATE_INTERVAL

        # Simulate '+' key press (faster = smaller interval)
        self.display.simulate_input(ord("+"))
        self.collector._handle_input()

        self.assertLess(
            live_collector.DISPLAY_UPDATE_INTERVAL, initial_interval
        )

    def test_decrease_refresh_rate(self):
        """Test decreasing refresh rate (slower updates)."""
        from profiling.sampling import live_collector

        initial_interval = live_collector.DISPLAY_UPDATE_INTERVAL

        # Simulate '-' key press (slower = larger interval)
        self.display.simulate_input(ord("-"))
        self.collector._handle_input()

        self.assertGreater(
            live_collector.DISPLAY_UPDATE_INTERVAL, initial_interval
        )

    def test_refresh_rate_minimum(self):
        """Test that refresh rate has a minimum (max speed)."""
        from profiling.sampling import live_collector

        live_collector.DISPLAY_UPDATE_INTERVAL = 0.05  # Set to minimum

        # Try to go faster
        self.display.simulate_input(ord("+"))
        self.collector._handle_input()

        # Should stay at minimum
        self.assertEqual(live_collector.DISPLAY_UPDATE_INTERVAL, 0.05)

    def test_refresh_rate_maximum(self):
        """Test that refresh rate has a maximum (min speed)."""
        from profiling.sampling import live_collector

        live_collector.DISPLAY_UPDATE_INTERVAL = 1.0  # Set to maximum

        # Try to go slower
        self.display.simulate_input(ord("-"))
        self.collector._handle_input()

        # Should stay at maximum
        self.assertEqual(live_collector.DISPLAY_UPDATE_INTERVAL, 1.0)

    def test_help_toggle(self):
        """Test help screen toggle."""
        self.assertFalse(self.collector.show_help)

        # Show help
        self.display.simulate_input(ord("h"))
        self.collector._handle_input()

        self.assertTrue(self.collector.show_help)

        # Pressing any key closes help
        self.display.simulate_input(ord("x"))
        self.collector._handle_input()

        self.assertFalse(self.collector.show_help)

    def test_help_with_question_mark(self):
        """Test help screen with '?' key."""
        self.display.simulate_input(ord("?"))
        self.collector._handle_input()

        self.assertTrue(self.collector.show_help)

    def test_filter_clear(self):
        """Test clearing filter."""
        self.collector.filter_pattern = "test"

        # Clear filter
        self.display.simulate_input(ord("c"))
        self.collector._handle_input()

        self.assertIsNone(self.collector.filter_pattern)

    def test_filter_clear_when_none(self):
        """Test clearing filter when no filter is set."""
        self.assertIsNone(self.collector.filter_pattern)

        # Should not crash
        self.display.simulate_input(ord("c"))
        self.collector._handle_input()

        self.assertIsNone(self.collector.filter_pattern)

    def test_paused_status_in_footer(self):
        """Test that paused status appears in footer."""
        self.collector.total_samples = 10
        self.collector.paused = True

        self.collector._update_display()

        # Check that PAUSED appears in display
        self.assertTrue(self.display.contains_text("PAUSED"))

    def test_filter_status_in_footer(self):
        """Test that filter status appears in footer."""
        self.collector.total_samples = 10
        self.collector.filter_pattern = "mytest"

        self.collector._update_display()

        # Check that filter info appears
        self.assertTrue(self.display.contains_text("Filter"))

    def test_help_screen_display(self):
        """Test that help screen is displayed."""
        self.collector.show_help = True

        self.collector._update_display()

        # Check for help content
        self.assertTrue(self.display.contains_text("Interactive Commands"))

    def test_pause_uppercase(self):
        """Test pause with uppercase 'P' key."""
        self.assertFalse(self.collector.paused)

        self.display.simulate_input(ord("P"))
        self.collector._handle_input()

        self.assertTrue(self.collector.paused)

    def test_help_uppercase(self):
        """Test help with uppercase 'H' key."""
        self.assertFalse(self.collector.show_help)

        self.display.simulate_input(ord("H"))
        self.collector._handle_input()

        self.assertTrue(self.collector.show_help)

    def test_reset_lowercase(self):
        """Test reset with lowercase 'r' key."""
        # Add some stats
        self.collector.total_samples = 100
        self.collector.result[("test.py", 1, "func")] = {
            "direct_calls": 50,
            "cumulative_calls": 75,
            "total_rec_calls": 0,
        }

        self.display.simulate_input(ord("r"))
        self.collector._handle_input()

        self.assertEqual(self.collector.total_samples, 0)
        self.assertEqual(len(self.collector.result), 0)

    def test_reset_uppercase(self):
        """Test reset with uppercase 'R' key."""
        self.collector.total_samples = 100

        self.display.simulate_input(ord("R"))
        self.collector._handle_input()

        self.assertEqual(self.collector.total_samples, 0)

    def test_filter_clear_uppercase(self):
        """Test clearing filter with uppercase 'C' key."""
        self.collector.filter_pattern = "test"

        self.display.simulate_input(ord("C"))
        self.collector._handle_input()

        self.assertIsNone(self.collector.filter_pattern)

    def test_increase_refresh_rate_with_equals(self):
        """Test increasing refresh rate with '=' key."""
        from profiling.sampling import live_collector

        initial_interval = live_collector.DISPLAY_UPDATE_INTERVAL

        # Simulate '=' key press (alternative to '+')
        self.display.simulate_input(ord("="))
        self.collector._handle_input()

        self.assertLess(
            live_collector.DISPLAY_UPDATE_INTERVAL, initial_interval
        )

    def test_decrease_refresh_rate_with_underscore(self):
        """Test decreasing refresh rate with '_' key."""
        from profiling.sampling import live_collector

        initial_interval = live_collector.DISPLAY_UPDATE_INTERVAL

        # Simulate '_' key press (alternative to '-')
        self.display.simulate_input(ord("_"))
        self.collector._handle_input()

        self.assertGreater(
            live_collector.DISPLAY_UPDATE_INTERVAL, initial_interval
        )

    def test_finished_state_displays_banner(self):
        """Test that finished state shows prominent banner."""
        # Add some sample data
        thread_info = MockThreadInfo(
            123,
            [
                MockFrameInfo("test.py", 10, "work"),
                MockFrameInfo("test.py", 20, "main"),
            ],
        )
        interpreter_info = MockInterpreterInfo(0, [thread_info])
        stack_frames = [interpreter_info]
        self.collector.collect(stack_frames)

        # Mark as finished
        self.collector.mark_finished()

        # Check that finished flag is set
        self.assertTrue(self.collector.finished)

        # Check that the banner message is displayed
        self.assertTrue(self.display.contains_text("PROFILING COMPLETE"))
        self.assertTrue(self.display.contains_text("Press 'q' to Quit"))

    def test_finished_state_ignores_most_input(self):
        """Test that finished state only responds to 'q' key."""
        self.collector.finished = True
        self.collector.running = True

        # Try pressing 's' (sort) - should be ignored
        self.display.simulate_input(ord("s"))
        self.collector._handle_input()
        self.assertTrue(self.collector.running)  # Still running

        # Try pressing 'p' (pause) - should be ignored
        self.display.simulate_input(ord("p"))
        self.collector._handle_input()
        self.assertTrue(self.collector.running)  # Still running
        self.assertFalse(self.collector.paused)  # Not paused

        # Try pressing 'r' (reset) - should be ignored
        old_total = self.collector.total_samples = 100
        self.display.simulate_input(ord("r"))
        self.collector._handle_input()
        self.assertEqual(self.collector.total_samples, old_total)  # Not reset

        # Press 'q' - should stop
        self.display.simulate_input(ord("q"))
        self.collector._handle_input()
        self.assertFalse(self.collector.running)  # Stopped

    def test_finished_state_footer_message(self):
        """Test that footer shows appropriate message when finished."""
        # Add some sample data
        thread_info = MockThreadInfo(
            123,
            [
                MockFrameInfo("test.py", 10, "work"),
                MockFrameInfo("test.py", 20, "main"),
            ],
        )
        interpreter_info = MockInterpreterInfo(0, [thread_info])
        stack_frames = [interpreter_info]
        self.collector.collect(stack_frames)

        # Mark as finished
        self.collector.mark_finished()

        # Check that footer contains finished message
        self.assertTrue(self.display.contains_text("PROFILING FINISHED"))


class TestLiveCollectorFiltering(unittest.TestCase):
    """Tests for filtering functionality."""

    def setUp(self):
        """Set up collector with test data."""
        self.display = MockDisplay(height=40, width=160)
        self.collector = LiveStatsCollector(
            1000, pid=12345, display=self.display
        )
        self.collector.start_time = time.perf_counter()
        self.collector.total_samples = 100

        # Add test data
        self.collector.result[("app/models.py", 10, "save")] = {
            "direct_calls": 50,
            "cumulative_calls": 75,
            "total_rec_calls": 0,
        }
        self.collector.result[("app/views.py", 20, "render")] = {
            "direct_calls": 30,
            "cumulative_calls": 40,
            "total_rec_calls": 0,
        }
        self.collector.result[("lib/utils.py", 30, "helper")] = {
            "direct_calls": 20,
            "cumulative_calls": 25,
            "total_rec_calls": 0,
        }

    def test_filter_by_filename(self):
        """Test filtering by filename pattern."""
        self.collector.filter_pattern = "models"

        stats_list = self.collector._build_stats_list()

        # Only models.py should be included
        self.assertEqual(len(stats_list), 1)
        self.assertIn("models.py", stats_list[0]["func"][0])

    def test_filter_by_function_name(self):
        """Test filtering by function name."""
        self.collector.filter_pattern = "render"

        stats_list = self.collector._build_stats_list()

        self.assertEqual(len(stats_list), 1)
        self.assertEqual(stats_list[0]["func"][2], "render")

    def test_filter_case_insensitive(self):
        """Test that filtering is case-insensitive."""
        self.collector.filter_pattern = "MODELS"

        stats_list = self.collector._build_stats_list()

        # Should still match models.py
        self.assertEqual(len(stats_list), 1)

    def test_filter_substring_matching(self):
        """Test substring filtering."""
        self.collector.filter_pattern = "app/"

        stats_list = self.collector._build_stats_list()

        # Should match both app files
        self.assertEqual(len(stats_list), 2)

    def test_no_filter(self):
        """Test with no filter applied."""
        self.collector.filter_pattern = None

        stats_list = self.collector._build_stats_list()

        # All items should be included
        self.assertEqual(len(stats_list), 3)

    def test_filter_partial_function_name(self):
        """Test filtering by partial function name."""
        self.collector.filter_pattern = "save"

        stats_list = self.collector._build_stats_list()

        self.assertEqual(len(stats_list), 1)
        self.assertEqual(stats_list[0]["func"][2], "save")

    def test_filter_combined_filename_funcname(self):
        """Test filtering matches filename:funcname pattern."""
        self.collector.filter_pattern = "views.py:render"

        stats_list = self.collector._build_stats_list()

        # Should match the combined pattern
        self.assertEqual(len(stats_list), 1)
        self.assertEqual(stats_list[0]["func"][2], "render")

    def test_filter_no_matches(self):
        """Test filter that matches nothing."""
        self.collector.filter_pattern = "nonexistent"

        stats_list = self.collector._build_stats_list()

        self.assertEqual(len(stats_list), 0)


class TestLiveCollectorFilterInput(unittest.TestCase):
    """Tests for filter input mode."""

    def setUp(self):
        """Set up collector with mock display."""
        self.display = MockDisplay(height=40, width=160)
        self.collector = LiveStatsCollector(
            1000, pid=12345, display=self.display
        )
        self.collector.start_time = time.perf_counter()

    def test_enter_filter_mode(self):
        """Test entering filter input mode."""
        self.assertFalse(self.collector.filter_input_mode)

        # Press '/' to enter filter mode
        self.display.simulate_input(ord("/"))
        self.collector._handle_input()

        self.assertTrue(self.collector.filter_input_mode)

    def test_filter_input_typing(self):
        """Test typing characters in filter input mode."""
        self.collector.filter_input_mode = True
        self.collector.filter_input_buffer = ""

        # Type 't', 'e', 's', 't'
        for ch in "test":
            self.display.simulate_input(ord(ch))
            self.collector._handle_input()

        self.assertEqual(self.collector.filter_input_buffer, "test")

    def test_filter_input_backspace(self):
        """Test backspace in filter input mode."""
        self.collector.filter_input_mode = True
        self.collector.filter_input_buffer = "test"

        # Press backspace (127)
        self.display.simulate_input(127)
        self.collector._handle_input()

        self.assertEqual(self.collector.filter_input_buffer, "tes")

    def test_filter_input_backspace_alt(self):
        """Test alternative backspace key (263) in filter input mode."""
        self.collector.filter_input_mode = True
        self.collector.filter_input_buffer = "test"

        # Press backspace (263)
        self.display.simulate_input(263)
        self.collector._handle_input()

        self.assertEqual(self.collector.filter_input_buffer, "tes")

    def test_filter_input_backspace_empty(self):
        """Test backspace on empty buffer."""
        self.collector.filter_input_mode = True
        self.collector.filter_input_buffer = ""

        # Press backspace - should not crash
        self.display.simulate_input(127)
        self.collector._handle_input()

        self.assertEqual(self.collector.filter_input_buffer, "")

    def test_filter_input_enter_applies_filter(self):
        """Test pressing Enter applies the filter."""
        self.collector.filter_input_mode = True
        self.collector.filter_input_buffer = "myfilter"

        # Press Enter (10)
        self.display.simulate_input(10)
        self.collector._handle_input()

        self.assertFalse(self.collector.filter_input_mode)
        self.assertEqual(self.collector.filter_pattern, "myfilter")
        self.assertEqual(self.collector.filter_input_buffer, "")

    def test_filter_input_enter_alt(self):
        """Test alternative Enter key (13) applies filter."""
        self.collector.filter_input_mode = True
        self.collector.filter_input_buffer = "myfilter"

        # Press Enter (13)
        self.display.simulate_input(13)
        self.collector._handle_input()

        self.assertFalse(self.collector.filter_input_mode)
        self.assertEqual(self.collector.filter_pattern, "myfilter")

    def test_filter_input_enter_empty_clears_filter(self):
        """Test pressing Enter with empty buffer clears filter."""
        self.collector.filter_input_mode = True
        self.collector.filter_input_buffer = ""
        self.collector.filter_pattern = "oldfilter"

        # Press Enter
        self.display.simulate_input(10)
        self.collector._handle_input()

        self.assertFalse(self.collector.filter_input_mode)
        self.assertIsNone(self.collector.filter_pattern)

    def test_filter_input_escape_cancels(self):
        """Test pressing ESC cancels filter input."""
        self.collector.filter_input_mode = True
        self.collector.filter_input_buffer = "newfilter"
        self.collector.filter_pattern = "oldfilter"

        # Press ESC (27)
        self.display.simulate_input(27)
        self.collector._handle_input()

        self.assertFalse(self.collector.filter_input_mode)
        self.assertEqual(
            self.collector.filter_pattern, "oldfilter"
        )  # Unchanged
        self.assertEqual(self.collector.filter_input_buffer, "")

    def test_filter_input_start_with_existing_filter(self):
        """Test entering filter mode with existing filter pre-fills buffer."""
        self.collector.filter_pattern = "existing"

        # Enter filter mode
        self.display.simulate_input(ord("/"))
        self.collector._handle_input()

        # Buffer should be pre-filled with existing pattern
        self.assertEqual(self.collector.filter_input_buffer, "existing")

    def test_filter_input_start_without_filter(self):
        """Test entering filter mode with no existing filter."""
        self.collector.filter_pattern = None

        # Enter filter mode
        self.display.simulate_input(ord("/"))
        self.collector._handle_input()

        # Buffer should be empty
        self.assertEqual(self.collector.filter_input_buffer, "")

    def test_filter_input_mode_blocks_other_commands(self):
        """Test that filter input mode blocks other commands."""
        self.collector.filter_input_mode = True
        initial_sort = self.collector.sort_by

        # Try to press 's' (sort) - should be captured as input
        self.display.simulate_input(ord("s"))
        self.collector._handle_input()

        # Sort should not change, 's' should be in buffer
        self.assertEqual(self.collector.sort_by, initial_sort)
        self.assertEqual(self.collector.filter_input_buffer, "s")

    def test_filter_input_non_printable_ignored(self):
        """Test that non-printable characters are ignored."""
        self.collector.filter_input_mode = True
        self.collector.filter_input_buffer = "test"

        # Try to input a control character (< 32)
        self.display.simulate_input(1)  # Ctrl-A
        self.collector._handle_input()

        # Buffer should be unchanged
        self.assertEqual(self.collector.filter_input_buffer, "test")

    def test_filter_input_high_ascii_ignored(self):
        """Test that high ASCII characters (>= 127, except backspace) are ignored."""
        self.collector.filter_input_mode = True
        self.collector.filter_input_buffer = "test"

        # Try to input high ASCII (128)
        self.display.simulate_input(128)
        self.collector._handle_input()

        # Buffer should be unchanged
        self.assertEqual(self.collector.filter_input_buffer, "test")

    def test_filter_prompt_displayed(self):
        """Test that filter prompt is displayed when in input mode."""
        self.collector.filter_input_mode = True
        self.collector.filter_input_buffer = "myfilter"
        self.collector.total_samples = 10

        self.collector._update_display()

        # Should show the filter prompt
        self.assertTrue(self.display.contains_text("Function filter"))
        self.assertTrue(self.display.contains_text("myfilter"))


if __name__ == "__main__":
    unittest.main()
