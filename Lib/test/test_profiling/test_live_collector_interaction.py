"""Interactive controls tests for LiveStatsCollector.

Tests for interactive controls, filtering, filter input, and thread navigation.
"""

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
from profiling.sampling.constants import (
    THREAD_STATUS_HAS_GIL,
    THREAD_STATUS_ON_CPU,
)
from ._live_collector_helpers import (
    MockFrameInfo,
    MockThreadInfo,
    MockInterpreterInfo,
)

class TestLiveCollectorInteractiveControls(unittest.TestCase):
    """Tests for interactive control features."""

    def setUp(self):
        """Set up collector with mock display."""
        from profiling.sampling.live_collector import constants
        # Save and reset the display update interval
        self._saved_interval = constants.DISPLAY_UPDATE_INTERVAL
        constants.DISPLAY_UPDATE_INTERVAL = 0.1

        self.display = MockDisplay(height=40, width=160)
        self.collector = LiveStatsCollector(
            1000, pid=12345, display=self.display
        )
        self.collector.start_time = time.perf_counter()

    def tearDown(self):
        """Restore the display update interval."""
        from profiling.sampling.live_collector import constants
        constants.DISPLAY_UPDATE_INTERVAL = self._saved_interval

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
        from profiling.sampling.live_collector import constants

        initial_interval = constants.DISPLAY_UPDATE_INTERVAL

        # Simulate '+' key press (faster = smaller interval)
        self.display.simulate_input(ord("+"))
        self.collector._handle_input()

        self.assertLess(
            constants.DISPLAY_UPDATE_INTERVAL, initial_interval
        )

    def test_decrease_refresh_rate(self):
        """Test decreasing refresh rate (slower updates)."""
        from profiling.sampling.live_collector import constants

        initial_interval = constants.DISPLAY_UPDATE_INTERVAL

        # Simulate '-' key press (slower = larger interval)
        self.display.simulate_input(ord("-"))
        self.collector._handle_input()

        self.assertGreater(
            constants.DISPLAY_UPDATE_INTERVAL, initial_interval
        )

    def test_refresh_rate_minimum(self):
        """Test that refresh rate has a minimum (max speed)."""
        from profiling.sampling.live_collector import constants

        constants.DISPLAY_UPDATE_INTERVAL = 0.05  # Set to minimum

        # Try to go faster
        self.display.simulate_input(ord("+"))
        self.collector._handle_input()

        # Should stay at minimum
        self.assertEqual(constants.DISPLAY_UPDATE_INTERVAL, 0.05)

    def test_refresh_rate_maximum(self):
        """Test that refresh rate has a maximum (min speed)."""
        from profiling.sampling.live_collector import constants

        constants.DISPLAY_UPDATE_INTERVAL = 1.0  # Set to maximum

        # Try to go slower
        self.display.simulate_input(ord("-"))
        self.collector._handle_input()

        # Should stay at maximum
        self.assertEqual(constants.DISPLAY_UPDATE_INTERVAL, 1.0)

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
        from profiling.sampling.live_collector import constants

        initial_interval = constants.DISPLAY_UPDATE_INTERVAL

        # Simulate '=' key press (alternative to '+')
        self.display.simulate_input(ord("="))
        self.collector._handle_input()

        self.assertLess(
            constants.DISPLAY_UPDATE_INTERVAL, initial_interval
        )

    def test_decrease_refresh_rate_with_underscore(self):
        """Test decreasing refresh rate with '_' key."""
        from profiling.sampling.live_collector import constants

        initial_interval = constants.DISPLAY_UPDATE_INTERVAL

        # Simulate '_' key press (alternative to '-')
        self.display.simulate_input(ord("_"))
        self.collector._handle_input()

        self.assertGreater(
            constants.DISPLAY_UPDATE_INTERVAL, initial_interval
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


class TestLiveCollectorThreadNavigation(unittest.TestCase):
    """Tests for thread navigation functionality."""

    def setUp(self):
        """Set up collector with mock display and multiple threads."""
        self.mock_display = MockDisplay(height=40, width=160)
        self.collector = LiveStatsCollector(
            1000, pid=12345, display=self.mock_display
        )
        self.collector.start_time = time.perf_counter()

        # Simulate data from multiple threads
        frames1 = [MockFrameInfo("file1.py", 10, "func1")]
        frames2 = [MockFrameInfo("file2.py", 20, "func2")]
        frames3 = [MockFrameInfo("file3.py", 30, "func3")]

        thread1 = MockThreadInfo(111, frames1)
        thread2 = MockThreadInfo(222, frames2)
        thread3 = MockThreadInfo(333, frames3)

        interpreter_info = MockInterpreterInfo(0, [thread1, thread2, thread3])
        stack_frames = [interpreter_info]

        # Collect data to populate thread IDs
        self.collector.collect(stack_frames)

    def test_initial_view_mode_is_all(self):
        """Test that collector starts in ALL mode."""
        self.assertEqual(self.collector.view_mode, "ALL")
        self.assertEqual(self.collector.current_thread_index, 0)

    def test_thread_ids_are_tracked(self):
        """Test that thread IDs are tracked during collection."""
        self.assertIn(111, self.collector.thread_ids)
        self.assertIn(222, self.collector.thread_ids)
        self.assertIn(333, self.collector.thread_ids)
        self.assertEqual(len(self.collector.thread_ids), 3)

    def test_toggle_to_per_thread_mode(self):
        """Test toggling from ALL to PER_THREAD mode with 't' key."""
        self.assertEqual(self.collector.view_mode, "ALL")

        self.mock_display.simulate_input(ord("t"))
        self.collector._handle_input()

        self.assertEqual(self.collector.view_mode, "PER_THREAD")
        self.assertEqual(self.collector.current_thread_index, 0)

    def test_toggle_back_to_all_mode(self):
        """Test toggling back from PER_THREAD to ALL mode."""
        # Switch to PER_THREAD
        self.mock_display.simulate_input(ord("t"))
        self.collector._handle_input()
        self.assertEqual(self.collector.view_mode, "PER_THREAD")

        # Switch back to ALL
        self.mock_display.simulate_input(ord("T"))
        self.collector._handle_input()
        self.assertEqual(self.collector.view_mode, "ALL")

    def test_arrow_right_navigates_threads_in_per_thread_mode(self):
        """Test that arrow keys navigate threads in PER_THREAD mode."""
        # Switch to PER_THREAD mode
        self.mock_display.simulate_input(ord("t"))
        self.collector._handle_input()

        # Navigate forward
        self.assertEqual(self.collector.current_thread_index, 0)

        self.mock_display.simulate_input(curses.KEY_RIGHT)
        self.collector._handle_input()
        self.assertEqual(self.collector.current_thread_index, 1)

        self.mock_display.simulate_input(curses.KEY_RIGHT)
        self.collector._handle_input()
        self.assertEqual(self.collector.current_thread_index, 2)

    def test_arrow_left_navigates_threads_backward(self):
        """Test that left arrow navigates threads backward."""
        # Switch to PER_THREAD mode
        self.mock_display.simulate_input(ord("t"))
        self.collector._handle_input()

        # Navigate backward (should wrap around)
        self.mock_display.simulate_input(curses.KEY_LEFT)
        self.collector._handle_input()
        self.assertEqual(self.collector.current_thread_index, 2)  # Wrapped to last

        self.mock_display.simulate_input(curses.KEY_LEFT)
        self.collector._handle_input()
        self.assertEqual(self.collector.current_thread_index, 1)

    def test_arrow_down_navigates_like_right(self):
        """Test that down arrow works like right arrow."""
        # Switch to PER_THREAD mode
        self.mock_display.simulate_input(ord("t"))
        self.collector._handle_input()

        self.mock_display.simulate_input(curses.KEY_DOWN)
        self.collector._handle_input()
        self.assertEqual(self.collector.current_thread_index, 1)

    def test_arrow_up_navigates_like_left(self):
        """Test that up arrow works like left arrow."""
        # Switch to PER_THREAD mode
        self.mock_display.simulate_input(ord("t"))
        self.collector._handle_input()

        self.mock_display.simulate_input(curses.KEY_UP)
        self.collector._handle_input()
        self.assertEqual(self.collector.current_thread_index, 2)  # Wrapped

    def test_arrow_keys_do_nothing_in_all_mode(self):
        """Test that arrow keys have no effect in ALL mode."""
        self.assertEqual(self.collector.view_mode, "ALL")

        self.mock_display.simulate_input(curses.KEY_RIGHT)
        self.collector._handle_input()
        self.assertEqual(self.collector.view_mode, "ALL")
        self.assertEqual(self.collector.current_thread_index, 0)

    def test_stats_list_in_all_mode(self):
        """Test that stats list uses aggregated data in ALL mode."""
        stats_list = self.collector._build_stats_list()

        # Should have all 3 functions
        self.assertEqual(len(stats_list), 3)
        func_names = {stat["func"][2] for stat in stats_list}
        self.assertEqual(func_names, {"func1", "func2", "func3"})

    def test_stats_list_in_per_thread_mode(self):
        """Test that stats list filters by thread in PER_THREAD mode."""
        # Switch to PER_THREAD mode
        self.collector.view_mode = "PER_THREAD"
        self.collector.current_thread_index = 0  # First thread (111)

        stats_list = self.collector._build_stats_list()

        # Should only have func1 from thread 111
        self.assertEqual(len(stats_list), 1)
        self.assertEqual(stats_list[0]["func"][2], "func1")

    def test_stats_list_switches_with_thread_navigation(self):
        """Test that stats list updates when navigating threads."""
        self.collector.view_mode = "PER_THREAD"

        # Thread 0 (111) -> func1
        self.collector.current_thread_index = 0
        stats_list = self.collector._build_stats_list()
        self.assertEqual(len(stats_list), 1)
        self.assertEqual(stats_list[0]["func"][2], "func1")

        # Thread 1 (222) -> func2
        self.collector.current_thread_index = 1
        stats_list = self.collector._build_stats_list()
        self.assertEqual(len(stats_list), 1)
        self.assertEqual(stats_list[0]["func"][2], "func2")

        # Thread 2 (333) -> func3
        self.collector.current_thread_index = 2
        stats_list = self.collector._build_stats_list()
        self.assertEqual(len(stats_list), 1)
        self.assertEqual(stats_list[0]["func"][2], "func3")

    def test_reset_stats_clears_thread_data(self):
        """Test that reset_stats clears thread tracking data."""
        self.assertGreater(len(self.collector.thread_ids), 0)
        self.assertGreater(len(self.collector.per_thread_result), 0)

        self.collector.reset_stats()

        self.assertEqual(len(self.collector.thread_ids), 0)
        self.assertEqual(len(self.collector.per_thread_result), 0)
        self.assertEqual(self.collector.view_mode, "ALL")
        self.assertEqual(self.collector.current_thread_index, 0)

    def test_toggle_with_no_threads_stays_in_all_mode(self):
        """Test that toggle does nothing when no threads exist."""
        collector = LiveStatsCollector(1000, display=MockDisplay())
        self.assertEqual(len(collector.thread_ids), 0)

        collector.display.simulate_input(ord("t"))
        collector._handle_input()

        # Should remain in ALL mode since no threads
        self.assertEqual(collector.view_mode, "ALL")

    def test_per_thread_data_isolation(self):
        """Test that per-thread data is properly isolated."""
        # Check that each thread has its own isolated data
        self.assertIn(111, self.collector.per_thread_result)
        self.assertIn(222, self.collector.per_thread_result)
        self.assertIn(333, self.collector.per_thread_result)

        # Thread 111 should only have func1
        thread1_funcs = list(self.collector.per_thread_result[111].keys())
        self.assertEqual(len(thread1_funcs), 1)
        self.assertEqual(thread1_funcs[0][2], "func1")

        # Thread 222 should only have func2
        thread2_funcs = list(self.collector.per_thread_result[222].keys())
        self.assertEqual(len(thread2_funcs), 1)
        self.assertEqual(thread2_funcs[0][2], "func2")

    def test_aggregated_data_sums_all_threads(self):
        """Test that ALL mode shows aggregated data from all threads."""
        # All three functions should be in the aggregated result
        self.assertEqual(len(self.collector.result), 3)

        # Each function should have 1 direct call
        for func_location, counts in self.collector.result.items():
            self.assertEqual(counts["direct_calls"], 1)


if __name__ == "__main__":
    unittest.main()
