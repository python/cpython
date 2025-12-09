"""UI and display tests for LiveStatsCollector.

Tests for MockDisplay, curses integration, display methods,
edge cases, update display, and display helpers.
"""

import sys
import time
import unittest
from unittest import mock
from test.support import requires
from test.support.import_helper import import_module

# Only run these tests if curses is available
requires("curses")
curses = import_module("curses")

from profiling.sampling.live_collector import LiveStatsCollector, MockDisplay
from ._live_collector_helpers import (
    MockThreadInfo,
    MockInterpreterInfo,
)


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
        self.collector.successful_samples = 450
        self.collector.failed_samples = 50

        colors = self.collector._setup_colors()
        self.collector._initialize_widgets(colors)

        # Test individual widget methods
        line = self.collector.header_widget.draw_header_info(0, 160, 100.5)
        self.assertEqual(line, 2)  # Title + header info line
        self.assertGreater(len(self.mock_display.buffer), 0)

        # Clear buffer and test next method
        self.mock_display.buffer.clear()
        line = self.collector.header_widget.draw_sample_stats(0, 160, 10.0)
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
        self.collector.successful_samples = 950
        self.collector.failed_samples = 50

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
        self.collector.successful_samples = 75
        self.collector.failed_samples = 25

        colors = self.collector._setup_colors()
        self.collector._initialize_widgets(colors)
        self.collector.header_widget.draw_efficiency_bar(0, 160)

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

            stats_list = self.collector.build_stats_list()
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
            collector.table_widget.draw_column_headers(0, 70)
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
            collector.table_widget.draw_column_headers(0, 60)
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
        stats_list = self.collector.build_stats_list()

        self.collector.header_widget.draw_top_functions(0, 160, stats_list)

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

        collector.header_widget.add_str(5, 10, "Test", 0)
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
        """Test handling 'S' key to cycle sort backward."""
        mock_display = MockDisplay()
        mock_display.simulate_input(ord("S"))
        collector = LiveStatsCollector(
            1000, sort_by="nsamples", display=mock_display
        )

        collector._handle_input()
        self.assertEqual(collector.sort_by, "cumtime")

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

        line = self.collector.header_widget.draw_header_info(0, 160, 100.5)
        self.assertEqual(line, 2)  # Title + header info line

    def test_draw_sample_stats(self):
        """Test drawing sample statistics."""
        self.collector.total_samples = 1000
        colors = {"cyan": curses.A_BOLD, "green": curses.A_BOLD}
        self.collector._initialize_widgets(colors)

        line = self.collector.header_widget.draw_sample_stats(0, 160, 10.0)
        self.assertEqual(line, 1)
        self.assertGreater(self.collector.max_sample_rate, 0)

    def test_progress_bar_uses_target_rate(self):
        """Test that progress bar uses target rate instead of max rate."""
        # Set up collector with specific sampling interval
        collector = LiveStatsCollector(
            10000, pid=12345, display=self.mock_display
        )  # 10ms = 100Hz target
        collector.start_time = time.perf_counter()
        collector.total_samples = 500
        collector.max_sample_rate = (
            150  # Higher than target to test we don't use this
        )

        colors = {"cyan": curses.A_BOLD, "green": curses.A_BOLD}
        collector._initialize_widgets(colors)

        # Clear the display buffer to capture only our progress bar content
        self.mock_display.buffer.clear()

        # Draw sample stats with a known elapsed time that gives us a specific sample rate
        elapsed = 10.0  # 500 samples in 10 seconds = 50 samples/second
        line = collector.header_widget.draw_sample_stats(0, 160, elapsed)

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

        self.assertTrue(
            found_current_target_label,
            "Should display current/target rate with percentage",
        )
        self.assertFalse(found_max_label, "Should not display max rate label")

    def test_progress_bar_different_intervals(self):
        """Test that progress bar adapts to different sampling intervals."""
        test_cases = [
            (
                1000,
                "1.0KHz",
                "100.0Hz",
            ),  # 1ms interval -> 1000Hz target (1.0KHz), 100Hz current
            (
                5000,
                "200.0Hz",
                "100.0Hz",
            ),  # 5ms interval -> 200Hz target, 100Hz current
            (
                20000,
                "50.0Hz",
                "100.0Hz",
            ),  # 20ms interval -> 50Hz target, 100Hz current
            (
                100000,
                "10.0Hz",
                "100.0Hz",
            ),  # 100ms interval -> 10Hz target, 100Hz current
        ]

        for (
            interval_usec,
            expected_target_formatted,
            expected_current_formatted,
        ) in test_cases:
            with self.subTest(interval=interval_usec):
                collector = LiveStatsCollector(
                    interval_usec, display=MockDisplay()
                )
                collector.start_time = time.perf_counter()
                collector.total_samples = 100

                colors = {"cyan": curses.A_BOLD, "green": curses.A_BOLD}
                collector._initialize_widgets(colors)

                # Clear buffer
                collector.display.buffer.clear()

                # Draw with 1 second elapsed time (gives us current rate of 100Hz)
                collector.header_widget.draw_sample_stats(0, 160, 1.0)

                # Check that the current/target format appears in the display with proper units
                found_current_target_format = False
                for (line_num, col), (
                    text,
                    attr,
                ) in collector.display.buffer.items():
                    # Looking for format like "100.0Hz/1.0KHz" or "100.0Hz/200.0Hz"
                    expected_format = f"{expected_current_formatted}/{expected_target_formatted}"
                    if expected_format in text and "%" in text:
                        found_current_target_format = True
                        break

                self.assertTrue(
                    found_current_target_format,
                    f"Should display current/target rate format with units for {interval_usec}µs interval",
                )

    def test_draw_efficiency_bar(self):
        """Test drawing efficiency bar."""
        self.collector.successful_samples = 900
        self.collector.failed_samples = 100
        self.collector.total_samples = 1000
        colors = {"green": curses.A_BOLD, "red": curses.A_BOLD}
        self.collector._initialize_widgets(colors)

        line = self.collector.header_widget.draw_efficiency_bar(0, 160)
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

        stats_list = self.collector.build_stats_list()
        colors = {
            "cyan": curses.A_BOLD,
            "green": curses.A_BOLD,
            "yellow": curses.A_BOLD,
            "magenta": curses.A_BOLD,
        }
        self.collector._initialize_widgets(colors)

        line = self.collector.header_widget.draw_function_stats(
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

        stats_list = self.collector.build_stats_list()
        colors = {
            "red": curses.A_BOLD,
            "yellow": curses.A_BOLD,
            "green": curses.A_BOLD,
        }
        self.collector._initialize_widgets(colors)

        line = self.collector.header_widget.draw_top_functions(
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
        ) = self.collector.table_widget.draw_column_headers(0, 160)
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
        ) = self.collector.table_widget.draw_column_headers(0, 70)
        self.assertEqual(line, 1)
        # Some columns should be hidden on narrow terminal
        self.assertFalse(show_cumul_pct)

    def test_draw_footer(self):
        """Test drawing footer."""
        colors = self.collector._setup_colors()
        self.collector._initialize_widgets(colors)
        self.collector.footer_widget.render(38, 160)
        # Should have written some content to the display buffer
        self.assertGreater(len(self.mock_display.buffer), 0)

    def test_draw_progress_bar(self):
        """Test progress bar drawing."""
        colors = self.collector._setup_colors()
        self.collector._initialize_widgets(colors)
        bar, length = self.collector.header_widget.progress_bar.render_bar(
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

        stats_list = collector.build_stats_list()
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
        self.collector.successful_samples = 90
        self.collector.failed_samples = 10
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


if __name__ == "__main__":
    unittest.main()
