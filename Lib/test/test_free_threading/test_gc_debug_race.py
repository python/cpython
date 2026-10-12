# Reproduction for the gc.set_debug()/gc.get_debug() data race under
# free-threading.
#
# CPython issue gh-153014: in free-threading builds (``--disable-gil``)
# ``gc.set_debug()`` writes ``gcstate->debug`` and ``gc.get_debug()`` reads it
# (both in ``Modules/gcmodule.c``) with no synchronisation, so a writer and a
# reader racing on that int is a data race.  The value is only a flag, so the
# race is benign at the Python level (no crash), which is what lets this script
# double as a regression test: once the fix accesses the flag atomically,
# ThreadSanitizer should stay quiet while the script keeps exiting cleanly.
#
# Running this under a free-threading CPython build compiled with
# ThreadSanitizer (``./configure --disable-gil --with-thread-sanitizer``) is
# expected to print a ``WARNING: ThreadSanitizer: data race`` whose stack traces
# point at ``gc_set_debug_impl`` and ``gc_get_debug_impl`` (gcmodule.c).
#
# Standalone usage:
#
#     ./python Lib/test/test_free_threading/test_gc_debug_race.py

import gc
import threading
import unittest

from test.support import threading_helper


ITERATIONS = 1000


def _stress_set_get_debug_race(iterations=ITERATIONS):
    """Race gc.set_debug() against gc.get_debug()."""
    done = threading.Event()

    def writer():
        try:
            for _ in range(iterations):
                # DEBUG_SAVEALL avoids the stderr output DEBUG_STATS prints.
                gc.set_debug(gc.DEBUG_SAVEALL)
                gc.set_debug(0)
        finally:
            done.set()

    def reader():
        while not done.is_set():
            gc.get_debug()

    threading_helper.run_concurrently([writer, reader])


@threading_helper.requires_working_threading()
class TestGCDebugRace(unittest.TestCase):
    def setUp(self):
        self.addCleanup(gc.set_debug, gc.get_debug())

    def test_set_get_debug_race(self):
        _stress_set_get_debug_race()

        # The race is benign at the Python level: gc.get_debug() must still
        # return an int and the interpreter must not crash.
        self.assertIsInstance(gc.get_debug(), int)


if __name__ == "__main__":
    print(f"Racing gc.set_debug() against gc.get_debug(), {ITERATIONS} iterations...")
    _stress_set_get_debug_race()
    print("Done (no Python-level crash). "
          "Run under a free-threading + TSAN build to observe the data race.")
