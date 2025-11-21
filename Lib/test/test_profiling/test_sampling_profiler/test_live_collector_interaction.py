"""Interactive controls tests for LiveStatsCollector.

Tests for interactive controls, filtering, filter input, and thread navigation.
"""

import time
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


class TestLiveCollectorInteractiveControls(unittest.TestCase):
    """Tests for interactive control features."""

    def setUp(self):
        """Set up collector with mock display."""
        self.display = MockDisplay(height=40, width=160)
        self.collector = LiveStatsCollector(
            1000, pid=12345, display=self.display
        )
        self.collector.start_time = time.perf_counter()
        # Set a consistent display update interval for tests
        self.collector.display_update_interval = 0.1

    def tearDown(self):
        """Clean up after test."""
        pass

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
        self.collector.successful_samples = 90
        self.collector.failed_samples = 10
        self.collector.result[("test.py", 1, "func")] = {
            "direct_calls": 50,
            "cumulative_calls": 75,
            "total_rec_calls": 0,
        }

        # Reset
        self.collector.reset_stats()

        self.assertEqual(self.collector.total_samples, 0)
        self.assertEqual(self.collector.successful_samples, 0)
        self.assertEqual(self.collector.failed_samples, 0)
        self.assertEqual(len(self.collector.result), 0)

    def test_increase_refresh_rate(self):
        """Test increasing refresh rate (faster updates)."""
        initial_interval = self.collector.display_update_interval

        # Simulate '+' key press (faster = smaller interval)
        self.display.simulate_input(ord("+"))
        self.collector._handle_input()

        self.assertLess(self.collector.display_update_interval, initial_interval)

    def test_decrease_refresh_rate(self):
        """Test decreasing refresh rate (slower updates)."""
        initial_interval = self.collector.display_update_interval

        # Simulate '-' key press (slower = larger interval)
        self.display.simulate_input(ord("-"))
        self.collector._handle_input()

        self.assertGreater(self.collector.display_update_interval, initial_interval)

    def test_refresh_rate_minimum(self):
        """Test that refresh rate has a minimum (max speed)."""
        self.collector.display_update_interval = 0.05  # Set to minimum

        # Try to go faster
        self.display.simulate_input(ord("+"))
        self.collector._handle_input()

        # Should stay at minimum
        self.assertEqual(self.collector.display_update_interval, 0.05)

    def test_refresh_rate_maximum(self):
        """Test that refresh rate has a maximum (min speed)."""
        self.collector.display_update_interval = 1.0  # Set to maximum

        # Try to go slower
        self.display.simulate_input(ord("-"))
        self.collector._handle_input()

        # Should stay at maximum
        self.assertEqual(self.collector.display_update_interval, 1.0)

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
        initial_interval = self.collector.display_update_interval

        # Simulate '=' key press (alternative to '+')
        self.display.simulate_input(ord("="))
        self.collector._handle_input()

        self.assertLess(self.collector.display_update_interval, initial_interval)

    def test_decrease_refresh_rate_with_underscore(self):
        """Test decreasing refresh rate with '_' key."""
        initial_interval = self.collector.display_update_interval

        # Simulate '_' key press (alternative to '-')
        self.display.simulate_input(ord("_"))
        self.collector._handle_input()

        self.assertGreater(self.collector.display_update_interval, initial_interval)

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

    def test_finished_state_allows_ui_controls(self):
        """Test that finished state allows UI controls but prioritizes quit."""
        self.collector.finished = True
        self.collector.running = True

        # Try pressing 's' (sort) - should work and trigger display update
        original_sort = self.collector.sort_by
        self.display.simulate_input(ord("s"))
        self.collector._handle_input()
        self.assertTrue(self.collector.running)  # Still running
        self.assertNotEqual(self.collector.sort_by, original_sort)  # Sort changed

        # Try pressing 'p' (pause) - should work
        self.display.simulate_input(ord("p"))
        self.collector._handle_input()
        self.assertTrue(self.collector.running)  # Still running
        self.assertTrue(self.collector.paused)  # Now paused

        # Try pressing 'r' (reset) - should be ignored when finished
        self.collector.total_samples = 100
        self.display.simulate_input(ord("r"))
        self.collector._handle_input()
        self.assertTrue(self.collector.running)  # Still running
        self.assertEqual(self.collector.total_samples, 100)  # NOT reset when finished

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

    def test_finished_state_freezes_time(self):
        """Test that time displays are frozen when finished."""
        import time as time_module

        # Set up collector with known start time
        self.collector.start_time = time_module.perf_counter() - 10.0  # 10 seconds ago

        # Mark as finished - this should freeze the time
        self.collector.mark_finished()

        # Get the frozen elapsed time
        frozen_elapsed = self.collector.elapsed_time
        frozen_time_display = self.collector.current_time_display

        # Wait a bit to ensure time would advance
        time_module.sleep(0.1)

        # Time should remain frozen
        self.assertEqual(self.collector.elapsed_time, frozen_elapsed)
        self.assertEqual(self.collector.current_time_display, frozen_time_display)

        # Verify finish timestamp was set
        self.assertIsNotNone(self.collector.finish_timestamp)

        # Reset should clear the frozen state
        self.collector.reset_stats()
        self.assertFalse(self.collector.finished)
        self.assertIsNone(self.collector.finish_timestamp)


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

        stats_list = self.collector.build_stats_list()

        # Only models.py should be included
        self.assertEqual(len(stats_list), 1)
        self.assertIn("models.py", stats_list[0]["func"][0])

    def test_filter_by_function_name(self):
        """Test filtering by function name."""
        self.collector.filter_pattern = "render"

        stats_list = self.collector.build_stats_list()

        self.assertEqual(len(stats_list), 1)
        self.assertEqual(stats_list[0]["func"][2], "render")

    def test_filter_case_insensitive(self):
        """Test that filtering is case-insensitive."""
        self.collector.filter_pattern = "MODELS"

        stats_list = self.collector.build_stats_list()

        # Should still match models.py
        self.assertEqual(len(stats_list), 1)

    def test_filter_substring_matching(self):
        """Test substring filtering."""
        self.collector.filter_pattern = "app/"

        stats_list = self.collector.build_stats_list()

        # Should match both app files
        self.assertEqual(len(stats_list), 2)

    def test_no_filter(self):
        """Test with no filter applied."""
        self.collector.filter_pattern = None

        stats_list = self.collector.build_stats_list()

        # All items should be included
        self.assertEqual(len(stats_list), 3)

    def test_filter_partial_function_name(self):
        """Test filtering by partial function name."""
        self.collector.filter_pattern = "save"

        stats_list = self.collector.build_stats_list()

        self.assertEqual(len(stats_list), 1)
        self.assertEqual(stats_list[0]["func"][2], "save")

    def test_filter_combined_filename_funcname(self):
        """Test filtering matches filename:funcname pattern."""
        self.collector.filter_pattern = "views.py:render"

        stats_list = self.collector.build_stats_list()

        # Should match the combined pattern
        self.assertEqual(len(stats_list), 1)
        self.assertEqual(stats_list[0]["func"][2], "render")

    def test_filter_no_matches(self):
        """Test filter that matches nothing."""
        self.collector.filter_pattern = "nonexistent"

        stats_list = self.collector.build_stats_list()

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
        self.assertEqual(
            self.collector.current_thread_index, 2
        )  # Wrapped to last

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

    def test_arrow_keys_switch_to_per_thread_mode(self):
        """Test that arrow keys switch from ALL mode to PER_THREAD mode."""
        self.assertEqual(self.collector.view_mode, "ALL")

        self.mock_display.simulate_input(curses.KEY_RIGHT)
        self.collector._handle_input()
        self.assertEqual(self.collector.view_mode, "PER_THREAD")
        self.assertEqual(self.collector.current_thread_index, 0)

    def test_stats_list_in_all_mode(self):
        """Test that stats list uses aggregated data in ALL mode."""
        stats_list = self.collector.build_stats_list()

        # Should have all 3 functions
        self.assertEqual(len(stats_list), 3)
        func_names = {stat["func"][2] for stat in stats_list}
        self.assertEqual(func_names, {"func1", "func2", "func3"})

    def test_stats_list_in_per_thread_mode(self):
        """Test that stats list filters by thread in PER_THREAD mode."""
        # Switch to PER_THREAD mode
        self.collector.view_mode = "PER_THREAD"
        self.collector.current_thread_index = 0  # First thread (111)

        stats_list = self.collector.build_stats_list()

        # Should only have func1 from thread 111
        self.assertEqual(len(stats_list), 1)
        self.assertEqual(stats_list[0]["func"][2], "func1")

    def test_stats_list_switches_with_thread_navigation(self):
        """Test that stats list updates when navigating threads."""
        self.collector.view_mode = "PER_THREAD"

        # Thread 0 (111) -> func1
        self.collector.current_thread_index = 0
        stats_list = self.collector.build_stats_list()
        self.assertEqual(len(stats_list), 1)
        self.assertEqual(stats_list[0]["func"][2], "func1")

        # Thread 1 (222) -> func2
        self.collector.current_thread_index = 1
        stats_list = self.collector.build_stats_list()
        self.assertEqual(len(stats_list), 1)
        self.assertEqual(stats_list[0]["func"][2], "func2")

        # Thread 2 (333) -> func3
        self.collector.current_thread_index = 2
        stats_list = self.collector.build_stats_list()
        self.assertEqual(len(stats_list), 1)
        self.assertEqual(stats_list[0]["func"][2], "func3")

    def test_reset_stats_clears_thread_data(self):
        """Test that reset_stats clears thread tracking data."""
        self.assertGreater(len(self.collector.thread_ids), 0)
        self.assertGreater(len(self.collector.per_thread_data), 0)

        self.collector.reset_stats()

        self.assertEqual(len(self.collector.thread_ids), 0)
        self.assertEqual(len(self.collector.per_thread_data), 0)
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
        self.assertIn(111, self.collector.per_thread_data)
        self.assertIn(222, self.collector.per_thread_data)
        self.assertIn(333, self.collector.per_thread_data)

        # Thread 111 should only have func1
        thread1_funcs = list(self.collector.per_thread_data[111].result.keys())
        self.assertEqual(len(thread1_funcs), 1)
        self.assertEqual(thread1_funcs[0][2], "func1")

        # Thread 222 should only have func2
        thread2_funcs = list(self.collector.per_thread_data[222].result.keys())
        self.assertEqual(len(thread2_funcs), 1)
        self.assertEqual(thread2_funcs[0][2], "func2")

    def test_aggregated_data_sums_all_threads(self):
        """Test that ALL mode shows aggregated data from all threads."""
        # All three functions should be in the aggregated result
        self.assertEqual(len(self.collector.result), 3)

        # Each function should have 1 direct call
        for func_location, counts in self.collector.result.items():
            self.assertEqual(counts["direct_calls"], 1)

    def test_per_thread_status_tracking(self):
        """Test that per-thread status statistics are tracked."""
        # Each thread should have status counts
        self.assertIn(111, self.collector.per_thread_data)
        self.assertIn(222, self.collector.per_thread_data)
        self.assertIn(333, self.collector.per_thread_data)

        # Each thread should have the expected attributes
        for thread_id in [111, 222, 333]:
            thread_data = self.collector.per_thread_data[thread_id]
            self.assertIsNotNone(thread_data.has_gil)
            self.assertIsNotNone(thread_data.on_cpu)
            self.assertIsNotNone(thread_data.gil_requested)
            self.assertIsNotNone(thread_data.unknown)
            self.assertIsNotNone(thread_data.total)
            # Each thread was sampled once
            self.assertEqual(thread_data.total, 1)

    def test_reset_stats_clears_thread_status(self):
        """Test that reset_stats clears per-thread status data."""
        self.assertGreater(len(self.collector.per_thread_data), 0)

        self.collector.reset_stats()

        self.assertEqual(len(self.collector.per_thread_data), 0)

    def test_per_thread_sample_counts(self):
        """Test that per-thread sample counts are tracked correctly."""
        # Each thread should have exactly 1 sample (we collected once)
        for thread_id in [111, 222, 333]:
            self.assertIn(thread_id, self.collector.per_thread_data)
            self.assertEqual(self.collector.per_thread_data[thread_id].sample_count, 1)

    def test_per_thread_gc_samples(self):
        """Test that per-thread GC samples are tracked correctly."""
        # Initially no threads have GC frames
        for thread_id in [111, 222, 333]:
            self.assertIn(thread_id, self.collector.per_thread_data)
            self.assertEqual(
                self.collector.per_thread_data[thread_id].gc_frame_samples, 0
            )

        # Now collect a sample with a GC frame in thread 222
        gc_frames = [MockFrameInfo("gc.py", 100, "gc_collect")]
        thread_with_gc = MockThreadInfo(222, gc_frames)
        interpreter_info = MockInterpreterInfo(0, [thread_with_gc])
        stack_frames = [interpreter_info]

        self.collector.collect(stack_frames)

        # Thread 222 should now have 1 GC sample
        self.assertEqual(self.collector.per_thread_data[222].gc_frame_samples, 1)
        # Other threads should still have 0
        self.assertEqual(self.collector.per_thread_data[111].gc_frame_samples, 0)
        self.assertEqual(self.collector.per_thread_data[333].gc_frame_samples, 0)

    def test_only_threads_with_frames_are_tracked(self):
        """Test that only threads with actual frame data are added to thread_ids."""
        # Create a new collector
        collector = LiveStatsCollector(1000, display=MockDisplay())

        # Create threads: one with frames, one without
        frames = [MockFrameInfo("test.py", 10, "test_func")]
        thread_with_frames = MockThreadInfo(111, frames)
        thread_without_frames = MockThreadInfo(222, None)  # No frames
        interpreter_info = MockInterpreterInfo(
            0, [thread_with_frames, thread_without_frames]
        )
        stack_frames = [interpreter_info]

        collector.collect(stack_frames)

        # Only thread 111 should be tracked (it has frames)
        self.assertIn(111, collector.thread_ids)
        self.assertNotIn(222, collector.thread_ids)

    def test_per_thread_status_isolation(self):
        """Test that per-thread status counts are isolated per thread."""
        # Create threads with different status flags

        frames1 = [MockFrameInfo("file1.py", 10, "func1")]
        frames2 = [MockFrameInfo("file2.py", 20, "func2")]

        # Thread 444: has GIL but not on CPU
        thread1 = MockThreadInfo(444, frames1, status=THREAD_STATUS_HAS_GIL)
        # Thread 555: on CPU but not has GIL
        thread2 = MockThreadInfo(555, frames2, status=THREAD_STATUS_ON_CPU)

        interpreter_info = MockInterpreterInfo(0, [thread1, thread2])
        stack_frames = [interpreter_info]

        collector = LiveStatsCollector(1000, display=MockDisplay())
        collector.collect(stack_frames)

        # Check thread 444 status
        self.assertEqual(collector.per_thread_data[444].has_gil, 1)
        self.assertEqual(collector.per_thread_data[444].on_cpu, 0)

        # Check thread 555 status
        self.assertEqual(collector.per_thread_data[555].has_gil, 0)
        self.assertEqual(collector.per_thread_data[555].on_cpu, 1)

    def test_display_uses_per_thread_stats_in_per_thread_mode(self):
        """Test that display widget uses per-thread stats when in PER_THREAD mode."""

        # Create collector with mock display
        collector = LiveStatsCollector(1000, display=MockDisplay())
        collector.start_time = time.perf_counter()

        # Create 2 threads with different characteristics
        # Thread 111: always has GIL (10 samples)
        # Thread 222: never has GIL (10 samples)
        for _ in range(10):
            frames1 = [MockFrameInfo("file1.py", 10, "func1")]
            frames2 = [MockFrameInfo("file2.py", 20, "func2")]
            thread1 = MockThreadInfo(
                111, frames1, status=THREAD_STATUS_HAS_GIL
            )
            thread2 = MockThreadInfo(222, frames2, status=0)  # No flags
            interpreter_info = MockInterpreterInfo(0, [thread1, thread2])
            collector.collect([interpreter_info])

        # In ALL mode, should show mixed stats (50% on GIL, 50% off GIL)
        self.assertEqual(collector.view_mode, "ALL")
        total_has_gil = collector.thread_status_counts["has_gil"]
        total_threads = collector.thread_status_counts["total"]
        self.assertEqual(total_has_gil, 10)  # Only thread 111 has GIL
        self.assertEqual(total_threads, 20)  # 10 samples * 2 threads

        # Switch to PER_THREAD mode and select thread 111
        collector.view_mode = "PER_THREAD"
        collector.current_thread_index = 0  # Thread 111

        # Thread 111 should show 100% on GIL
        thread_111_data = collector.per_thread_data[111]
        self.assertEqual(thread_111_data.has_gil, 10)
        self.assertEqual(thread_111_data.total, 10)

        # Switch to thread 222
        collector.current_thread_index = 1  # Thread 222

        # Thread 222 should show 0% on GIL
        thread_222_data = collector.per_thread_data[222]
        self.assertEqual(thread_222_data.has_gil, 0)
        self.assertEqual(thread_222_data.total, 10)

    def test_display_uses_per_thread_gc_stats_in_per_thread_mode(self):
        """Test that GC percentage uses per-thread data in PER_THREAD mode."""
        # Create collector with mock display
        collector = LiveStatsCollector(1000, display=MockDisplay())
        collector.start_time = time.perf_counter()

        # Thread 111: 5 samples, 2 with GC
        # Thread 222: 5 samples, 0 with GC
        for i in range(5):
            if i < 2:
                # First 2 samples for thread 111 have GC
                frames1 = [MockFrameInfo("gc.py", 100, "gc_collect")]
            else:
                frames1 = [MockFrameInfo("file1.py", 10, "func1")]

            frames2 = [MockFrameInfo("file2.py", 20, "func2")]  # No GC

            thread1 = MockThreadInfo(111, frames1)
            thread2 = MockThreadInfo(222, frames2)
            interpreter_info = MockInterpreterInfo(0, [thread1, thread2])
            collector.collect([interpreter_info])

        # Check aggregated GC stats (ALL mode)
        # 2 GC samples out of 10 total = 20%
        self.assertEqual(collector.gc_frame_samples, 2)
        self.assertEqual(collector.total_samples, 5)  # 5 collect() calls

        # Check per-thread GC stats
        # Thread 111: 2 GC samples out of 5 = 40%
        self.assertEqual(collector.per_thread_data[111].gc_frame_samples, 2)
        self.assertEqual(collector.per_thread_data[111].sample_count, 5)

        # Thread 222: 0 GC samples out of 5 = 0%
        self.assertEqual(collector.per_thread_data[222].gc_frame_samples, 0)
        self.assertEqual(collector.per_thread_data[222].sample_count, 5)

        # Now verify the display would use the correct stats
        collector.view_mode = "PER_THREAD"

        # For thread 111
        collector.current_thread_index = 0
        thread_id = collector.thread_ids[0]
        self.assertEqual(thread_id, 111)
        thread_gc_pct = (
            collector.per_thread_data[111].gc_frame_samples
            / collector.per_thread_data[111].sample_count
        ) * 100
        self.assertEqual(thread_gc_pct, 40.0)

        # For thread 222
        collector.current_thread_index = 1
        thread_id = collector.thread_ids[1]
        self.assertEqual(thread_id, 222)
        thread_gc_pct = (
            collector.per_thread_data[222].gc_frame_samples
            / collector.per_thread_data[222].sample_count
        ) * 100
        self.assertEqual(thread_gc_pct, 0.0)

    def test_function_counts_are_per_thread_in_per_thread_mode(self):
        """Test that function counts (total/exec/stack) are per-thread in PER_THREAD mode."""
        # Create collector with mock display
        collector = LiveStatsCollector(1000, display=MockDisplay())
        collector.start_time = time.perf_counter()

        # Thread 111: calls func1, func2, func3 (3 functions)
        # Thread 222: calls func4, func5 (2 functions)
        frames1 = [
            MockFrameInfo("file1.py", 10, "func1"),
            MockFrameInfo("file1.py", 20, "func2"),
            MockFrameInfo("file1.py", 30, "func3"),
        ]
        frames2 = [
            MockFrameInfo("file2.py", 40, "func4"),
            MockFrameInfo("file2.py", 50, "func5"),
        ]

        thread1 = MockThreadInfo(111, frames1)
        thread2 = MockThreadInfo(222, frames2)
        interpreter_info = MockInterpreterInfo(0, [thread1, thread2])
        collector.collect([interpreter_info])

        # In ALL mode, should have 5 total functions
        self.assertEqual(len(collector.result), 5)

        # In PER_THREAD mode for thread 111, should have 3 functions
        collector.view_mode = "PER_THREAD"
        collector.current_thread_index = 0  # Thread 111
        thread_111_result = collector.per_thread_data[111].result
        self.assertEqual(len(thread_111_result), 3)

        # Verify the functions are the right ones
        thread_111_funcs = {loc[2] for loc in thread_111_result.keys()}
        self.assertEqual(thread_111_funcs, {"func1", "func2", "func3"})

        # In PER_THREAD mode for thread 222, should have 2 functions
        collector.current_thread_index = 1  # Thread 222
        thread_222_result = collector.per_thread_data[222].result
        self.assertEqual(len(thread_222_result), 2)

        # Verify the functions are the right ones
        thread_222_funcs = {loc[2] for loc in thread_222_result.keys()}
        self.assertEqual(thread_222_funcs, {"func4", "func5"})


class TestLiveCollectorNewFeatures(unittest.TestCase):
    """Tests for new features added to live collector."""

    def setUp(self):
        """Set up test fixtures."""
        self.display = MockDisplay()
        self.collector = LiveStatsCollector(1000, display=self.display)
        self.collector.start_time = time.perf_counter()

    def test_filter_input_takes_precedence_over_commands(self):
        """Test that filter input mode blocks command keys like 'h' and 'p'."""
        # Enter filter input mode
        self.collector.filter_input_mode = True
        self.collector.filter_input_buffer = ""

        # Press 'h' - should add to filter buffer, not show help
        self.display.simulate_input(ord("h"))
        self.collector._handle_input()

        self.assertFalse(self.collector.show_help)  # Help not triggered
        self.assertEqual(self.collector.filter_input_buffer, "h")  # Added to filter
        self.assertTrue(self.collector.filter_input_mode)  # Still in filter mode

    def test_reset_blocked_when_finished(self):
        """Test that reset command is blocked when profiling is finished."""
        # Set up some sample data and mark as finished
        self.collector.total_samples = 100
        self.collector.finished = True

        # Press 'r' for reset
        self.display.simulate_input(ord("r"))
        self.collector._handle_input()

        # Should NOT have been reset
        self.assertEqual(self.collector.total_samples, 100)
        self.assertTrue(self.collector.finished)

    def test_time_display_fix_when_finished(self):
        """Test that time display shows correct frozen time when finished."""
        import time as time_module

        # Mark as finished to freeze time
        self.collector.mark_finished()

        # Should have set both timestamps correctly
        self.assertIsNotNone(self.collector.finish_timestamp)
        self.assertIsNotNone(self.collector.finish_wall_time)

        # Get the frozen time display
        frozen_time = self.collector.current_time_display

        # Wait a bit
        time_module.sleep(0.1)

        # Should still show the same frozen time (not jump to wrong time)
        self.assertEqual(self.collector.current_time_display, frozen_time)


if __name__ == "__main__":
    unittest.main()
