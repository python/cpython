import unittest

from test.support import import_helper, threading_helper

_testcapi = import_helper.import_module("_testcapi")

ITERS = 100
NTHREADS = 20


@threading_helper.requires_working_threading()
class TestDictWatcherThreadSafety(unittest.TestCase):
    # Watcher kinds from _testcapi
    EVENTS = 0  # appends dict events as strings to global event list

    def test_concurrent_add_clear_watchers(self):
        """Race AddWatcher and ClearWatcher from multiple threads.

        Uses more threads than available watcher slots (5 user slots out
        of DICT_MAX_WATCHERS=8).
        """
        results = []

        def worker():
            for _ in range(ITERS):
                try:
                    wid = _testcapi.add_dict_watcher(self.EVENTS)
                except RuntimeError:
                    continue  # All slots taken
                self.assertGreaterEqual(wid, 0)
                results.append(wid)
                _testcapi.clear_dict_watcher(wid)

        threading_helper.run_concurrently(worker, NTHREADS)

        # Verify at least some watchers were successfully added
        self.assertGreater(len(results), 0)

    def test_concurrent_watch_unwatch(self):
        """Race Watch and Unwatch on the same dict from multiple threads."""
        wid = _testcapi.add_dict_watcher(self.EVENTS)
        dicts = [{} for _ in range(10)]

        def worker():
            for _ in range(ITERS):
                for d in dicts:
                    _testcapi.watch_dict(wid, d)
                for d in dicts:
                    _testcapi.unwatch_dict(wid, d)

        try:
            threading_helper.run_concurrently(worker, NTHREADS)

            # Verify watching still works after concurrent watch/unwatch
            _testcapi.watch_dict(wid, dicts[0])
            dicts[0]["key"] = "value"
            events = _testcapi.get_dict_watcher_events()
            self.assertIn("new:key:value", events)
        finally:
            _testcapi.clear_dict_watcher(wid)

    def test_concurrent_modify_watched_dict(self):
        """Race dict mutations (triggering callbacks) with watch/unwatch."""
        wid = _testcapi.add_dict_watcher(self.EVENTS)
        d = {}
        _testcapi.watch_dict(wid, d)

        def mutator():
            for i in range(ITERS):
                d[f"key_{i}"] = i
                d.pop(f"key_{i}", None)

        def toggler():
            for i in range(ITERS):
                _testcapi.watch_dict(wid, d)
                d[f"toggler_{i}"] = i
                _testcapi.unwatch_dict(wid, d)

        workers = [mutator, toggler] * (NTHREADS // 2)
        try:
            threading_helper.run_concurrently(workers)
            events = _testcapi.get_dict_watcher_events()
            self.assertGreater(len(events), 0)
        finally:
            _testcapi.clear_dict_watcher(wid)


if __name__ == "__main__":
    unittest.main()
