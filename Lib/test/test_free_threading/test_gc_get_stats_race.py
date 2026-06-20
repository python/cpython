# Reproduction for the gc.get_stats() data race under free-threading.
#
# CPython issue gh-151646: in free-threading builds (``--disable-gil``) the
# concurrent collector and ``gc.get_stats()`` touch the same per-generation
# statistics struct (``gcstate->generation_stats``) without any synchronisation:
#
#   * READER -- ``gc_get_stats_impl()`` in ``Modules/gcmodule.c`` copies the
#     ``struct gc_generation_stats`` for each generation (``collections``,
#     ``collected``, ``uncollectable``, ``candidates``, ``duration``) with a
#     plain struct assignment and no lock.
#
#   * WRITER -- at the end of every collection cycle ``gc_collect_main()`` in
#     ``Python/gc_free_threading.c`` mutates that very struct in place
#     (``stats->collections++``, ``stats->collected += m``, ...), again with no
#     lock.
#
# When one thread is collecting while several others read the stats, the
# unsynchronised read/write to the same memory is a data race.  The values are
# only statistics, so the race is benign at the Python level (no crash), which
# is exactly what lets this script double as a regression test: once the fix
# adds proper synchronisation, ThreadSanitizer should stay quiet while the
# script keeps exiting cleanly.
#
# Running this script under a free-threading CPython build compiled with
# ThreadSanitizer (``./configure --disable-gil --with-thread-sanitizer``) is
# expected to print a ``WARNING: ThreadSanitizer: data race`` report whose stack
# traces point at ``gc_get_stats_impl`` (gcmodule.c) and ``gc_collect_main``
# (gc_free_threading.c).
#
# Standalone usage:
#
#     ./python Lib/test/test_free_threading/test_gc_get_stats_race.py
#
# or as part of the test suite (only meaningful under a TSAN build):
#
#     ./python -m test test_free_threading.test_gc_get_stats_race

import gc
import threading
import unittest

from test.support import threading_helper


# One thread hammers gc.collect() (the writer side); enough reader threads run
# gc.get_stats() concurrently to make the race easy to observe.  The iteration
# counts are deliberately fixed and finite so the script always terminates.
NUM_COLLECTORS = 1
NUM_READERS = 8
ITERATIONS = 50_000


def _stress_get_stats_race(num_collectors=NUM_COLLECTORS,
                           num_readers=NUM_READERS,
                           iterations=ITERATIONS):
    """Race gc.collect() against gc.get_stats() and return collected stats."""

    # Synchronise the start so collectors and readers overlap for as long as
    # possible, maximising the chance of the read and write landing on the
    # statistics struct at the same time.
    barrier = threading.Barrier(num_collectors + num_readers)

    def collector():
        barrier.wait()
        for _ in range(iterations):
            # Writer: each full collection updates gcstate->generation_stats.
            gc.collect()

    def reader():
        barrier.wait()
        for _ in range(iterations):
            # Reader: copies the per-generation stats structs with no lock.
            gc.get_stats()

    threads = [threading.Thread(target=collector) for _ in range(num_collectors)]
    threads += [threading.Thread(target=reader) for _ in range(num_readers)]

    with threading_helper.start_threads(threads):
        pass


@threading_helper.requires_working_threading()
class TestGCGetStatsRace(unittest.TestCase):
    def test_get_stats_collect_race(self):
        # Use a reduced iteration count under the regular test suite so the test
        # stays reasonably quick while still exercising the race; the standalone
        # __main__ path below uses the full ITERATIONS for a reliable repro.
        _stress_get_stats_race(iterations=2_000)

        # The race is benign at the Python level: gc.get_stats() must still
        # return well-formed data and the interpreter must not crash.
        stats = gc.get_stats()
        self.assertEqual(len(stats), 3)
        for generation in stats:
            self.assertIn("collections", generation)
            self.assertIn("collected", generation)
            self.assertIn("uncollectable", generation)


if __name__ == "__main__":
    # Standalone reproduction: run the full-size race and exit cleanly so the
    # script can be reused as a regression check once the fix lands.
    print(f"Racing {NUM_COLLECTORS} gc.collect() thread(s) against "
          f"{NUM_READERS} gc.get_stats() thread(s), {ITERATIONS} iterations each...")
    _stress_get_stats_race()
    print("Done (no Python-level crash). "
          "Run under a free-threading + TSAN build to observe the data race.")
