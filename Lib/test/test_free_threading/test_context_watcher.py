import contextvars
import sys
import threading
import unittest

from test import support
from test.support import import_helper, threading_helper

_testcapi = import_helper.import_module("_testcapi")

ITERS = 1000
NTHREADS = 8


@threading_helper.requires_working_threading()
class TestContextWatcherThreadSafety(unittest.TestCase):
    # gh-155619: the per-interpreter context watcher registry
    # (PyInterpreterState.context_watchers plus the active_context_watchers
    # bitmask) is read and written without synchronization.  Every callback
    # used here is a no-op, so any failure is CPython's registry rather than
    # the test's own bookkeeping.

    def test_concurrent_add_clear_watchers(self):
        """Race AddWatcher against ClearWatcher.

        Both scan/modify the callback array and do a read-modify-write on
        the bitmask, so two callers can claim the same slot or lose a
        bitmask update.
        """
        results = []

        def worker():
            for _ in range(ITERS):
                try:
                    wid = _testcapi.add_noop_context_watcher()
                except RuntimeError:
                    continue  # all CONTEXT_MAX_WATCHERS slots taken
                self.assertGreaterEqual(wid, 0)
                results.append(wid)
                _testcapi.clear_context_watcher(wid)

        threading_helper.run_concurrently(worker, NTHREADS)
        self.assertGreater(len(results), 0)

    def test_concurrent_clear_same_watcher(self):
        """Concurrent ClearWatcher calls on one ID: exactly one succeeds.

        Called one after another, every call after the first raises
        ValueError, so overlapping calls must give the same result.  The
        threads busy-wait on a round counter instead of a Barrier so that
        they reach PyContext_ClearWatcher() close enough together to overlap.
        """
        if sys._is_gil_enabled():
            # The calls cannot overlap, and the busy-waits would just
            # contend for the GIL.
            self.skipTest("needs the GIL to be disabled")
        ROUNDS = 100
        round_no = 0
        wid = None
        successes = [0] * NTHREADS
        done = threading.Barrier(NTHREADS + 1)

        def worker(k):
            for r in range(1, ROUNDS + 1):
                while round_no < r:
                    if done.broken:
                        return
                try:
                    _testcapi.clear_context_watcher(wid)
                    successes[k] += 1
                except ValueError:
                    pass
                try:
                    done.wait(support.SHORT_TIMEOUT)
                except threading.BrokenBarrierError:
                    return

        threads = [threading.Thread(target=worker, args=(k,))
                   for k in range(NTHREADS)]
        with threading_helper.start_threads(threads):
            try:
                for r in range(1, ROUNDS + 1):
                    before = sum(successes)
                    wid = _testcapi.add_noop_context_watcher()
                    round_no = r
                    done.wait(support.SHORT_TIMEOUT)
                    self.assertEqual(sum(successes) - before, 1)
            except BaseException:
                # Release the workers so that start_threads() can join them.
                done.abort()
                raise

    def test_clear_watcher_races_notification(self):
        """Race ClearWatcher against notification.

        notify_context_watchers() snapshots the bitmask, then loads
        context_watchers[i], which ClearWatcher may have set to NULL in
        between, so that load must be atomic and must tolerate NULL.
        """
        def switcher():
            # Context().run() enters and exits a context, and each switch
            # dispatches to every active watcher on this thread.
            for _ in range(ITERS):
                contextvars.Context().run(lambda: None)

        def churner():
            for _ in range(ITERS):
                try:
                    wid = _testcapi.add_noop_context_watcher()
                except RuntimeError:
                    continue
                _testcapi.clear_context_watcher(wid)

        workers = [switcher, churner] * (NTHREADS // 2)
        threading_helper.run_concurrently(workers)


if __name__ == "__main__":
    unittest.main()
