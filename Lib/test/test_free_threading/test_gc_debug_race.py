# gh-153014: data race on the GC debug flag in free-threading builds.
#
# gc.set_debug() stores gcstate->debug with a plain write (no lock, unlike
# gc_set_threshold() which runs under stop-the-world), while gc.get_debug() and
# the collector read it without synchronisation.  Under a free-threading build
# with ThreadSanitizer this is a data race; the flag is only an int, so it stays
# benign at the Python level, which lets this double as a regression test.

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
                # DEBUG_SAVEALL has no stderr side effect (unlike DEBUG_STATS);
                # the collector still reads gcstate->debug either way.
                gc.set_debug(gc.DEBUG_SAVEALL)
                gc.set_debug(0)
        finally:
            done.set()

    def reader():
        while not done.is_set():
            gc.get_debug()

    def collector():
        # The collector reads gcstate->debug while walking the graph.
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
        # gc.set_debug() mutates process-global state; DEBUG_SAVEALL also parks
        # unreachable objects in gc.garbage.  Restore both afterwards.
        self.addCleanup(gc.garbage.clear)
        self.addCleanup(gc.set_debug, gc.get_debug())

    def test_set_get_debug_race(self):
        _stress_debug_race()
        self.assertIsInstance(gc.get_debug(), int)


if __name__ == "__main__":
    _stress_debug_race()
    print("Done. Run under a free-threading + TSAN build to observe the race.")
