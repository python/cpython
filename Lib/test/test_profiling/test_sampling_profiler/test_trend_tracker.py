"""Simple unit tests for TrendTracker."""

import unittest
from test.support import requires
from test.support.import_helper import import_module

# Only run these tests if curses is available
requires("curses")
curses = import_module("curses")

from profiling.sampling.live_collector.trend_tracker import TrendTracker


class TestTrendTracker(unittest.TestCase):
    """Tests for TrendTracker class."""

    def setUp(self):
        """Set up test fixtures."""
        self.colors = {
            "trend_up": curses.A_BOLD,
            "trend_down": curses.A_REVERSE,
            "trend_stable": curses.A_NORMAL,
        }

    def test_basic_trend_detection(self):
        """Test basic up/down/stable trend detection."""
        tracker = TrendTracker(self.colors, enabled=True)

        # First value is always stable
        self.assertEqual(tracker.update("func1", "nsamples", 10), "stable")

        # Increasing value
        self.assertEqual(tracker.update("func1", "nsamples", 20), "up")

        # Decreasing value
        self.assertEqual(tracker.update("func1", "nsamples", 15), "down")

        # Small change (within threshold) is stable
        self.assertEqual(tracker.update("func1", "nsamples", 15.0001), "stable")

    def test_multiple_metrics(self):
        """Test tracking multiple metrics simultaneously."""
        tracker = TrendTracker(self.colors, enabled=True)

        trends = tracker.update_metrics("func1", {
            "nsamples": 10,
            "tottime": 5.0,
        })

        self.assertEqual(trends["nsamples"], "stable")
        self.assertEqual(trends["tottime"], "stable")

        # Update with changes
        trends = tracker.update_metrics("func1", {
            "nsamples": 15,
            "tottime": 3.0,
        })

        self.assertEqual(trends["nsamples"], "up")
        self.assertEqual(trends["tottime"], "down")

    def test_toggle_enabled(self):
        """Test enable/disable toggle."""
        tracker = TrendTracker(self.colors, enabled=True)
        self.assertTrue(tracker.enabled)

        tracker.toggle()
        self.assertFalse(tracker.enabled)

        # When disabled, should return A_NORMAL
        self.assertEqual(tracker.get_color("up"), curses.A_NORMAL)

    def test_get_color(self):
        """Test color selection for trends."""
        tracker = TrendTracker(self.colors, enabled=True)

        self.assertEqual(tracker.get_color("up"), curses.A_BOLD)
        self.assertEqual(tracker.get_color("down"), curses.A_REVERSE)
        self.assertEqual(tracker.get_color("stable"), curses.A_NORMAL)

    def test_clear(self):
        """Test clearing tracked values."""
        tracker = TrendTracker(self.colors, enabled=True)

        # Add some data
        tracker.update("func1", "nsamples", 10)
        tracker.update("func1", "nsamples", 20)

        # Clear
        tracker.clear()

        # After clear, first update should be stable
        self.assertEqual(tracker.update("func1", "nsamples", 30), "stable")


if __name__ == "__main__":
    unittest.main()
