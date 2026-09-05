# gh-153014: gc.set_debug() writes gcstate->debug without a lock, racing
# gc.get_debug() and the collector.  The flag is an int, so the race stays
# benign, which lets this stress test double as a regression test under a
# free-threading ThreadSanitizer build.

import gc
import threading
import unittest

from test.support import threading_helper


NUM_WRITERS = 2
NUM_READERS = 4
ITERATIONS = 50


def _stress_debug_race(num_writers=NUM_WRITERS, num_readers=NUM_READERS,
                       iterations=ITERATIONS):
    done = threading.Event()

    def writer():
        try:
            for _ in range(iterations):
                # DEBUG_SAVEALL avoids the stderr spam DEBUG_STATS would emit.
                gc.set_debug(gc.DEBUG_SAVEALL)
                gc.set_debug(0)
        finally:
            done.set()

    def reader():
        while not done.is_set():
            gc.get_debug()

    def collector():
        while not done.is_set():
            a = {}
            b = {}
            a["b"] = b
            b["a"] = a
            del a, b
            gc.collect()

    threading_helper.run_concurrently(
        [writer] * num_writers + [reader] * num_readers + [collector]
    )


@threading_helper.requires_working_threading()
class TestGCDebugRace(unittest.TestCase):
    def setUp(self):
        self.addCleanup(gc.garbage.clear)
        self.addCleanup(gc.set_debug, gc.get_debug())

    def test_set_get_debug_race(self):
        _stress_debug_race()
        self.assertIsInstance(gc.get_debug(), int)


if __name__ == "__main__":
    _stress_debug_race()
    print("Done. Run under a free-threading + TSAN build to observe the race.")
