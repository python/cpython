"""Test for gh-143545: UAF in lsprof via re-entrant external timer."""
import unittest
import cProfile


class ReentrantTimerTest(unittest.TestCase):
    """Test that profiler handles re-entrant timer correctly."""

    def test_reentrant_timer_index(self):
        """Clearing profiler during timer.__index__ should not crash."""

        class Timer:
            def __init__(self, profiler):
                self.profiler = profiler

            def __call__(self):
                return self

            def __index__(self):
                self.profiler.clear()
                return 0

        prof = cProfile.Profile()
        prof.timer = Timer(prof)

        def victim():
            pass

        # Should not crash with use-after-free
        try:
            prof._pystart_callback(victim.__code__, 0)
        except (RuntimeError, ValueError):
            pass  # Expected if we block the operation


if __name__ == '__main__':
    unittest.main()
